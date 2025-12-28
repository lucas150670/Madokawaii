//
// Created by madoka on 2025/12/28.
//

#ifndef MADOKAWAII_NOTE_HIT_H
#define MADOKAWAII_NOTE_HIT_H
#include "Madokawaii/app/res_pack.h"

int InitializeNoteHitSfxManager(const Madokawaii::App::ResPack::ResPack&);
void RegisterNoteHitSfx(const Madokawaii::App::NoteType type);
void CleanupNoteHitSfxManager();
void UpdateNoteHitSfx();
void UnloadNoteHitSfxManager();

#endif //MADOKAWAII_NOTE_HIT_H