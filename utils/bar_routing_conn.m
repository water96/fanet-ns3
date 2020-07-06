
function [b, n, v, unvm] = bar_routing_conn(mob_name, nodes, vel)

addpath('/home/aleksey/work/export_fig');
if strcmp(mob_name, '')
    print('Error!');
end

r = ["AODV", "GPSR", "OLSR"];

b = 0;
c = zeros(length(nodes), length(r));

c_max = 0;
n = 0;
v = 0;
unvm = 0;
dnvm = 0;
v_i = 0;
for i = 1 : length(r)
    n = importnodes(sprintf('%s_%s_NVM/%s_%s_node_key.csv', mob_name, r(i), mob_name, r(i)));
    v = importvel(sprintf('%s_%s_NVM/%s_%s_vel_key.csv', mob_name, r(i), mob_name, r(i)));
    unvm = importnvm(sprintf('%s_%s_NVM/%s_%s_NV_udp_conn.csv', mob_name, r(i), mob_name, r(i)));
    dnvm = importnvm(sprintf('%s_%s_NVM/%s_%s_NV_data_link_conn.csv', mob_name, r(i), mob_name, r(i)));
    v_i = find(v == vel, 1);
    if isempty(v_i)
        print('Error!');
        return;
    end
    
    c_local = unvm(nodes, v_i);
    c(:, i) = c_local';
    
    c_m = max(c_local);
    if c_m > c_max
        c_max = c_m;
    end
end


create_bar_figure(c', n(nodes), r, mob_name, vel);

%b = bar(c');
%set(gca, 'XTickLabel',r, 'XTick',1:numel(r));
%grid on;
%legend('n = ' + string(n(nodes)), 'Location', 'southoutside', 'Orientation', 'horizont');
%ylabel('Коэффициент связности');
%title(sprintf('Реальная связность сети в модели мобильности %s, v=%f', mob_name, vel));
%x = xlim;
%for i = 1 : length(nodes)
%    y = dnvm(nodes(i), v_i);
%    plot(x,[y, y],'LineWidth', 2);
%end

name=sprintf('%s_%f.png', mob_name, vel);
set(gcf, 'Position', [0, 0, 1000, 600]);
saveas(gcf, name);
export_fig MOB_V;


return;