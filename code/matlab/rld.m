classdef rld
    %RL_MEASUREMENT Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (Constant)
        % TODO: rl_types here?
    end
    
    properties
        header;
        channels;
        time; % TODO: do something
    end
    
    methods
        % constructor
        function obj = rld(file_name)
            obj = read_file(obj, file_name );
        end
        
        % file reading
        function obj = read_file(obj, file_name )
            %RL_READ_FILE Summary of this function goes here
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
                unit_text = UNIT_NAMES(unit+1,:);
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

            channel_scales = ones(1, channel_count);
            for i=1:channel_count
                channel_scales(i) = obj.channels(channel_bin_count + i).channel_scale;
            end

            %% READ DATA
            num_bin_vals = ceil(channel_bin_count / (RL_FILE_SAMPLE_SIZE * 8));

            % values
            obj.time = zeros(data_block_count, TIME_STAMP_SIZE);
            bin_channel_data = zeros(sample_count, num_bin_vals);
            vals = zeros(sample_count, channel_count);

            % read values
            for i=0:data_block_count-1
                % read time stamps
                obj.time(i+1, :) = fread(file, TIME_STAMP_SIZE, 'uint64')';

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
            % TODO: not add to channels?
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
        function plot(obj, channels)
            if ~exist('channels', 'var')
                % TODO: pretty plot
                figure;
                hold on;
                for i=1:obj.header.channel_count
                    plot(obj.channels(obj.header.channel_bin_count + i).values);
                end
            else
                % TODO
            end
        end
        
        % get channels
        function show_channels(obj)
            for i=1:length(obj.channels(1,:))
                disp(obj.channels(i).name);
            end
        end
        
        % get channel data
        function values = get_data(obj, channel)
            for i=1:length(obj.channels(1,:))
                if strcmp(channel, obj.channels(i).name) == 1
                    values = obj.channels(i).values;
                    break;
                end
                values = nan;
            end
            if isnan(values)
                warning('No such channel found');
            end
        end
        
    end 
end

