//
// Created by madoka on 2025/12/15.
//

#ifndef MADOKAWAII_NOTE_OPERATION_H
#define MADOKAWAII_NOTE_OPERATION_H
#include "res_pack.hpp"
#include "Madokawaii/app/chart.hpp"

namespace Madokawaii::App {
struct AppContext;

namespace NoteRenderer {
    void Initialize(AppContext& context, const ResPack::ResPack& resPack);
    void RenderNote(const AppContext& context, const chart::judgeline::note& note);
    void AddHoldNoteClickingRender(AppContext& context, const chart::judgeline::note& note);
    void RenderHoldCallback(AppContext& context, float thisFrameTime);
    void Unload(AppContext& context);
}

} // namespace Madokawaii::App

#endif //MADOKAWAII_NOTE_OPERATION_H
