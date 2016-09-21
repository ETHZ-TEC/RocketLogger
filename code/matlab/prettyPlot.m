function [ fig ] = prettyPlot( n )
%PRETTYPLOT Summary of this function goes here
%   Detailed explanation goes here

if exist('n', 'var')
    fig = figure(n);
else
    fig = figure;
end

axes1 = fig.CurrentAxes;
set(axes1,'FontSize',24,'XGrid','on','YGrid','on','YMinorTick','on');
set(axes1.Children, 'LineWidth', 3);

end

