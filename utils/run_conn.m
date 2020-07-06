function [] = run_conn(mob_name)

addpath('/home/aleksey/work/export_fig')
if strcmp(mob_name, '')
    print('Error!')
end

r = ["AODV", "GPSR", "OLSR"];
dnvm = 0;
n = 0;
v = 0;

for i = 1 : length(r)
    cd(sprintf('%s_%s_NVM', mob_name, r(i)));
    n = importnodes(sprintf('%s_%s_node_key.csv', mob_name, r(i)));
    v = importvel(sprintf('%s_%s_vel_key.csv', mob_name, r(i)));
    unvm = importnvm(sprintf('%s_%s_NV_udp_conn.csv', mob_name, r(i)));
    dnvm = importnvm(sprintf('%s_%s_NV_data_link_conn.csv', mob_name, r(i)));
    eff = unvm./ dnvm;
    ttl = sprintf('m=%s, r=%s', mob_name, r(i));
    create_connectivity_figure(v, n, unvm, eff, 'n=', ttl);
    cd('../');
end

% Create figure
figure1 = figure;

% Create subplot
subplot1 = subplot(1,1,1,'Parent',figure1);
hold(subplot1,'on');

% Create multiple lines using matrix input to plot
plot1 = plot(v, dnvm,'Parent',subplot1,'MarkerSize',8,'Marker','+',...
    'LineWidth',3,...
    'LineStyle',':');
yl = ylim;
ylim([0, yl(2)]);
for i = 1:length(n)
    set(plot1(i),'DisplayName', strcat('n=', num2str(n(i))));
end

% Create title
title(sprintf('Потенциальная связность m=%s', mob_name));

grid(subplot1,'on');
set(subplot1,'FontSize',18);
ylabel('Коэффициент связности');
xlabel('Скорость, м/с');

legend1 = legend(subplot1,'show');
set(legend1,...
    'Position',[0.476164533373724 0.869757058735899 0.425595231176841 0.0554635747280342],...
    'Orientation','horizontal', 'FontSize',18);
hold(subplot1,'off');
name=strcat(mob_name, '_conn.png');
set(gcf, 'Position', [0, 0, 1000, 600]);
saveas(gcf, name);
export_fig mob_conn.png;

return;