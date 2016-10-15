function [ intervals ] = get_intervals( points , num_points, step_size)
global failCount;

margin = 10;
intervals = zeros(2,num_points);

% find starting point
i=1;
v0 = points(i);
v1 = points(i);
while(abs(v0-v1) < step_size) % larger first step ( to avoid noise triggering )
%TODO: why is num_points used here?! while(abs(v0-v1) < num_points/10 * step_size) % larger first step ( to avoid noise triggering )
    i = i+1;
    v0 = points(i);
end
intervals(1,1) = i + margin;

% find all transitions
for j=1:num_points-1
    i = intervals(1,j) + margin;
    v0 = points(i);
    v1 = points(i);
    while(abs(v0-v1) < step_size)
        i = i+1;
        assert(i <= length(points), ['One of the input waveforms does not have enough transitions: ', num2str(j)]);
        v1 = points(i);
    end

    intervals(2,j) = i - margin;
    intervals(1,j+1) = i + margin;
end

% find end point
j = j+1;
i = intervals(1,j) + margin;
v0 = points(i);
v1 = points(i);
while(abs(v0-v1) < step_size)
    i = i+1;
    v1 = points(i); % IF YOU GET AN ERROR HERE: you may probably have chosen wrong values on the SMU
end

intervals(2,j) = i - margin;

% sanity check
for i=1:num_points
    var = abs(min(points(intervals(1,i):intervals(2,i))) - max(points(intervals(1,i):intervals(2,i))));
    if var > step_size/3
        figure;
        plot(points(intervals(1,i):intervals(2,i)));
        disp(['Warning: Sanity check failed. Large variation on interval ', int2str(i), ': x = [', int2str(intervals(1,i)),',', int2str(intervals(2,i)),'].']);
        disp(['   Variation is: ' , int2str(var)]);
        
        % avoid 200 plots
        failCount = failCount + 1;
        assert(failCount < 10, 'To many sanity checks failed');
    end
end

end

