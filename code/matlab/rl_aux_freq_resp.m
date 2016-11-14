function [ fDec, dataDB ] = rl_aux_freq_resp( data, dcValue, fig )
%RL_AUX_FREQ_RESP Plot the frequncy response for the measurement of a 
%logarithmic frequency sweep from 10 Hz to 100 kHz
%   Parameters:
%      - data:      Samples for a single sweep
%      - dcValue:   The expected value at DC for the sweep
%      - fig:       (optional) Figure to plot in

% Constants
decades = 4;
startF = 10;
decimationFactor = 100;

% Calculate envelope, convert to dB
t = 1:length(data);
f = startF*10.^(decades/length(t).*t);
fDec = decimate(f, decimationFactor);
dataEnv = envelope(data);
dataDec = decimate(dataEnv, decimationFactor);
dataDB = 20*log10(dataDec/dcValue);

if ~exist('fig', 'var')
    fig = figure;
else
    figure(fig);
end

% Create plot
semilogx(fDec, dataDB);
axis([100 30e3 -21 3])
ax = gca;
ax.YTick = -300:3:30;
grid on;

xlabel('Frequency [Hz]');
ylabel('Attenuation [dB]');
rl_aux_pretty_plot(fig);

end

