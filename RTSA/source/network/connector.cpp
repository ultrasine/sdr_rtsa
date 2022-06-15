
#include <common/convert.hpp>
#include <common/fifo.hpp>
#include <common/system.hpp>

#include <network/protocol.hpp>
#include <network/client/client.hpp>
#include <pb_framework/pb_connector.hpp>
#include <pb_framework/protocol/rtsa.pb.h>
#include <io_device.hpp>
#include <data_2_pb.hpp>

#include "../../business/rtsa/source/rtsa/network/protocol.hpp"

#include "connector.hpp"

namespace rtsa {


	struct connector_t::impl {
		log_handler_t log_handler_;
		::network::client_t data_connector_;
		framework::pb::connector_t cmd_connector_;

		const callbacks_t& callbacks_;
		bool is_connected_ = false;
		
		std::string svr_ip_;
		std::shared_ptr<device_base_t> device_;
		device_type_t type_;
		std::uint32_t channel_nums_ = 0;
		std::uint32_t per_samples_ = 0;
		double iq_rate_ = 0.0;
		intellicube::utility::dynamic_fifo_t<::network::buffer_t::element_type*> buffers_;

		impl(const log_handler_t& log_hander, const callbacks_t& callbacks)
			: callbacks_(callbacks)
			, log_handler_(log_hander)
			, data_connector_([](const auto&) {},
				[]() ->std::uint32_t
				{
					return sizeof(rtsa::network::acq_data_decode_t);
				},
				[](const char* header)
				{
					return reinterpret_cast<const rtsa::network::acq_data_decode_t*>(header)->size_;
				},
					[this](auto size) {
					if (buffers_.empty()) {
						auto buffer = ::network::make_buffer(size, [this](auto val) {
							buffers_.push(val);
							});
						return buffer;
					}
					else {
						::network::buffer_t::element_type* buffer;
						buffers_.pop(buffer);
						buffer->resize(size);
						return ::network::buffer_t(buffer, [this](auto val) {
							buffers_.push(val);
							});
					}
				})
			, cmd_connector_("x16_calibration connector", [this](const std::string& msg, std::uint32_t type, log_level_t level) {
					log_handler_(stdex::to_utf8(stdex::to_w(msg)), log_type_t::r_log, log_level_t::info);
				})
		{
		}
	};

	connector_t::connector_t(const log_handler_t& log_hander, const callbacks_t& callbacks)
		: impl_(std::make_unique<impl>(log_hander, callbacks)) {

		connect(this, &connector_t::sig_connected, this, &connector_t::slot_connected, Qt::QueuedConnection);
		connect(this, &connector_t::sig_disconnected, this, &connector_t::slot_disconnected, Qt::QueuedConnection);
		connect(this, &connector_t::sig_info, this, &connector_t::slot_info, Qt::QueuedConnection);
	}

	connector_t::~connector_t() {
		stop();
	}

	bool connector_t::is_connected() const
	{
		return impl_->is_connected_;
	}


	void connector_t::req_info()
	{
		auto buffer = ::network::make_buffer(0);
		rtsa::network::acq_data_encode_t header{0, rtsa::network::op_t::info};
		rtsa::network::acq_data_encode_protocol_t protocol{ buffer, header };

		impl_->data_connector_.async_send(protocol.buffer());
	}

	void connector_t::req_config(std::uint8_t ch_id, std::uint64_t sample_num)
	{
		auto buffer = ::network::make_buffer(0);
		rtsa::network::acq_data_encode_t header{ 0, rtsa::network::op_t::config };
		rtsa::network::acq_data_encode_protocol_t protocol{ buffer, header };
		protocol << ch_id << sample_num ;
		
		impl_->data_connector_.async_send(protocol.buffer());
	}

	void connector_t::begin_data()
	{
		auto buffer = ::network::make_buffer(0);
		rtsa::network::acq_data_encode_t header{ 0, rtsa::network::op_t::start };
		rtsa::network::acq_data_encode_protocol_t protocol{ buffer, header };
	
		impl_->data_connector_.async_send(protocol.buffer());
	}

	void connector_t::end_data()
	{
		auto buffer = ::network::make_buffer(0);
		rtsa::network::acq_data_encode_t header{ 0, rtsa::network::op_t::stop };
		rtsa::network::acq_data_encode_protocol_t protocol{ buffer, header };


		impl_->data_connector_.async_send(protocol.buffer());
	}


	bool connector_t::start() {
		auto path = intellicube::system::app_path() / "data/config/ip.dat";
		std::string ip_info = { "127.0.0.1" };
		std::fstream input(path, std::ios::in | std::ios::binary);
		if (input) {
			input >> ip_info;
		}
		impl_->svr_ip_ = ip_info;

		impl_->data_connector_.set_status_handler(
			[this]() {
			impl_->is_connected_ = true;
			emit sig_connected(QString::fromStdString(impl_->svr_ip_));
			}, 
		[this]() {
			impl_->is_connected_ = false;
			emit sig_disconnected(QString::fromStdString(impl_->svr_ip_));
			return true;
		});

		auto ret = impl_->data_connector_.start(impl_->svr_ip_, ACQ_PORT, 
			[this](const auto& recv_buffer) {
			rtsa::network::acq_data_decode_protocol_t protocol{recv_buffer};
			switch (protocol.header_.op_) {
			case network::op_t::info: {
				bool is_ok{ false };
				size_t channel_num ;
				std::vector<channel_info> channels_info;
				double iq_rate{ .0 };
				std::uint8_t id_0 = 0;
				double ref_level = 0.0; 
				double carrier_frequency = 0.0; 
				IQ_t iq_type;
				protocol >> is_ok;

				if (!is_ok) {
					impl_->log_handler_(u8"服务未加载设备", 1,log_level_t::error);
				}
				else {
					protocol >> iq_rate >> iq_type >> impl_->per_samples_ >> channel_num;
					for (auto i = 0; i != channel_num; ++i)
					{
						channel_info info;
						protocol >> info.id_ >> info.ref_level_ >> info.carrier_frequency_ >> info.power_level_
							>> info.wfm_info_.gain_ >> info.wfm_info_.offset_ >> info.wfm_info_.inc_t >> info.wfm_info_.abs_t >> info.wfm_info_.rlt_t
							>> info.gain_;
						channels_info.emplace_back(info);
					}
						//id_0 >> ref_level >> carrier_frequency;
					impl_->iq_rate_ = iq_rate;
								
					emit sig_info(iq_rate, impl_->per_samples_, channel_num, channels_info.begin()->carrier_frequency_);
				}

			}
				break;
			case network::op_t::data: {
				auto data = recv_buffer->data() + sizeof(protocol.header_);
				impl_->callbacks_.data_handler_(recv_buffer, data, impl_->per_samples_, impl_->channel_nums_, impl_->iq_rate_,impl_->device_->rx_channels_.at(0)->carrier_frequency_);
			}			
				break;
			default:
				break;
			}

			}, std::chrono::seconds{ 30 });

		if (!ret) {
			impl_->log_handler_(u8"未能连接上数据服务[" + impl_->svr_ip_ + "]", log_type_t::r_log,log_level_t::error);
			return false;
		}

		ret = impl_->cmd_connector_.start(impl_->svr_ip_, RTSA_SERVICE_PORT,
			[this]() {
				auto req = framework::pb::create_msg<rtsa::all_devices_t>();
				impl_->cmd_connector_.send_msg(req);
			},
			[this]() {
			});
		if (!ret) {
			impl_->log_handler_(u8"未能连接上命令服务[" + impl_->svr_ip_ + "]", log_type_t::r_log,log_level_t::error);
			return false;
		}
		else {

			impl_->cmd_connector_.reg_msg([this](const std::shared_ptr<rtsa::all_devices_response_t>& msg) {
				for (const auto& device : msg->settings()) {
					auto type = (device_type_t)device.device_type();
					impl_->type_ = type;//设置设备类型
					impl_->device_ = to_device(type, device.setting());

					if (impl_->device_->rx_channels_.size() >= 2)
						break;
				}
				
			});

			impl_->cmd_connector_.reg_default_msg_handler([this](const ::network::session_ptr&, const framework::msg_ptr& msg) {
				assert(0);
				impl_->log_handler_(u8"未注册消息: " + msg->DebugString(), log_type_t::r_log,log_level_t::warnning);
				});
		}

		return true;
	}


	void connector_t::stop() {
		impl_->cmd_connector_.stop();
		impl_->data_connector_.stop();
	}


	void connector_t::slot_connected(QString ip){
		impl_->callbacks_.connected_handler_(ip.toStdString());
	}

	void connector_t::slot_disconnected(QString ip){
		impl_->callbacks_.disconnected_handler_(ip.toStdString());
	}

	void connector_t::slot_info(double iq_rate, std::uint32_t samples_num, std::uint32_t channels_num, double carrier_frequency)
	{
		impl_->callbacks_.info_handler_(iq_rate, samples_num, channels_num, carrier_frequency);
	}

}