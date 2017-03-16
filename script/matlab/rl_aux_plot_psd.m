%%
%% Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
%%

function psd = rl_aux_plot_psd(samples, f_samp)
%RL_AUX_PLOT_PSD Plot the spectrum of a measured signal (voltage/current)
%   rl_aux_plot_psd(samples, f_samp)
%   Parameters:
%      - samples:    Samples
%      - f_samp:     Sampling Frequency [Hz]

sample_count = length(samples);
df = f_samp / sample_count;
f_orig = df * (0:1:sample_count-1);
f = fftshift(f_orig);
f(f >= f_samp / 2) = f(f >= f_samp / 2) - f_samp;
psd = abs(fftshift(fft(samples))) .^ 2 * (1 / (f_samp * sample_count));
psd_db = 10 * log10(psd);

plot(f, psd_db);
xlabel('Frequency [Hz]');
ylabel('PSD [dB/Hz]');

end
