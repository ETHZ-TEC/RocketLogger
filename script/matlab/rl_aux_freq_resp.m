function [ f_dec, data_db ] = rl_aux_freq_resp( data, dcValue, fig )
%RL_AUX_FREQ_RESP Plot the frequncy response for the measurement of a 
%logarithmic frequency sweep from 10 Hz to 100 kHz
%   Parameters:
%      - data:      Samples for a single sweep
%      - dcValue:   The expected value at DC for the sweep
%      - fig:       (optional) Figure to plot in
%   Return values:
%      - f_dec:     Frequency bins
%      - data_db:   Attenuation for each frequency bin (f_dec)

% Constants
decades = 4;
start_f = 10;
decimation_factor = 100;

% Calculate envelope, convert to dB
t = 1:length(data);
f = start_f*10.^(decades/length(t).*t);
f_dec = decimate(f, decimation_factor);
data_env = envelope(data);
data_dec = decimate(data_env, decimation_factor);
data_db = 20*log10(data_dec/dcValue);

if ~exist('fig', 'var')
    fig = figure;
else
    figure(fig);
end

% Create plot
semilogx(f_dec, data_db);
axis([100 30e3 -21 3])
ax = gca;
ax.YTick = -300:3:30;
grid on;

xlabel('Frequency [Hz]');
ylabel('Attenuation [dB]');
rl_aux_pretty_plot(fig);

end

