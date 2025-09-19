// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/app/app_config.h
// Usage: Store app config. Maybe initialized by app boot.
// ============================================================

#ifndef MADOKAWAII_APP_CONFIG_H
#define MADOKAWAII_APP_CONFIG_H

#include <string>

namespace Madokawaii::AppConfig {
    struct GlobalConfig {
        std::string chartPath = "assets/charts/chart.json",
                // illustrationPath = "assets/charts/Stasis/Illustration.png",
                musicPath = "assets/charts/music.wav";
                // schemaFilePath = "assets/charts/chart.v3.schema.json",
                          };
}

#endif //MADOKAWAII_APP_CONFIG_H
