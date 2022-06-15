#pragma once

#include <QObject>
#include <map>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <common/system.hpp>
#include "compute_fft/compute_fft.hpp"
#include "network/connector.hpp"

class MainWindow;
//void x16_uhd_t::config_calibration(const x16_uhd_calibration_t& setting, const std::function<void(const x16_uhd_calibration_t&)>& handler){
	//	AE_UNUSED(setting);
	//	//if (setting.is_need_recalibration_) {
	//	//	calibration_setting_.calibration_duration_ = setting.calibration_duration_;
	//	//	//assert(calibration_setting_.calibration_duration_ != 0);
	//	//	handler(calibration());
	//	//	calibration_setting_.is_need_recalibration_ = false;
	//	//	for (auto i = 0u; i < calibration_setting_.phase_factor_.size(); i++) {
	//	//		calibration_setting_.phase_factor_[i] = std::abs(calibration_setting_.phase_[i] - calibration_setting_.phase_[0]);
	//	//	}
	//	//	make_phase_factor();
	//	//	for (auto i = 0u; i < calibration_setting_.power_factor_.size(); i++) {
	//	//		calibration_setting_.power_factor_[i] = std::abs(calibration_setting_.power_[i] - calibration_setting_.power_[0]);
	//	//	}
	//	//}
	//	//if(calibration_enable_ = calibration_setting_.is_calibration_enbale_){
	//	//	calibration_setting_.is_calibration_enbale_ = setting.is_calibration_enbale_;
	//	//
	//	//	calibration_enable_ = true;
	//	//	write_calibration_info();
	//	//}
	//	//handler(calibration());
	//	AE_UNUSED(handler);
	//}

	/*void x16_uhd_t::make_phase_factor() {
		calibration_setting_.phase_factor_.clear();
		for (auto i = 0u; i < calibration_setting_.phase_factor_.size(); i++) {
			auto radians = (calibration_setting_.phase_[i] - calibration_setting_.phase_[0]) / 180.0 * 2.0 * pi;
			auto factor = radians * std::complex<double>{0, 1};
			calibration_setting_.phase_factor_[i] = std::exp(factor);
		}
	}*/

class cail_impl_t:public QObject {

	Q_OBJECT

	log_handler_t log_handler_;
	std::unique_ptr<rtsa::connector_t> connector_;
	std::unique_ptr<compute_fft_t> fft_;
	rtsa::callbacks_t callbacks_;
	
public:
	
	cail_impl_t(const log_handler_t &);
	~cail_impl_t();
	
	std::uint32_t  cail_times_ = 0;//次数
	double passing_rate_ = 0.;//测试总通过率
	std::ofstream outFile;

	std::vector<std::vector<double>> cail_data_;
	std::string write_path_ = "./data/config/calibration_config.txt";
	void recailbrate();
	void end_data();
	void save();

signals:
	void sig_setlist(const std::vector<double>&, double);
	void sig_channelinfo(std::uint32_t, double, double);
};