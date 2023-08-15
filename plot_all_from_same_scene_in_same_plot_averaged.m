path_directory = 'timings';
files = dir(path_directory);
dirFlags = [files.isdir];
subFolders = files(dirFlags);
subFolderNames = {subFolders(3:end).name};

fig = figure;
hold on;

titleStringPlot = "Cornell Box (9261 Probes) - All Configurations (100 Runs)";
% titleStringPlot = "Room With Door (5292 Probes) - All Configurations (100 Runs)";
titleStringPlot = strrep(titleStringPlot, "_", " ");
titlePlotSplit = strsplit(titleStringPlot, '-');
titlePlot = titlePlotSplit{1};

title(titleStringPlot);
set(fig, 'units', 'points', 'position', [100, 100, 700, 350]);
xlabel("Frame No.");
ylabel('Time (ms)');
set(fig, 'Units', 'Inches');

% temp = [3, 5, 6];

for i=1:6
    ProcessFolder(subFolderNames{i}, path_directory);
end

leg = legend('AutoUpdate','off');
leg.Location = "east";

beginLine = xline(200, '--', {'Animation begin'});
beginLine.LabelHorizontalAlignment = 'left';
beginLine.LabelVerticalAlignment = 'middle';
beginLine.FontSize = 13;
endLine = xline(350, '--', {'Animation end'});
endLine.LabelHorizontalAlignment = 'left';
endLine.LabelVerticalAlignment = 'middle';
endLine.FontSize = 13;

pos = get(fig, 'Position');
set(fig, 'PaperPositionMode', 'Auto', 'PaperUnits', 'Inches', 'PaperSize', ...
    [pos(3), pos(4)]);

titleForFileName = strrep(titleStringPlot, " ", "_");
fileName = "../report/Figures/generated_graphs/" + titleForFileName + ".pdf";
print(fig, fileName, '-dpdf', '-r0');

function ProcessFolder(folderName, basePath)
    original_files = dir([basePath '/' folderName '/*.csv']);

    inputFileName = [basePath '/' folderName '/' original_files(1).name];
    data = readtable(inputFileName);
%     titleString = folderName + "_(" + length(original_files) + "_Runs)";
    titleString = folderName;
    titleString = strrep(titleString, "_", " ");
    titleString = strrep(titleString, " Per ", "/");
    titleSecondHalf = strsplit(titleString, '-');
    
    titleString = "";
    for j=2:length(titleSecondHalf)
        if (j == 2)
            titleString = titleSecondHalf(j);
        else
            titleString = titleString + "-" + titleSecondHalf(j);
        end
    end

    computeTimesAcc = zeros(592, 1);
    primaryRayTimesAcc = zeros(592, 1);

    for i=1:length(original_files)
        inputFileName = [basePath '/' folderName '/' original_files(i).name];
        data = readtable(inputFileName);
        frameNrs = data{10:end, "FrameNr_"};

        computeTimes = data{10:end, "Compute"};
        computeTimesAcc = computeTimesAcc + computeTimes;
        
        primaryRayTimes = data{10:end, "PrimaryRayTrace"};
        primaryRayTimesAcc = primaryRayTimesAcc + primaryRayTimes;
    end

    computeTimesAveraged = computeTimesAcc / length(original_files);

    plot(frameNrs, computeTimesAveraged, 'DisplayName', titleString, ...
        'LineWidth', 2);

    ax = gca;
    ax.FontSize = 15;
end

