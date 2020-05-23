function [ dist ] = get_all_dists_from( m, n_src )
%get_all_dists_from Use file dump-all-nodes-mobs.csv
%   Detailed explanation goes here

    S = size(m);
    n = S(2) - 1;

    dist = 0;
    
    if mod(n, 4) ~= 0
        return;
    end
    
    N = n / 4;
    
    if n_src == 0
        return;
    end
    
    if n_src > N
        return;
    end
    
    dist = zeros(S(1), N);
    
    for i = 1 : N
        dist(:, i) = sqrt((m(:, n_src*4) - m(:, i*4)).^2 + (m(:, n_src*4 - 1) - m(:, i*4 - 1)).^2 + (m(:, n_src*4 - 2) - m(:, i*4 - 2)).^2);
        plot(m(:, 1), dist(:, i), 'DisplayName', sprintf('from %d to %d', n_src, i));
        hold on;
    end
    grid on
    legend('show');
    title(sprintf('Расстояние от узла %d до всех остальных узлов сети', n_src));
    xlabel('Время, с');
    ylabel('Расстояние, м');
    hold off;
end

