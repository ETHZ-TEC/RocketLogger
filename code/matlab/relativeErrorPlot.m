function [ output_args ] = relativeErrorPlot( target, measured )
%RELATIVEERRORPLOT Summary of this function goes here
%   Detailed explanation goes here

assert(length(target) == size(measured,2), 'Invalid sizes');

for i=1:length(target)
    relative(:,i) = measured(:,i) ./ target(i);
end

avg = mean(relative,1);
dev = std(relative, 1);

fig = figure; errorbar(target, (avg), dev);

prettyPlot(fig);
axis = gca;
set(axis, 'XScale', 'log');

end

