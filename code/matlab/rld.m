classdef rld
    %RL_MEASUREMENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (Constant)
        % TODO: rl_types here?
    end
    
    properties
        header;
        channels;
        time;
    end
    
    methods
        % constructor
        function obj = rld(file_name)
            obj = read_file(obj, file_name );
        end
        
        % file reading
        function obj = read_file(obj, file_name )
            %RL_READ_FILE Read in RocketLogger binary file
            %   Detailed explanation goes here

            %% IMPORT CONSTANTS
            rl_types;

            %% CHECK FILE
            % open file
            file = fopen(num2str(file_name));
            if file == -1
                error(['Could not open file: ', file_name]);
            end

            % check magic number
            assert(fread(file, 1, 'uint32') == RL_FILE_MAGIC, 'File is no correct RocketLogger data file');

            % check file version
            file_version = fread(file, 1, 'uint16');


            %% READ HEADER

            % lead-in
            if file_version == 1

                header_length = fread(file, 1, 'uint16');
                data_block_size = fread(file, 1, 'uint32');
                data_block_count = fread(file, 1, 'uint32');
                sample_count = fread(file, 1, 'uint64');
                sample_rate = fread(file, 1, 'uint16');
                mac_address = fread(file, MAC_ADDRESS_LENGTH, 'uint8')';
                start_time = fread(file, 2, 'uint64');
                comment_length = fread(file, 1, 'uint32');
                channel_bin_count = fread(file, 1, 'uint16');
                channel_count = fread(file, 1, 'uint16');

            else
               error(['Unknown file version ', num2str(file_version)]); 
            end

            % comment
            comment = fread(file, comment_length, 'int8=>char')';
            
            

            % channels
            % initialize
            obj.channels = struct('unit', 0, 'unit_text', 0, 'channel_scale', 0, 'data_size', 0, ...
                    'valid_data_channel', 0, 'name',0);
            % read
            for i=1:channel_bin_count+channel_count

                unit = fread(file, 1, 'uint32');
                unit_text = UNIT_NAMES(unit+1);
                channel_scale = fread(file, 1, 'int32');
                data_size = fread(file, 1, 'uint16');
                valid_data_channel = fread(file, 1, 'uint16');
                name = cellstr(fread(file, RL_FILE_CHANNEL_NAME_LENGTH, 'int8=>char')');

                % channel struct
                obj.channels(i) = struct('unit', unit, 'unit_text', unit_text, 'channel_scale', channel_scale, 'data_size', data_size, ...
                    'valid_data_channel', valid_data_channel, 'name', name);

            end

            % header struct
            obj.header = struct('header_length', header_length, 'data_block_size', data_block_size, ...
                'data_block_count', data_block_count, 'sample_count', sample_count, 'sample_rate', sample_rate, ...
                'mac_address', mac_address, 'start_time', start_time, 'comment_length', comment_length, ...
                'channel_bin_count', channel_bin_count, 'channel_count', channel_count, 'comment', comment);

            %% PARSE HEADER
            % digital inputs
            digital_inputs_count = 0;
            for i=1:channel_bin_count
                if obj.channels(i).unit == RL_UNIT_BINARY
                    digital_inputs_count = digital_inputs_count+1;
                end
            end
            
            % scales
            channel_scales = ones(1, channel_count);
            for i=1:channel_count
                channel_scales(i) = obj.channels(channel_bin_count + i).channel_scale;
            end

            %% READ DATA
            num_bin_vals = ceil(channel_bin_count / (RL_FILE_SAMPLE_SIZE * 8));

            % values
            obj.time = datetime(0, 'ConvertFrom', 'posixTime');
            bin_channel_data = zeros(sample_count, num_bin_vals);
            vals = zeros(sample_count, channel_count);

            % read values
            for i=0:data_block_count-1
                % read time stamps
                temp_time = fread(file, TIME_STAMP_SIZE, 'uint64')';
                obj.time(i+1, :) = datetime(temp_time(1) + temp_time(2) * 1e-9, 'ConvertFrom', 'posixTime');

                % read
                if i == data_block_count-1 && mod(sample_count, data_block_size) ~= 0
                    buffer_size = mod(sample_count, data_block_size); % unfull buffer size
                else
                    buffer_size = data_block_size;
                end

                buffer_values = fread(file, [num_bin_vals + channel_count, buffer_size], 'int32=>int32')';

                % split data
                bin_channel_data(i*data_block_size+1 : (i*data_block_size + buffer_size), :) = buffer_values(:,1:num_bin_vals);
                vals(i*data_block_size+1 : (i*data_block_size + buffer_size), :) = buffer_values(:,num_bin_vals+1:end);

            end

            %% SPLIT BINARY DATA
            % TODO: implement if num_bin_vals > 1
            
            % digital inputs
            for i=1:digital_inputs_count
                obj.channels(i).values = bitand(bin_channel_data, 2^i) > 0;
            end
            
            % valid data
             for i=digital_inputs_count+1:channel_bin_count
                obj.channels(i).values = ~(bitand(bin_channel_data, 2^i) > 0);
            end

            %% PROCESS CHANNEL DATA
            % scaling
            vals = double(vals) .* repmat((10 .^ channel_scales), sample_count, 1);
            for i=1:channel_count
                obj.channels(channel_bin_count+i).values = vals(:,i);
                if obj.channels(channel_bin_count+i).valid_data_channel ~= NO_VALID_CHANNEL
                    % add range info
                    obj.channels(channel_bin_count+i).valid = obj.channels(obj.channels(channel_bin_count+i).valid_data_channel).values;
                end
            end
            fclose(file);
        end 
        
        % plotting
        function plot(obj, pretty_plot, varargin )
            
            % constants
            rl_types;
            
            % arguments
            if ~exist('pretty_plot', 'var')
                pretty_plot = false;
            end
            separate_axis = true;
            
            % selective plot
            if nargin > 2
                num_channels = length(varargin);
                plot_channels = zeros(1,num_channels);
                for i=1:num_channels
                    plot_channels(i) =  channel_index(obj, varargin{i});
                    if channel_index(obj, varargin{i}) < 1
                        warning(['Channel ', varargin{i}, ' not found']);
                    end
                end
            else
                num_channels = obj.header.channel_count;
                plot_channels = (1:num_channels) + obj.header.channel_bin_count;
            end
            
            % no valid channels
            if sum(plot_channels) == 0
               error('No valid channel found'); 
            end
            
            % plotting
            fig = figure;
            hold on;
            grid on;
            
            % time interpolation
            points = 0:(obj.header.data_block_count - 1);
            interp_points = (0:(obj.header.sample_count - 1)) / obj.header.data_block_size;
            t = interp1(points, obj.time, interp_points);%(1:obj.header.sample_count)/obj.header.sample_rate;
            
            colormap = [
                0    0.4470    0.7410
                0.8500    0.3250    0.0980
                0.9290    0.6940    0.1250
                0.4940    0.1840    0.5560
                0.4660    0.6740    0.1880
                0.3010    0.7450    0.9330
                0.6350    0.0780    0.1840];

            j = 1;
            for i=1:num_channels
                channel_ind = plot_channels(i);
                if channel_ind > 0
                    %channel_ind = i + obj.header.channel_bin_count;
                    if (separate_axis && obj.channels(channel_ind).unit == RL_UNIT_AMPERE)
                        yyaxis right;
                    elseif (separate_axis &&  obj.channels(channel_ind).unit == RL_UNIT_VOLT)
                        yyaxis left;
                    end
                    plot(t,obj.channels(channel_ind).values, 'LineStyle', '-', 'Color', ...
                        colormap(mod(i-1,size(colormap, 1))+1,:), 'Marker', 'none');%*scales_on(i));
                    legends{j} = obj.channels(channel_ind).name;
                    j = j+1;
                end
            end

            if separate_axis
                yyaxis left;
                axis_left = gca();
                axis_left.YColor = 'black';
                if pretty_plot 
                    prettyPlot(fig) 
                end

                yyaxis right;
                axis_right = gca();
                axis_right.YColor = 'black';
                if pretty_plot 
                    prettyPlot(fig) 
                end

            elseif pretty_plot
                prettyPlot(fig)       
            end

            legend(legends);
            
            title('Data xxx');
            if separate_axis
                yyaxis left;
                ylabel('Voltage [V]');
                yyaxis right;
                ylabel('Current [A]');
            else
                ylabel('Voltage [V], current [A]');
            end
            xlabel('Time [s]');
            
        end
        
        % get channels
        function names = get_channels(obj)
            for i=1:length(obj.channels(1,:))
                names{i} = obj.channels(i).name;
            end
        end
        
        % get channel data
        function values = get_data(obj, channel)
            
            ch_ind = channel_index(obj, channel);
            if ch_ind > 0
                values = obj.channels(ch_ind).values;
            else
                values = nan;
                warning('No such channel found');
            end
%             for i=1:length(obj.channels(1,:))
%                 if strcmp(channel, obj.channels(i).name) == 1
%                     values = obj.channels(i).values;
%                     break;
%                 end
%                 values = nan;
%             end
%             if isnan(values)
%                 warning('No such channel found');
%             end
        end
        
        % convert to old format (for backward comptability)
        function [values, header] = convert(obj)
            
            % old constants
            CHANNEL_NAMES = [cellstr('I1H'),cellstr('I1M'),cellstr('I1L'),cellstr('V1'),cellstr('V2'),cellstr('I2H'),cellstr('I2M'),cellstr('I2L'),cellstr('V3'),cellstr('V4')];
            VALID_NAMES = [cellstr('I1L_valid'), cellstr('I2L_valid')];
            MAX_CHANNEL_COUNT = 10;
            
            % header
            header = nan(5,1);
            
            % rate
            rate = obj.header.sample_rate;
            if rate == 1 || rate == 1 || rate == 1 || rate == 1 || rate == 1
                precision = 24;
            else
                precision = 16;
            end
            
            % values
            values = zeros(obj.header.sample_count, obj.header.channel_count + 2);
            
            % valid information
            r1 = channel_index(obj, VALID_NAMES(1));
            r2 = channel_index(obj, VALID_NAMES(2));
            if r1 > 0
                values(1) = obj.channels(r1).values;
            end
            if r2 > 0
                values(2) = obj.channels(r2).values;
            end
            
            % channels
            ch = 0;
            ind = 3;
            for i=1:MAX_CHANNEL_COUNT
                c = channel_index(obj,CHANNEL_NAMES(i));
                if c > 0
                    ch = bitor(ch, 2^(i-1));
                    values(:, ind) = obj.channels(c).values;
                    ind = ind + 1;
                end
            end    
            
            % header
            header(1) = obj.header.sample_count;
            header(2) = obj.header.data_block_size;
            header(3) = rate * 1000;
            header(4) = ch;
            header(5) = precision;
            
        end
        
        % returns index of selected channel (0, if off)
        function on = channel_index(obj, channel)
            for i=1:length(obj.channels(1,:))
                if strcmp(channel, obj.channels(i).name) == 1
                    on = i;
                    break;
                end
                on = 0;
            end
        end
        
        
    end 
end

