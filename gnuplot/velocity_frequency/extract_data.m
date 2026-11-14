clc;
clear;
data_dir = getenv('MSC_DATA');
if isempty(data_dir)
    error('MSC_DATA is not set: export MSC_DATA=/path/to/data (see Data availability in README.md)');
end
fileID = fopen(fullfile(data_dir, 'regression', 'linear_regression_data.txt'));
formatSpec = '%s %s %f %f %f %f %f %f';
N = 8;
input = textscan(fileID, formatSpec, 'HeaderLines',1,'Delimiter','\t');
% FPS = 59.95;
% dt = 1/FPS;
frequency =input{6};
velocity = -1 * input{7};
velocity_error = sqrt(-1 * input{8});
frequency_error = 0.2 * ones(70,1);
full = [frequency,frequency_error,velocity,velocity_error];
table_2 = array2table(full);
table_2.Properties.VariableNames{'full1'} = 'Frequency';
table_2.Properties.VariableNames{'full2'} = 'Frequency_error';
table_2.Properties.VariableNames{'full3'} = 'Velocity';
table_2.Properties.VariableNames{'full4'} = 'Velocity_error';
out_dir = fullfile('..', 'images');
if ~exist(out_dir, 'dir'), mkdir(out_dir); end
writetable(table_2, fullfile(out_dir, 'data_ok.txt'), 'Delimiter', '\t');

% plot(Time,Length);
% magniTime = length(Time);
% magniLength = length(Length);
% ylabel('Length (mm)');
% xlabel('Time (s)');
% zeroTime = num2str(Time(1));
% zeroLength = num2str(Length(1));
% txt1 = '\leftarrow at T= 0(s), Length{i}= '+ zeroTime;
% txt2 = '\leftarrow at T= 7.8732(s), Length{f}= '+ zeroLength;
% strValues = strtrim(cellstr(num2str([X(:) Y(:)],'(%d,%d)')));
% hold on
%  text(Time(1),Length(1),txt1);
%  text(Time(magniTime),Length(magniLength),txt2);
% hold off
%cftool;



