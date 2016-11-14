classdef rl_cal
    %RL_CAL Class that handles the calibration of the RocketLogger
    %   Detailed explanation goes here
    
    properties(Constant)
        CHANNEL_COUNT = 8;
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
        
    end
    
end

