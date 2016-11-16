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
            %CALIBRATE Creates a calibration object from capture waveforms
            %(dual linear sweeps) generated by an accurate SMU
            %   Parameters:
            %      - v_rld:      rld object with all voltage waveforms
            %      - i1l_rld:    rld object with the I1 low waveform
            %      - i1h_rld:    rld object with the I1 high waveform
            %      - i2l_rld:    rld object with the I2 low waveform
            %      - i2h_rld:    rld object with the I2 high waveform
            %      - plotPareto: Create plots of residuals and pareto
            %                      optimal errors numbers
            %   See also RL_DO_CAL
            if ~exist('plotPareto', 'var')
                plotPareto = 0;
            end
            
            i1l = i1l_rld.get_data({'I1L'});
            i1h = i1h_rld.get_data({'I1H'});
            i2l = i2l_rld.get_data({'I2L'});
            i2h = i2h_rld.get_data({'I2H'});

            v = v_rld.get_data({'V1','V2','V3','V4'});

            % Undo the scaling that was done while reading the file
            v   = v   / rl_cal.FILE_SCALE_V;
            ih(:,1) = i1h / rl_cal.FILE_SCALE_IH;
            ih(1:length(i2h),2) = i2h / rl_cal.FILE_SCALE_IH;
            il(:,1) = i1l / rl_cal.FILE_SCALE_IL;
            il(1:length(i2l),2) = i2l / rl_cal.FILE_SCALE_IL;
            
            % calculate the step sizes in ADC steps
            v_step_uncal = rl_cal.CAL_STEP_V / abs(rl_cal.UNCAL_STEP_V);
            il_step_uncal = rl_cal.CAL_STEP_IL / abs(rl_cal.UNCAL_STEP_IL);
            ih_step_uncal = rl_cal.CAL_STEP_IH / abs(rl_cal.UNCAL_STEP_IH);

            % init
            scales = zeros(1,rl_cal.CHANNEL_COUNT);
            offsets = zeros(1,rl_cal.CHANNEL_COUNT);

            %% Voltages

            % ideal values
            v_max = (rl_cal.CAL_NUM_POINTS-1)/4*rl_cal.CAL_STEP_V;
            v_ideal = rl_cal.gen_dual_sweep_values(-v_max, v_max, rl_cal.CAL_STEP_V)/rl_cal.FILE_SCALE_V;

            % init
            avg_points_v = zeros(4,rl_cal.CAL_NUM_POINTS);
            residual_v = zeros(4, rl_cal.CAL_NUM_POINTS);

            % average and fitting
            for i=1:4
                index = rl_cal.V_INDEX(i);
                disp(['Voltage Channel: ', int2str(i)]);
                avg_points_v(i,:) = rl_aux_average_points(v(:,i), ...
                    rl_cal.CAL_NUM_POINTS, v_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(index), offsets(index), ~ ] = rl_aux_lin_fit(avg_points_v(i,:),v_ideal);
                residual_v(i,:) = (scales(index)*avg_points_v(i,:)+offsets(index)-v_ideal)*rl_cal.FILE_SCALE_V;
            end

            if plotPareto ~= 0
                rl_aux_pareto_error(v_ideal*rl_cal.FILE_SCALE_V, residual_v);
                title('Voltage Pareto Optimal Error Numbers');
            end

            %% Low Currents

            % ideal values
            il_max = (rl_cal.CAL_NUM_POINTS-1)/4*rl_cal.CAL_STEP_IL;
            il_ideal = rl_cal.gen_dual_sweep_values(-il_max, il_max, rl_cal.CAL_STEP_IL)/rl_cal.FILE_SCALE_IL;

            % init
            avg_points_il = zeros(2,rl_cal.CAL_NUM_POINTS);
            residual_il = zeros(2, rl_cal.CAL_NUM_POINTS);


            % average and fitting
            for i=1:2
                index = rl_cal.IL_INDEX(i);
                disp(['Current Channel ', num2str(i),', LOW']);
                
                avg_points_il(i,:) = rl_aux_average_points(il(:,i), ...
                    rl_cal.CAL_NUM_POINTS, il_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(index), offsets(index), ~ ] = rl_aux_lin_fit(avg_points_il(i,:), il_ideal);
            
                residual_il(i,:) = (scales(index)*avg_points_il(i,:) + ...
                    offsets(index) - il_ideal)*rl_cal.FILE_SCALE_IL;            
            end
            
            if plotPareto ~= 0
                rl_aux_pareto_error(il_ideal*rl_cal.FILE_SCALE_IL, residual_il);
                title('Current Low Pareto Optimal Error Numbers');
            end


            %% High Currents

            % ideal values
            ih_max = (rl_cal.CAL_NUM_POINTS-1)/4*rl_cal.CAL_STEP_IH;
            ih_ideal = rl_cal.gen_dual_sweep_values(-ih_max, ih_max, rl_cal.CAL_STEP_IH) / rl_cal.FILE_SCALE_IH;

            % init
            avg_points_ih = zeros(2,rl_cal.CAL_NUM_POINTS);
            residual_ih = zeros(2, rl_cal.CAL_NUM_POINTS);

            % average and fitting
            for i=1:2
                index = rl_cal.IH_INDEX(i);
                disp(['Current Channel ', num2str(i),', HIGH']);
                avg_points_ih(i,:) = rl_aux_average_points(ih(:,i), rl_cal.CAL_NUM_POINTS, ih_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(index), offsets(index), ~ ] = rl_aux_lin_fit(avg_points_ih(i,:),ih_ideal);
                residual_ih(i,:) = (scales(index)*avg_points_ih(i,:)+offsets(index)-ih_ideal)*rl_cal.FILE_SCALE_IH;

            end

            if plotPareto ~= 0
                rl_aux_pareto_error(ih_ideal*rl_cal.FILE_SCALE_IH, residual_ih);
                title('Current High Pareto Optimal Error Numbers');
            end


            %% Finishing
            offsets = offsets ./ scales;
            %scales = - 1 .* scales;
            
            obj = rl_cal(offsets, scales);
        end
        
        function [ values ] = gen_dual_sweep_values( start, stop, step )
        %GEN_DUAL_SWEEP_VALUES Generate the values of a dual linear sweep 
        %of the Keithley 2450 SMU
        %   Parameters:
        %      - start:    Lowest value
        %      - stop:     Highest value
        %      - step:     Step size
            values = [start:step:stop-step, stop:-step:start];
        end
        
    end
    
end

