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

function [ intervals ] = rl_aux_get_intervals( data , num_points, ...
    expected_step_size, min_stable_samples)
%RL_AUX_GET_INTERVALS Finds stable points in a measurement, and returns the
%start and end points of these intervals
%   Parameters:
%       - data:                    Input waveform samples
%       - num_points:              Number of levels that are expected
%       - expected_step_size:      Expected step between the levels
%       - min_stable_points:       Minimum number of stable samples per
%                                    level
%   Return Values:
%      - intervals:                [2xn] array with start and end points of
%                                    stable intervals
%   See also RL_AUX_AVERAGE_POINTS

% magic constants
margin = 20;
step_size = expected_step_size * 0.2;
stable_pp_max = expected_step_size * 0.05;

intervals = zeros(2, num_points);

%% find starting point
i = 1;
v0 = data(i);
v1 = data(i);
while(abs(v0 - v1) < step_size)
    i = i + 1;
    v0 = data(i);
end

%% find all transitions
j = 1;
i = i + margin;

while (i <= length(data) && j <= num_points)
    
    % find stable region
    while(i + min_stable_samples <= length(data) && max(data(i:i+min_stable_samples)) - ...
            min(data(i:i+min_stable_samples)) > stable_pp_max)
        i = i + 1;
    end
    i = i + margin;
    if i + min_stable_samples > length(data)
        break;
    end
    intervals(1, j) = i;
    stable_value = mean(data(i:i+min_stable_samples));
    
    % find end of stable region
    while(i <= length(data) && abs(data(i) - stable_value) < stable_pp_max / 2)
        i = i + 1;
    end
    intervals(2, j) = i - margin;
    j = j + 1;
    
    % find transition
    while(i <= length(data) && abs(data(i) - stable_value) < step_size)
        i = i + 1;
    end
end

% Debug Plot:
%
% figure;
% plot(data);
% y0 = min(data);
% y1 = max(data);
% for i = 1:size(intervals, 2)
%     line([intervals(1, i) intervals(1, i)], [y0 y1], 'LineWidth', 1, 'Color', 'Black');
%     line([intervals(2, i) intervals(2, i)], [y0 y1], 'LineWidth', 1, 'Color', 'Green');
% end

assert(j == num_points + 1, 'Could not find enough valid regions in the provided data. (%i/%i)',  ...
    j - 1, num_points);

end
