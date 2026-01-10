//
// Created by madoka on 2026/1/11.
//

// pec.cpp
#include <algorithm>

#include "Madokawaii/app/chart.h"
#include "Madokawaii/platform/core.h"
#include "Madokawaii/platform/log.h"

#include <sstream>
#include <string>
#include <map>
#include <cmath>

namespace Madokawaii::App::Chart {

    chart LoadChartFromPEC(const char* path) {
        if (!Platform::Core::FileExists(path)) {
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                                    "CHART: PEC file does not exist!");
            return {};
        }

        auto fileData = Platform::Core::LoadFileText(path);
        auto result = LoadChartFromPECMemory(fileData, strlen(fileData));
        Platform::Core::UnloadFileText(fileData);
        return result;
    }

    chart LoadChartFromPECMemory(const char* data, size_t size) {
        chart mainChart;
        mainChart.formatVersion = 3;
        mainChart.offset = 0;
        mainChart.judgelineCount = mainChart.speedEventsCount =
            mainChart.disappearEventCount = mainChart.moveEventCount =
            mainChart.rotateEventCount = mainChart.numOfNotes = 0;

		std::istringstream stream(std::string(data, size));
		std::string line;
		std::getline(stream, line);
        try {
            mainChart.offset = std::stod(line);
        }
        catch (...) {
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                "CHART: PEC Chart Load Error - Invalid offset value");
            return {};
        }
        if (!stream) {
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                                    "CHART: PEC Chart Load Error - No judgeline data");
            return {};
        }
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            // Parse notes
            auto line_splited = std::vector<std::string>{};
            std::istringstream line_stream(line);
            std::string item;
            while (std::getline(line_stream, item, ' ')) {
                line_splited.push_back(item);
            }

            // parse bp data
            if (line_splited.size() > 0) {
                if (line_splited[0] == "bp") {
                    if (line_splited.size() != 3) {
                        Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                                                "CHART: PEC Chart Load Error - Invalid bp data");
                        return {};
                    }
                    try {
                        double start_beat = std::stod(line_splited[1]);
                        double bpm = std::stod(line_splited[2]);
                        if (start_beat != 0.0f) {
                            throw std::invalid_argument("start_beat must be 0.0");
                        }
                        Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_INFO,
                                                "CHART: PEC Chart BP Set - BPM: %f at Beat %f",
                                                bpm, start_beat);
                        mainChart.global_bpm = bpm;
                    } catch (...) {
                        Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                                                "CHART: PEC Chart Load Error - Invalid bp data values");
                        return {};
                    }
                }
            }
        }

        Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_INFO,
                                "CHART: PEC Chart Loaded");
        return mainChart;
    }

}
