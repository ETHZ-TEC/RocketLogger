%%
%% Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
%%

function fig = rl_aux_pretty_plot(figure_in)
%RL_AUX_PRETTY_PLOT Makes a figure better suited for use in a document
%   Parameters:
%      - figure_in:    Number / handle to the figure, a new figure is
%                        create if the parameter is omitted

if exist('figure_in', 'var')
    fig = figure(figure_in);
else
    fig = figure;
end

axes1 = fig.CurrentAxes;
set(axes1, 'FontSize', 24, 'XGrid', 'on', 'YGrid', 'on', 'YMinorTick', 'on');
set(axes1.Children, 'LineWidth', 3);

end
