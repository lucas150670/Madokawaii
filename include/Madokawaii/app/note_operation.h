//
// Created by madoka on 2025/12/15.
//

#ifndef MADOKAWAII_NOTE_OPERATION_H
#define MADOKAWAII_NOTE_OPERATION_H
#include "res_pack.h"
#include "Madokawaii/app/chart.h"

void InitializeNoteRenderer(const Madokawaii::App::ResPack::ResPack&);
void RenderNote(const Madokawaii::App::chart::judgeline::note& note);

#endif //MADOKAWAII_NOTE_OPERATION_H
