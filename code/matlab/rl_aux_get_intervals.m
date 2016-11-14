function [ intervals ] = rl_aux_get_intervals( points , num_points, expected_step_size, min_stable_points)

% magic constants
margin = 20;
step_size = expected_step_size * 0.2;
stable_pp_max = expected_step_size * 0.05;

intervals = zeros(2,num_points);

%% find starting point
i=1;
v0 = points(i);
v1 = points(i);
while(abs(v0-v1) < step_size)
    i = i+1;
    v0 = points(i);
end

%% find all transitions
j = 1;
i = i + margin;

while (i <= length(points) && j <= num_points)
    
    % find stable region
    while( i+min_stable_points <= length(points) && max(points(i:i+min_stable_points)) - ...
            min(points(i:i+min_stable_points)) > stable_pp_max)
        i = i + 1;
    end
    i = i + margin;
    if i+min_stable_points > length(points)
        break;
    end
    intervals(1, j) = i;
    stable_value = mean(points(i:i+min_stable_points));
    
    % find end of stable region
    while(i <= length(points) && abs(points(i) - stable_value) < stable_pp_max/2)
        i = i+1;
    end
    intervals(2,j) = i - margin;
    j = j + 1;
    
    % find transition
    while(i <= length(points) && abs(points(i) - stable_value) < step_size)
        i = i+1;
    end
end

% Debug Plot:
%
% figure;
% plot(points);
% y0 = min(points);
% y1 = max(points);
% for i = 1:size(intervals, 2)
%     line([intervals(1,i) intervals(1,i)],[y0 y1],'LineWidth',1, 'Color', 'Black');
%     line([intervals(2,i) intervals(2,i)],[y0 y1],'LineWidth',1, 'Color', 'Green');
% end

assert(j == num_points + 1, ['Could not find enough valid regions in the provided data. (' ...
    ,num2str(j-1),' / ', num2str(num_points),')']);

end

