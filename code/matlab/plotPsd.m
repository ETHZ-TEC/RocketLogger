function [ psd ] = plotPsd( samples, fsamp )
%PLOTPSD Summary of this function goes here
%   Detailed explanation goes here
sampleCount = length(samples);
df = fsamp/sampleCount;
fOrig = df * (0:1:sampleCount-1);
f = fftshift(fOrig);
f(f >= fsamp/2) = f(f >= fsamp/2) - fsamp;
psd = abs(fftshift(fft(samples))).^2 * (1/(fsamp*sampleCount));
psdDb = 10*log10(psd);

plot(f, psdDb);
xlabel('Frequency [Hz]');
ylabel('Power [dB/Hz]');
end

