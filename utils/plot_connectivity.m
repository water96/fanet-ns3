function [ r ] = plot_connectivity( x, y, m, desc )
%UNTITLED3 Summary of this function goes here
%   Detailed explanation goes here

    xn = size(x, 1);
    yn = size(y, 1);
    Sm = size(m);
    
    if Sm(2) ~= xn || Sm(1) ~= yn
        return;
    end
    
    hold on
    for i = 1:yn
        plot(x, m(i, :), 'DisplayName', strcat(desc, num2str(y(i))),...
                               'LineStyle', ':',...
                               'LineWidth', 1.,...
                               'Marker', 'o',...
                               'MarkerSize', 8)
    end
    legend('show')
    grid on
    hold off
    r = zeros(Sm);
    
end

