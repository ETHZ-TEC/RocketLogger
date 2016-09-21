function [ scales, offsets ] = rl_calibrate( v, i1l, i1m, i1h, i2l, i2m, i2h, plotPareto )

global failCount
failCount = 0;

if ~exist('plotPareto', 'var')
    plotPareto = 0;
end

% constants
num_points = 201;

% scale values
scale      = [1e-9;  1e-9;1e-11; 1e-6;   1e-6; 1e-9; 1e-9;  1e-11; 1e-6; 1e-6];

v   = v   / scale(4);
i1h = i1h / scale(1);
i1m = i1m / scale(2);
i1l = i1l / scale(3);
i2h = i2h / scale(6);
i2m = i2m / scale(7);
i2l = i2l / scale(8);

v_step = 50000;                    % actual steps are about 80k
v_step_ideal = 100000;

il_step = 30000;                   % actual steps are about 45k
il_step_ideal = 2000000;

im_step = 30e3;                    % actual steps are about 40k
im_step_ideal = 700000;

ih_step = 7.5e3;                   % actual steps are about 11k
ih_step_ideal = 2000000;
ih_max = 100e6;

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
v_ideal2 = v_ideal*1e-6;

% values
avg_points = zeros(4,num_points);

% average and fitting
for i=1:4
    disp(['Voltage Channel: ', int2str(i)]);
    avg_points(i,:) = average_points(v(:,i+2), num_points, v_step);
    [scales(v_indices(i)), offsets(v_indices(i)), ~ ] = lin_fit(avg_points(i,:),v_ideal);
    residual(i,:) = (scales(v_indices(i))*avg_points(i,:)+offsets(v_indices(i))-v_ideal)*1e-6;
end

if plotPareto ~= 0
    rl_pareto_error(v_ideal*1e-6, residual);
    title('Voltage Pareto Optimal Error Numbers');
end



%% Low Currents
% indices in all channels
il_indices = [3,8];

% ideal values
tmp1 = -100000000:il_step_ideal:(100000000-il_step_ideal);
tmp2 = 100000000:-il_step_ideal:-100000000;
il_ideal = horzcat(tmp1, tmp2);

% init
avg_points = zeros(2,num_points);

% average and fitting
disp('Current Channel 1, LOW');
avg_points(1,:) = average_points(i1l(:,3), num_points, il_step);
[scales(il_indices(1)), offsets(il_indices(1)), ~ ] = lin_fit(avg_points(1,:),il_ideal);

disp('Current Channel 2, LOW');
avg_points(2,:) = average_points(i2l(:,3), num_points, il_step);
[scales(il_indices(2)), offsets(il_indices(2)), ~ ] = lin_fit(avg_points(2,:),il_ideal);

residual = [];
residual(1,:) = (scales(il_indices(1))*avg_points(1,:)+offsets(il_indices(1))-il_ideal)*1e-11;
residual(2,:) = (scales(il_indices(2))*avg_points(2,:)+offsets(il_indices(2))-il_ideal)*1e-11;
if plotPareto ~= 0
    rl_pareto_error(il_ideal*1e-11, residual);
    title('Current Low Pareto Optimal Error Numbers');
end


%% Medium Currents



% indices in all channels
im_indices = [2,7];

% ideal values
tmp1 = -35000000:im_step_ideal:(35000000-im_step_ideal);
tmp2 = 35000000:-im_step_ideal:-35000000;
im_ideal = horzcat(tmp1, tmp2);

% init
avg_points = zeros(2,num_points);

% average and fitting
disp('Current Channel 1, MED');
avg_points(1,:) = average_points(i1m(:,3), num_points, im_step);
[scales(im_indices(1)), offsets(im_indices(1)), ~ ] = lin_fit(avg_points(1,:),im_ideal);

disp('Current Channel 2, MED');
avg_points(2,:) = average_points(i2m(:,3), num_points, im_step);
[scales(im_indices(2)), offsets(im_indices(2)), ~ ] = lin_fit(avg_points(2,:),im_ideal);

residual = [];
residual(1,:) = (scales(im_indices(1))*avg_points(1,:)+offsets(im_indices(1))-im_ideal)*1e-9;
residual(2,:) = (scales(im_indices(2))*avg_points(2,:)+offsets(im_indices(2))-im_ideal)*1e-9;
if plotPareto ~= 0
    rl_pareto_error(im_ideal*1e-9, residual);
    title('Current Medium Pareto Optimal Error Numbers');
end

%% High Currents

% indices in all channels
ih_indices = [1,6];

% ideal values
tmp1 = -ih_max:ih_step_ideal:(ih_max-ih_step_ideal);
tmp2 = ih_max:-ih_step_ideal:-ih_max;
ih_ideal = horzcat(tmp1, tmp2);

% init
avg_points = zeros(2,num_points);

% average and fitting
disp('Current Channel 1, HIGH');
avg_points(1,:) = average_points(i1h(:,3), num_points, ih_step);
[scales(ih_indices(1)), offsets(ih_indices(1)), ~ ] = lin_fit(avg_points(1,:),ih_ideal);

disp('Current Channel 2, HIGH');
avg_points(2,:) = average_points(i2h(:,3), num_points, ih_step);
[scales(ih_indices(2)), offsets(ih_indices(2)), ~ ] = lin_fit(avg_points(2,:),ih_ideal);

residual = [];
residual(1,:) = (scales(ih_indices(1))*avg_points(1,:)+offsets(ih_indices(1))-ih_ideal)*1e-9;
residual(2,:) = (scales(ih_indices(2))*avg_points(2,:)+offsets(ih_indices(2))-ih_ideal)*1e-9;
if plotPareto ~= 0
    rl_pareto_error(ih_ideal*1e-9, residual);
    title('Current High Pareto Optimal Error Numbers');
end


%% Finishing
offsets = offsets ./ scales;
%scales = - 1 .* scales;

file = fopen('calibration.dat','w');
fwrite(file,offsets,'int');
fwrite(file,scales,'double');
fclose(file);

disp(' ');
disp(['Calibration file name: ', pwd, '\calibration.dat']);
disp('  -> Copy this file to /etc/rocketlogger');
end

