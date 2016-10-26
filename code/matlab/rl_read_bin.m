function [ values, header, dig_inps, time ] = rl_read_bin( filename, old_file, start_buffer_index, max_buffer_count, decimation_factor)
%RL_READ_BIN Summary of this function goes here
%   Detailed explanation goes here
%
%   Arguments:
%    filename
%    old_file              Set to 1 to read old files (default: 0)
%    startBuffer           Index of the first buffer to read (default: 0)
%    max_buffer_count      Number of buffers to read (max, default: all = -1)
%    decimation_factor     Factor by which the sample frequency should be
%                          reduced, has to be a dividor of the
%                          buffer size [usually 1000] (default: 1)
%
%   Return Values:
%    values
%    header
%    time                  Timestamps (note: this will take a long time!)
%    dig_inps              Digital inputs (note: this will take a long time too)

if ~exist('old_file', 'var')
    old_file = 0;
end
if ~exist('start_buffer_index', 'var')
    start_buffer_index = 0;
end
if ~exist('max_buffer_count', 'var')
    max_buffer_count = -1;
end
if ~exist('decimation_factor', 'var')
    decimation_factor = 1;
end

% scale (per bit) for each channel
scale      = [1e-9;  1e-9;1e-11; 1e-6;   1e-6; 1e-9; 1e-9;  1e-11; 1e-6; 1e-6];

% size of timestamp depends on file format version
if old_file == 1
    timestamp_size = 1;
else
    timestamp_size = 9;
end

% open file
file = fopen(num2str(filename));
if file == -1
    error(['Could not open file: ', filename]);
end

% read header length
[header_length, bytes_read] = fread(file, 1, 'uint');

if bytes_read == 0
    fclose(file);
    error(['Invalid file: ', filename]);
end

% read header values
header = fread(file, header_length-1, 'uint');
header(3) = rl_get_rate(header(3)); % convertion

number_samples = header(1);
input_buffer_size = header(2);
rate = header(3);
channels = header(4);
precision = header(5);
number_channels = count_bits(channels);
number_buffers = floor(number_samples/input_buffer_size);
data_points_per_buffer = ceil(input_buffer_size/decimation_factor);
decimatedRate = rate/input_buffer_size*data_points_per_buffer;

if data_points_per_buffer*decimation_factor ~= input_buffer_size
    error('The buffer size needs to be divisible by the decimation factor');
end


% sanity check file size
f = dir(filename);
number_samples_expected = (f.bytes-header_length*4)/ ...
    (input_buffer_size*(number_channels+2)*4+timestamp_size*4)*input_buffer_size;
if number_samples ~= number_samples_expected 
    warning(['The number of samples in this file seems wrong (expected from filesize: ', ...
        num2str(number_samples_expected), ', header: ', num2str(number_samples), ...
        '). Ignoring file header.']);
    number_buffers = floor(number_samples_expected/input_buffer_size);
end

% determine the last buffer
if max_buffer_count ~= -1
    read_buffer_count = min(number_buffers-start_buffer_index, max_buffer_count);
else
    read_buffer_count = number_buffers-start_buffer_index;
end
time = repmat(datetime(0,0,0), read_buffer_count, 1);

% write the updated data to the header
header(1) = read_buffer_count*data_points_per_buffer;
header(2) = data_points_per_buffer;
header(3) = decimatedRate;

% values (for ranges and channels)
values = zeros(read_buffer_count*data_points_per_buffer, number_channels + 2); 
decimated_values = zeros(data_points_per_buffer, number_channels + 2);

% skip the first startBuffer buffers if this was specified
buffer_size_bytes = (timestamp_size+(number_channels + 2)*input_buffer_size)*4;    
fseek(file, start_buffer_index*buffer_size_bytes, 'cof');

% read values
for i=0:read_buffer_count-1
    if old_file == 1
        time(i+1,:) = datetime(fread(file, 1, 'uint'), 'ConvertFrom', 'posixtime'); % this is used for old files!
    else
        % time = [sec, min, hour, month_day, month, year, week_day, year_day, magic]
        temp = fread(file, 9, 'uint');
        if nargout > 3
            time(i+1,:) = datetime(temp(6)+1900, temp(5)+1, temp(4), temp(3)+2, temp(2), temp(1));
        end
    end
    
    buffer_values = fread(file, [number_channels + 2, input_buffer_size], 'int')';
    if decimation_factor == 1
        values(i*input_buffer_size+1 : (i+1)*input_buffer_size, : ) = buffer_values;
    else
        for j=1:size(buffer_values, 2)
            decimated_values(:,j) = decimateMean(buffer_values(:,j), decimation_factor);
        end
        values(i*data_points_per_buffer+1 : (i+1)*data_points_per_buffer, : ) = decimated_values;
    end
    
end

fclose(file);

j = 3;
for i=0:9
    if rl_channel_on(channels, i)
       values(:,j) = values(:,j) .* scale(i+1);
       j = j+1;
    end
end

%% Digital Inputs (workaround)
if nargout > 2
    l = size(values(:,1));
    dig_inps = nan(l(:,1), 6);

    for i=1:l
        for j=1:3
            dig_inps(i,j) = rl_channel_on(values(i,1),j);
            dig_inps(i,j+3) = rl_channel_on(values(i,2),j); 
        end
        values(i,1) = rl_channel_on(values(i,1),0);
        values(i,2) = rl_channel_on(values(i,2),0);
    end
end
end

