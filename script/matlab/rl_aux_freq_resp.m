%%
%% Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
%% All rights reserved.
%% 
%% Redistribution and use in source and binary forms, with or without
%% modification, are permitted provided that the following conditions are met:
%% 
%% * Redistributions of source code must retain the above copyright notice, this
%%   list of conditions and the following disclaimer.
%% 
%% * Redistributions in binary form must reproduce the above copyright notice,
%%   this list of conditions and the following disclaimer in the documentation
%%   and/or other materials provided with the distribution.
%% 
%% * Neither the name of the copyright holder nor the names of its
%%   contributors may be used to endorse or promote products derived from
%%   this software without specific prior written permission.
%% 
%% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
%% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
%% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
%% DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
%% FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
%% DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
%% SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
%% CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
%% OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
%% OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
%%

function [f_dec, data_db] = rl_aux_freq_resp(data, dcValue, fig)
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
f = start_f * 10 .^ (decades / length(t) .* t);
f_dec = decimate(f, decimation_factor);
data_env = envelope(data);
data_dec = decimate(data_env, decimation_factor);
data_db = 20 * log10(data_dec / dcValue);

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
