#include <iostream>
#include <math.h>
#include <gtest/gtest.h>
#include <memory>
#include <numbers>
#include "config.hpp"
#include "uart.hpp"
#include "channel.hpp"

using namespace std;

TEST(test, lol) {
    const int n = 8*SAMPLING_RATE;
    std::unique_ptr<float[]> samples (new float[n]);
    for (int i = 0; i < n; i++) {
        samples[i] = 44.721359549995796*sin(2.*std::numbers::pi*1180.*i/48000.);
    }
    std::mt19937 gen {42};
    int ni;
    std::unique_ptr<float[]> out_samples = channel_model_EbN0_dB(gen, 2.228787452803376, 1.00, samples.get(), n, ni);
    cout << scientific << setprecision(8) << showpos;
    for (int i = 0; i < ni; i++) {
        cout << ((float)i)/48000. << "\t" << out_samples[i] << endl;
    }
}
