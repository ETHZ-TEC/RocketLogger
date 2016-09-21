function [ offsets, scales ] = rl_cal_read( filename )

% init
%scales = zeros(1,10);
%offsets = zeros(1,10);

if ~exist('filename', 'var')
    filename = 'calibration.dat';
end

file = fopen(num2str(filename),'r');
offsets = fread(file,10,'int');
scales = fread(file,10,'double');
fclose(file);

end