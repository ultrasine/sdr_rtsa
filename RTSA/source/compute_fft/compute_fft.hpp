#pragma once

#include <vector>
#include <memory>
#include <functional>

class compute_fft_t { 
	struct impl;
	std::unique_ptr<impl> impl_;

public:
	compute_fft_t(double, std::uint32_t, std::uint32_t);
	~compute_fft_t();

	void process(const void*, std::uint32_t,const std::function<void(const std::vector<double>&, const std::vector<double>&,const std::vector<double>&)>&);
};
