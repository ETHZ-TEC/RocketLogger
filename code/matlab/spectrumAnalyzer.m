function [ fBin, psdBin ] = spectrumAnalyzer( x, fsamp )
%SPECTRUMANALYZER Summary of this function goes here
%   x: input samples
%   fsamp: sampling frequency [Hz]
%   fBin: frequency bin center frequency, BW: 1 MHz (+/- 500 kHz)
%   psdBin: PSD of this bin [dB/MHz]

sampleCount = length(x);
fBin = 3.5e9:500e3:4.5e9;
%fBin = 3.5e9:500e5:4.5e9;
psdBin = zeros(length(fBin), 1);
bw = 1e6;
integrationTime = 1e-3;
%dutyCycle = sampleCount*fsamp/integrationTime;

% Band-Pass filter the signal
for i = 1:length(fBin);
    i = 100;
    disp(int2str(i));
    
    fCenter = fBin(i);
    xFiltered = bandPass(x, fsamp, fCenter, bw);
    assert(sum(imag(xFiltered) ~= 0) == 0, 'Invalid filter!');
    
    envelopeSq = (hilbert(xFiltered).^2 + xFiltered.^2)/2;
    
    figure;
    plot(xFiltered);
    hold on;
    plot(sqrt(abs(envelopeSq)));
    averagePower = sum(abs(envelopeSq)) / (fsamp * integrationTime);
    psdBin(i) = 10*log10(averagePower);
end

f = figure; 
plot(fBin, psdBin)
prettyPlot(f);

end

