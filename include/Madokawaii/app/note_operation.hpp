//
// Created by madoka on 2025/12/15.
//

#ifndef MADOKAWAII_NOTE_OPERATION_H
#define MADOKAWAII_NOTE_OPERATION_H
#include "res_pack.hpp"
#include "Madokawaii/app/chart.hpp"

void InitializeNoteRenderer(const Madokawaii::App::ResPack::ResPack&, float, float);
void RenderNote(const Madokawaii::App::chart::judgeline::note& note);
void AddHoldNoteClickingRender(const Madokawaii::App::chart::judgeline::note& note);
void RenderHoldCallback(float thisFrameTime, const Madokawaii::App::chart& thisChart);

void UnloadNoteRenderer();

#endif //MADOKAWAII_NOTE_OPERATION_H
