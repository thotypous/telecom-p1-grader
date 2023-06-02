#include <iostream>
#include <math.h>
#include <gtest/gtest.h>
#include <numbers>
#include "uart.hpp"
#include "channel.hpp"

using namespace std;

TEST(test, lol) {
    float samples[48000];
    for (int i = 0; i < 48000; i++) {
        samples[i] = sin(2.*std::numbers::pi*1180.*i/48000.);
    }
    channel_model_EbN0_dB(24, 0.99, samples, 48000);
    cout << scientific << setprecision(8) << showpos;
    for (int i = 0; i < 48000; i++) {
        cout << ((float)i)/48000. << "\t" << samples[i] << endl;
    }
}
