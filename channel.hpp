#ifndef CHANNEL_HPP
#define CHANNEL_HPP

void channel_model(float noise_avg_amplitude, float timing_offset, float *samples, int n);
float signal_avg_power(float *samples, int n);
void channel_model_EbN0_dB(float EbN0_dB, float timing_offset, float *samples, int n);

#endif
