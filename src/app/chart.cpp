//
// Created by madoka on 25-9-11.
//

// rapidjson call only allowed in chart.cpp
#include "Madokawaii/app/def.h"
#include "Madokawaii/app/chart.h"
#include "Madokawaii/platform/core.h"
#include "Madokawaii/platform/log.h"

#include <rapidjson/schema.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <algorithm>
#include <cfloat>
#include <tuple>

namespace Madokawaii::App::Chart {

    const std::string chartSchemaV3String =
        R"({"type":"object","properties":{"formatVersion":{"type":"integer"},"offset":{"type":"number"},"judgeLineList":{"type":"array","items":{"type":"object","properties":{"bpm":{"type":"number"},"speedEvents":{"type":"array","items":{"type":"object","properties":{"startTime":{"type":"number"},"endTime":{"type":"number"},"value":{"type":"number"}},"required":["startTime","endTime","value"]}},"judgeLineDisappearEvents":{"type":"array","items":{"type":"object","properties":{"startTime":{"type":"number"},"endTime":{"type":"number"},"start":{"type":"number"},"end":{"type":"number"}},"required":["startTime","endTime","start","end"]}},"judgeLineMoveEvents":{"type":"array","items":{"type":"object","properties":{"startTime":{"type":"number"},"endTime":{"type":"number"},"start":{"type":"number"},"end":{"type":"number"},"start2":{"type":"number"},"end2":{"type":"number"}},"required":["startTime","endTime","start","end","start2","end2"]}},"judgeLineRotateEvents":{"type":"array","items":{"type":"object","properties":{"startTime":{"type":"number"},"endTime":{"type":"number"},"start":{"type":"number"},"end":{"type":"number"}},"required":["startTime","endTime","start","end"]}},"notesAbove":{"type":"array","items":{"type":"object","properties":{"type":{"type":"integer"},"time":{"type":"integer"},"positionX":{"type":"number"},"holdTime":{"type":"number"},"speed":{"type":"number"},"floorPosition":{"type":"number"}},"required":["type","time","positionX","holdTime","speed","floorPosition"]}},"notesBelow":{"type":"array","items":{"type":"object","properties":{"type":{"type":"integer"},"time":{"type":"integer"},"positionX":{"type":"number"},"holdTime":{"type":"number"},"speed":{"type":"number"},"floorPosition":{"type":"number"}},"required":["type","time","positionX","holdTime","speed","floorPosition"]}}},"required":["bpm","speedEvents","judgeLineDisappearEvents","judgeLineMoveEvents","judgeLineRotateEvents","notesAbove","notesBelow"]}}},"required":["formatVersion","offset","judgeLineList"]})";

    chart LoadChartFromFile(const char *path) {
        if (!Platform::Core::FileExists(path)) {
            Madokawaii::Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                                                "CHART: Chart file does not exist!");
            return {};
        }

        auto fileData = Platform::Core::LoadFileText(path);
        auto result = LoadChartFromMemory(fileData, strlen(fileData));
        Madokawaii::Platform::Core::UnloadFileText(fileData);

        return result;
    }

    chart LoadChartFromMemory(const char *data, size_t size) {
        // Schema validation
        rapidjson::Document schemaDocument;
        // there shouldn't be errors
        assert(!schemaDocument.Parse(chartSchemaV3String.c_str()).HasParseError());

        rapidjson::SchemaDocument schema(schemaDocument);

        // Chart parsing
        rapidjson::Document chartDocument;
        if (chartDocument.Parse(data, size).HasParseError()) {
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR,
                                                "CHART: JSON Parse Error! (Offset %d): %s",
                                                chartDocument.GetErrorOffset(),
                                                rapidjson::GetParseError_En(chartDocument.GetParseError()));
            return {};
        }

        // Schema validation
        rapidjson::SchemaValidator validator(schema);
        if (!chartDocument.Accept(validator)) {
            rapidjson::StringBuffer sb;
            validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "CHART: Invalid Chart!");
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "    > Invalid schema:    %s", sb.GetString());
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "    > Invalid keyword:   %s", validator.GetInvalidSchemaKeyword());
            sb.Clear();
            validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "    > Invalid document:  %s", sb.GetString());
            return {};
        }

        // Parse chart data
        chart mainChart;
        mainChart.formatVersion = chartDocument["formatVersion"].GetInt();
        mainChart.offset = chartDocument["offset"].GetDouble();
        mainChart.judgelineCount = mainChart.speedEventsCount =
                                   mainChart.disappearEventCount =
                                   mainChart.moveEventCount = mainChart.rotateEventCount = mainChart.numOfNotes = 0;

        // Parse judgelines
        for (const auto &judgeline: chartDocument["judgeLineList"].GetArray()) {
            auto thisjudgeline = chart::judgeline();
            thisjudgeline.id = mainChart.judgelineCount++;
            thisjudgeline.bpm = judgeline["bpm"].GetDouble();

            // Parse speed events
            for (const auto &speedEvent: judgeline["speedEvents"].GetArray()) {
                thisjudgeline.speedEvents.emplace_back(chart::judgeline::event_base{
                    .startTime = (
                        !(speedEvent["startTime"].GetDouble() <
                          (thisjudgeline.speedEvents.empty() ? 0 : thisjudgeline.speedEvents.back().endTime))
                            ? speedEvent["startTime"].GetDouble()
                            : (std::abort(), -1)),
                    .endTime = speedEvent["endTime"].GetDouble(),
                    .start = 0, .end = 0, .start2 = 0, .end2 = 0,
                    .value = speedEvent["value"].GetDouble(),
                    .floorPosition = ((++mainChart.speedEventsCount, thisjudgeline.speedEvents.empty())
                                          ? 0
                                          : thisjudgeline.speedEvents.back().floorPosition
                                            + thisjudgeline.speedEvents.back().value
                                            * CalcRealTime(thisjudgeline.bpm,
                                                           static_cast<int>(
                                                               speedEvent["startTime"].GetDouble() - thisjudgeline.
                                                               speedEvents.back().startTime)))
                });
            }

            // Parse disappear events
            for (const auto &judgelineDisappearEvent: judgeline["judgeLineDisappearEvents"].GetArray()) {
                thisjudgeline.judgelineDisappearedEvents.emplace_back(chart::judgeline::event_base{
                    .startTime = (
                        !(judgelineDisappearEvent["startTime"].GetDouble() <
                          (thisjudgeline.judgelineDisappearedEvents.empty()
                               ? -DBL_MAX
                               : thisjudgeline.judgelineDisappearedEvents.back().endTime))
                            ? judgelineDisappearEvent["startTime"].GetDouble()
                            : (std::abort(), -1)),
                    .endTime = judgelineDisappearEvent["endTime"].GetDouble(),
                    .start = judgelineDisappearEvent["start"].GetDouble(),
                    .end = (++mainChart.disappearEventCount, judgelineDisappearEvent["end"].GetDouble()),
                    .start2 = 0, .end2 = 0, .value = 0
                });
            }

            // Parse move events
            for (const auto &judgelineMoveEvent: judgeline["judgeLineMoveEvents"].GetArray()) {
                thisjudgeline.judgelineMoveEvents.emplace_back(chart::judgeline::event_base{
                    .startTime = (
                        !(judgelineMoveEvent["startTime"].GetDouble() <
                          (thisjudgeline.judgelineMoveEvents.empty()
                               ? -DBL_MAX
                               : thisjudgeline.judgelineMoveEvents.back().endTime))
                            ? judgelineMoveEvent["startTime"].GetDouble()
                            : (std::abort(), -1)),
                    .endTime = judgelineMoveEvent["endTime"].GetDouble(),
                    .start = judgelineMoveEvent["start"].GetDouble(),
                    .end = judgelineMoveEvent["end"].GetDouble(),
                    .start2 = judgelineMoveEvent["start2"].GetDouble(),
                    .end2 = (++mainChart.moveEventCount, judgelineMoveEvent["end2"].GetDouble()),
                    .value = 0
                });
            }

            // Parse rotate events
            for (const auto &judgelineRotateEvent: judgeline["judgeLineRotateEvents"].GetArray()) {
                thisjudgeline.judgelineRotateEvents.emplace_back(chart::judgeline::event_base{
                    .startTime = (
                        !(judgelineRotateEvent["startTime"].GetDouble() <
                          (thisjudgeline.judgelineRotateEvents.empty()
                               ? -DBL_MAX
                               : thisjudgeline.judgelineRotateEvents.back().endTime))
                            ? judgelineRotateEvent["startTime"].GetDouble()
                            : (std::abort(), -1)),
                    .endTime = judgelineRotateEvent["endTime"].GetDouble(),
                    .start = judgelineRotateEvent["start"].GetDouble(),
                    .end = (++mainChart.rotateEventCount, judgelineRotateEvent["end"].GetDouble()),
                    .start2 = 0, .end2 = 0, .value = 0
                });
            }

            // Parse notes above
            for (const auto &notesAbove: judgeline["notesAbove"].GetArray()) {
                thisjudgeline.notesAbove.emplace_back(chart::judgeline::note{
                    .type = notesAbove["type"].GetInt(),
                    .time = notesAbove["time"].GetInt(),
                    .positionX = notesAbove["positionX"].GetDouble(),
                    .holdTime = notesAbove["holdTime"].GetDouble(),
                    .speed = notesAbove["speed"].GetDouble(),
                    .floorPosition = notesAbove["floorPosition"].GetDouble(),
                    .realTime = (++mainChart.numOfNotes, CalcRealTime(thisjudgeline.bpm, notesAbove["time"].GetInt())),
                    .isNoteBelow = false,
                    .state = invisible_or_appeared,
                    .parent_line_id = thisjudgeline.id
                });
            }

            // Parse notes below
            for (const auto &notesBelow: judgeline["notesBelow"].GetArray()) {
                thisjudgeline.notesBelow.emplace_back(chart::judgeline::note{
                    .type = notesBelow["type"].GetInt(),
                    .time = notesBelow["time"].GetInt(),
                    .positionX = notesBelow["positionX"].GetDouble(),
                    .holdTime = notesBelow["holdTime"].GetDouble(),
                    .speed = notesBelow["speed"].GetDouble(),
                    .floorPosition = notesBelow["floorPosition"].GetDouble(),
                    .realTime = (++mainChart.numOfNotes, CalcRealTime(thisjudgeline.bpm, notesBelow["time"].GetInt())),
                    .isNoteBelow = true,
                    .state = invisible_or_appeared,
                    .parent_line_id = thisjudgeline.id
                });
            }

            // Sort notes
            std::ranges::sort(thisjudgeline.notesAbove,
                              [](const chart::judgeline::note &l, const chart::judgeline::note &r) {
                                  return l.time < r.time;
                              });
            std::ranges::sort(thisjudgeline.notesBelow,
                              [](const chart::judgeline::note &l, const chart::judgeline::note &r) {
                                  return l.time < r.time;
                              });

            thisjudgeline.info.notesAboveIndex = thisjudgeline.info.notesBelowIndex = 0;
            mainChart.judgelines.push_back(thisjudgeline);
        }

        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "CHART: Chart Loaded");
        return mainChart;
    }

    bool IsValidChart(const chart& chart) {
        return chart.formatVersion == 3 && !chart.judgelines.empty();
    }

    double CalcRealTime(const double bpm, const int time) {
        return time * 1.875 / bpm;
    }

    double CalcBeatTime(const double bpm, const float realTime) {
        return realTime * bpm / 1.875;
    }

    double CalcEventProgress1Param(const chart::judgeline::event_base &event,
                                   const double thisFrameTime,
                                   const std::function<double(double)>& postProcessor) {
        const auto progress = (thisFrameTime - event.startTime) / (event.endTime - event.startTime);
        const auto val = event.start * (1 - progress) + progress * event.end;
        return postProcessor ? postProcessor(val) : val;
    }

    std::tuple<double, double> CalcEventProgress2Params(const chart::judgeline::event_base &event,
                                                        const double thisFrameTime) {
        const auto progress = (thisFrameTime - event.startTime) / (event.endTime - event.startTime);
        return std::make_tuple(event.start * (1 - progress) + progress * event.end,
                               event.start2 * (1 - progress) + progress * event.end2);
    }

    void InitializeJudgelines(chart& mainChart) {
        for (auto &judgeline: mainChart.judgelines) {
            judgeline.info.opacity = 0;
            judgeline.info.posX = judgeline.info.posY = 0.5;
            judgeline.info.rotateAngle = 0;
            judgeline.info.speed = 0;

            judgeline.info.disappearEventPointer = judgeline.judgelineDisappearedEvents.cbegin();
            judgeline.info.moveEventPointer = judgeline.judgelineMoveEvents.cbegin();
            judgeline.info.rotateEventPointer = judgeline.judgelineRotateEvents.cbegin();
            judgeline.info.speedEventPointer = judgeline.speedEvents.cbegin();
        }
    }

    bool IsNoteInScreen(const double x, const double y, const int screenWidth, const int screenHeight) {
        return !(x < 0 || x >= screenWidth || y < 0 || y >= screenHeight);
    }
}