//
// Created by madoka on 2025/9/19.
//

#ifndef MADOKAWAII_LINE_OPERATION_H
#define MADOKAWAII_LINE_OPERATION_H
#include <vector>

#include "Madokawaii/app/chart.hpp"
#include "Madokawaii/platform/graphics.hpp"

namespace Madokawaii::App {
struct AppContext;

namespace Line {
    void UpdateJudgeline(AppContext& context, chart::judgeline& judgeline, double thisFrameTime,
                         std::vector<chart::judgeline::note *>& noteRenderList, int* playedNoteCount = nullptr);
    void RenderJudgeline(const AppContext& context, const chart::judgeline& judgeline);
    void RenderDebugInfo(const AppContext& context);
}

} // namespace Madokawaii::App

#endif //MADOKAWAII_LINE_OPERATION_H
