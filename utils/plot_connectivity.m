function [ y ] = plot_connectivity( n_key, v_key, nvm )
%UNTITLED3 Summary of this function goes here
%   Detailed explanation goes here

    Nn = size(n_key, 1);
    Nv = size(v_key, 1);
    Snvm = size(nvm);
    
    if Snvm(1) ~= Nn || Snvm(2) ~= Nv
        return;
    end
    
    figure
    hold on
    for i = 1:Nv
        plot(n_key, nvm(:, i))
        legend(strcat('v=', num2str(v_key(i))))
    end
    grid on
    hold off
    y = zeros(Snvm);
    
end

