#include "cail_impl.hpp"
#include "MainWindow.h"

cail_impl_t::cail_impl_t(const log_handler_t &log_handler)
	: log_handler_(log_handler)
{
	callbacks_.connected_handler_ = [this](const std::string& ip) {
		connector_->req_info();
	};
	callbacks_.disconnected_handler_ = [this](const std::string& ip) {
	
	};
	callbacks_.info_handler_ = [this](double iq_rate, std::uint32_t samples_num, std::uint32_t channels_num, double carrier_frequency) {
		fft_ = std::make_unique<compute_fft_t>(iq_rate, samples_num, channels_num);
		emit sig_channelinfo(channels_num, iq_rate, carrier_frequency);
		connector_->req_config(std::numeric_limits<std::uint8_t>::max(), 1ull);
	};
	callbacks_.data_handler_ = [this](auto buffer, const char* data, std::uint32_t samples, std::uint32_t channels, double iq_rate, double center_freq) {
			fft_->process(data, samples,[this, buffer, iq_rate, center_freq](const auto& amp_db , const auto& amp,const auto & phase) {

				auto total_result = 0.;
				auto channle_size = phase.size();
				auto avr = 0.0;
				std::vector<double> phase_differ;
				std::vector<double> str;
				str.emplace_back(center_freq);
				str.emplace_back(iq_rate);
				str.emplace_back(channle_size);
				for (auto i = 0; i < channle_size; i++) {
					if (i == 0)
						str.emplace_back(phase[i] - phase[0]);
					else
					{
						auto data = phase[i] - phase[0];
						if (data < -180.0)
							data += 360.0;
						if (data > 180.0)
							data -= 360.0;
						str.emplace_back(data);
						phase_differ.emplace_back(data);
					}
				}
				for (auto i = 0; i < channle_size; i++) {
					auto data = amp_db[i];
					str.emplace_back(data);
				}
				cail_data_.emplace_back(str);
				if (cail_times_) {
					for (auto i = 1; i < channle_size; i++) {
						auto index = i + 4;
						auto val = abs(cail_data_[cail_times_][index] - cail_data_[cail_times_ - 1][index]);
						total_result += (100. - (val / 360.0) * 100.);
					}
					auto result = total_result / (channle_size - 1);
					passing_rate_ += result;
					avr = passing_rate_ / cail_times_;
				}		
				emit sig_setlist(phase_differ, avr);
				save();
				cail_times_++;
			});	
	};
	connector_ = std::make_unique<rtsa::connector_t>(log_handler_, callbacks_);
	connector_->start();
}

cail_impl_t::~cail_impl_t() {
	connector_->stop();
}

void cail_impl_t::end_data() {
	connector_->end_data();
}

void cail_impl_t::recailbrate() {
	if (!connector_->is_connected()) {
		log_handler_(u8"与服务未连接", 1,log_level_t::error);
	}
	else {
		connector_->end_data();
		connector_->begin_data();

		if (!cail_times_)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			connector_->end_data();
			connector_->begin_data();
		}

	}
}

void cail_impl_t::save()
{
	if(!cail_times_)
		outFile.open(write_path_, std::ios::out | std::ios::binary | std::ios::trunc);
	else
		outFile.open(write_path_, std::ios::out | std::ios::binary | std::ios::app);

	if (outFile) {
		for (auto i = 0; i < cail_data_[cail_times_].size(); i++) {
				outFile << cail_data_[cail_times_][i] << '\t';
		}
		outFile << '\n';
		outFile.close();//结束写入
	}
}

