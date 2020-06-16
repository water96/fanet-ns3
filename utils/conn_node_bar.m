function conn_node_bar(xvector1, ymatrix1)
%CREATEFIGURE(XVECTOR1, YMATRIX1)
%  XVECTOR1:  bar xvector
%  YMATRIX1:  bar matrix data

%  Auto-generated by MATLAB on 14-Jun-2020 00:22:29

% Create figure
figure1 = figure;

% Create axes
axes1 = axes('Parent',figure1);
hold(axes1,'on');

% Create multiple lines using matrix input to bar
bar1 = bar(xvector1,ymatrix1,'Parent',axes1);
set(bar1(4),'DisplayName','n = 4');
set(bar1(3),'DisplayName','n = 8');
set(bar1(2),'DisplayName','n = 12');
set(bar1(1),'DisplayName','n = 16');

% Create text
text('Parent',axes1,'Position',[1 0.144724 0]);

% Create ylabel
ylabel('Коэффициент связности');

box(axes1,'on');
grid(axes1,'on');
% Create legend
legend1 = legend(axes1,'show');
set(legend1,'Orientation','horizontal','Location','southoutside');

% Create textbox
annotation(figure1,'textbox',...
    [0.30686592178771 0.869172932330828 0.0436927374301674 0.0375939849624061],...
    'String',{'v1'},...
    'FitBoxToText','off');

% Create textbox
annotation(figure1,'textbox',...
    [0.565245810055866 0.882706766917294 0.0436927374301673 0.0375939849624061],...
    'String',{'v1'},...
    'FitBoxToText','off');

% Create textbox
annotation(figure1,'textbox',...
    [0.823625698324023 0.902255639097746 0.0436927374301672 0.0375939849624061],...
    'String',{'v1'},...
    'FitBoxToText','off');
