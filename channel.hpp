#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <random>
#include <memory>

std::unique_ptr<float[]> awgn_channel(std::mt19937 &gen, float noise_amplitude, float timing_offset, const float *samples, int nxd, int &ni);
float signal_avg_power(const float *samples, int n);
std::unique_ptr<float[]> awgn_channel_EbN0_dB(std::mt19937 &gen, float EbN0_dB, float timing_offset, const float *samples, int nxd, int &ni);
std::unique_ptr<unsigned int[]> bs_transition_channel(std::mt19937 &gen, float flip_probability, int samples_affected_on_transition, float timing_offset, const unsigned int *samples, int nxd, int &ni);

#endif
