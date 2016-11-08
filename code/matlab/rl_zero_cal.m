function [ offsetsNew, scales ] = rl_zero_cal( zero_rld )
%RL_ZERO_CAL Corrects the zero offset of the RocketLogger based on a
%post-calibration (zero) measurement

[offsets, scales] = rl_cal_read('calibration.dat');
movefile('calibration.dat','calibration.dat.bak');

realScales = scales;
realScales([1,5]) = scales([1,5]) * ...
    10 ^ zero_rld.channels(zero_rld.channel_index('I1H')).channel_scale;
realScales([2,6]) = scales([2,6]) * ...
    10 ^ zero_rld.channels(zero_rld.channel_index('I1L')).channel_scale;
realScales([3,4,7,8]) = scales([3,4,7,8]) * ...
    10 ^ zero_rld.channels(zero_rld.channel_index('V1')).channel_scale;

offsetsMeasured = mean(zero_rld.get_data({'I1H','I1L','V1','V2','I2H','I2L','V3', 'V4'}));
offsetsComp = offsetsMeasured' ./ realScales;
offsetsNew = offsets - offsetsComp;

rl_cal_write(offsetsNew, scales);


end

