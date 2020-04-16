function [ diff ] = get_diff( x )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
    N = length(x);

    x_e = x(2:2:end);
    x_o = x(1:2:end);
    x_diff = x_e - x_o;
    x_diff2 = x_o(2:end) - x_e(1:length(x_e)-1);
    
    diff = zeros(N, 1);
    
    diff(2:2:end) = x_diff;
    diff(3:2:end) = x_diff2;
end

