//
// Created by madoka on 2025/9/19.
//

#ifndef MADOKAWAII_LINE_OPERATION_H
#define MADOKAWAII_LINE_OPERATION_H
#include "Madokawaii/app/chart.h"

void UpdateJudgeline(Madokawaii::App::chart::judgeline& judgeline, double thisFrameTime, int screenWidth, int screenHeight, std::vector<Madokawaii::App::chart::judgeline::note *>& noteRenderList);
void RenderJudgeline(const Madokawaii::App::chart::judgeline& judgeline, int screenWidth, int screenHeight);
void RenderDebugInfo();

#endif //MADOKAWAII_LINE_OPERATION_H