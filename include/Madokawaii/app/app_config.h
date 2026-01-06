// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/app/app_config.h
// Usage: Store app config. Maybe initialized by app boot.
// ============================================================

#ifndef MADOKAWAII_APP_CONFIG_H
#define MADOKAWAII_APP_CONFIG_H

#include <string>

namespace Madokawaii::AppConfig {

    struct GlobalConfig
    {
        std::string chartPath{"assets/charts/chart.json"};
        std::string musicPath{"assets/charts/music.wav"};
        std::string backgroundPath{"assets/charts/illustration.png"};
        std::string resPackPath{"assets/respacks/default.zip"};
    };

    class ConfigManager {
    public:
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;

        static ConfigManager& Instance();
        [[nodiscard]] const GlobalConfig& Get() const;
        void Set(const GlobalConfig& cfg);

        [[nodiscard]] const std::string& GetChartPath() const;
        [[nodiscard]] const std::string& GetMusicPath() const;
        [[nodiscard]] const std::string& GetResPackPath() const;
        [[nodiscard]] const std::string& GetBackgroundPath() const;

        void SetChartPath(const std::string& path);
        void SetMusicPath(const std::string& path);
        void SetResPackPath(const std::string& path);
        void SetBackgroundPath(const std::string& path);

        bool LoadDefaults();
        static bool LoadFromFile(const std::string& filePath);
        [[nodiscard]] static bool SaveToFile(const std::string& filePath) ;

    private:
        ConfigManager();
        GlobalConfig config_;
    };

} // namespace Madokawaii::AppConfig

#endif //MADOKAWAII_APP_CONFIG_H