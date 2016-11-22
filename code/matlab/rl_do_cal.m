function [cal] = rl_do_cal (createPlots)

if ~exist('createPlots', 'var')
    createPlots = 0;
end

%% read files
rld_il1 = rld('20161122_i1l.rld');
rld_il2 = rld('20161122_i2l.rld');
rld_ih = rld('20161122_ih.rld');
rld_v = rld('20161122_v.rld');

%% perform fitting
cal = rl_cal.calibrate( ...
    rld_v, ...
    rld_il1, ...
    rld_ih, ...
    rld_il2, ...
    rld_ih, ...
    createPlots);

%% choose the correct sign if both channels are connected in series
cal.fix_signs();

%% write output file
cal.write_file();

end