function [ xFiltered ] = bandPass( x, fsamp, center, bw )
%BANDPASS Summary of this function goes here
%   Detailed explanation goes here

figure;
plotPsd(x, fsamp);
hold on;

sampleCount = length(x);
df = fsamp/sampleCount;
fOrig = df * (0:1:sampleCount-1);
fOrig(fOrig >= fsamp/2) = fOrig(fOrig >= fsamp/2) - fsamp;

xf = fft(x);
xfFiltered = xf;
xfFiltered( abs(abs(fOrig)-center) > bw/2) = 0;

xFiltered = real(ifft(xfFiltered));

plotPsd(xFiltered, fsamp);

end

