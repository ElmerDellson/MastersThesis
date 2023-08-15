path_directory = 'timings';
files = dir(path_directory);
dirFlags = [files.isdir];
subFolders = files(dirFlags);
subFolderNames = {subFolders(3:end).name};

for i=1:length(subFolderNames)
    [titleString, staticComputeAverage, animationComputeAverage, ...
        staticPrimaryAverage, animationPrimaryAverage] = ...
    ProcessFolder(subFolderNames{i}, path_directory);

    values = [staticComputeAverage animationComputeAverage ...
        staticPrimaryAverage animationPrimaryAverage];
    labels = {'Probe Static'; 'Probe Animation'; 'Primary Static'; ...
        'Primary Animation'};
    
    fig = figure;
    bar(values);
    ylim([0 6]);
    set(gca, 'xticklabel', labels);

    splitTitle = strsplit(titleString, '-');

    if (length(splitTitle) > 2)
        title({splitTitle(1), splitTitle(2) + "-" + splitTitle(3)});
    else
        title({splitTitle(1), splitTitle(2)});
    end
    text(1:length(values),values,num2str(values'),'vert','bottom', ...
        'horiz','center');
    ylabel('Time (ms)');

    ax = gca;
    ax.FontSize = 15;

    titleString = strrep(titleString, ' ', '_');
    titleString = titleString + '_bar_graph';

    set(fig, 'Units', 'Inches');
    pos = get(fig, 'Position');
    
    set(fig, 'PaperPositionMode', 'Auto', 'PaperUnits', 'Inches', ...
        'PaperSize', [pos(3), pos(4)]);
    fileName = "../report/Figures/generated_graphs/" + ... 
        titleString + ".pdf";
    print(fig, fileName, '-dpdf', '-r0');
end

function [titleString, staticComputeAverage, animationComputeAverage, ...
    staticPrimaryAverage, animationPrimaryAverage] = ...
    ProcessFolder(folderName, basePath)

    original_files = dir([basePath '/' folderName '/*.csv']);
    titleString = folderName + "_(" + length(original_files) + "_Runs)";
    titleString = strrep(titleString, "_", " ");

    computeTimesAcc = zeros(601, 1);
    primaryRayTimesAcc = zeros(601, 1);

    for i=1:length(original_files)
        inputFileName = [basePath '/' folderName '/' original_files(i).name];
        data = readtable(inputFileName);

        computeTimes = data{1:end, "Compute"};
        computeTimesAcc = computeTimesAcc + computeTimes;
        
        primaryRayTimes = data{1:end, "PrimaryRayTrace"};
        primaryRayTimesAcc = primaryRayTimesAcc + primaryRayTimes;
    end

    computeTimesAveraged = computeTimesAcc / length(original_files);
    staticComputeTotal = computeTimesAveraged([80:202, 352:601]);
    animateComputeTotal = computeTimesAveraged(203:351);
    staticComputeAverage = mean(staticComputeTotal);
    animationComputeAverage = mean(animateComputeTotal);

    primaryTimesAveraged = primaryRayTimesAcc / length(original_files);
    staticPrimaryTotal = primaryTimesAveraged([80:202, 352:601]);
    animatePrimaryTotal = primaryTimesAveraged(203:351);
    staticPrimaryAverage = mean(staticPrimaryTotal);
    animationPrimaryAverage = mean(animatePrimaryTotal);
end










