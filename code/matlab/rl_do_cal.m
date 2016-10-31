function [scales, offsets] = rl_do_cal 

rld_il = rld('C:\Users\user\Desktop\data\20161031_cal_il.rld');
rld_ih = rld('C:\Users\user\Desktop\data\20161031_cal_ih.rld');


values_i1l = rld_il.get_data({'I1L_valid','I2L_valid','I1L'});
values_i2l = rld_il.get_data({'I1L_valid','I2L_valid','I2L'});

values_i1h = rld_ih.get_data({'I1H','I1H','I1H'});
values_i2h = rld_ih.get_data({'I2H','I2H','I2H'});



[scales, offsets] = rl_calibrate(...
    rl_read_bin('C:\Users\user\Desktop\data\calibrationV.dat',0), ...
    values_i1l, ...
    rl_read_bin('C:\Users\user\Desktop\data\cal_ch1_i_mid.dat',1), ...
    values_i1h, ...
    values_i2l, ...
    rl_read_bin('C:\Users\user\Desktop\data\cal_i2_mid_new2.dat',0), ...
    values_i2h, ...
    1);

% choose the correct sign if both channels are connected in series

% low range, high range are positive
for i = [1,3,6,8]
    if scales(i) < 0
        scales(i) = scales(i) * -1;
        disp(['Info: Scale ', num2str(i), ' was inverted.']);
    end
end

rl_cal_write(offsets, scales);

end