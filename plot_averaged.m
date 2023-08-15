path_directory = 'timings';
files = dir(path_directory);
dirFlags = [files.isdir];
subFolders = files(dirFlags);
subFolderNames = {subFolders(3:end).name};

for i=1:length(subFolderNames)
    ProcessFolder(subFolderNames{i}, path_directory);
end

function ProcessFolder(folderName, basePath)
    original_files = dir([basePath '/' folderName '/*.csv']);

    inputFileName = [basePath '/' folderName '/' original_files(1).name];
    data = readtable(inputFileName);
    xlab = "Frame No.";%data.Properties.VariableDescriptions(1);
    ylab1 = "Probe Update";%data.Properties.VariableDescriptions(2);
    ylab2 = data.Properties.VariableDescriptions(3);
    titleString = folderName + "_(" + length(original_files) + "_Runs)";

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
    primaryRayTimesAveraged = primaryRayTimesAcc / length(original_files);

    PlotTimings(frameNrs, computeTimesAveraged, primaryRayTimesAveraged, ...
        xlab, ylab1, ylab2, titleString);
end

function PlotTimings(frameNrs, computeTimes, primaryRayTimes, xlab, ylab1, ...
    ylab2, titleString)
    fig = figure;
    plot(frameNrs, [computeTimes, primaryRayTimes], 'LineWidth', 2);
    
    ax = gca;
    ax.FontSize = 15;

    set(fig, 'units', 'points', 'position', [500, 500, 700, 350]);
    titleStringWithSpaces = strrep(titleString, "_", " ");
    title(titleStringWithSpaces);
    
    xlabel(xlab);
    ylabel('Time (ms)');
   
    beginLine = xline(200, '--', {'Animation begin'});
    beginLine.LabelVerticalAlignment = 'middle';
    beginLine.FontSize = 13;
    beginLine.LabelHorizontalAlignment = 'left';
    endLine = xline(350, '--', {'Animation end'});
    endLine.LabelVerticalAlignment = 'middle';
    endLine.FontSize = 13; 
    
    leg = legend(string(ylab1), string(ylab2));
    leg.Location = 'east';
    
    set(fig, 'Units', 'Inches');
    pos = get(fig, 'Position');
    
    set(fig, 'PaperPositionMode', 'Auto', 'PaperUnits', 'Inches', ...
        'PaperSize', [pos(3), pos(4)]);
    
    fileName = "../report/Figures/generated_graphs/" + titleString + ".pdf";
    print(fig, fileName, '-dpdf', '-r0');
end