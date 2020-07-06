function [ dist ] = get_dist( m, n1, n2 )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here

    S = size(m);
    n = S(2) - 1;

    dist = 0;
    
    if mod(n, 4) ~= 0
        return;
    end
    
    N = n / 4;
    
    if n1 == 0 || n2 == 0
        return;
    end
    
    if n1 > N || n2 > N
        return;
    end
    
    
    dist = sqrt((m(:, n1*4) - m(:, n2*4)).^2 + (m(:, n1*4 - 1) - m(:, n2*4 - 1)).^2 + (m(:, n1*4 - 2) - m(:, n2*4 - 2)).^2);
    
end

