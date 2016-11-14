function [ avg_points ] = rl_aux_average_points( points, num_points, ...
    expceted_step_size, min_stable_points)

intervals = rl_aux_get_intervals( points, num_points, expceted_step_size, min_stable_points );

num_points = length(intervals);
avg_points = zeros(1,num_points);

for i=1:num_points
    avg_points(i) = mean(points(intervals(1,i):intervals(2,i)));
end
end

