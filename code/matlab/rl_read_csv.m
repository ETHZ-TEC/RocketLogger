function [ values, header ] = rl_read_csv( filename )

scale      = [1e-9;  1e-9;1e-11; 1e-6;   1e-6; 1e-9; 1e-9;  1e-11; 1e-6; 1e-6];

% open file
file = num2str(filename);

% read header length
header_length = csvread(file, 0, 1, [0,1,0,1]);

% read header values (except channel names)
header = csvread(file, 1, 1,[1,1,header_length-2,1]);
header(3) = rl_get_rate(header(3));

number_samples = header(1);
buffer_size = header(2);
rate = header(3);
channels = header(4);
precision = header(5);
number_channels = count_bits(channels);

% read values TODO: cope with time stamps (placed in first column)
i = header_length;
values = csvread(file, i, 1);

j = 3;
for i=0:9
    if rl_channel_on(channels, i)
       values(:,j) = values(:,j) .* scale(i+1);
       j = j+1;
    end
end
end