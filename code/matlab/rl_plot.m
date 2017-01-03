function [ values, header ] = rl_plot( filename, binary, old_file )

% default values
if ~exist('binary','var')
    binary = 0;
end

if ~exist('old_file','var')
    old_file = 0;
end

% read file
if binary == 1
    [values, header] = rl_read_bin(filename, old_file);
else
    [values, header] = rl_read_csv(filename);
end

% plot
rl_plot_values(values, header, true);

end