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
    UART_TX uart_tx;
    const char msg[] = "Hello";
    for (int i = 0; i < sizeof(msg); i++) {
        uart_tx.put_byte(msg[i]);
    }

    int n = SAMPLES_PER_SYMBOL * 10 * sizeof(msg);
    unsigned int digital_samples[n];
    uart_tx.get_samples(digital_samples, n);

    std::mt19937 gen {42};
    int ni;
    std::unique_ptr<unsigned int[]> out_samples = bs_transition_channel(gen, .5, SAMPLES_PER_SYMBOL/2, 1.02, digital_samples, n, ni);

    for (int i = 0; i < ni; i++) {
        cout << out_samples[i] << endl;
    }
}
