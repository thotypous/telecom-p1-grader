#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <random>
#include <memory>

std::unique_ptr<float[]> channel_model(std::mt19937 &gen, float noise_amplitude, float timing_offset, const float *samples, int nxd, int &ni);
float signal_avg_power(const float *samples, int n);
std::unique_ptr<float[]> channel_model_EbN0_dB(std::mt19937 &gen, float EbN0_dB, float timing_offset, const float *samples, int nxd, int &ni);

#endif
