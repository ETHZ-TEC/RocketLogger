function [  ] = rl_plot_values( values, header, separate_axis, pretty_plot )
legends    = ['I1H';'I1M';'I1L';'V1 ';  'V2 ';'I2H';'I2M';  'I2L';'V3 ';'V4 '];
%scale      = [    1;    1; 1e-3;    1;      1;    1;    1;   1e-3;    1;    1];
is_current = [true;  true; true; false; false; true;  true; true; false; false];

if ~exist('separate_axis', 'var')
    separate_axis = false;
end
if ~exist('pretty_plot', 'var')
    pretty_plot = false;
end

number_samples = header(1);
buffer_size = header(2);
rate = header(3);
channels = header(4);
precision = header(5);
number_channels = count_bits(channels);

j=1;
for i=1:10
    if rl_channel_on( channels, i-1 ) == 1
        legends_on(j,:) = legends(i,:);
        %scales_on(j) = scale(i);
        is_current_on(j) = is_current(i);
        j = j+1;
    end
end

% plotting
fig = figure;
hold on;
grid on;
t = (1:number_samples)/rate;
colormap = [
    0    0.4470    0.7410
    0.8500    0.3250    0.0980
    0.9290    0.6940    0.1250
    0.4940    0.1840    0.5560
    0.4660    0.6740    0.1880
    0.3010    0.7450    0.9330
    0.6350    0.0780    0.1840];

for i=1:number_channels
    if (separate_axis && is_current_on(i))
        yyaxis right;
    elseif (separate_axis && ~is_current_on(i))
        yyaxis left;
    end
    plot(t,values(:,i+2), 'LineStyle', '-', 'Color', ...
        colormap(mod(i-1,size(colormap, 1))+1,:), 'Marker', 'none');%*scales_on(i));
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

% If we have separate axis, the left values appear first in the legend
if separate_axis
    legend([legends_on(~is_current_on, :); legends_on(is_current_on, :)]);
else
    legend(legends_on);
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

