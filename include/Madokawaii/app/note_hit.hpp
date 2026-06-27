//
// Created by madoka on 2025/12/28.
//

#ifndef MADOKAWAII_NOTE_HIT_H
#define MADOKAWAII_NOTE_HIT_H
#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/app/res_pack.hpp"

namespace Madokawaii::App {
struct AppContext;

namespace NoteHit {
    int InitializeSfxManager(AppContext& context, ResPack::ResPack& resPack);
    void RegisterSfx(AppContext& context, int type);
    void CleanupSfxManager(AppContext& context);
    void UpdateSfx(AppContext& context);
    void UnloadSfxManager(AppContext& context);

    int InitializeFxManager(AppContext& context, ResPack::ResPack& resPack,
                            Platform::Graphics::Color color = Platform::Graphics::M_WHITE);
    // position_X/Y are normalized coordinates. Origin is bottom-left, top-right is (1, 1).
    void RegisterFx(AppContext& context, float thisFrameTime, float positionX, float positionY);
    void UpdateFx(AppContext& context, float thisFrameTime);
    void UnloadFxManager(AppContext& context);
}

} // namespace Madokawaii::App

#endif //MADOKAWAII_NOTE_HIT_H
