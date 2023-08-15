#include "stdafx.h"
#include "Thesis.h"
#include <filesystem>

using namespace std;

string FindIndexForExportFile(string title);

void Thesis::ExportTimings() {
	int nbProbes = m_nbProbesPerLayer * m_nbProbeLayers;

	string title;
	string sceneTitle = m_scene == Scene1 ? "Room_With_Door_(" + to_string(nbProbes) + "_Probes)_-_" :
		"Cornell_Box_(" + to_string(nbProbes) + "_Probes)_-_";
	if (m_GIPreset == Off) {
		title = sceneTitle + "GI_Off";
	}
	else if (m_GIPreset == NoSurfels) {
		title = sceneTitle + "No_Surfels_-_" + to_string(m_nbSurfelsPerProbe) + "_Rays_Per_Probe";
	}
	else
		title = sceneTitle + "Full_GI_-_" + to_string(m_nbSurfelsPerProbe) + "_Surfels_Per_Probe";

	string directory{ "timings/" + title };
	std::wstring stemp = std::wstring(directory.begin(), directory.end());
	LPCWSTR sw = stemp.c_str();
	CreateDirectory(sw, NULL);
	string index{ FindIndexForExportFile(title) };

	ofstream outputFileStreamNumbered{};
	outputFileStreamNumbered.open(directory + "/" + title + index + ".csv");

	ofstream outputFileStream{};
	outputFileStream.open("timings/" + title + ".csv");

	ofstream outputFileStreamLatest{};
	outputFileStreamLatest.open("timings/latestRun.csv");

	outputFileStream << "Frame Nr., Compute, Primary Ray Trace, " << title << endl;
	outputFileStreamNumbered << "Frame Nr., Compute, Primary Ray Trace, " << title << endl;
	outputFileStreamLatest << "Frame Nr., Compute, Primary Ray Trace, " << title << endl;

	for (int i{ 0 }; i < m_computeDispatchTimes.size(); i++) {
		outputFileStream << i << ", " << m_computeDispatchTimes[i] << ", " << m_graphicsDispatchTimes[i] << ", 1" << endl;
		outputFileStreamNumbered << i << ", " << m_computeDispatchTimes[i] << ", " << m_graphicsDispatchTimes[i] << ", 1" << endl;
		outputFileStreamLatest << i << ", " << m_computeDispatchTimes[i] << ", " << m_graphicsDispatchTimes[i] << ", 1" << endl;
	}

	outputFileStream.close();
	outputFileStreamNumbered.close();
	outputFileStreamLatest.close();
}

string FindIndexForExportFile(string title) {
	string path{ "timings/" + title };
	vector<string> files{};

	vector<int> indices{ -1 };
	for (const auto& entry : filesystem::directory_iterator(path)) {
		string fileName{ entry.path().generic_string() };
		string currentFileIndex{fileName.substr(fileName.length() - 6, 2)};

		try {
			indices.push_back(stoi(currentFileIndex));
		}
		catch (invalid_argument) { }

		files.push_back(fileName);
	}

	sort(indices.begin(), indices.end(), greater<int>());

	string newIndex{ to_string(indices[0] + 1) };

	if (stoi(newIndex) < 10)
		newIndex = "0" + newIndex;

	return newIndex;
}