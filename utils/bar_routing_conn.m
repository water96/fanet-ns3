
function b = bar_routing_conn(mob_name, nodes, vel)

addpath('/home/aleksey/work/export_fig');
if strcmp(mob_name, '')
    print('Error!');
end

r = ["AODV", "GPSR", "OLSR"];

b = 0;
c = zeros(nodes, length(r));

c_max = 0;
r_i = 1;
n = 0;
for i = 1 : length(r)
    n = importnodes(sprintf('%s_%s_NVM/%s_%s_node_key.csv', mob_name, r(i), mob_name, r(i)));
    v = importvel(sprintf('%s_%s_NVM/%s_%s_vel_key.csv', mob_name, r(i), mob_name, r(i)));
    nvm = importnvm(sprintf('%s_%s_NVM/%s_%s_NV_udp_conn.csv', mob_name, r(i), mob_name, r(i)));
    v_i = find(v == vel, 1);
    if isempty(v_i)
        print('Error!');
        return;
    end
    
    c_local = nvm(:, v_i);
    c(:, i) = c_local';
    
    c_m = max(c_local);
    if c_m > c_max
        c_max = c_m;
        r_i = i;
    end
end

X = categorical(r);

b = bar(X, c');
grid on;
legend('n = ' + string(n), 'Location', 'southoutside', 'Orientation', 'horizont');
ylabel('Коэффициент связности');
title(sprintf('Реальная связность сети в модели мобильности %s, v=%f', mob_name, vel));

name=sprintf('%s_%f.png', mob_name, vel);
set(gcf, 'Position', [0, 0, 700, 900]);
saveas(gcf, name);
export_fig MOB_V;


return;