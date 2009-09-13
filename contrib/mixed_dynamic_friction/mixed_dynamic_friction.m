% addpath ~/source++/getfem++/contrib/mixed_dynamic_friction

%
% current
%

if (1)

A = load('mixed_dynamic_friction.data');

% energy curves

plot(A(:, 1), A(:, 2), '-k', 'linewidth', 2, 'MarkerSize', 15);
% axis([0 0.02 0 0.1]);
pause;

% displacement curves

plot(A(:, 1), A(:, 3), '-k', 'linewidth', 2, 'MarkerSize', 15);
% axis([0 0.02 -0.01 0.06]);
pause;

% contact stress curves

plot(A(:, 1), A(:, 4), '-k', 'linewidth', 2, 'MarkerSize', 15);
% axis([0 0.02 -0.4 0.02]);
pause;

else


%
% P1plusP0
%

A1 = load('mdf_P2P1_1E-3.data');
A2 = load('mdf_P2P1_1E-4.data');
A3 = load('mdf_P2P1_1E-5.data');

% energy curves

plot(A1(:, 1), A1(:, 2), '-k', 'linewidth', 2, 'MarkerSize', 15);
hold on;
plot(A2(:, 1), A2(:, 2), '--k', 'linewidth', 2, 'MarkerSize', 15);
plot(A3(:, 1), A3(:, 2), '-.k', 'linewidth', 2, 'MarkerSize', 15);
hold off;
axis([0 0.02 690 850]);
% axis([0 0.7 0 0.02]);
xlabel('t');
ylabel('total energy');
legend('dt = 10^{-3}', 'dt = 10^{-4}', 'dt = 10^{-5}', 'Location', 'SouthWest');
axesobj = findobj('type', 'axes'); set(axesobj, 'fontname', 'times'); set(axesobj, 'fontunits', 'points'); set(axesobj, 'fontsize', 24); set(axesobj, 'fontweight', 'bold'); set(axesobj, 'linewidth', 2);
pause;
print(gcf,'-deps','-r450', 'energy.eps');
print(gcf,'-dpng','-r450', 'energy.png');

% contact stress curves

plot(A1(:, 1), A1(:, 3), '-k', 'linewidth', 2, 'MarkerSize', 15);
hold on;
plot(A2(:, 1), A2(:, 3) -50, '--k', 'linewidth', 2, 'MarkerSize', 15);
plot(A3(:, 1), A3(:, 3) -100, '-.k', 'linewidth', 2, 'MarkerSize', 15);
hold off;
axis([0 0.02 -400 0.01]);
% axis([0 0.7 -0.01 0.06]);
xlabel('t');
ylabel('point A contact stress');
legend('dt = 10^{-3}', 'dt = 10^{-4}', 'dt = 10^{-5}', 'Location', 'SouthWest');
axesobj = findobj('type', 'axes'); set(axesobj, 'fontname', 'times'); set(axesobj, 'fontunits', 'points'); set(axesobj, 'fontsize', 24); set(axesobj, 'fontweight', 'bold'); set(axesobj, 'linewidth', 2);
pause;
print(gcf,'-deps','-r450', 'stress.eps');
print(gcf,'-dpng','-r450', 'stress.png');

% displacement curves

plot(A1(:, 1), A1(:, 4), '-k', 'linewidth', 2, 'MarkerSize', 15);
hold on;
plot(A2(:, 1), A2(:, 4), '--k', 'linewidth', 2, 'MarkerSize', 15);
plot(A3(:, 1), A3(:, 4), '-.k', 'linewidth', 2, 'MarkerSize', 15);
hold off;
axis([0 0.02 -0.1 10]);
xlabel('t');
ylabel('point A normal displacement');
legend('dt = 10^{-3}', 'dt = 10^{-4}', 'dt = 10^{-5}', 'Location', 'SouthWest');
axesobj = findobj('type', 'axes'); set(axesobj, 'fontname', 'times'); set(axesobj, 'fontunits', 'points'); set(axesobj, 'fontsize', 24); set(axesobj, 'fontweight', 'bold'); set(axesobj, 'linewidth', 2);
pause;
print(gcf,'-deps','-r450', 'displacement.eps');
print(gcf,'-dpng','-r450', 'displacement.png');



end;


  

%  loglog(H(1:7), L2_1(1:7), 'o-k', 'linewidth', 2, 'MarkerSize', 15);
%  hold on;
%  loglog(H(1:7), L2_2(1:7), 'x-.k', 'linewidth', 2, 'MarkerSize', 15);
%  loglog(H(1:7), L2_3(1:7), '+--k', 'linewidth', 2, 'MarkerSize', 15);
%  loglog(H(1:7), L2_4(1:7), '*-k', 'linewidth', 2, 'MarkerSize', 15);
%  loglog(H(1:7), L2_5(1:7), 's-.k', 'linewidth', 2, 'MarkerSize', 15);
%  hold off;
%  P1 = polyfit(log(H(1:7)), log(L2_1(1:7)), 1);
%  P2 = polyfit(log(H(1:7)), log(L2_2(1:7)), 1);
%  P3 = polyfit(log(H(1:7)), log(L2_3(1:7)), 1);
%  P4 = polyfit(log(H(1:7)), log(L2_4(1:7)), 1);
%  P5 = polyfit(log(H(2:5)), log(L2_5(2:5)), 1);
%  legend(strcat('P1/P0  (slope=',num2str(P1(1)), ')'), ...
%         strcat('P1+/P0 (slope=',num2str(P2(1)), ')'), ...
%         strcat('P2/P1  (slope=',num2str(P3(1)), ')'), ...
%         strcat('Q1/Q0  (slope=',num2str(P4(1)), ')'), ...
%         strcat('Q2/Q1  (slope=',num2str(P5(1)), ')'), ...
%         'Location', 'NorthWest');
%  grid on;
%  axesobj = findobj('type', 'axes');
%  set(axesobj, 'fontname', 'times'); set(axesobj, 'fontunits', 'points');
%  set(axesobj, 'fontsize', 18); set(axesobj, 'fontweight', 'bold');
%  set(axesobj, 'linewidth', 2);
%  xlabel('h');
%  ylabel('L^2(\Omega) relative error');
%  set(gca,'XTickLabel',{'0.001';'0.01';'0.1';'1';'...'}) 
%  % axis([0.05 7 1e-4 10]);
%  pause;

 

% Pour mettre des fontes plus grosses.
% une commande
% get(findobj, 'type')
% renseigne sur les type d'objets � chercher.
% ensuite on recup�re les handles par
% axesobj = findobj('type', 'axes')
% par exemple, puis on peut faire
% set(axesobj, 'fontunits', 'points');
% set(axesobj, 'fontsize', 15);
% set(axesobj, 'fontweight', 'bold');
% Il vaut mieux a la fin decouper les images avec gimp par exemple.

% axesobj = findobj('type', 'axes'); set(axesobj, 'fontname', 'times'); set(axesobj, 'fontunits', 'points'); set(axesobj, 'fontsize', 18); set(axesobj, 'fontweight', 'bold'); set(axesobj, 'linewidth', 2);


% Pour certains graphiques, il vaut mieux renommer les "ticks" par
%  set(gca,'XTickLabel',{'0.1';'1';'10';'...'})
%  set(gca,'YTickLabel',{'0.0001%';'0.001%';'0.01%';'0.1%';'1%';'10%'})     


% Pour sortir le graphique en png, faire par exemple :
% print(gcf,'-dpng','-r450', 'toto.png');
