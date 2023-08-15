processAndPrint('timings/latestRun.csv');

function processAndPrint(inputFileName)
    data = readtable(inputFileName);
    xlab = data.Properties.VariableDescriptions(1);
    ylab1 = data.Properties.VariableDescriptions(2);
    ylab2 = data.Properties.VariableDescriptions(3);
    titleString = data.Properties.VariableDescriptions(4);
    
    frameNr = data{10:end, "FrameNr_"};
    computeTimes = data{10:end, "Compute"};
    primaryRayTime = data{10:end, "PrimaryRayTrace"};
    
    fig = figure;
    plot(frameNr, [computeTimes, primaryRayTime]);
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
    
    set(fig, 'PaperPositionMode', 'Auto', 'PaperUnits', 'Inches', 'PaperSize', [pos(3), pos(4)]);
    
    fileName = "../report/Figures/" + titleString + ".pdf";
    print(fig, fileName, '-dpdf', '-r0');
end