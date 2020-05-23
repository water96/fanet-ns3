x = categorical(node_id);
x = reordercats(x, node_id);
bar(x, forw_pckts);
grid on;
xlabel('ID узла'); ylabel('Пакеты'); title('Количество пересланных каждым узлом пакетов');