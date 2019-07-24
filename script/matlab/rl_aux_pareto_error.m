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

function [offset_error, min_scale_error] = rl_aux_pareto_error(ideal, residual)
%RL_AUX_PARETO_ERROR Creates a plot of the pareto optimal error figures
%(offset error and scale error)
%   Parameters:
%      - ideal:            Ideal values
%      - residual:         Deviation from the ideal values (one or multiple
%                            vectors)
%   Return Values:
%      - offset_error:     Vector of possible offset errors
%      - min_scale_error:  Vector of corresponding scale errors

jMax = size(residual, 1);

figure;
hold on;
for j=1:jMax
    plot(ideal, residual(j, :));
end

% determine the minimum and maximum (optimal) offset error
offsetErrorMax = max(max(abs(residual)));
minOffsetError = max(max(abs(residual(:, ideal == 0))));
offset_error = linspace(minOffsetError, offsetErrorMax, 1000);

% calculate the minimum scale error for each chose offset error
for i=1:length(offset_error)
    remainingError2(:, :, i) = max(max(residual - offset_error(i), 0), -min(residual + offset_error(i), 0));
    for j=1:jMax
        scaleError2(j, :, i) = abs(remainingError2(j, :, i) ./ ideal);
    end
    temp2 = max(scaleError2(:, :, i), [], 1);
    min_scale_error(i) = max(temp2(isfinite(temp2)));
end

figure;
scatter(offset_error, min_scale_error * 100);
xlabel('Offset Error');
ylabel('Scale Error [%]');

end
