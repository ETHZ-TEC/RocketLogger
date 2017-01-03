function [ values_out, range ] = rl_merge_currents_old(  values, header )

channels = header(4);

number_samples = length(values(:,1));

% low range valid filter
margin = 3;
filter = ones(1, 2*margin + 1);

%% collapse currents
col_in = 3;
col_out = 1;

range1 = nan(number_samples, 1);
range2 = nan(number_samples, 1);

%% determine i1 range (0: low, 1: med, 2 high)
if rl_channel_on(channels,0) || rl_channel_on(channels,1) || rl_channel_on(channels,2)
    
    % i1h
    if rl_channel_on(channels, 0)
        range1(:) = 2;
        col_in = col_in + 1;
    end
    
    % i1m
    if rl_channel_on(channels, 1)
        range1 = range1 - ( abs(values(:, col_in)) < 0.04); % switch at 40mA
        col_in = col_in + 1;
    end
    
    % i1l
    if rl_channel_on(channels, 2)
        % filter low range valid (to have a margin)
        low1 = conv(values(:,1),filter);
        values(:,1) = ~(low1(margin+1:end-margin) == 2*margin + 1);
        range1 = range1 .* values(:,1); % range = 0, when low range active
        col_in = col_in + 1;
    end
    
    % select value
    for i=1:number_samples
        values_out(i,col_out) = values(i,col_in - 1 - range1(i));
    end
    col_out = col_out + 1;
    
    
end

%% v1,2
if rl_channel_on(channels,3)
    values_out(:,col_out) = values(:, col_in);
    col_in = col_in + 1;
    col_out = col_out + 1;
end
if rl_channel_on(channels,4)
    values_out(:,col_out) = values(:, col_in);
    col_in = col_in + 1;
    col_out = col_out + 1;
end

%% determine i2 range (0: low, 1: med, 2 high)
if rl_channel_on(channels,5) || rl_channel_on(channels,6) || rl_channel_on(channels,7)
    
    % i2h
    if rl_channel_on(channels, 5)
        range2(:) = 2;
        col_in = col_in + 1;
    end
    
    % i2m
    if rl_channel_on(channels, 6)
        range2 = range2 - ( abs(values(:, col_in)) < 0.04); % switch at 40mA
        col_in = col_in + 1;
    end
    
    % i2l
    if rl_channel_on(channels, 7)
        % filter low range valid (to have a margin)
        low2 = conv(values(:,2),filter);
        values(:,2) = ~(low2(margin+1:end-margin) == 2*margin + 1);
        range2 = range2 .* values(:,2); % range = 0, when low range active
        col_in = col_in + 1;
    end
    
    % select value
    for i=1:number_samples
        values_out(i,col_out) = values(i,col_in - 1 - range2(i));
    end
    col_out = col_out + 1;
    
    
end

%% v3,4
if rl_channel_on(channels,8)
    values_out(:,col_out) = values(:,col_in);
    col_in = col_in + 1;
    col_out = col_out+1;
end
if rl_channel_on(channels,9)
    values_out(:,col_out) = values(:,col_in);
    col_in = col_in + 1;
    col_out = col_out+1;
end

range = [range1,range2];

end
