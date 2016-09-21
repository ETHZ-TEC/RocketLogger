function [  ] = calculate_accuracy()
%CALCULATE_ACCURACY Summary of this function goes here
%   Detailed explanation goes here

filenameVoltages = 'C:\Users\user\Desktop\data\20160908_voltage_accuracy_2.dat';
 
v_step = 50000;                    % actual steps are about 80k
v_step_ideal = 100000;

il_step = 30000;                   % actual steps are about 45k
il_step_ideal = 2000000;

im_step = 30e3;                    % actual steps are about 40k
im_step_ideal = 700000;

ih_step = 7.5e3;                   % actual steps are about 11k
ih_step_ideal = 2000000;
ih_max = 100e6;

% ideal values
tmp1 = -5000000:v_step_ideal:(5000000+-v_step_ideal);
tmp2 = 5000000:-v_step_ideal:-5000000;
v_ideal = horzcat(tmp1, tmp2);
v_ideal2 = v_ideal*1e-6;

% init
scales = zeros(1,10);
offsets = zeros(1,10);

%% Voltages
% indices channels vector
v_indices = [4,5,9,10];

% ideal values
tmp1 = -5000000:v_step_ideal:(5000000+-v_step_ideal);
tmp2 = 5000000:-v_step_ideal:-5000000;
v_ideal = horzcat(tmp1, tmp2);
vIdeal = v_ideal*1e-6;

[values, header] = rl_read_bin(filenameVoltages);

for i=1:4
    vAvg(i,:) = average_points(values(:,i+2), 201, 50e-3);
    vResidual(i,:) = vAvg(i,:) - vIdeal;
end

rl_pareto_error(vIdeal, vResidual);


end

