//
// Created by madoka on 2025/12/28.
//

#ifndef MADOKAWAII_NOTE_HIT_H
#define MADOKAWAII_NOTE_HIT_H
#include "Madokawaii/app/res_pack.h"

int InitializeNoteHitSfxManager(Madokawaii::App::ResPack::ResPack&);
void RegisterNoteHitSfx(int type);
void CleanupNoteHitSfxManager();
void UpdateNoteHitSfx();
void UnloadNoteHitSfxManager();
int InitializeNoteHitFxManager(Madokawaii::App::ResPack::ResPack&);
void RegisterNoteHitFx(int type);
void UpdateNoteHitFx();

int InitializeNoteHitFx(const Madokawaii::App::ResPack::ResPack&);

#endif //MADOKAWAII_NOTE_HIT_H