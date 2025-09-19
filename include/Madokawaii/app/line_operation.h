//
// Created by madoka on 2025/9/19.
//

#ifndef MADOKAWAII_LINE_OPERATION_H
#define MADOKAWAII_LINE_OPERATION_H
#include "Madokawaii/app/chart.h"

void UpdateJudgeline(Madokawaii::Defs::chart::judgeline& judgeline, double thisFrameTime, int screenWidth, int screenHeight, std::vector<Madokawaii::Defs::chart::judgeline::note *>& noteRenderList);
void RenderJudgeline(const Madokawaii::Defs::chart::judgeline& judgeline, int screenWidth, int screenHeight);
void RenderDebugInfo();

#endif //MADOKAWAII_LINE_OPERATION_H