x = categorical(node_id);
x = reordercats(x, node_id);
bar(x, rx);
grid on;
xlabel('IP-адрес узла'); ylabel('Пакеты'); title('Количество полученных пакетов связности');
hold on
plot(xlim, [tx(1) tx(1)]);