//
// Created by madoka on 2026/6/29.
//

#ifndef MADOKAWAII_AUDIO_ENGINE_CREDIT_H
#define MADOKAWAII_AUDIO_ENGINE_CREDIT_H

namespace Madokawaii::App {
struct AppContext;

// meet with some specific audio engine's attribution requirement without accessing it's api directly.
// for example, FMod(implemented), CriWare adx, etc.
namespace AudioEngineCredit {
    int Iterate(AppContext& context);
    void Unload(AppContext& context);
}

} // namespace Madokawaii::App

#endif //MADOKAWAII_AUDIO_ENGINE_CREDIT_H
