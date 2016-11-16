classdef rl_cal < handle
    %RL_CAL Class that handles the calibration of the RocketLogger
    %   Detailed explanation goes here
    
    properties(Constant)
        CHANNEL_COUNT = 8;
        V_INDEX = [3,4,7,8];
        IH_INDEX = [1,5];
        IL_INDEX = [2,6];
        POSITIVE_SCALE_CHANNELS = [rl_cal.IH_INDEX, rl_cal.IL_INDEX];
        
        FILE_SCALE_V = 1e-6;
        FILE_SCALE_IL = 1e-11;
        FILE_SCALE_IH = 1e-9;
        
        UNCAL_STEP_V = -1.22e-6;
        UNCAL_STEP_IL = 351e-12;
        UNCAL_STEP_IH = 63.6e-9;
        
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
            offsets = fread(file,rl_cal.CHANNEL_COUNT,'int');
            scales = fread(file,rl_cal.CHANNEL_COUNT,'double');
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
            
            v_step_uncal = rl_cal.CAL_STEP_V / abs(rl_cal.UNCAL_STEP_V);
            il_step_uncal = rl_cal.CAL_STEP_IL / abs(rl_cal.UNCAL_STEP_IL);
            ih_step_uncal = rl_cal.CAL_STEP_IH / abs(rl_cal.UNCAL_STEP_IH);

            % init
            scales = zeros(1,rl_cal.CHANNEL_COUNT);
            offsets = zeros(1,rl_cal.CHANNEL_COUNT);

            %% Voltages

            % ideal values
            v_max = (rl_cal.CAL_NUM_POINTS-1)/4*rl_cal.CAL_STEP_V;
            v_ideal = rl_gen_dual_sweep_values(-v_max, v_max, rl_cal.CAL_STEP_V)/rl_cal.FILE_SCALE_V;

            % init
            avg_points_v = zeros(4,rl_cal.CAL_NUM_POINTS);
            residual_v = zeros(4, rl_cal.CAL_NUM_POINTS);

            % average and fitting
            for i=1:4
                disp(['Voltage Channel: ', int2str(i)]);
                avg_points_v(i,:) = rl_aux_average_points(v(:,i), rl_cal.CAL_NUM_POINTS, v_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(rl_cal.V_INDEX(i)), offsets(rl_cal.V_INDEX(i)), ~ ] = rl_aux_lin_fit(avg_points_v(i,:),v_ideal);
                residual_v(i,:) = (scales(rl_cal.V_INDEX(i))*avg_points_v(i,:)+offsets(rl_cal.V_INDEX(i))-v_ideal)*1e-6;
            end

            if plotPareto ~= 0
                rl_pareto_error(v_ideal*1e-6, residual_v);
                title('Voltage Pareto Optimal Error Numbers');
            end



            %% Low Currents

            % ideal values
            il_max = (rl_cal.CAL_NUM_POINTS-1)/4*rl_cal.CAL_STEP_IL;
            il_ideal = rl_gen_dual_sweep_values(-il_max, il_max, rl_cal.CAL_STEP_IL)/rl_cal.FILE_SCALE_IL;

            % init
            avg_points_il = zeros(2,rl_cal.CAL_NUM_POINTS);

            % average and fitting
            disp('Current Channel 1, LOW');
            avg_points_il(1,:) = rl_aux_average_points(i1l, rl_cal.CAL_NUM_POINTS, il_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(rl_cal.IL_INDEX(1)), offsets(rl_cal.IL_INDEX(1)), ~ ] = rl_aux_lin_fit(avg_points_il(1,:),il_ideal);

            disp('Current Channel 2, LOW');
            avg_points_il(2,:) = rl_aux_average_points(i2l, rl_cal.CAL_NUM_POINTS, il_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(rl_cal.IL_INDEX(2)), offsets(rl_cal.IL_INDEX(2)), ~ ] = rl_aux_lin_fit(avg_points_il(2,:),il_ideal);

            residual_il = zeros(2, rl_cal.CAL_NUM_POINTS);
            residual_il(1,:) = (scales(rl_cal.IL_INDEX(1))*avg_points_il(1,:)+offsets(rl_cal.IL_INDEX(1))-il_ideal)*1e-11;
            residual_il(2,:) = (scales(rl_cal.IL_INDEX(2))*avg_points_il(2,:)+offsets(rl_cal.IL_INDEX(2))-il_ideal)*1e-11;
            if plotPareto ~= 0
                rl_pareto_error(il_ideal*1e-11, residual_il);
                title('Current Low Pareto Optimal Error Numbers');
            end


            %% High Currents

            % ideal values
            ih_max = (rl_cal.CAL_NUM_POINTS-1)/4*rl_cal.CAL_STEP_IH;
            ih_ideal = rl_gen_dual_sweep_values(-ih_max, ih_max, rl_cal.CAL_STEP_IH) / rl_cal.FILE_SCALE_IH;

            % init
            avg_points_ih = zeros(2,rl_cal.CAL_NUM_POINTS);

            % average and fitting
            disp('Current Channel 1, HIGH');
            avg_points_ih(1,:) = rl_aux_average_points(i1h, rl_cal.CAL_NUM_POINTS, ih_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(rl_cal.IH_INDEX(1)), offsets(rl_cal.IH_INDEX(1)), ~ ] = rl_aux_lin_fit(avg_points_ih(1,:),ih_ideal);

            disp('Current Channel 2, HIGH');
            avg_points_ih(2,:) = rl_aux_average_points(i2h, rl_cal.CAL_NUM_POINTS, ih_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
            [scales(rl_cal.IH_INDEX(2)), offsets(rl_cal.IH_INDEX(2)), ~ ] = rl_aux_lin_fit(avg_points_ih(2,:),ih_ideal);

            residual_ih = zeros(2, rl_cal.CAL_NUM_POINTS);
            residual_ih(1,:) = (scales(rl_cal.IH_INDEX(1))*avg_points_ih(1,:)+offsets(rl_cal.IH_INDEX(1))-ih_ideal)*1e-9;
            residual_ih(2,:) = (scales(rl_cal.IH_INDEX(2))*avg_points_ih(2,:)+offsets(rl_cal.IH_INDEX(2))-ih_ideal)*1e-9;
            if plotPareto ~= 0
                rl_pareto_error(ih_ideal*1e-9, residual_ih);
                title('Current High Pareto Optimal Error Numbers');
            end


            %% Finishing
            offsets = offsets ./ scales;
            %scales = - 1 .* scales;
            
            obj = rl_cal(offsets, scales);
        end
        
        function tf = eq(h1, h2) 
            tf = isequal(h1.offsets, h2.offsets) && isequal(h2.offsets, h2.offsets);
        end
        
    end
    
end

