#include "compute_fft.hpp"

#include <vector>
#include <thread>
#include <tuple>
#include <fftw3.h>
#include <cmath>
#include <complex>
#include <fstream>
#include "../../common/include/common/fifo.hpp"


constexpr auto pi = 3.14159265358979323846;

struct compute_fft_t::impl {
   
    double gain_ = 1.0 / 32767./100.;
	double offset_ = 0.0;
    std::uint64_t number_ = 0;
    std::uint32_t samples_num_ = 0;
    std::uint32_t channel_num_ = 0;
    double iq_rate_ = 0.0;
    double multi_ = 1.0;
    intellicube::utility::blocking_queue_t<std::function<void()>> ops_;
    std::unique_ptr<std::thread> thr_;


    impl(double iq_rate, std::uint32_t samples_num, std::uint32_t channel_num)
    : samples_num_(samples_num)
    , channel_num_(channel_num) 
    , iq_rate_(iq_rate)
    {
        number_ = (std::uint64_t)(pow(2,ceil(log2(iq_rate))));
        
        thr_ = std::make_unique<std::thread>([this]() {
            while (1) {
                std::function<void()> op;
                ops_.pop(op);
                if (op == nullptr)
                    break;

                op();
            }
        });
    }

    ~impl() {
        ops_.push(nullptr);

        if(thr_ && thr_->joinable())
            thr_->join();
    }

    std::vector<std::complex<double>> type_cast(const std::int16_t* in, std::uint64_t in_number) {
        std::vector<std::complex<double>> out;
        out.resize(in_number);

        for (auto i = 0u; i < in_number; ++i) {
            out[i] = { offset_ + gain_ * in[i * 2], offset_ + gain_ * in[i * 2 + 1] };
        }

        return out;
    }

    //ref:https://people.math.sc.edu/Burkardt/c_src/fftw/fftw_test.c
    std::vector<std::complex<double>> fft(const std::vector<std::complex<double>> &in) {
        std::vector<std::complex<double>> out;
        out.resize(in.size());

        auto plan_forward = ::fftw_plan_dft_1d((int)in.size(), (fftw_complex*)in.data(), (fftw_complex*)out.data(), FFTW_FORWARD, FFTW_ESTIMATE);
        auto test_type = (fftw_complex*)in.data();
        ::fftw_execute(plan_forward);
        ::fftw_destroy_plan(plan_forward); 
        return out;
    }

    std::tuple<double, double, double> phase_amp(void* spectrum_ptr, void* spec_amp_ptr, std::uint64_t number) {
        auto spectrum = (fftw_complex*)spectrum_ptr;//FFT 过后的复数 
        auto spec_amp = (double*)spec_amp_ptr;
        auto bin = 0u;

        auto recip_number = 1.0 / number;//number 采样点个数  
        for (auto i = 0u; i < number; i++) {
            spec_amp[i] = std::hypot(spectrum[i][0], spectrum[i][1]) * recip_number;
            if (spec_amp[bin] < spec_amp[i]) {
                bin = i;
            }
        }


        auto tone_amp = spec_amp[bin];//电压
        auto tone_amp_dbm = 10.0 * log10(pow(tone_amp, 2) * 1000.0 / 50.0);//幅度
        auto tone_phase_degree = atan(spectrum[bin][1] / spectrum[bin][0]) * 180.0 / pi;//相位
        return { tone_amp_dbm, tone_amp, tone_phase_degree};
    }


    std::tuple<double, double, double> phase_amp_test(std::uint8_t a,std::vector<std::complex<double>> spectrum_ptr, std::uint64_t number) {
        double  spectrum_max = 0.0;//最大值
        double amp = 0.0;
        double amp_db = 0.0;
        int index = 0;//最大值的下标

        auto recip_number = 1.0 / number;//number 采样点个数  
        for (auto i = 0u; i < number; i++) {
            auto real = spectrum_ptr[i].real();
            auto ima = spectrum_ptr[i].imag();
            auto temp_amp = std::hypot(spectrum_ptr[i].real(), spectrum_ptr[i].imag()) * recip_number;
            if (temp_amp > spectrum_max) {
                spectrum_max = temp_amp;
                index = i;
            }
        }
    
        auto tone_amp = spectrum_max * 100;//电压
        auto tone_power = (pow(tone_amp, 2) / 50.0) * 1000;//mw  功率
        auto tone_amp_dbm = 10.0 * log10(pow(tone_amp, 2) / 50.0 * 1000);//幅度

        if (a == 0){
            amp = tone_amp_dbm;
            multi_ = amp;
        }
        else
            amp = tone_amp_dbm - multi_;

        auto real = spectrum_ptr[index].real();
        auto imag = spectrum_ptr[index].imag();
        auto tone_phase_degree_ = atan2(imag , real) * 180.0 / pi;
     
        return { amp, 0, tone_phase_degree_ };
    }

    auto process(const void* data, std::uint32_t data_num) {
        std::vector<double> phase, amp, amp_dbm;//幅度，功率，相位
        phase.resize(channel_num_);
        amp_dbm.resize(channel_num_);
        amp.resize(channel_num_);
     
        auto temp_test = (char*)data;
        auto test_size = data_num * 4 ;//每个samples 有32k个iq 每个iq有4B  data_num单位为个 显示的采样点数为data_num/1024
        auto temp_test1 = temp_test + test_size*0;//数据偏移量
  
        for (std::uint32_t i = 0; i < channel_num_; ++i) 
        {
            auto buffer = (const std::int16_t*)data  + i * 2 * samples_num_;//设置偏移量
            auto double_datas = type_cast(buffer, samples_num_);

            auto fft_datas = fft(double_datas);
		
            auto [amp_dbm_, amp_, phase_] = phase_amp_test(i,fft_datas,samples_num_);//幅度，功率，相位
            phase[i] = phase_;
            amp_dbm[i] = amp_dbm_;
            amp[i] = amp_;
        }
        return std::make_tuple(amp_dbm, amp, phase);
    }
};


compute_fft_t::compute_fft_t(double iq_rate, std::uint32_t samples_num, std::uint32_t channel_number)
: impl_(std::make_unique<impl>(iq_rate, samples_num, channel_number)){

}


compute_fft_t::~compute_fft_t()
{

}

void compute_fft_t::process(const void *data, std::uint32_t data_num,
	const std::function<void(const std::vector<double>&, const std::vector<double>&,const std::vector<double> &)>& handler) {
   
        impl_->ops_.push([this, data, data_num, handler]() {
            auto [phase, amp, amp_dbm] = impl_->process(data, data_num);//
            handler(std::cref(phase), std::cref(amp), std::cref(amp_dbm));
            });
}
