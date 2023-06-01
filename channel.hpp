#ifndef CHANNEL_HPP
#define CHANNEL_HPP

void channel_model(float noise_avg_amplitude, float timing_offset, float *samples, int n);
float signal_avg_watts(float *samples, int n);
void channel_model_snr_db(float target_snr_db, float timing_offset, float *samples, int n);

#endif
