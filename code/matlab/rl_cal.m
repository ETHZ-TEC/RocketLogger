classdef rl_cal
    %RL_CAL Class that handles the calibration of the RocketLogger
    %   Detailed explanation goes here
    
    properties(Constant, Hidden)
        CHANNEL_COUNT = 8;
        POSITIVE_SCALE_CHANNELS = [1,2,5,6];
        
        FILE_SCALE_V = 1e-6;
        FILE_SCALE_IL = 1e-11;
        FILE_SCALE_IH = 1e-9;
        
        CAL_NUM_POINTS = 201;
        CAL_STEP_V = 100e-3;
        CAL_STEP_IL = 20e-6;
        CAL_STEP_IH = 2e-3;
        CAL_MIN_STABLE_SAMPLES = 200;
    end
    
    properties
        offsets;
        scales;
    end
    
    methods
        function obj = rl_cal(offsets, scales)
            %RL_CAL Creates an rl_cal object from calibration vectors
            %   Parameters:
            %      - offests:    Offsets [ADC LSBs]
            %      - scales:     Linear scaling factor
            assert(length(offsets) == obj.CHANNEL_COUNT, 'Invalid size of offsets');
            assert(length(scales) == obj.CHANNEL_COUNT, 'Invalid size of scales');
            obj.offsets = offsets;
            obj.scales = scales;
        end
        
        function write_file(obj, filename)
            %WRITE_FILE Write the calibration to a file
            %   Parameters:
            %      - filename:   Filename to write to (default:
            %                       calibration.dat)
            if ~exist('filename', 'var')
                filename = 'calibration.dat';
            end
            
            file = fopen(filename,'w');
            fwrite(file,obj.offsets,'int');
            fwrite(file,obj.scales,'double');
            fclose(file);

            disp(' ');
            disp(['Calibration file name: ', pwd, '\', filename]);
            disp('  -> Copy this file to /etc/rocketlogger/calibration.dat');            
        end
        
        function fix_signs(obj)
            %FIX_SIGNS Fix the signs of the scales if two current channels
            %were calibrated simulateously
            
            % low range, high range are positive
            for i = obj.POSITIVE_SCALE_CHANNELS;
                if obj.scales(i) < 0
                    obj.scales(i) = obj.scales(i) * -1;
                    disp(['Info: Scale ', num2str(i), ' was inverted.']);
                end
            end
        end
    end
    
    methods(Static)
        function [ obj ] = from_file( filename )
            %RL_CAL_READ Reads the calibration a RocketLogger calibration 
            %file
            %   Parameters:
            %      - filename:    Calibration filename (default: 
            %                       calibration.dat)
            
            if ~exist('filename', 'var')
                filename = 'calibration.dat';
            end
            
            % read in file
            file = fopen(num2str(filename),'r');
            offsets = fread(file,8,'int');
            scales = fread(file,8,'double');
            fclose(file);
            
            % create object            
            obj =  rl_cal(offsets, scales);
        end
        
        function [obj] = calibrate( v_rld, i1l_rld, i1h_rld, i2l_rld, i2h_rld, plotPareto )
            if ~exist('plotPareto', 'var')
                plotPareto = 0;
            end
            
            i1l = i1l_rld.get_data({'I1L'});
            i1h = i1h_rld.get_data({'I1H'});
            i2l = i2l_rld.get_data({'I2L'});
            i2h = i2h_rld.get_data({'I2H'});

            v = v_rld.get_data({'V1','V2','V3','V4'});

            % constants

            v   = v   / rl_cal.FILE_SCALE_V;
            i1h = i1h / rl_cal.FILE_SCALE_IH;
            i1l = i1l / rl_cal.FILE_SCALE_IL;
            i2h = i2h / rl_cal.FILE_SCALE_IH;
            i2l = i2l / rl_cal.FILE_SCALE_IL;

            v_step = 80e3;                    % actual steps are about 80k
            v_step_ideal = 100000;

            il_step = 45e3;                   % actual steps are about 45k
            il_step_ideal = 2000000;

            ih_step = 11e3;                   % actual steps are about 11k
            ih_step_ideal = 2000000;
            ih_max = 100e6;

            % init
            scales = zeros(1,8);
            offsets = zeros(1,8);

            %% Voltages
            % indices channels vector
            v_indices = [3,4,7,8];

            % ideal values
            tmp1 = -5000000:v_step_ideal:(5000000+-v_step_ideal);
            tmp2 = 5000000:-v_step_ideal:-5000000;
            v_ideal = horzcat(tmp1, tmp2);
            v_ideal2 = v_ideal*1e-6;

            % values
            avg_points = zeros(4,rl_cal.CAL_NUM_POINTS);

            % average and fitting
            for i=1:4
                disp(['Voltage Channel: ', int2str(i)]);
                avg_points(i,:) = rl_aux_average_points(v(:,i), rl_cal.CAL_NUM_POINTS, v_step, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(v_indices(i)), offsets(v_indices(i)), ~ ] = rl_aux_lin_fit(avg_points(i,:),v_ideal);
                residual(i,:) = (scales(v_indices(i))*avg_points(i,:)+offsets(v_indices(i))-v_ideal)*1e-6;
            end

            if plotPareto ~= 0
                rl_pareto_error(v_ideal*1e-6, residual);
                title('Voltage Pareto Optimal Error Numbers');
            end



            %% Low Currents
            % indices in all channels
            il_indices = [2,6];

            % ideal values
            tmp1 = -100000000:il_step_ideal:(100000000-il_step_ideal);
            tmp2 = 100000000:-il_step_ideal:-100000000;
            il_ideal = horzcat(tmp1, tmp2);

            % init
            avg_points = zeros(2,rl_cal.CAL_NUM_POINTS);

            % average and fitting
            disp('Current Channel 1, LOW');
            avg_points(1,:) = rl_aux_average_points(i1l, rl_cal.CAL_NUM_POINTS, il_step, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(il_indices(1)), offsets(il_indices(1)), ~ ] = rl_aux_lin_fit(avg_points(1,:),il_ideal);

            disp('Current Channel 2, LOW');
            avg_points(2,:) = rl_aux_average_points(i2l, rl_cal.CAL_NUM_POINTS, il_step, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(il_indices(2)), offsets(il_indices(2)), ~ ] = rl_aux_lin_fit(avg_points(2,:),il_ideal);

            residual = [];
            residual(1,:) = (scales(il_indices(1))*avg_points(1,:)+offsets(il_indices(1))-il_ideal)*1e-11;
            residual(2,:) = (scales(il_indices(2))*avg_points(2,:)+offsets(il_indices(2))-il_ideal)*1e-11;
            if plotPareto ~= 0
                rl_pareto_error(il_ideal*1e-11, residual);
                title('Current Low Pareto Optimal Error Numbers');
            end


            %% High Currents

            % indices in all channels
            ih_indices = [1,5];

            % ideal values
            tmp1 = -ih_max:ih_step_ideal:(ih_max-ih_step_ideal);
            tmp2 = ih_max:-ih_step_ideal:-ih_max;
            ih_ideal = horzcat(tmp1, tmp2);

            % init
            avg_points = zeros(2,rl_cal.CAL_NUM_POINTS);

            % average and fitting
            disp('Current Channel 1, HIGH');
            avg_points(1,:) = rl_aux_average_points(i1h, rl_cal.CAL_NUM_POINTS, ih_step, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(ih_indices(1)), offsets(ih_indices(1)), ~ ] = rl_aux_lin_fit(avg_points(1,:),ih_ideal);

            disp('Current Channel 2, HIGH');
            avg_points(2,:) = rl_aux_average_points(i2h, rl_cal.CAL_NUM_POINTS, ih_step, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(ih_indices(2)), offsets(ih_indices(2)), ~ ] = rl_aux_lin_fit(avg_points(2,:),ih_ideal);

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
            
            obj = rl_cal(offsets, scales);
        end
        
    end
    
end

