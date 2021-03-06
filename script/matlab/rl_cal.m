%%
%% Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
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

classdef rl_cal < handle
    %RL_CAL Class that handles the calibration of the RocketLogger
    %   Detailed explanation goes here
    
    properties(Constant)
        % Number of channels
        CHANNEL_COUNT = 8;
        % Index definitions
        V_INDEX = [3,4,7,8];
        IH_INDEX = [1,5];
        IL_INDEX = [2,6];
        POSITIVE_SCALE_CHANNELS = [rl_cal.IH_INDEX, rl_cal.IL_INDEX];
        
        % Scales (per bit) for the values, that are stored in the binary files
        FILE_SCALE_V = 1e-8;
        FILE_SCALE_IL = 1e-11;
        FILE_SCALE_IH = 1e-9;
        
        % Voltage/Current per ADC bit
        UNCAL_STEP_V = -1.22e-6;
        UNCAL_STEP_IL = 1.755e-10;
        UNCAL_STEP_IH = 3.18e-08;
        
        % Calibration sweep settings (dual sweep, Keithley 2450 SMU)
        CAL_NUM_POINTS = 201;
        CAL_STEP_V = 100e-3;
        CAL_STEP_IL = 20e-6;
        CAL_STEP_IH = 2e-3;
        CAL_MIN_STABLE_SAMPLES = 150;
    end
    
    properties
        time;
        offsets;
        scales;
        error_offsets;
        error_scales;
    end
    
    methods
        function obj = rl_cal(offsets, scales, time, error_offsets, error_scales)
            %RL_CAL Creates an rl_cal object from calibration vectors
            %   Parameters:
            %      - offests:       Offsets [ADC LSBs]
            %      - scales:        Linear scaling factor
            %      - time:          Date/time of the calibration measurements
            %      - error_offsets: Calibration offset errors in SI units (default: empty)
            %      - error_scales:  Calibration scale errors (default: empty)
            assert(length(offsets) == obj.CHANNEL_COUNT, 'Invalid size of offsets');
            assert(length(scales) == obj.CHANNEL_COUNT, 'Invalid size of scales');
            obj.offsets = offsets;
            obj.scales = scales;
            obj.time = time;
            if exist('error_scales', 'var')
                obj.error_offsets = error_offsets;
                obj.error_scales = error_scales;
            else
                obj.error_offsets = [];
                obj.error_scales = [];
            end
        end
        
        function write_file(obj, cal_filename, log_filename)
            %WRITE_FILE Write the calibration to a file
            %   Parameters:
            %      - filename:   Filename to write to (default:
            %                       calibration.dat)
            
            if ~exist('filename', 'var')
                cal_filename = 'calibration.dat';
            end
            if ~exist('logfilename', 'var')
                log_filename = 'calibration.log';
            end
            
            % write calibration file
            file = fopen(cal_filename, 'w');
            fwrite(file, obj.time, 'int64');
            fwrite(file, obj.offsets, 'int32');
            fwrite(file, obj.scales, 'double');
            fclose(file);
            
            % write log file if fresh calibration
            if length(obj.error_offsets) == obj.CHANNEL_COUNT
                calibration_date = datetime(obj.time, 'ConvertFrom', 'posixtime');
                
                logfile = fopen(log_filename, 'w');
                fprintf(logfile, 'RocketLogger Calibration Log\n');
                fprintf(logfile, '\n');
                fprintf(logfile, 'Calibration Time:\t%s\n', datestr(calibration_date, 'yyyy-mm-dd, HH:MM:ss'));
                fprintf(logfile, '\n');
                fprintf(logfile, 'Voltage Channel Calibration Errors\n');
                for ch=1:4
                    i = rl_cal.V_INDEX(ch);
                    fprintf(logfile, '  Voltage V%i:               %6.3f%% + %9.3f uV\n', ...
                        ch, 100 * obj.error_scales(i),  1e6 * obj.error_offsets(i));
                end
                fprintf(logfile, '\n');
                fprintf(logfile, 'Current Channel Calibration Errors\n');
                for ch=1:2
                    il = rl_cal.IL_INDEX(ch);
                    ih = rl_cal.IH_INDEX(ch);
                    fprintf(logfile, '  Current I%i (Low Range):   %6.3f%% + %9.3f nA\n', ...
                        ch, 100 * obj.error_scales(il), 1e9 * obj.error_offsets(il));
                    fprintf(logfile, '  Current I%i (High Range):  %6.3f%% + %9.3f nA\n', ...
                        ch, 100 * obj.error_scales(ih), 1e9 * obj.error_offsets(ih));
                end
                fclose(logfile);
            end
            
            fprintf('Calibration data file: %s\n', fullfile(pwd, cal_filename));
            fprintf(' -> copy file to /etc/rocketlogger/calibration.dat on the logger to apply calibration.\n');
        end
        
        function fix_signs(obj)
            %FIX_SIGNS Fix the signs of the scales if two current channels
            %were calibrated simulateously
            
            % low range, high range are positive
            for i = obj.POSITIVE_SCALE_CHANNELS
                if obj.scales(i) < 0
                    obj.scales(i) = obj.scales(i) * -1;
                    fprintf('Info: Scale %i was inverted.\n', i);
                end
            end
        end
    end
    
    methods(Static)
        function obj = from_file(filename)
            %RL_CAL_READ Reads the calibration a RocketLogger calibration
            %file
            %   Parameters:
            %      - filename:    Calibration filename (default:
            %                       calibration.dat)
            
            if ~exist('filename', 'var')
                filename = 'calibration.dat';
            end
            
            % read in file
            file = fopen(num2str(filename), 'r');
            time = fread(file, 1, 'int64');
            offsets = fread(file, rl_cal.CHANNEL_COUNT, 'int32');
            scales = fread(file, rl_cal.CHANNEL_COUNT, 'double');
            fclose(file);
            
            % create object
            obj =  rl_cal(offsets, scales, time);
        end
        
        function obj = calibrate(v_rld, i1l_rld, i1h_rld, i2l_rld, i2h_rld, plotPareto)
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
            
            v = v_rld.get_data({'V1', 'V2', 'V3', 'V4'});
            
            % get earliest time
            time = min([v_rld.header.start_time(1), ...
                i1l_rld.header.start_time(1), i1h_rld.header.start_time(1), ...
                i2l_rld.header.start_time(1), i2h_rld.header.start_time(1)]);
            
            % Undo the scaling that was done while reading the file
            v = v / rl_cal.FILE_SCALE_V;
            ih(:, 1) = i1h / rl_cal.FILE_SCALE_IH;
            ih(1:length(i2h), 2) = i2h / rl_cal.FILE_SCALE_IH;
            il(:, 1) = i1l / rl_cal.FILE_SCALE_IL;
            il(1:length(i2l), 2) = i2l / rl_cal.FILE_SCALE_IL;
            
            % calculate the step sizes in ADC steps
            v_step_uncal = rl_cal.CAL_STEP_V / abs(rl_cal.UNCAL_STEP_V);
            il_step_uncal = rl_cal.CAL_STEP_IL / abs(rl_cal.UNCAL_STEP_IL);
            ih_step_uncal = rl_cal.CAL_STEP_IH / abs(rl_cal.UNCAL_STEP_IH);
            
            % init
            scales = zeros(rl_cal.CHANNEL_COUNT, 1);
            offsets = zeros(rl_cal.CHANNEL_COUNT, 1);
            residuals = zeros(rl_cal.CHANNEL_COUNT, rl_cal.CAL_NUM_POINTS);
            error_scales = zeros(rl_cal.CHANNEL_COUNT, 1);
            error_offsets = zeros(rl_cal.CHANNEL_COUNT, 1);
            
            %% Voltages
            
            % ideal values
            v_max = (rl_cal.CAL_NUM_POINTS - 1) / 4 * rl_cal.CAL_STEP_V;
            v_ideal = rl_cal.gen_dual_sweep_values(-v_max, v_max, rl_cal.CAL_STEP_V) / rl_cal.FILE_SCALE_V;
            
            % init
            avg_points_v = zeros(4, rl_cal.CAL_NUM_POINTS);
            
            % average and fitting
            for i=1:4
                index = rl_cal.V_INDEX(i);
                fprintf('Voltage Channel: %i\n', i);
                avg_points_v(i, :) = rl_aux_average_points(v(:, i), ...
                    rl_cal.CAL_NUM_POINTS, v_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(index), offsets(index), residuals(index, :), ...
                    error_scales(index), error_offsets(index)] = ...
                    rl_aux_lin_fit(avg_points_v(i, :), v_ideal);
                % scale errors
                error_offsets(index) = error_offsets(index) * rl_cal.FILE_SCALE_V;
            end
            
            if plotPareto ~= 0
                rl_aux_pareto_error(v_ideal * rl_cal.FILE_SCALE_V, ...
                    residuals(rl_cal.V_INDEX, :) * rl_cal.FILE_SCALE_V);
                title('Voltage Pareto Optimal Error Numbers');
            end
            
            %% Low Currents
            
            % ideal values
            il_max = (rl_cal.CAL_NUM_POINTS - 1) / 4 * rl_cal.CAL_STEP_IL;
            il_ideal = rl_cal.gen_dual_sweep_values(-il_max, il_max, rl_cal.CAL_STEP_IL) / rl_cal.FILE_SCALE_IL;
            
            % init
            avg_points_il = zeros(2, rl_cal.CAL_NUM_POINTS);
            
            % average and fitting
            for i=1:2
                index = rl_cal.IL_INDEX(i);
                fprintf('Current Channel %i (low)\n', i);
                
                avg_points_il(i,:) = rl_aux_average_points(il(:,i), ...
                    rl_cal.CAL_NUM_POINTS, il_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(index), offsets(index), residuals(index, :), ...
                    error_scales(index), error_offsets(index)] = ...
                    rl_aux_lin_fit(avg_points_il(i, :), il_ideal);
                % scale errors
                error_offsets(index) = error_offsets(index) * rl_cal.FILE_SCALE_IL;
            end
            
            if plotPareto ~= 0
                rl_aux_pareto_error(il_ideal*rl_cal.FILE_SCALE_IL, ...
                    residuals(rl_cal.IL_INDEX, :) * rl_cal.FILE_SCALE_IL);
                title('Current Low Pareto Optimal Error Numbers');
            end
            
            %% High Currents
            
            % ideal values
            ih_max = (rl_cal.CAL_NUM_POINTS - 1) / 4 * rl_cal.CAL_STEP_IH;
            ih_ideal = rl_cal.gen_dual_sweep_values(-ih_max, ih_max, rl_cal.CAL_STEP_IH) / rl_cal.FILE_SCALE_IH;
            
            % init
            avg_points_ih = zeros(2,rl_cal.CAL_NUM_POINTS);
            
            % average and fitting
            for i=1:2
                index = rl_cal.IH_INDEX(i);
                fprintf('Current Channel %i (high)\n', i);
                avg_points_ih(i, :) = rl_aux_average_points(ih(:, i), rl_cal.CAL_NUM_POINTS, ih_step_uncal, rl_cal.CAL_MIN_STABLE_SAMPLES);
                [scales(index), offsets(index), residuals(index, :), ...
                    error_scales(index), error_offsets(index)] = ...
                    rl_aux_lin_fit(avg_points_ih(i, :), ih_ideal);
                % scale errors
                error_offsets(index) = error_offsets(index) * rl_cal.FILE_SCALE_IH;
            end
            
            if plotPareto ~= 0
                rl_aux_pareto_error(ih_ideal * rl_cal.FILE_SCALE_IH, ...
                    residuals(rl_cal.IH_INDEX, :) * rl_cal.FILE_SCALE_IH);
                title('Current High Pareto Optimal Error Numbers');
            end
            
            %% Finishing
            offsets = offsets ./ scales;
            error_scales = error_scales ./ 100;

            obj = rl_cal(offsets, scales, time, error_offsets, error_scales);
        end
        
        function values = gen_dual_sweep_values(start, stop, step)
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
