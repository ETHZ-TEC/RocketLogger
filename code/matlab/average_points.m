function [ avg_points ] = average_points( points, num_points, step_size)

intervals = get_intervals( points, num_points, step_size );

num_points = length(intervals);
avg_points = zeros(1,num_points);

for i=1:num_points
    avg_points(i) = mean(points(intervals(1,i):intervals(2,i)));
end
end

