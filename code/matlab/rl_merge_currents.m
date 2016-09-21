function [ values_out, range ] = rl_merge_currents(  values, header )

channels = header(4);

number_samples = length(values(:,1));

% check, if medium channel on -> stop
assert(~(rl_channel_on(channels,1) || rl_channel_on(channels, 6)), 'Error: medium current channel data found. Use rl_merge_currents_old.');

% low range valid filter
margin = 3;
filter = ones(1, 2*margin + 1);

%% collapse currents
col_in = 3;
col_out = 1;

range1 = nan(number_samples, 1);
range2 = nan(number_samples, 1);

%% i1

% both channels on
if rl_channel_on(channels,0) && rl_channel_on(channels,2)
    % filter low range valid (to have margin)
    range1 = double(~values(:,1)); % invert
    range1 = conv(range1, filter) > 0.5; % filter
    range1 = range1(margin+1:end-margin); % resize
    
    % select values
    for i=1:number_samples
        values_out(i,col_out) = values(i, col_in + 1 - range1(i));
    end
        
    % update current column
    col_in = col_in + 2;
    col_out = col_out + 1;
    
% only high range on
elseif rl_channel_on(channels,0)
    % always high range
    range1 = zeros(number_samples,1);
    
    % select values
    values_out(:,col_out) = values(:, col_in);
    
    % update current column
    col_in = col_in + 1;
    col_out = col_out + 1;
    
% only low range on
elseif rl_channel_on(channels,2)
    % always low range
    range1 = ones(number_samples,1);
    
    % select values
    values_out(:,col_out) = values(:, col_in);
    
    % update current column
    col_in = col_in + 1;
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


%% i2

% both channels on
if rl_channel_on(channels,5) && rl_channel_on(channels,7)
    % filter low range valid (to have margin)
    range2 = double(~values(:,2)); % invert
    range2 = conv(range2, filter) > 0.5; % filter
    range2 = range2(margin+1:end-margin); % resize
    
    % select values
    for i=1:number_samples
        values_out(i,col_out) = values(i, col_in + 1 - range2(i));
    end
        
    % update current column
    col_in = col_in + 2;
    col_out = col_out + 1;
    
% only high range on
elseif rl_channel_on(channels,5)
    % always high range
    range2 = zeros(number_samples,1);
    
    % select values
    values_out(:,col_out) = values(:, col_in);
    
    % update current column
    col_in = col_in + 1;
    col_out = col_out + 1;
    
% only low range on
elseif rl_channel_on(channels,7)
    % always low range
    range2 = ones(number_samples,1);
    
    % select values
    values_out(:,col_out) = values(:, col_in);
    
    % update current column
    col_in = col_in + 1;
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
