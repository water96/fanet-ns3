function [  ] = plot_traces( m )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here

    S = size(m);
    n = S(2) - 1;

    if mod(n, 4) ~= 0
        return;
    end
    
    N = n / 4;
    
    X = zeros(S(1), N);
    Y = zeros(S(1), N);
    Z = zeros(S(1), N);
    
    cnt = 1;
    for i = 2 : 4 : n
        X(:, cnt) = m(:, i);
        Y(:, cnt) = m(:, i + 1);
        Z(:, cnt) = m(:, i + 2);
        cnt = cnt+1;
    end
    
    p = plot3(X, Y, Z);
    legend('show');
    grid on;
    title('Nodes traces');
    xlabel('x(t)');
    ylabel('y(t)');
    zlabel('z(t)');
end

