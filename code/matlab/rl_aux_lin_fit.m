function [ scale, offset, res ] = rl_aux_lin_fit( points, points_ideal )
%RL_AUX_LIN_FIT Calculate an optimal (least-mean-squares), linear fit from
%points to points_ideal
%   Parameters:
%      - points:        Argument of function
%      - points_ideal:  Desired result of function

[p, s] = polyfit(points, points_ideal,1);

scale = p(1);
offset = p(2);
res = s.normr;
optf = @(x) get_errors(points, points_ideal, x(1), x(2), x(3));
initialPoint = [offset, scale, 1];
[e1] = optf(initialPoint);
numberOfVariables = 3; % Number of decision variables
scaleLimits = scale * [0.99, 1.01];
offsetLimits = offset * [-3, 3];
targetOffsetLimits = e1(2) * [-10, 10];
lb = [min(offsetLimits), min(scaleLimits), min(targetOffsetLimits)]; % Lower bound
ub = [max(offsetLimits), max(scaleLimits), max(targetOffsetLimits)]; % Upper bound
A = []; % No linear inequality constraints
b = []; % No linear inequality constraints
Aeq = []; % No linear equality constraints
beq = []; % No linear equality constraints
options = gaoptimset('PlotFcn', [], ... %@gaplotpareto , ...
    'InitialPopulation',initialPoint, 'PopulationSize', 200);
[x,Fval,exitFlag,Output] = gamultiobj(optf, numberOfVariables,A, ...
    b,Aeq,beq,lb,ub,options);
value = sqrt( (Fval(:,1) ./ median(Fval(:,1))).^2 + (Fval(:,2) ./ median(Fval(:,2))).^2);
[~, best] = min(value);
offset = x(best,1);
scale = x(best,2);

disp(['Chose error points: SE: ', num2str(Fval(best,1)), '%; OE: ', num2str(Fval(best,2))]);

end

function [ret] = get_errors(points, points_ideal, offset, scale, offset_error_target)
    estimate = points*scale+offset;
    error = estimate - points_ideal;
    zero_error = error(abs(points_ideal) < 1e-5);
    if abs(offset_error_target) > max(abs(zero_error))
        offset_error = offset_error_target;
    else
        offset_error = max(abs(zero_error)) * sign(offset_error_target);
    end
    abs_offset_error = abs(offset_error);
    scale_error = abs((error - offset_error)./points_ideal);
    max_scale_error = max(scale_error(points_ideal ~= 0));
    ret = [max_scale_error*100, abs_offset_error];
end