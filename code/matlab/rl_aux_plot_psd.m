function [ psd ] = rl_aux_plot_psd( samples, f_samp )
%PLOTPSD Plot the spectrum of a measured signal (voltage/current)
%   Parameters:
%      - samples:    Samples
%      - fsamp:      Sampling Frequency [Hz]
sample_count = length(samples);
df = f_samp/sample_count;
f_orig = df * (0:1:sample_count-1);
f = fftshift(f_orig);
f(f >= f_samp/2) = f(f >= f_samp/2) - f_samp;
psd = abs(fftshift(fft(samples))).^2 * (1/(f_samp*sample_count));
psd_db = 10*log10(psd);

plot(f, psd_db);
xlabel('Frequency [Hz]');
ylabel('PSD [dB/Hz]');
end

