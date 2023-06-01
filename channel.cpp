#include <random>
#include <math.h>
#include "mlinterp.hpp"
#include "channel.hpp"


void channel_model(float noise_avg_amplitude, float timing_offset, float *samples, int n) {
    std::random_device rd {};
    std::mt19937 gen {rd()};
    std::normal_distribution<> d {0, sqrt(noise_avg_amplitude)};

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

float signal_avg_watts(float *samples, int n) {
    float res = 0.;
    for (int i = 0; i < n; i++) {
        res += (samples[i]*samples[i])/n;
    }
    return res;
}

void channel_model_snr_db(float target_snr_db, float timing_offset, float *samples, int n) {
    float sig_avg_db = 10. * log10(signal_avg_watts(samples, n));
    float noise_avg_db = sig_avg_db - target_snr_db;
    float noise_avg_watts = pow(10., noise_avg_db/10.);
    channel_model(sqrt(noise_avg_watts), timing_offset, samples, n);
}
