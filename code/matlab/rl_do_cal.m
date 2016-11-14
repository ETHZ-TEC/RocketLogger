function [scale, offset] = rl_do_cal (createPlots)

if ~exist('createPlots', 'var')
    createPlots = 0;
end

rld_il1 = rld('20161102_cal_i1l_auto.rld');
rld_il2 = rld('20161102_cal_i2l_auto.rld');
rld_ih = rld('20161103_cal_ih_2x_auto.rld');
rld_v = rld('20161027_cal_v_auto_500ms.rld');


values_i1l = rld_il1.get_data({'I1L_valid','I1L_valid','I1L'});
values_i2l = rld_il2.get_data({'I2L_valid','I2L_valid','I2L'});

values_i1h = rld_ih.get_data({'I1H','I1H','I1H'});
values_i2h = rld_ih.get_data({'I2H','I2H','I2H'});

values_v = rld_v.convert();



[scale, offset] = rl_calibrate(...
    values_v, ...
    values_i1l, ...
    values_i1h, ...
    values_i2l, ...
    values_i2h, ...
    createPlots);

%% choose the correct sign if both channels are connected in series

% low range, high range are positive
for i = [1,3,6,8]
    if scale(i) < 0
        scale(i) = scale(i) * -1;
        disp(['Info: Scale ', num2str(i), ' was inverted.']);
    end
end

rl_cal_write(offset, scale);

end