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

end