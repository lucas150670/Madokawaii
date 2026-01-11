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
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace Madokawaii::App::Chart {

    namespace {
        struct BpmEventPec { double start{}, end{}, bpm{}, value{}; };
        class BpmList {
        public:
            explicit BpmList(double baseBpm) : baseBpm_(std::isnan(baseBpm) ? 120.0 : baseBpm), accTime_(0.0) {}
            void push(double start, double end, double bpm) {
                list_.push_back({start, end, bpm, accTime_});
                accTime_ += (end - start) / bpm;
            }
            double calc(double beat) const {
                double time = 0.0;
                for (const auto &e : list_) {
                    if (beat > e.end) continue;
                    if (beat < e.start) break;
                    time = std::round(((beat - e.start) / e.bpm + e.value) * baseBpm_ * 32.0);
                }
                return time;
            }
            double base() const { return baseBpm_; }
        private:
            double baseBpm_{120.0};
            double accTime_{0.0};
            std::vector<BpmEventPec> list_;
        };

        struct SpeedEventPec { double time{}, value{}; };
        struct NotePec {
            int type{};
            double time{}, posX{}, holdTime{}, speed{};
            bool above{}, fake{};
        };
        struct LineEventPec { double startTime{}, endTime{}, v1{}, v2{}, motion{}; enum Type { Alpha, Move, Rotate } type; };
        struct LinePec {
            explicit LinePec(double bpm) : bpm_(std::isnan(bpm) ? 120.0 : bpm) {}
            void pushNote(int type, double time, double posX, double hold, double speed, bool above, bool fake) {
                notes_.push_back({type, time, posX, hold, speed, above, fake});
            }
            void pushSpeed(double t, double v) { speedEvents_.push_back({t, v}); }
            void pushAlpha(double s, double e, double v, double motion) { events_.push_back({s, e, v, 0.0, motion, LineEventPec::Alpha}); }
            void pushMove(double s, double e, double x, double y, double motion) { events_.push_back({s, e, x, y, motion, LineEventPec::Move}); }
            void pushRotate(double s, double e, double r, double motion) { events_.push_back({s, e, r, 0.0, motion, LineEventPec::Rotate}); }

            chart::judgeline format(int id, chart &mainChart) const {
                chart::judgeline jl{};
                jl.id = id;
                jl.bpm = bpm_;

                auto speedEvents = speedEvents_;
                std::ranges::sort(speedEvents, {}, &SpeedEventPec::time);
                double floor = 0.0;
                for (size_t i = 0; i < speedEvents.size(); ++i) {
                    const auto startTime = std::max(speedEvents[i].time, 0.0);
                    const auto endTime = (i + 1 < speedEvents.size()) ? speedEvents[i + 1].time : 1e9;
                    const auto value = speedEvents[i].value;
                    jl.speedEvents.push_back({
                        .startTime = startTime,
                        .endTime = endTime,
                        .start = 0, .end = 0, .start2 = 0, .end2 = 0,
                        .value = value,
                        .floorPosition = floor
                    });
                    floor = std::fma(endTime - startTime, value / bpm_ * 1.875, floor);
                }

                auto notes = notes_;
                std::ranges::sort(notes, {}, &NotePec::time);
                for (const auto &n : notes) {
                    double v1 = 0, v2 = 0, v3 = 0;
                    for (const auto &se : jl.speedEvents) {
                        if (n.time > se.endTime) continue;
                        if (n.time < se.startTime) break;
                        v1 = se.floorPosition;
                        v2 = se.value;
                        v3 = n.time - se.startTime;
                    }
                    chart::judgeline::note note{
                        .type = n.type,
                        .time = n.fake ? n.time + 1e9 : n.time,
                        .positionX = n.posX,
                        .holdTime = n.holdTime,
                        .speed = (n.type == 3 ? v2 : 1.0) * n.speed,
                        .floorPosition = std::fma(v2, v3 / bpm_ * 1.875, v1),
                        .realTime = CalcRealTime(bpm_, n.fake ? n.time + 1e9 : n.time),
                        .coordinateX = -10000.0,
                        .coordinateY = -10000.0,
                        .isNoteBelow = !n.above,
                        .state = invisible_or_appeared,
                        .parent_line_id = id
                    };
                    if (n.above) {
                        jl.notesAbove.push_back(note);
                    } else {
                        jl.notesBelow.push_back(note);
                    }
                    if (!n.fake) ++mainChart.numOfNotes;
                }
                std::ranges::sort(jl.notesAbove, {}, &chart::judgeline::note::time);
                std::ranges::sort(jl.notesBelow, {}, &chart::judgeline::note::time);

                auto events = events_;
                std::ranges::sort(events, [](const auto &a, const auto &b) {
                    return (a.startTime == b.startTime) ? (a.endTime < b.endTime) : (a.startTime < b.startTime);
                });

                auto pushRangeEvents = [&](LineEventPec::Type type, auto &&pushFn) {
                    double lastTime = 0.0, last1 = 0.0, last2 = 0.0;
                    for (const auto &e : events) {
                        if (e.type != type) continue;
                        pushFn(lastTime, e.startTime, last1, last1, last2, last2);
                        // 别急 现在只支持官方线性插值缓动，在做了
                        pushFn(e.startTime, e.endTime,
                               last1, e.v1,
                               last2, e.type == LineEventPec::Move ? e.v2 : last2);
                        lastTime = e.endTime;
                        last1 = e.v1;
                        last2 = type == LineEventPec::Move ? e.v2 : last2;
                    }
                    pushFn(lastTime, 1e9, last1, last1, last2, last2);
                };

                pushRangeEvents(LineEventPec::Alpha, [&](double s, double e, double v1, double v2, double, double) {
                    jl.judgelineDisappearedEvents.push_back({s, e, v1, v2, 0, 0, 0});
                    ++mainChart.disappearEventCount;
                });
                pushRangeEvents(LineEventPec::Move, [&](double s, double e, double v1, double v2, double v3, double v4) {
                    jl.judgelineMoveEvents.push_back({s, e, v1, v2, v3, v4, 0});
                    ++mainChart.moveEventCount;
                });
                pushRangeEvents(LineEventPec::Rotate, [&](double s, double e, double v1, double v2, double, double) {
                    jl.judgelineRotateEvents.push_back({s, e, v1, v2, 0, 0, 0});
                    ++mainChart.rotateEventCount;
                });

                return jl;
            }

        private:
            double bpm_{120.0};
            std::vector<SpeedEventPec> speedEvents_;
            std::vector<NotePec> notes_;
            std::vector<LineEventPec> events_;
        };

        struct BpmCommand { double time{}, bpm{}; };
        struct NoteCommand {
            int type{};
            int lineId{};
            double time{}, time2{};
            double offsetX{};
            int isAbove{};
            int isFake{};
            double speed{1.0};
            double size{1.0};
        };
        struct LineCommand {
            char type{};
            int lineId{};
            double time{}, time2{};
            double speed{};
            double offsetX{}, offsetY{}, rotation{}, alpha{};
            double motionType{};
        };

        int mapNoteType(int pecType) {
            // mirror of [0,1,4,2,3].indexOf in TS
            const int mapping[5] = {0, 1, 3, 4, 2};
            if (pecType >= 1 && pecType <= 4) return mapping[pecType];
            return 0;
        }
    }

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
        if (!data || size == 0) return {};
        std::string_view text(data, size);
        // tokenize by whitespace
        std::vector<std::string> tokens;
        tokens.reserve(size / 2);
        std::string cur;
        for (char c : text) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!cur.empty()) { tokens.emplace_back(std::move(cur)); cur.clear(); }
            } else cur.push_back(c);
        }
        if (!cur.empty()) tokens.emplace_back(std::move(cur));
        if (tokens.empty()) return {};

        size_t ptr = 0;
        double rawOffset = 0.0;
        try { rawOffset = std::stod(tokens[ptr]); }
        catch (...) { rawOffset = 0.0; }
        ++ptr;

        std::vector<BpmCommand> bpms;
        std::vector<NoteCommand> notes;
        std::vector<LineCommand> lines;

        auto read = [&](auto &out) -> bool {
            if (ptr >= tokens.size()) return false;
            try { out = std::stod(tokens[ptr++]); return true; }
            catch (...) { return false; }
        };

        while (ptr < tokens.size()) {
            const auto &cmd = tokens[ptr++];
            if (cmd.empty()) continue;
            if (cmd == "bp") {
                BpmCommand b{};
                if (!read(b.time) || !read(b.bpm)) return {};
                bpms.push_back(b);
            } else if (cmd.size() >= 2 && cmd[0] == 'n' && std::string("1234").find(cmd[1]) != std::string::npos) {
                NoteCommand n{};
                n.type = cmd[1] - '0';
                double tmpLine = 0;
                double tmp;
                if (!read(tmpLine) || !read(n.time)) return {};
                n.lineId = tmpLine;
                n.time2 = (n.type == 2 ? (read(tmp) ? tmp : n.time) : n.time);
                n.offsetX = (read(tmp) ? tmp : 0.0);
                int above = 0, fake = 0;
                if (!read(tmp)) return {};
                above = static_cast<int>(tmp);
                if (!read(tmp)) return {};
                fake = static_cast<int>(tmp);
                n.isAbove = above;
                n.isFake = fake;
                // speed optional marked by #
                if (ptr < tokens.size() && !tokens[ptr].empty() && tokens[ptr][0] == '#') {
                    ++ptr;
                    if (!read(tmp)) return {};
                    n.speed = tmp;
                }
                // size optional marked by &
                if (ptr < tokens.size() && !tokens[ptr].empty() && tokens[ptr][0] == '&') {
                    ++ptr;
                    if (!read(tmp)) return {};
                    n.size = tmp;
                }
                notes.push_back(n);
            } else if (cmd.size() >= 2 && cmd[0] == 'c' && std::string("vpdamrf").find(cmd[1]) != std::string::npos) {
                LineCommand c{};
                c.type = cmd[1];
                int tmpLine = 0;
                double tmp;
                if (!read(tmpLine) || !read(c.time)) return {};
                c.lineId = tmpLine;
                if (c.type == 'v') { if (!read(c.speed)) return {}; }
                c.time2 = std::string("mrf").find(c.type) != std::string::npos ? (read(tmp) ? tmp : c.time) : c.time;
                if (std::string("pm").find(c.type) != std::string::npos) { if (!read(c.offsetX) || !read(c.offsetY)) return {}; }
                if (std::string("dr").find(c.type) != std::string::npos) { if (!read(c.rotation)) return {}; }
                if (std::string("af").find(c.type) != std::string::npos) { if (!read(c.alpha)) return {}; }
                if (std::string("mr").find(c.type) != std::string::npos) { if (!read(c.motionType)) return {}; }
                if (std::string("pdaf").find(c.type) != std::string::npos) c.motionType = 1;
                lines.push_back(c);
            } else {
                Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR,
                                        "CHART: PEC Chart Load Error - Unsupported command %s", cmd.c_str());
                return {};
            }
        }

        if (bpms.empty()) {
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR, "CHART: PEC Chart Load Error - Missing bpm");
            return {};
        }

        std::ranges::sort(bpms, std::ranges::less{}, &BpmCommand::time);

        BpmList bpmList(bpms.front().bpm);
        for (size_t i = 0; i < bpms.size(); ++i) {
            if (i + 1 < bpms.size() && bpms[i + 1].time <= 0) continue;
            double start = bpms[i].time < 0 ? 0 : bpms[i].time;
            double end = (i + 1 < bpms.size()) ? bpms[i + 1].time : 1e9;
            bpmList.push(start, end, bpms[i].bpm);
        }

        // prepare lines
        std::vector<std::unique_ptr<LinePec>> lineObjs;
        auto ensureLine = [&](int id) {
            if (id < 0) return;
            if (static_cast<size_t>(id) >= lineObjs.size()) lineObjs.resize(id + 1);
            if (!lineObjs[id]) lineObjs[id] = std::make_unique<LinePec>(bpmList.base());
        };

        for (const auto &n : notes) {
            ensureLine(n.lineId);
            const int mappedType = mapNoteType(n.type);
            const double time = bpmList.calc(n.time);
            const double hold = bpmList.calc(n.time2) - time;
            const double speed = std::isnan(n.speed) ? 1.0 : n.speed ;
            lineObjs[n.lineId]->pushNote(mappedType, time, n.offsetX / 115.2, hold, speed, n.isAbove == 1, n.isFake != 0);
            if (n.isFake != 0) {
                Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_WARNING,
                                        "CHART: PEC Fake note detected on line %d", n.lineId);
            }
        }

        auto isMotion = [](double m) { return static_cast<int>(std::trunc(m)) != m ? false : (m == 1 || (m >= 2 && m <= 29)); };
        for (const auto &c : lines) {
            ensureLine(c.lineId);
            const double t1 = bpmList.calc(c.time);
            const double t2 = bpmList.calc(c.time2);
            if (t1 > t2) {
                Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_WARNING,
                                        "CHART: PEC event start > end ignored on line %d", c.lineId);
                continue;
            }
            if (c.type == 'v') lineObjs[c.lineId]->pushSpeed(t1, c.speed / 7.0);
            if (c.type == 'a' || c.type == 'f') {
                lineObjs[c.lineId]->pushAlpha(t1, t2, std::max(c.alpha / 255.0, 0.0), c.motionType);
            }
            if (c.type == 'p' || c.type == 'm') {
                lineObjs[c.lineId]->pushMove(t1, t2, c.offsetX / 2048.0, c.offsetY / 1400.0,
                                             isMotion(c.motionType) ? c.motionType : 1);
            }
            if (c.type == 'd' || c.type == 'r') {
                lineObjs[c.lineId]->pushRotate(t1, t2, -c.rotation, isMotion(c.motionType) ? c.motionType : 1);
            }
        }

        chart mainChart{};
        mainChart.formatVersion = 3;
        mainChart.offset = rawOffset / 1000.0 - 0.175;
        mainChart.judgelineCount = mainChart.speedEventsCount = mainChart.disappearEventCount =
            mainChart.moveEventCount = mainChart.rotateEventCount = mainChart.numOfNotes = 0;
        mainChart.global_bpm = bpmList.base();

        for (size_t i = 0; i < lineObjs.size(); ++i) {
            if (!lineObjs[i]) continue;
            auto jl = lineObjs[i]->format(static_cast<int>(i), mainChart);
            mainChart.speedEventsCount += static_cast<int>(jl.speedEvents.size());
            mainChart.judgelines.push_back(std::move(jl));
            ++mainChart.judgelineCount;
        }

        if (mainChart.judgelines.empty()) {
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR, "CHART: PEC Chart Load Error - No judgelines");
            return {};
        }

        Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_INFO, "CHART: PEC Chart Loaded");
        return mainChart;
    }

}