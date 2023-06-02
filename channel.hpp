#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <random>

void channel_model(std::mt19937 &gen, float noise_amplitude, float timing_offset, float *samples, int n);
float signal_avg_power(float *samples, int n);
void channel_model_EbN0_dB(std::mt19937 &gen, float EbN0_dB, float timing_offset, float *samples, int n);

#endif
