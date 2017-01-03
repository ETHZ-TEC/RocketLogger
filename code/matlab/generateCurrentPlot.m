x = 1:1024;
s1 = 50e-6*cos(x/1460+2356) + 20e-6*cos(x/300+34);
r1 = s1 + 1e-6 * randn(1,1024);
s2f = and(x>650, x < 820);

s2 = 10e-6 + s2f*300e-3;

r2 = s2 + 1000e-6 * randn(1,1024);
X1 = x; Y1= r1; Y2 = r2;


% Create figure
figure1 = figure(1);

axis1 = subplot(2,1,1);
plot(x,r1);

axis2 = subplot(2,1,2);
plot(x,r2);

return;

% Create axes
axes1 = axes('Parent',figure1);
hold(axes1,'on');

% Create plot
plot(X1,Y1,'Parent',axes1);

%% Uncomment the following line to preserve the X-limits of the axes
% xlim(axes1,[0 1200]);
%% Uncomment the following line to preserve the Y-limits of the axes
ylim(axes1,[0 7e-05]);
box(axes1,'on');
% Set the remaining axes properties
set(axes1,'YColor',[0 0.447 0.741],'YTick',[3e-05 4e-05 5e-05 6e-05]);
% Create axes
axes2 = axes('Parent',figure1,...
    'ColorOrder',[0.85 0.325 0.098;0.929 0.694 0.125;0.494 0.184 0.556;0.466 0.674 0.188;0.301 0.745 0.933;0.635 0.078 0.184;0 0.447 0.741]);
hold(axes2,'on');

% Create plot
plot(X1,Y2,'Parent',axes2);

%% Uncomment the following line to preserve the X-limits of the axes
% xlim(axes2,[0 1200]);
%% Uncomment the following line to preserve the Y-limits of the axes
ylim(axes2,[-0.02 0.32]);
% Set the remaining axes properties
set(axes2,'Color','none','HitTest','off','YAxisLocation','right','YColor',...
    [0.85 0.325 0.098],'YTick',[-0.2 2.77555756156289e-17 0.2 0.4]);

set(axes1,'FontSize',16,'XGrid','on','YGrid','off','YMinorTick','on');
set(axes1.Children, 'LineWidth', 3);
set(axes2,'FontSize',16,'XGrid','on','YGrid','off','YMinorTick','on');
set(axes2.Children, 'LineWidth', 3);

ylabel(axes1, 'Current')
ylabel(axes2, 'Current')
xlabel(axes1, 'Time')
