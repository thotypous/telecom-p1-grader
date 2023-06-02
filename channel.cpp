#include <random>
#include <math.h>
#include "mlinterp.hpp"
#include "channel.hpp"
#include "config.hpp"


void channel_model(float noise_amplitude, float timing_offset, float *samples, int n) {
    std::random_device rd {};
    std::mt19937 gen {rd()};
    std::normal_distribution<> d {0, noise_amplitude};

    float *noisy = new float[n];
    for (int i = 0; i < n; i++) {
        noisy[i] = samples[i] + d(gen);
    }
    
    float *xd = new float[n];
    float *xi = new float[n];

    for (int i = 0; i < n; i++) {
        xd[i] = ((float)i)/n;
        xi[i] = timing_offset * xd[i];
    }

    const int nd[] = { n };

    mlinterp::interp(
            nd, n,
            noisy, samples,
            xd, xi
            );

    delete [] noisy;
    delete [] xd;
    delete [] xi;
}

float signal_avg_power(float *samples, int n) {
    float res = 0.;
    for (int i = 0; i < n; i++) {
        res += (samples[i]*samples[i])/n;
    }
    return res;
}

void channel_model_EbN0_dB(float EbN0_dB, float timing_offset, float *samples, int n) {
    // see https://www.mathworks.com/help/comm/ug/awgn-channel.html
    // in our case, Eb == Es, since we have one bit per symbol
    const float SNR_dB = EbN0_dB - 10.*log10((float)SAMPLES_PER_SYMBOL);

    const float S_dB = 10. * log10(signal_avg_power(samples, n));
    const float N_dB = S_dB - SNR_dB;
    const float N = pow(10., N_dB/10.);

    channel_model(sqrt(N), timing_offset, samples, n);
}
