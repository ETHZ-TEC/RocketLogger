function [ga_scale, ga_offset, residual, error_scale, error_offset] = ...
    rl_aux_lin_fit(points, points_ideal)
%RL_AUX_LIN_FIT Calculates a linear fitting function from points to
%points_ideal, with form: scale * points + offset = points_ideal
%   The function first creates an LMS fit and then searches for pareto
%   optimal (offset error, scale error & MSE) fits based on this. The final
%   candidate is selected using a RMS of all (normalized) metrics
%   Parameters:
%      - points:        Argument of function
%      - points_ideal:  Desired result of function
%   Return Values:
%      - ga_scale:      The scaling factor for the selected fit
%      - ga_offset:     The offset for the selected fit (to be added)
%      - residual:      Residual errors
%      - error_scale:   The scale error in percent
%      - error_offset:  The offset error in measurement units

%% Create LMS fit
p = polyfit(points, points_ideal, 1);
lms_scale = p(1);
lms_offset = p(2);

%% Setup of GA optimization
optf = @(x) get_errors(points, points_ideal, x(1), x(2), x(3));
initialPoint = [lms_offset, lms_scale, 1];
numberOfVariables = 3; % Number of decision variables
scaleLimits = lms_scale * [0.99, 1.01];
offsetLimits = lms_offset * [-3, 3];
% e1 = optf(initialPoint);
% targetOffsetLimits = e1(2) * [0, 10];
targetOffsetLimits = [1, 10];
lb = [min(offsetLimits), min(scaleLimits), min(targetOffsetLimits)]; % Lower bound
ub = [max(offsetLimits), max(scaleLimits), max(targetOffsetLimits)]; % Upper bound
A = []; % No linear inequality constraints
b = []; % No linear inequality constraints
Aeq = []; % No linear equality constraints
beq = []; % No linear equality constraints
options = gaoptimset('PlotFcn', [], ... % @gaplotpareto , ...
    'InitialPopulation', initialPoint, 'PopulationSize', 200);

%% Perform GA optimization
[x, Fval, ~, ~] = gamultiobj(optf, numberOfVariables, ...
    A, b, Aeq, beq, lb, ub, options);

%% Select one of the pareto-optimal candidates
value =  (Fval(:, 1) ./ median(Fval(:, 1))) .^ 2 + ...
    (Fval(:, 2) ./ median(Fval(:, 2))) .^ 2 + (Fval(:, 3) ./ median(Fval(:, 3))) .^ 2;
[~, best] = min(value);

ga_offset = x(best, 1);
ga_scale = x(best, 2);
error_scale = Fval(best, 1);
error_offset = Fval(best, 2);

fprintf('Chose error points: scale %9.6f%% -- offset %9.3f units\n', ...
    error_scale, error_offset);

% Calculate the residual error
residual = (points * ga_scale + ga_offset) - points_ideal;

end


function ret = get_errors(points, points_ideal, offset, scale, offset_error_target)
%GET_ERRORS Calculate the scale error and offset error for a fit
%   Parameters:
%      - points:                Arguments for fit function
%      - points_ideal:          Targets for fit function
%      - offset:                Linear offset of fit function
%      - scale:                 Scale of fit function
%      - offset_error_target:   The target for the offset error in
%                               multiples of the maximum error at x = 0
%                               (determines the balance between offset
%                               error and scale error): [1,inf[
%   Return Values:
%      - ret = [scale_error, offset_error, mean_square_error], where:
%        - scale_error:         Max scale error of non-zero values in percent
%        - offset_error:        Offset error in measurement units
%        - mean_square_error:   Mean square error over all value

residual = (points * scale + offset) - points_ideal;
mean_square_error = mean(residual .^ 2);
zero_error = residual(abs(points_ideal) < 1e-5);

% the offset error is at least as large as the error at x = 0
offset_error_target = max(1, offset_error_target);
offset_error = offset_error_target * max(abs(zero_error));

% determine the scale error
error_offest_corrected = min(residual - offset_error, residual + offset_error);
scale_error_points = abs(error_offest_corrected ./ points_ideal);
scale_error = max(scale_error_points(abs(points_ideal) > 1e-5));

% return value
ret = [100 * scale_error, offset_error, mean_square_error];

end
