%use DumpPDR-pdr-udp-cbr-traffic.csv
pdr_diff = diff(pdr);
decr = find(pdr_diff < 0);
subplot(2, 1, 1);
plot(time, d, '-o', 'MarkerIndices', decr(1));
grid on;
plot(time, d, '-o', 'MarkerIndices', decr(1), 'MarkerFaceColor', 'r');
grid on;
txt = ['Разрыв связи при ', num2str(d(decr(1))), 'м'];
text(time(decr(1)) - 40, d(decr(1)) + 100, txt);
ylabel('Расстояние, м');
title('Коэффициент доставки пакетов и расстояние между узлами');
subplot(2, 1, 2);
plot(time, pdr);
grid on
ylabel('PDR');
xlabel('Время, с');
ax1 = subplot(2, 1, 1);
x_break = [time(1) time(length(time))];
y_break = [d(decr(1)) d(decr(1))];
line(ax1, x_break, y_break, 'Color', 'red', 'LineStyle', '--');
set(ax1);
pos = get(ax1, 'Position');
pos(2) = 0.5;
pos(4) = 0.45;
set(ax1, 'Position', pos);