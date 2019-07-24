%%
%% Copyright (c) 2016-2018, ETH Zurich, Computer Engineering Group
%% All rights reserved.
%% 
%% Redistribution and use in source and binary forms, with or without
%% modification, are permitted provided that the following conditions are met:
%% 
%% * Redistributions of source code must retain the above copyright notice, this
%%   list of conditions and the following disclaimer.
%% 
%% * Redistributions in binary form must reproduce the above copyright notice,
%%   this list of conditions and the following disclaimer in the documentation
%%   and/or other materials provided with the distribution.
%% 
%% * Neither the name of the copyright holder nor the names of its
%%   contributors may be used to endorse or promote products derived from
%%   this software without specific prior written permission.
%% 
%% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
%% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
%% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
%% DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
%% FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
%% DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
%% SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
%% CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
%% OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
%% OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
%%

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
fprintf('-----\n');
cal = rl_cal.calibrate( ...
    rld_v, ...
    rld_il1, ...
    rld_ih, ...
    rld_il2, ...
    rld_ih, ...
    create_plots);

%% choose the correct sign if both channels are connected in series
fprintf('-----\n');
cal.fix_signs();

%% write output file
fprintf('-----\n');
cal.write_file();
fprintf('-----\n');

% copy file with prefix
copyfile('calibration.dat', sprintf('%scalibration.dat', file_prefix), 'f');
copyfile('calibration.log', sprintf('%scalibration.log', file_prefix), 'f');

end
