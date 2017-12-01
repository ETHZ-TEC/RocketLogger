%%
%% Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
%%

classdef rld
    %RLD Class to read in and handle RocketLogger data
    %   Parameters:
    %      - file_name:          Data file name
    %      - decimation_factor:  Decimation factor for values read
    %                            (buffer size needs to be divisible
    %                             by the decimation factor)
    
    properties
        % RLD header (includes measurement information)
        header;
        % All channels with information and data
        channels;
        % Absolute buffer times
        time;
    end
    
    methods
        % constructor
        function [ obj ] = rld(file_name, decimation_factor)
            %RLD Creates an RLD object from RocketLogger data file
            %   Parameters:
            %      - file_name:          File name
            %      - decimation_factor:  Decimation factor for values read
            %                            (buffer size needs to be divisible
            %                             by the decimation factor)
            
            if ~exist('decimation_factor', 'var')
                decimation_factor = 1;
            end
            
            if exist('file_name', 'var')
                obj = read_file(obj, file_name, decimation_factor);
            end
        end
        
        % file reading
        function [ obj ] = read_file(obj, file_name, decimation_factor)
            %READ_FILE Reads a RocketLogger data file and returns a RLD object
            %   Parameters:
            %      - file_name:          Data file name
            %      - decimation_factor:  Decimation factor for values read
            %                            (buffer size needs to be divisible
            %                             by the decimation factor)
            
            %% IMPORT CONSTANTS
            rl_types;
            
            if ~exist('decimation_factor', 'var')
                decimation_factor = 1;
            end
            
            %% CHECK FILE
            % open file
            file = fopen(num2str(file_name));
            if file == -1
                error(['Could not open file: ', file_name]);
            else
                file_exists = true;
            end
            
            file_number = 1;
            while file_exists
                % check magic number
                [file_magic, bytes_read] = fread(file, 1, 'uint32');
                assert(bytes_read > 0, 'Failed to read file');
                
                % check file version
                file_version = fread(file, 1, 'uint16');
                switch file_version
                    case 1
                        % old magic number
                        assert(file_magic == RL_FILE_MAGIC_OLD, 'File is no correct RocketLogger data file');
                        warning('Old file version');
                    case {2, 3}
                        % new magic number
                        assert(file_magic == RL_FILE_MAGIC, 'File is no correct RocketLogger data file');
                    otherwise
                        error(['Unsupported file version ', num2str(file_version)]);
                end
                
                %% READ HEADER
                
                % lead-in
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
                
                % comment
                comment = fread(file, comment_length, 'int8=>char')';
                
                % channels
                % initialize
                
                % TODO: add check if files match!
                if file_number == 1
                    obj.channels = struct('unit', 0, 'unit_text', 0, 'channel_scale', 0, 'data_size', 0, ...
                        'valid_data_channel', 0, 'name',0, 'values', 0, 'valid', 0);
                end
                
                % read
                for i=1:channel_bin_count+channel_count
                    unit = fread(file, 1, 'uint32');
                    try
                        unit_text = UNIT_NAMES(unit+1);
                    catch
                        if unit == RL_UNIT_UNDEFINED
                            unit_text = 'undefined';
                        else
                            error(['Invalid channel unit: ', num2str(unit)]);
                        end
                    end
                    channel_scale = fread(file, 1, 'int32');
                    data_size = fread(file, 1, 'uint16');
                    valid_data_channel = fread(file, 1, 'uint16');
                    name = cellstr(fread(file, RL_FILE_CHANNEL_NAME_LENGTH, 'int8=>char')');
                    
                    % channel struct
                    if file_number == 1
                        obj.channels(i) = struct('unit', unit, 'unit_text', unit_text, 'channel_scale', channel_scale, 'data_size', data_size, ...
                            'valid_data_channel', valid_data_channel, 'name', name, 'values', 0, 'valid', 0);
                    end
                    
                end
                
                % header struct
                if file_number == 1
                    obj.header = struct('file_magic', file_magic, 'file_version', file_version, 'header_length', header_length, ...
                        'data_block_size', data_block_size, 'data_block_count', data_block_count, 'sample_count', sample_count, ...
                        'sample_rate', sample_rate, 'mac_address', mac_address, 'start_time', start_time, ...
                        'comment_length', comment_length, 'channel_bin_count', channel_bin_count, 'channel_count', channel_count, ...
                        'comment', comment);
                end
                
                %% PARSE HEADER
                % sanity checks
                if sample_count ~= data_block_count*data_block_size
                    warning('Inconsistency in number of samples taken');
                end
                
                % digital inputs
                digital_inputs_count = 0;
                for i=1:channel_bin_count
                    if obj.channels(i).unit == RL_UNIT_BINARY
                        digital_inputs_count = digital_inputs_count+1;
                    end
                end
                
                % valid
                range_valid_count = 0;
                for i=1:channel_bin_count
                    if obj.channels(i).unit == RL_UNIT_RANGE_VALID
                        range_valid_count = range_valid_count+1;
                    end
                end
                
                % scales
                channel_scales = ones(1, channel_count);
                for i=1:channel_count
                    channel_scales(i) = obj.channels(channel_bin_count + i).channel_scale;
                end
                
                % decimation
                data_points_per_buffer = ceil(data_block_size/decimation_factor);
                decimated_rate = sample_rate/data_block_size*data_points_per_buffer;
                if data_points_per_buffer*decimation_factor ~= data_block_size
                    error('The buffer size needs to be divisible by the decimation factor');
                end
                
                % update header for decimation
                decimated_sample_count = floor(sample_count/decimation_factor);
                
                if file_number == 1
                    obj.header.sample_count = decimated_sample_count;
                    obj.header.data_block_size = data_points_per_buffer;
                    obj.header.sample_rate = decimated_rate;
                else
                    obj.header.sample_count = obj.header.sample_count + decimated_sample_count;
                    obj.header.data_block_count = obj.header.data_block_count + data_block_count;
                end
                
                %% READ DATA
                num_bin_vals = ceil(channel_bin_count / (RL_FILE_SAMPLE_SIZE * 8));
                
                % values
                temp_time = nan(data_block_count, TIME_STAMP_SIZE);
                digital_data = zeros(decimated_sample_count, digital_inputs_count);
                valid_data = zeros(decimated_sample_count, range_valid_count);
                vals = zeros(decimated_sample_count, channel_count);
                
                % read values
                for i=0:data_block_count-1
                    % read time stamps
                    temp_time(i+1, :) = fread(file, TIME_STAMP_SIZE, 'uint64')';
                    
                    % read values
                    if i == data_block_count-1 && mod(sample_count, data_block_size) ~= 0
                        input_buffer_size = mod(sample_count, data_points_per_buffer); % unfull buffer size
                        buffer_size = mod(decimated_sample_count, data_points_per_buffer);
                    else
                        input_buffer_size = data_block_size;
                        buffer_size = data_points_per_buffer;
                    end
                    
                    buffer_values = fread(file, [num_bin_vals + channel_count, input_buffer_size], 'int32=>int32')';
                    
                    % split data
                    % binary
                    bin_buffer_values = buffer_values(:,1:num_bin_vals);
                    
                    digital_values = nan(input_buffer_size, digital_inputs_count);
                    valid_values = nan(input_buffer_size, range_valid_count);
                    
                    % TODO: implement if num_bin_vals > 1
                    for j=1:digital_inputs_count % digital inputs
                        digital_values(:,j) = bitand(bin_buffer_values, 2^(j-1)) > 0;
                    end
                    
                    k = 1;
                    for j=digital_inputs_count+1:channel_bin_count % valid data
                        valid_values(:,k) = bitand(bin_buffer_values, 2^(j-1)) > 0;
                        k = k + 1;
                    end
                    
                    % other
                    buffer_values = buffer_values(:,num_bin_vals+1:end);
                    
                    % decimation
                    if decimation_factor == 1
                        decimated_digital_values = digital_values;
                        decimated_valid_values = valid_values;
                        decimated_values = buffer_values;
                    else
                        for j=1:digital_inputs_count
                            decimated_digital_values(:,j) = rld.decimate_min(digital_values(:,j), decimation_factor); % TODO: preallocating
                        end
                        for j=1:range_valid_count
                            decimated_valid_values(:,j) = rld.decimate_min(valid_values(:,j), decimation_factor);
                        end
                        for j=1:channel_count
                            decimated_values(:,j) = rld.decimate_mean(buffer_values(:,j), decimation_factor);
                        end
                    end
                    
                    % merge buffers
                    if digital_inputs_count > 0
                        digital_data(i*data_points_per_buffer+1 : (i*data_points_per_buffer + buffer_size), :) = decimated_digital_values;
                    end
                    if range_valid_count > 0
                        valid_data(i*data_points_per_buffer+1 : (i*data_points_per_buffer + buffer_size), :) = decimated_valid_values;
                    end
                    vals(i*data_points_per_buffer+1 : (i*data_points_per_buffer + buffer_size), :) = decimated_values;
                    
                end
                
                %% PROCESS TIMESTAMPS
                if file_number == 1
                    obj.time = datetime(temp_time(:, 1) + temp_time(:, 2) .* 1e-9, 'ConvertFrom', 'posixTime');
                else
                    obj.time = [obj.time; datetime(temp_time(:, 1) + temp_time(:, 2) .* 1e-9, 'ConvertFrom', 'posixTime')];
                end
                %% STORE BINARY DATA
                
                if file_number == 1
                    for i=1:digital_inputs_count
                        obj.channels(i).values = digital_data(:,i);
                    end
                    j=1;
                    for i=digital_inputs_count+1:channel_bin_count
                        obj.channels(i).values = valid_data(:,j);
                        j = j + 1;
                    end
                else
                    for i=1:digital_inputs_count
                        obj.channels(i).values = [obj.channels(i).values; digital_data(:,i)];
                    end
                    j=1;
                    for i=digital_inputs_count+1:channel_bin_count
                        obj.channels(i).values = [obj.channels(i).values; valid_data(:,j)];
                        j = j + 1;
                    end
                end
                
                %% PROCESS CHANNEL DATA
                % scaling
                vals = double(vals) .* repmat((10 .^ channel_scales), decimated_sample_count, 1);
                
                if file_number == 1
                    for i=1:channel_count
                        obj.channels(channel_bin_count+i).values = vals(:,i);
                        if obj.channels(channel_bin_count+i).valid_data_channel ~= NO_VALID_CHANNEL
                            % add range info
                            valid_channel_index = obj.channels(channel_bin_count+i).valid_data_channel + 1;
                            % fix one-based channel link indexes for file version <= 2
                            if file_version <= 2
                                valid_channel_index = obj.channels(channel_bin_count+i).valid_data_channel;
                            end
                            obj.channels(channel_bin_count+i).valid = obj.channels(valid_channel_index).values;
                        end
                    end
                else
                    for i=1:channel_count
                        obj.channels(channel_bin_count+i).values = [obj.channels(channel_bin_count+i).values; vals(:,i)];
                        if obj.channels(channel_bin_count+i).valid_data_channel ~= NO_VALID_CHANNEL
                            % add range info
                            valid_channel_index = obj.channels(channel_bin_count+i).valid_data_channel + 1;
                            % fix one-based channel link indexes for file version <= 2
                            if file_version <= 2
                                valid_channel_index = obj.channels(channel_bin_count+i).valid_data_channel;
                            end
                            obj.channels(channel_bin_count+i).valid = obj.channels(valid_channel_index).values;
                        end
                    end
                end
                fclose(file);
                
                %% CHECK FOR ADDITIONAL FILES
                
                % check if file is RL part-file
                expression = '_p\d+\.';
                start_index = regexp(file_name, expression, 'ONCE');
                
                if ~isempty(start_index)
                    file_exists = false;
                else
                    start_index = regexp(file_name, '\.');
                    if isempty(start_index)
                        file_exists = false;
                    else
                        index = start_index(end);
                        before = file_name(1:index-1);
                        after = file_name(index+1:end);
                        new_file_name = [before, '_p', num2str(file_number),'.', after];
                        if exist(new_file_name, 'file')
                            file_number = file_number+1;
                            file = fopen(new_file_name);
                        else
                            file_exists = false;
                        end
                    end
                end
            end
            
            disp(['Read ', num2str(file_number), ' file(s).']);
        end
        
        % plotting
        function plot(obj, channel, absolute_time, pretty_plot)
            %PLOT Plots different channels of a RLD object
            %   Parameters:
            %      - channel:       Cell with channel names to plot
            %                       Also allowed: 'all', 'voltages',
            %                                     'currents', 'digital'
            %      - absolute_time: Set to 1, if x-axis should be in
            %                       absolute time
            %      - pretty_plot:   Set to 1, if pretty plot should be
            %                       applied
            
            % constants
            rl_types;
            
            % arguments
            if ~exist('pretty_plot', 'var')
                pretty_plot = false;
            end
            if ~exist('absolute_time', 'var')
                absolute_time = false;
            end
            
            % separate voltage/current axis
            separate_axis = true;
            
            % all channels
            if ~exist('channel', 'var') || sum(strcmp(channel, 'all')) == 1
                num_channels = obj.header.channel_count;
                plot_channels = (1:num_channels) + obj.header.channel_bin_count;
                
            % all voltages
            elseif sum(strcmp(channel, 'voltages')) == 1
                num_channels = 0;
                for i=1:(obj.header.channel_bin_count+obj.header.channel_count)
                    if obj.channels(i).unit == RL_UNIT_VOLT
                        num_channels = num_channels + 1;
                        plot_channels(num_channels) = i;
                    end
                end
                if num_channels == 0
                    error('No voltage channel found');
                end
                
            % all currents
            elseif sum(strcmp(channel, 'currents')) == 1
                num_channels = 0;
                for i=1:(obj.header.channel_bin_count+obj.header.channel_count)
                    if obj.channels(i).unit == RL_UNIT_AMPERE
                        num_channels = num_channels + 1;
                        plot_channels(num_channels) = i;
                    end
                end
                if num_channels == 0
                    error('No current channel found');
                end
                
            % all digital channels
            elseif sum(strcmp(channel, 'digital')) == 1
                num_channels = 0;
                for i=1:(obj.header.channel_bin_count+obj.header.channel_count)
                    if obj.channels(i).unit == RL_UNIT_BINARY
                        num_channels = num_channels + 1;
                        plot_channels(num_channels) = i;
                    end
                end
                if num_channels == 0
                    error('No digital channel found');
                end
                
            % selective
            else
                assert(iscell(channel), 'Channel argument has to be of type cell array');
                num_channels = numel(channel);
                plot_channels = zeros(1,num_channels);
                for i=1:num_channels
                    plot_channels(i) =  channel_index(obj, channel{i});
                    if channel_index(obj, channel{i}) < 1
                        error(['Channel ', channel{i}, ' not found']);
                    end
                end
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
            t = obj.get_time(absolute_time);
            
            colormap = [
                0    0.4470    0.7410
                0.8500    0.3250    0.0980
                0.9290    0.6940    0.1250
                0.4940    0.1840    0.5560
                0.4660    0.6740    0.1880
                0.3010    0.7450    0.9330
                0.6350    0.0780    0.1840];
            
            j1=1;
            j2=1;
            for i=1:num_channels
                channel_ind = plot_channels(i);
                if channel_ind > 0
                    %channel_ind = i + obj.header.channel_bin_count;
                    if (separate_axis && obj.channels(channel_ind).unit == RL_UNIT_AMPERE)
                        yyaxis right;
                        legends_i{j1} = obj.channels(channel_ind).name;
                        j1 = j1 + 1;
                    elseif (separate_axis &&  obj.channels(channel_ind).unit ~= RL_UNIT_AMPERE)
                        yyaxis left;
                        legends_v{j2} = obj.channels(channel_ind).name;
                        j2 = j2 + 1;
                    end
                    plot(t,obj.channels(channel_ind).values, 'LineStyle', '-', 'Color', ...
                        colormap(mod(i-1,size(colormap, 1))+1,:), 'Marker', 'none');%*scales_on(i));
                end
            end
            
            if separate_axis
                yyaxis left;
                axis_left = gca();
                axis_left.YColor = 'black';
                if pretty_plot
                    rl_aux_pretty_plot(fig)
                end
                
                yyaxis right;
                axis_right = gca();
                axis_right.YColor = 'black';
                if pretty_plot
                    rl_aux_pretty_plot(fig)
                end
                
            elseif pretty_plot
                rl_aux_pretty_plot(fig)
            end
            
            % legend
            if exist('legends_v', 'var')
                if exist('legends_i', 'var')
                    legend([legends_v, legends_i]);
                else
                    legend(legends_v);
                end
            else
                legend(legends_i);
            end
            
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
        function [ names ] = get_channels(obj)
            %GET_CHANNELS Returns a cell including all channel names of the RLD object
            
            for i=1:length(obj.channels(1,:))
                names{i} = obj.channels(i).name;
            end
        end
        
        % get channel data
        function [ values ] = get_data(obj, channel)
            %GET_DATA Returns a matrix with channel data
            %   Parameters:
            %      - channel:  Cell with channel names of selected channels
            
            if ~exist('channel', 'var') || sum(strcmp(channel, 'all')) == 1
                channel = obj.get_channels();
            end
            
            assert(iscell(channel), 'Channel argument has to be of type cell array');
            
            num_channels = numel(channel);
            values = nan(obj.header.sample_count, num_channels);
            
            for i=1:num_channels
                ch_ind = channel_index(obj, channel{i});
                if ch_ind > 0
                    values(:,i) = obj.channels(ch_ind).values;
                else
                    error(['No channel ', channel{i}, ' found']);
                end
            end
        end
        
        % get measurement timestamps
        function [ timestamps ] = get_time(obj, absolute_time)
            %GET_TIME Returns a matrix with the timestamps of the data of the RLD object
            
            if ~exist('absolute_time', 'var')
                absolute_time = false;
            end
            
            % time interpolation
            if absolute_time
                points = 0:obj.header.data_block_count;
                temp_time = [obj.time; obj.time(end) + seconds(1)];
                interp_points = (0:(obj.header.sample_count-1)) / obj.header.data_block_size;
                timestamps = interp1(points, temp_time, interp_points)';
            else
                timestamps = (0:obj.header.sample_count-1)'/obj.header.sample_rate;
            end
        end
        
        % convert to old format (for backward comptability)
        function [values, header] = convert(obj)
            %CONVERT Returns data in old format
            
            % old constants
            CHANNEL_NAMES = [cellstr('I1H'),cellstr('I1M'),cellstr('I1L'),cellstr('V1'),cellstr('V2'),cellstr('I2H'),cellstr('I2M'),cellstr('I2L'),cellstr('V3'),cellstr('V4')];
            VALID_NAMES = [cellstr('I1L_valid'), cellstr('I2L_valid')];
            MAX_CHANNEL_COUNT = 10;
            
            % header
            header = nan(5,1);
            
            % rate
            rate = obj.header.sample_rate;
            if rate == 1000 || rate == 2000 || rate == 4000 || rate == 8000 || rate == 16000
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
                values(:,1) = obj.channels(r1).values;
            end
            if r2 > 0
                values(:,2) = obj.channels(r2).values;
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
        function [ on ] = channel_index(obj, channel)
            %CHANNEL_INDEX Returns channel index (position in channel array)
            %              (0 if not available)
            %   Parameters:
            %      - channel:  Name of selected channel
            
            on = 0;
            
            for i=1:size(obj.channels, 2)
                if strcmp(channel, obj.channels(i).name) == 1
                    on = i;
                    break;
                end
            end
        end
        
        % merges two channels to a new one
        function [ merged_obj ] = merge_channels(obj)
            %MERGE_CHANNELS Merges current low/high ranges and returns new RLD object
            
            rl_types;
            
            num_channels_to_merge = 2;
            low_ind(1) = channel_index(obj, 'I1L');
            high_ind(1) = channel_index(obj, 'I1H');
            low_ind(2) = channel_index(obj, 'I2L');
            high_ind(2) = channel_index(obj, 'I2H');
            merged_name = {'I1', 'I2'};
            
            channels_to_remove = zeros(1,6);
            if low_ind(1) > 0 && high_ind(1) > 0
                channels_to_remove(1) = low_ind(1);
                channels_to_remove(2) = high_ind(1);
                channels_to_remove(3) = obj.channels(low_ind(1)).valid_data_channel;
            end
            if low_ind(2) > 0 && high_ind(2) > 0
                channels_to_remove(4) = low_ind(2);
                channels_to_remove(5) = high_ind(2);
                channels_to_remove(6) = obj.channels(low_ind(2)).valid_data_channel;
            end
            
            % return object
            merged_obj = rld();
            merged_obj.header = obj.header;
            merged_obj.time = obj.time;
            % initialize
            merged_obj.channels = struct('unit', {}, 'unit_text', {}, 'channel_scale', {}, 'data_size', {}, ...
                'valid_data_channel', {}, 'name', {}, 'values', {}, 'valid', {});
            num_new_channels = 0;
            num_new_bin_channels = 0;
            for i=1:obj.header.channel_bin_count + obj.header.channel_count
                if isempty(find(channels_to_remove == i,1))
                    merged_obj.channels(num_new_channels+num_new_bin_channels+1) = obj.channels(i);
                    if obj.channels(i).unit == RL_UNIT_BINARY || obj.channels(i).unit == RL_UNIT_RANGE_VALID
                        num_new_bin_channels = num_new_bin_channels+1;
                    else
                        num_new_channels = num_new_channels+1;
                    end
                end
            end
            
            for i=1:num_channels_to_merge
                if low_ind(i) < 1 || high_ind(i) < 1
                    warning(['Channel (', merged_name{i}, ') not valid']);
                else
                    if obj.channels(low_ind(i)).valid_data_channel == NO_VALID_CHANNEL
                        error('Low range has no valid data');
                    end
                    
                    % filter range valid data
                    filter_size = 2*RANGE_MARGIN + 1;
                    filter = ones(1, filter_size);
                    range_valid = ~(conv(double(~obj.channels(low_ind(i)).valid), filter) > 0);
                    range_valid = range_valid(RANGE_MARGIN+1:end-RANGE_MARGIN); % resize
                    
                    % set properties
                    new_channel_ind = length(merged_obj.channels) + 1;
                    unit = RL_UNIT_AMPERE;
                    unit_text = UNIT_NAMES(unit+1);
                    channel_scale = 0; % TODO
                    data_size = 0; % TODO
                    valid_data_channel = NO_VALID_CHANNEL;
                    name = [merged_name{i}];
                    if channel_index(merged_obj, name) > 0
                        error(['Channel name ', name, ' already used']);
                    end
                    valid = 0;
                    
                    % merge values
                    values = obj.channels(low_ind(i)).values .* range_valid + obj.channels(high_ind(i)).values .* ~range_valid;
                    
                    % add new channel
                    merged_obj.channels(new_channel_ind) = struct('unit', unit, 'unit_text', unit_text, 'channel_scale', channel_scale, ...
                        'data_size', data_size, 'valid_data_channel', valid_data_channel, 'name', name, 'values', values, 'valid', valid);
                    num_new_channels = num_new_channels  + 1;
                end
            end
            merged_obj.header.channel_bin_count = num_new_bin_channels;
            merged_obj.header.channel_count = num_new_channels;
        end
        
        
    end
    
    methods(Static)
        
        % decimate functions
        function [ decimated_values ] = decimate_bin(values, decimation_factor)
            %DECIMATE_BIN Decimates binary values (with threshold)
            %   Parameters:
            %      - values:            Values to decimate
            %      - decimation_factor: Decimation factor
            
            new_num = floor(length(values)/decimation_factor);
            old_num = new_num * decimation_factor;
            M = reshape(values(1:old_num), [decimation_factor, new_num]);
            decimated_values = mean(M, 1)>0.5;
        end
        
        function [ decimated_values ] = decimate_min(values, decimation_factor)
            %DECIMATE_MIN Decimates valid values (only valid if all samples valid)
            %   Parameters:
            %      - values:            Values to decimate
            %      - decimation_factor: Decimation factor
            
            new_num = floor(length(values)/decimation_factor);
            old_num = new_num * decimation_factor;
            M = reshape(values(1:old_num), [decimation_factor, new_num]);
            decimated_values = ~(mean(M, 1)<1);
        end
        
        function [ decimated_values ] = decimate_mean(values, decimation_factor)
            %DECIMATE_MEAN Decimates analog values (with MEAN)
            %   Parameters:
            %      - values:            Values to decimate
            %      - decimation_factor: Decimation factor
            
            new_num = floor(length(values)/decimation_factor);
            old_num = new_num * decimation_factor;
            M = reshape(values(1:old_num), [decimation_factor, new_num]);
            decimated_values = mean(M, 1);
        end
        
    end
    
end
