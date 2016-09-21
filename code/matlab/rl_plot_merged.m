function [  ] = rl_plot_merged( values, header )

number_samples = header(1);
% disable med/low currents
header(4) = bitset(header(4), 2, 0);
header(4) = bitset(header(4), 3, 0);
header(4) = bitset(header(4), 7, 0);
header(4) = bitset(header(4), 8, 0);

% add useless low-range values
values = [zeros(number_samples,2),values];

rl_plot_values(values,header, 1);

end
