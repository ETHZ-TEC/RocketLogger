function [ fDec, dataDB ] = freq_resp( data, dcValue, fig )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here

decades = 4;
startF = 10;
decimationFactor = 100;

t = 1:length(data);
f = startF*10.^(decades/length(t).*t);
fDec = decimate(f, decimationFactor);
dataEnv = envelope(data);
dataDec = decimate(dataEnv, decimationFactor);
dataDB = 20*log10(dataDec/dcValue);

if ~exist('fig', 'var')
    fig = figure;
end
semilogx(fDec, dataDB);
axis([100 30e3 -21 3])
ax = gca;
ax.YTick = -300:3:30;
grid on;

xlabel('Frequency [Hz]');
ylabel('Attenuation [dB]');
prettyPlot(fig);

end

