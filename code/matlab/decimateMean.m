function [ decimatedValues ] = decimateMean( values, decimationFactor )
%DECIMATEMEAN decimation of data, similar to decimate(...)
%   Faster version of decimate, uses averaging instead of FIR filter

M = reshape(values, [decimationFactor, length(values)/decimationFactor]);
decimatedValues = mean(M, 1);


end

