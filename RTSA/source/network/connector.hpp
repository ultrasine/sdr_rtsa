#pragma once

#include <memory>
#include <set>
#include <bitset>
#include <functional>
#include <QObject>
#include <QVariant>
#include <network/buffer.hpp>
#include <network/protocol.hpp>
#include <network/client/client.hpp>
#include <pb_framework/pb_connector.hpp>
#include <pb_framework/protocol/rtsa.pb.h>
#include <data_2_pb.hpp>
#include "../../business/rtsa/source/rtsa/network/protocol.hpp"


#include "io_device.hpp"



namespace rtsa {

	struct callbacks_t {

		// status
		std::function<void(const std::string&)> connected_handler_;
		std::function<void(const std::string&)> disconnected_handler_;
		std::function<void(double, std::uint32_t, std::uint32_t, double)> info_handler_;
		std::function<void(::network::buffer_t, const char *, std::uint32_t, std::uint32_t, double, double)> data_handler_;
	};

	struct channel_info {
		std::uint8_t id_; //通道名称
		double ref_level_ = 0.0; //参考电平
		double carrier_frequency_ = 0.0; //中心频率
		double power_level_ = 0.0;// 输出功率 
		wfm_info_t wfm_info_;
		double gain_ = 0.0; //增益
	};

	class connector_t : public QObject {
		Q_OBJECT

		struct impl;
		std::unique_ptr<impl> impl_;
		

	public:
		connector_t(const log_handler_t&, const callbacks_t&);
		~connector_t();
	

		bool start();
		void stop();

		bool is_connected() const;

		void req_info();
		void req_config(std::uint8_t, std::uint64_t);
		void begin_data();
		void end_data();
	private slots:
		void slot_connected(QString);
		void slot_disconnected(QString);
		void slot_info(double, std::uint32_t, std::uint32_t, double);
	signals:
		void sig_connected(QString);
		void sig_disconnected(QString);
		void sig_info(double, std::uint32_t, std::uint32_t, double);
	};
}
