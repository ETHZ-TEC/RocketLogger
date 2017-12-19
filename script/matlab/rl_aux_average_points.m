%%
%% Copyright (c) 2016-2017, Swiss Federal Institute of Technology (ETH Zurich)
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

function avg_points = rl_aux_average_points(data, num_points, ...
    expceted_step_size, min_stable_samples)
%RL_AUX_AVERAGE_POINTS Find stable points in a measurement using
%rl_aux_get_intervals and calculate the average at each step
%   Parameters:
%       - data:                    Input waveform samples
%       - num_points:              Number of levels that are expected
%       - expected_step_size:      Expected step between the levels
%       - min_stable_points:       Minimum number of stable samples per
%                                    level
%   Return Values:
%      - avg_points:               Averaged values of the stable levels
%   See also RL_AUX_GET_INTERVALS

% find intervals
intervals = rl_aux_get_intervals(data, num_points, expceted_step_size, min_stable_samples);

num_points = length(intervals);
avg_points = zeros(1, num_points);

% calculate averages
for i=1:num_points
    avg_points(i) = mean(data(intervals(1, i):intervals(2, i)));
end

end
