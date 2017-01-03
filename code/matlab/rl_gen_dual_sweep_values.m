function [ values ] = rl_gen_dual_sweep_values( start, stop, step )
%RL_GEN_DUAL_SWEEP_VALUES Summary of this function goes here
%   Detailed explanation goes here

values = [start:step:stop-step, stop:-step:start];

end

