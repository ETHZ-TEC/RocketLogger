function cal = rl_do_cal(create_plots)
%RL_DO_CAL Perform a calibration of the RocketLogger using predefined files
%   Parameters:
%      - create_plots:     Plot residuals and pareto optimal error figures
%                            (default: no)
%   Return Values:
%      - cal:              rl_cal object from the calibration

if ~exist('create_plots', 'var')
    create_plots = 0;
end

%% read files
file_prefix = '20170214_';
rld_il1 = rld(sprintf('%scalibration_i1l.rld', file_prefix));
rld_il2 = rld(sprintf('%scalibration_i2l.rld', file_prefix));
rld_ih = rld(sprintf('%scalibration_ih.rld', file_prefix));
rld_v = rld(sprintf('%scalibration_v.rld', file_prefix));

%% seed random number generator
% optional, uncomment for determistic selection among Parto calibration points
rng(0);

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

% copy file with prefix
copyfile('calibration.dat', sprintf('%scalibration.dat', file_prefix), 'f');
copyfile('calibration.log', sprintf('%scalibration.log', file_prefix), 'f');

end
