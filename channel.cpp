#include <algorithm>
#include <math.h>
#include <memory>
#include <random>
#include <stdexcept>
#include "mlinterp.hpp"
#include "channel.hpp"
#include "config.hpp"

std::unique_ptr<float[]> apply_timing_offset(float timing_offset, std::unique_ptr<float[]> &yd, int nxd, int &ni)
{
    ni = (int)((nxd-1)/timing_offset) + 1;
    
    std::unique_ptr<float[]> xd (new float[nxd]);
    std::unique_ptr<float[]> xi (new float[ni]);

    for (int i = 0; i < nxd; i++) {
        xd[i] = ((float)i)/(nxd-1);
    }
    for (int i = 0; i < ni; i++) {
        xi[i] = timing_offset * ((float)i)/(nxd-1);
    }

    const int nd[] = { nxd };
    std::unique_ptr<float[]> yi (new float[ni]);

    mlinterp::interp(
            nd, ni,
            yd.get(), yi.get(),
            xd.get(), xi.get()
            );

    return yi;
}

std::unique_ptr<float[]> awgn_channel(std::mt19937 &gen, float noise_amplitude, float timing_offset, const float *samples, int nxd, int &ni)
{
    std::unique_ptr<float[]> yd (new float[nxd]);
    std::normal_distribution<> d {0, noise_amplitude};
    for (int i = 0; i < nxd; i++) {
        yd[i] = samples[i] + d(gen);
    }
    return apply_timing_offset(timing_offset, yd, nxd, ni);
}

float signal_avg_power(const float *samples, int n)
{
    float res = 0.;
    for (int i = 0; i < n; i++) {
        res += (samples[i]*samples[i])/n;
    }
    return res;
}

std::unique_ptr<float[]> awgn_channel_EbN0_dB(std::mt19937 &gen, float EbN0_dB, float timing_offset, const float *samples, int nxd, int &ni)
{
    // see https://www.mathworks.com/help/comm/ug/awgn-channel.html
    // in our case, Eb == Es, since we have one bit per symbol
    const float SNR_dB = EbN0_dB - 10.*log10(0.5*SAMPLES_PER_SYMBOL);

    const float S_dB = 10. * log10(signal_avg_power(samples, nxd));
    const float N_dB = S_dB - SNR_dB;
    const float N = pow(10., N_dB/10.);

    return awgn_channel(gen, sqrt(N), timing_offset, samples, nxd, ni);
}

std::unique_ptr<unsigned int[]> bs_transition_channel(std::mt19937 &gen, float flip_probability, int samples_affected_on_transition, float timing_offset, const unsigned int *samples, int nxd, int &ni)
{
    std::unique_ptr<float[]> yd (new float[nxd]);

    std::uniform_real_distribution<> d {0., 1.};
    unsigned int previous_sample = samples[0];
    
    for (int i = 0; i < nxd; i++) {
        yd[i] = samples[i];
        if (samples[i] != previous_sample) {
            // transition, apply BSC model
            const int s = std::max(0, i - samples_affected_on_transition/2);
            const int e = std::min(i + samples_affected_on_transition/2, nxd);
            for (int j = s; j < e; j++) {
                if (d(gen) < flip_probability) {
                    yd[j] = !samples[j];
                }
                else {
                    yd[j] = samples[j];
                }
            }
            i = e - 1;
        }
        previous_sample = samples[i];
    }

    auto yi = apply_timing_offset(timing_offset, yd, nxd, ni);

    std::unique_ptr<unsigned int[]> res (new unsigned int[ni]);
    for (int i = 0; i < ni; i++) {
        res[i] = yi[i] > 0.5;
    }
    return res;
}
