function [ offsetError, minScaleError ] = rl_pareto_error(ideal, residual )
%RL_PARETO_ERROR Summary of this function goes here
%   Detailed explanation goes here


jMax = size(residual,1);

figure;
hold on;
for j=1:jMax
    plot(ideal, residual(j, :));
end

offsetErrorMax = max(max(abs(residual)));
minOffsetError = max(max(abs(residual(:, ideal == 0))));
offsetError = linspace(minOffsetError, offsetErrorMax, 1000);

for i=1:length(offsetError)
    remainingError2(:,:,i) = max(max(residual-offsetError(i),0), -min(residual+offsetError(i),0));
    for j=1:jMax
        scaleError2(j,:,i) = abs(remainingError2(j,:,i) ./ ideal);
    end
    temp2 = max(scaleError2(:,:,i),[], 1);
    minScaleError(i) = max(temp2(isfinite(temp2)));
end

figure;
scatter(offsetError, minScaleError);
xlabel('Offset');
ylabel('Scale');

end

