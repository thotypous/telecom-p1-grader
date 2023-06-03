#include <math.h>
#include "mlinterp.hpp"
#include "channel.hpp"
#include "config.hpp"

std::unique_ptr<float[]> channel_model(std::mt19937 &gen, float noise_amplitude, float timing_offset, const float *samples, int nxd, int &ni) {
    std::normal_distribution<> d {0, noise_amplitude};

    std::unique_ptr<float[]> yd (new float[nxd]);
    for (int i = 0; i < nxd; i++) {
        yd[i] = samples[i] + d(gen);
    }

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

float signal_avg_power(const float *samples, int n) {
    float res = 0.;
    for (int i = 0; i < n; i++) {
        res += (samples[i]*samples[i])/n;
    }
    return res;
}

std::unique_ptr<float[]> channel_model_EbN0_dB(std::mt19937 &gen, float EbN0_dB, float timing_offset, const float *samples, int nxd, int &ni) {
    // see https://www.mathworks.com/help/comm/ug/awgn-channel.html
    // in our case, Eb == Es, since we have one bit per symbol
    const float SNR_dB = EbN0_dB - 10.*log10(0.5*SAMPLES_PER_SYMBOL);

    const float S_dB = 10. * log10(signal_avg_power(samples, nxd));
    const float N_dB = S_dB - SNR_dB;
    const float N = pow(10., N_dB/10.);

    return channel_model(gen, sqrt(N), timing_offset, samples, nxd, ni);
}
