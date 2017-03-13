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
