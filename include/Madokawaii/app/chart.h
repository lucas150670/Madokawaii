//
// Created by madoka on 25-9-18.
//


//
// Created by madoka on 25-9-18.
//

#ifndef MADOKAWAII_CHART_H
#define MADOKAWAII_CHART_H

#include "Madokawaii/app/def.h"
#include <functional>

namespace Madokawaii::App::Chart {
    enum class ChartType {
        Official_FV3,
        Official_FV1,
        RPE,
        PE,
        Madokawaii_Internal
    };

    chart LoadChartFromFile(const char *path);
    chart LoadChartFromMemory(const char *data, size_t size);
    bool IsValidChart(const chart&);

    double CalcRealTime(double bpm, double time);
    double CalcBeatTime(double bpm, double realTime);

    double CalcEventProgress1Param(const chart::judgeline::event_base &event,
                                   double thisFrameTime,
                                   const std::function<double(double)>& postProcessor = nullptr);

    std::tuple<double, double> CalcEventProgress2Params(const chart::judgeline::event_base &event,
                                                        double thisFrameTime);

    void InitializeJudgelines(chart& mainChart);

    bool IsNoteInScreen(double x, double y, int screenWidth, int screenHeight);
}

#endif //MADOKAWAII_CHART_H