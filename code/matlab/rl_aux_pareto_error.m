function [ offsetError, minScaleError ] = rl_aux_pareto_error(ideal, residual )
%RL_AUX_PARETO_ERROR Creates a plot of the pareto optimal error figures 
%(offset error and scale error)
%   Parameters:
%      - ideal:       Ideal values
%      - residual:    Deviation from the ideal values (one or multiple
%                       vectors)

jMax = size(residual,1);

figure;
hold on;
for j=1:jMax
    plot(ideal, residual(j, :));
end

% determine the minimum and maximum (optimal) offset error
offsetErrorMax = max(max(abs(residual)));
minOffsetError = max(max(abs(residual(:, ideal == 0))));
offsetError = linspace(minOffsetError, offsetErrorMax, 1000);

% calculate the minimum scale error for each chose offset error
for i=1:length(offsetError)
    remainingError2(:,:,i) = max(max(residual-offsetError(i),0), -min(residual+offsetError(i),0));
    for j=1:jMax
        scaleError2(j,:,i) = abs(remainingError2(j,:,i) ./ ideal);
    end
    temp2 = max(scaleError2(:,:,i),[], 1);
    minScaleError(i) = max(temp2(isfinite(temp2)));
end

figure;
scatter(offsetError, minScaleError*100);
xlabel('Offset Error');
ylabel('Scale Error [%]');

end

