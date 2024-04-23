#include <cstdio>
#include <iostream>
#include <math.h>
#include <gtest/gtest.h>
#include <memory>
#include <numbers>
#include <random>
#include "config.hpp"
#include "uart.hpp"
#include "channel.hpp"
#include "v21.hpp"

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
        const int msg_samples = 10 * SAMPLES_PER_SYMBOL * msg_bytes;
        const int n = idle_samples + msg_samples;

        unsigned int transmitted_samples[n];
        uart_tx.get_samples(transmitted_samples, idle_samples);

        uint8_t orig_msg[msg_bytes];
        for (int i = 0; i < msg_bytes; i++) {
            orig_msg[i] = d_byte(gen);
            uart_tx.put_byte(orig_msg[i]);
        }
        uart_tx.get_samples(&transmitted_samples[idle_samples], msg_samples);

        int ni;
        auto received_samples = bs_transition_channel(gen, 
                add_noise ? .5 : 0.,
                add_noise ? SAMPLES_PER_SYMBOL/4 : 0,
                add_timing_offset ? d_timing_offset(gen) : 1.,
                transmitted_samples,
                n, ni);

        std::uniform_int_distribution<> d_cut {1, ni-1};
        const int cut = d_cut(gen);
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

TEST(uart, unsync)
{
    test_uart(false, true);
}

TEST(uart, noisy)
{
    test_uart(true, false);
}

TEST(uart, noisy_unsync)
{
    test_uart(true, true);
}

static float compute_v21_ber_on_direction(bool tx_call, float EbN0_dB, bool add_timing_offset)
{
    std::deque<uint8_t> received_bytes;

    float tx_omega0, tx_omega1;
    if (tx_call) {
        tx_omega0 = 2*std::numbers::pi*(1080 + 100);
        tx_omega1 = 2*std::numbers::pi*(1080 - 100);
    }
    else {
        tx_omega0 = 2*std::numbers::pi*(1750 + 100);
        tx_omega1 = 2*std::numbers::pi*(1750 - 100);
    }

    std::mt19937 gen {42};
    std::uniform_int_distribution<> d_idle_samples {2*SAMPLES_PER_SYMBOL, 4*SAMPLES_PER_SYMBOL};
    std::uniform_int_distribution<> d_msg_bytes {1, 100};
    std::uniform_int_distribution<> d_byte {0, 255};
    std::uniform_real_distribution<> d_timing_offset {0.98f, 1.02f};

    float mean_ber = 0.;
    constexpr int num_iterations = 50;

    for (int iteration = 0; iteration < num_iterations; iteration++) {
        UART_TX uart_tx;
        UART_RX uart_rx([&received_bytes](uint8_t b){ received_bytes.push_back(b); });
        V21_RX v21_rx(tx_omega1, tx_omega0, [&uart_rx](const unsigned int *s, unsigned int n){ uart_rx.put_samples(s, n); });
        V21_TX v21_tx(tx_omega1, tx_omega0);

        const int idle_samples = d_idle_samples(gen);
        constexpr int idle_end = 2*SAMPLES_PER_SYMBOL;
        const int msg_bytes = d_msg_bytes(gen);
        const int msg_samples = 10 * SAMPLES_PER_SYMBOL * msg_bytes;
        const int n = idle_samples + msg_samples + idle_end;

        unsigned int digital_buffer[n];
        float transmitted_samples[n];
        uart_tx.get_samples(digital_buffer, idle_samples);

        uint8_t orig_msg[msg_bytes];
        for (int i = 0; i < msg_bytes; i++) {
            orig_msg[i] = d_byte(gen);
            uart_tx.put_byte(orig_msg[i]);
        }
        uart_tx.get_samples(&digital_buffer[idle_samples], msg_samples);
        uart_tx.get_samples(&digital_buffer[idle_samples+msg_samples], idle_end);
        v21_tx.modulate(digital_buffer, transmitted_samples, n);

        int ni;
        auto received_samples = awgn_channel_EbN0_dB(gen,
                EbN0_dB,
                add_timing_offset ? d_timing_offset(gen) : 1.,
                transmitted_samples,
                n, ni);

        std::uniform_int_distribution<> d_cut {1, ni-1};
        const int cut = d_cut(gen);
        v21_rx.demodulate(received_samples.get(), cut);
        v21_rx.demodulate(&received_samples.get()[cut], ni-cut);

        int bit_errors = 0;
        const int max_size = std::max((int)received_bytes.size(), msg_bytes);
        for (int i = 0; i < max_size; i++) {
            const uint8_t a = !received_bytes.empty() ? received_bytes.front() : 0;
            const uint8_t b = (i < msg_bytes) ? orig_msg[i] : 0;
            uint8_t diff = a ^ b;
            if (!received_bytes.empty())
                received_bytes.pop_front();
            while (diff != 0) {
                bit_errors += diff & 1;
                diff >>= 1;
            }
        }

        const float ber = ((float)bit_errors)/(8.*max_size);
        mean_ber += ber / num_iterations;
    }

    return mean_ber;
}

static float compute_v21_ber(float EbN0_dB, bool add_timing_offset)
{
    return (compute_v21_ber_on_direction(true,  EbN0_dB, add_timing_offset) +
            compute_v21_ber_on_direction(false, EbN0_dB, add_timing_offset))*.5;
}

static void test_v21(bool add_timing_offset)
{
    constexpr int max_EbN0_dB = 20;
    float ber_EbN0_dB[max_EbN0_dB];
    for (int EbN0_dB = 0; EbN0_dB < max_EbN0_dB; EbN0_dB++) {
        const float ber = compute_v21_ber(EbN0_dB, add_timing_offset);
        std::cout << "EbN0 = " << EbN0_dB << " dB, BER = " << ber << std::endl;
        ber_EbN0_dB[EbN0_dB] = ber;
    }
    ASSERT_LE(ber_EbN0_dB[10], 1e-1);
    ASSERT_LE(ber_EbN0_dB[13], 1e-2);
    ASSERT_LE(ber_EbN0_dB[16], 1e-3);
    ASSERT_LE(ber_EbN0_dB[19], 1e-5);
}

TEST(v21, sync)
{
    test_v21(false);
}

TEST(v21, unsync)
{
    test_v21(true);
}
