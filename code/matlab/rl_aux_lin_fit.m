function [ scale, offset, res ] = rl_aux_lin_fit( points, points_ideal )

[p, s] = polyfit(points, points_ideal,1);

scale = p(1);
offset = p(2);
res = s.normr;

% calculate linear fit
%points_fit = scale .* points + offset;

%res = std(points_fit - points_ideal);

%figure;
%scatter(points_ideal, (points_fit - points_ideal));


end