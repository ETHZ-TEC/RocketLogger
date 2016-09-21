function [ xFiltered ] = bandStop( x, fsamp, center, bw )
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

selectRemove = zeros(1, length(fOrig));
for i=1:length(center)
    selectRemove( abs(abs(fOrig)-center(i)) < bw(i)/2) = 1;
end

xfFiltered(selectRemove == 1) = 0;

xFiltered = real(ifft(xfFiltered));

plotPsd(xFiltered, fsamp);

end

