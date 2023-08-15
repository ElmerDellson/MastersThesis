path_directory = 'timings';
original_files = dir([path_directory '/*.csv']);

for i=1:length(original_files)
    inputFileName = [path_directory '/' original_files(i).name];
    processAndPrint(inputFileName);
end

function processAndPrint(inputFileName)
    data = readtable(inputFileName);
    xlab = data.Properties.VariableDescriptions(1);
    ylab1 = data.Properties.VariableDescriptions(2);
    ylab2 = data.Properties.VariableDescriptions(3);
    titleString = data.Properties.VariableDescriptions(4);
    
    frameNrs = data{10:end, "FrameNr_"};
    computeTimes = data{10:end, "Compute"};
    primaryRayTimes = data{10:end, "PrimaryRayTrace"};
    
    fig = figure;
    plot(frameNrs, [computeTimes, primaryRayTimes]);
    set(fig, 'units', 'points', 'position', [500, 500, 700, 350]);
    titleStringWithSpaces = strrep(titleString, "_", " ");
    title(titleStringWithSpaces);
    
    xlabel(xlab);
    ylabel('Time (ms)');
    
    beginLine = xline(200, '--', {'Animation begin'});
    beginLine.LabelHorizontalAlignment = 'left';
    xline(350, '--', {'Animation end'});
    
    legend(string(ylab1), string(ylab2));
    
    set(fig, 'Units', 'Inches');
    pos = get(fig, 'Position');
    
    set(fig, 'PaperPositionMode', 'Auto', 'PaperUnits', 'Inches', ...
        'PaperSize', [pos(3), pos(4)]);
    
    fileName = "../report/Figures/" + titleString + ".pdf";
    print(fig, fileName, '-dpdf', '-r0');
end