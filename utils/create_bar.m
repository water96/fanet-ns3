Labels = categorical({'Служебные пакеты', 'Полезные данные'});
Labels = reordercats(Labels, {'Служебные пакеты', 'Полезные данные'});
Stats=[service usefull_send];
b = bar(Labels, Stats);
b.BarWidth = 0.4;
grid on;