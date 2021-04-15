% Lab4 data import and system ID
T = readtable('test2ms_0start.csv');
T = T(1:height(T)-3,:);

time = table2array(T(:,3));
L_PWM = table2array(T(:,4));
R_PWM = table2array(T(:,5));
L_encoder = table2array(T(:,6));
R_encoder = table2array(T(:,7));

L_angles = L_encoder*2*pi/909.7;
R_angles = R_encoder*2*pi/909.7;


L_speed = zeros(length(L_angles),1);
R_speed = zeros(length(R_angles),1);

for i = 1:(length(L_speed)-1)
    L_speed(i) = (L_angles(i+1) - L_angles(i)) / (time(i+1) - time(i));
    R_speed(i) = (R_angles(i+1) - R_angles(i)) / (time(i+1) - time(i));
end
dt = 0.0026278;

%% Plots
close all

figure
subplot(3,1,1)

hold on
plot(time, L_angles, 'red')
plot(time, R_angles, 'blue')
title("Angle Values")
hold off

subplot(3,1,2)
hold on
plot(time, L_PWM, 'red')
plot(time, R_PWM, 'blue')
title("PWM Values")
hold off

subplot(3,1,3)
hold on
plot(time, L_speed, 'red')
plot(time, R_speed, 'blue')
title("Speed")
hold off

