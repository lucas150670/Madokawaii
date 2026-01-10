//
// Created by madoka on 2025/9/19.
//

#ifndef MADOKAWAII_LINE_OPERATION_H
#define MADOKAWAII_LINE_OPERATION_H
#include "Madokawaii/app/chart.h"
#include "Madokawaii/platform/graphics.h"

void UpdateJudgeline(Madokawaii::App::chart::judgeline& judgeline, double thisFrameTime, int screenWidth, int screenHeight, std::vector<Madokawaii::App::chart::judgeline::note *>& noteRenderList);
void RenderJudgeline(const Madokawaii::App::chart::judgeline& judgeline, int screenWidth, int screenHeight,
    Madokawaii::Platform::Graphics::Color = Madokawaii::Platform::Graphics::M_WHITE);
void RenderDebugInfo(int screenWidth, int screenHeight);

#endif //MADOKAWAII_LINE_OPERATION_H