function [ fig ] = rl_aux_pretty_plot( figure_in )
%PRETTYPLOT Summary of this function goes here
%   Detailed explanation goes here

if exist('n', 'var')
    fig = figure(figure_in);
else
    fig = figure;
end

axes1 = fig.CurrentAxes;
set(axes1,'FontSize',24,'XGrid','on','YGrid','on','YMinorTick','on');
set(axes1.Children, 'LineWidth', 3);

end

