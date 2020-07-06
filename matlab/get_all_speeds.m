function [ s ] = get_all_speeds( m )
%get_all_dists_from Use file dump-all-nodes-mobs.csv
%   Detailed explanation goes here

    S = size(m);
    n = S(2) - 1;

    s = 0;
    
    if mod(n, 4) ~= 0
        return;
    end
    
    N = n / 4;

    s = zeros(S(1), N);
    
    cnter = 1;
    for i = 5 : 4 : S(2)
        s(:, cnter) = m(:, i);
        plot(m(:, 1), s(:, cnter), 'DisplayName', sprintf('Speed node %d', cnter));
        hold on;
        cnter = cnter + 1;
    end
    grid on
    legend('show');
    title('Зависимость скорости узлов от времени');
    xlabel('Время, с');
    ylabel('Скорость, м/с');
    hold off;
end
