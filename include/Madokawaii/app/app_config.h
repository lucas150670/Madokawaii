// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/app/app_config.h
// Usage: Store app config. Maybe initialized by app boot.
// ============================================================

#ifndef MADOKAWAII_APP_CONFIG_H
#define MADOKAWAII_APP_CONFIG_H

#include <string>

namespace Madokawaii::AppConfig {

    struct ColorSave
    {
        unsigned char r, g, b, a;
    };

    struct GlobalConfig
    {
#if !defined(PLATFORM_ANDROID)
        std::string chartPath{"assets/charts/chart.json"};
        std::string musicPath{"assets/charts/music.wav"};
        std::string backgroundPath{"assets/charts/illustration.png"};
        std::string resPackPath{"assets/respacks/default.zip"};
#else
        std::string chartPath{"chart.json"};
        std::string musicPath{"music.wav"};
        std::string backgroundPath{"illustration.png"};
        std::string resPackPath{"default.zip"};
#endif
        ColorSave perfectColor{255,236,160, 226};
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
        [[nodiscard]] ColorSave GetPerfectColor() const;

        void SetChartPath(const std::string& path);
        void SetMusicPath(const std::string& path);
        void SetResPackPath(const std::string& path);
        void SetBackgroundPath(const std::string& path);
        void SetPerfectColor(const ColorSave& color);

        bool LoadDefaults();
        static bool LoadFromFile(const std::string& filePath);
        [[nodiscard]] static bool SaveToFile(const std::string& filePath) ;

    private:
        ConfigManager();
        GlobalConfig config_;
    };

} // namespace Madokawaii::AppConfig

#endif //MADOKAWAII_APP_CONFIG_H