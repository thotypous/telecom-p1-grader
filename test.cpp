#include <iostream>
#include <math.h>
#include <gtest/gtest.h>
#include <memory>
#include <numbers>
#include <random>
#include "config.hpp"
#include "uart.hpp"
#include "channel.hpp"

using namespace std;

static void test_uart(bool add_noise, bool add_timing_offset)
{
    std::deque<uint8_t> received_bytes;

    UART_TX uart_tx;
    UART_RX uart_rx([&received_bytes](uint8_t b){ received_bytes.push_back(b); });

    std::mt19937 gen {42};
    std::uniform_int_distribution<> d_idle_samples {0, 2*SAMPLES_PER_SYMBOL};
    std::uniform_int_distribution<> d_msg_bytes {1, 100};
    std::uniform_int_distribution<> d_byte {0, 255};
    std::uniform_real_distribution<> d_timing_offset {0.98f, 1.02f};

    for (int iteration = 0; iteration < 100; iteration++) {
        const int idle_samples = d_idle_samples(gen);
        const int msg_bytes = d_msg_bytes(gen);
        const int msg_samples = 10 * SAMPLES_PER_SYMBOL * (msg_bytes + 1);
        const int n = idle_samples + msg_samples;

        unsigned int transmitted_samples[n];
        uart_tx.get_samples(transmitted_samples, idle_samples);

        uint8_t orig_msg[msg_bytes];
        for (int i = 0; i < n; i++) {
            orig_msg[i] = d_byte(gen);
            uart_tx.put_byte(orig_msg[i]);
        }
        uart_tx.get_samples(&transmitted_samples[idle_samples], msg_samples);

        int ni;
        auto received_samples = bs_transition_channel(gen, 
                add_noise ? .5 : 0.,
                add_timing_offset ? SAMPLES_PER_SYMBOL/2 : 0, 
                add_timing_offset ? d_timing_offset(gen) : 1.,
                transmitted_samples,
                n, ni);

        std::uniform_int_distribution<> d_cut {1, ni-1};
        int cut = d_cut(gen);
        uart_rx.put_samples(received_samples.get(), cut);
        uart_rx.put_samples(&received_samples.get()[cut], ni-cut);

        ASSERT_EQ(received_bytes.size(), msg_bytes) << "on iteration " << iteration;

        for (int i = 0; i < msg_bytes; i++) {
            ASSERT_EQ(orig_msg[i], received_bytes.front()) << "on iteration " << iteration << ", i=" << i;
            received_bytes.pop_front();
        }
    }
}

TEST(uart, trivial)
{
    test_uart(false, false);
}

TEST(uart, sync)
{
    test_uart(false, true);
}

TEST(uart, noisy)
{
    test_uart(true, false);
}

TEST(uart, noisy_sync)
{
    test_uart(true, true);
}
