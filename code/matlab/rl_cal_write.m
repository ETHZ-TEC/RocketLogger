function [  ] = rl_cal_write(offsets, scales)
file = fopen('calibration.dat','w');
fwrite(file,offsets,'int');
fwrite(file,scales,'double');
fclose(file);

end
