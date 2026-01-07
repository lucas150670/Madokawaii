//
// Created by madoka on 2025/12/28.
//

#ifndef MADOKAWAII_NOTE_HIT_H
#define MADOKAWAII_NOTE_HIT_H
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/app/res_pack.h"

int InitializeNoteHitSfxManager(Madokawaii::App::ResPack::ResPack&);
void RegisterNoteHitSfx(int type);
void CleanupNoteHitSfxManager();
void UpdateNoteHitSfx();
void UnloadNoteHitSfxManager();
int InitializeNoteHitFxManager(Madokawaii::App::ResPack::ResPack&, Madokawaii::Platform::Graphics::Color color = Madokawaii::Platform::Graphics::M_WHITE);
void RegisterNoteHitFx(float, float position_X, float position_Y);
void UpdateNoteHitFx(float);

void UnloadNoteHitFxManager();

#endif //MADOKAWAII_NOTE_HIT_H