function [cal] = rl_do_cal (create_plots)
%RL_DO_CAL Perform a calibration of the RocketLogger using predefined files
%   Parameters:
%      - create_plots:     Plot residuals and pareto optimal error figures
%                            (default: no)
%   Return Values:
%      - cal:              rl_cal object from the calibration

if ~exist('createPlots', 'var')
    create_plots = 0;
end

%% read files
rld_il1 = rld('20161102_cal_i1l_auto.rld');
rld_il2 = rld('20161102_cal_i2l_auto.rld');
rld_ih = rld('20161103_cal_ih_2x_auto.rld');
rld_v = rld('20161027_cal_v_auto_500ms.rld');

%% perform fitting
cal = rl_cal.calibrate( ...
    rld_v, ...
    rld_il1, ...
    rld_ih, ...
    rld_il2, ...
    rld_ih, ...
    create_plots);

%% choose the correct sign if both channels are connected in series
cal.fix_signs();

%% write output file
cal.write_file();

end