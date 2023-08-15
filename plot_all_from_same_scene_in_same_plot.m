path_directory = 'timings';
original_files = dir([path_directory '/*.csv']);

scene_0 = string.empty;
scene_1 = string.empty;

for i=1:length(original_files)
    split_at_first_parentheses = strsplit(original_files(i).name, '(');
    scene_title = split_at_first_parentheses(1);

    if (strcmp(char(scene_title), 'Room_With_Door_'))
        scene_1 = [scene_1 original_files(i).name];
    elseif (strcmp(char(scene_title), 'Cornell_Box_'))
        scene_0 = [scene_0 original_files(i).name];
    end
end

ProcessAndPrint(scene_0, path_directory);
ProcessAndPrint(scene_1, path_directory);

function ProcessAndPrint(original_files_scene, path_directory)
    fig = figure;
    hold on;

    titleStringPlot = original_files_scene{1};
    titleStringPlot = strrep(titleStringPlot, "_", " ");
    titlePlotSplit = strsplit(titleStringPlot, '-');
    titlePlot = titlePlotSplit{1};

    title(titlePlot);
    set(fig, 'units', 'points', 'position', [100, 100, 700, 350]);
    xlabel("Frame Nr.");
    ylabel('Time (ms)');
    set(fig, 'Units', 'Inches');
    
    for i=1:length(original_files_scene)
        inputFileName = [path_directory, '/', original_files_scene{i}];
        data = readtable(convertStringsToChars(inputFileName(1,:)));
        titleString = data.Properties.VariableDescriptions(4);
        titleString = strrep(titleString, "_", " ");
        titleSecondHalf = strsplit(titleString, '-');
        
        titleString = "";
        for j=2:length(titleSecondHalf)
            if (j == 2)
                titleString = titleSecondHalf(j);
            else
                titleString = titleString + "-" + titleSecondHalf(j);
            end
        end
        
        frameNr = data{10:end, "FrameNr_"};
        computeTimes = data{10:end, "Compute"};
    
        plot(frameNr, computeTimes, 'DisplayName', titleString);
    end
    
    legend('AutoUpdate','off');
    beginLine = xline(200, '--', {'Animation begin'});
    beginLine.LabelHorizontalAlignment = 'left';
    xline(350, '--', {'Animation end'});
    
    pos = get(fig, 'Position');
    set(fig, 'PaperPositionMode', 'Auto', 'PaperUnits', 'Inches', 'PaperSize', [pos(3), pos(4)]);
    fileName = "../report/Figures/All_Configurations.pdf";
    print(fig, fileName, '-dpdf', '-r0');
end