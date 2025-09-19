//
// Created by madoka on 2025/9/19.
//

#include "Madokawaii/app/app_config.h"
namespace Madokawaii::AppConfig {

    ConfigManager::ConfigManager() {
        LoadDefaults();
    }

    ConfigManager& ConfigManager::Instance() {
        static ConfigManager instance;
        return instance;
    }

    const GlobalConfig& ConfigManager::Get() const {
        return config_;
    }

    void ConfigManager::Set(const GlobalConfig& cfg) {
        config_ = cfg;
    }

    const std::string& ConfigManager::GetChartPath() const {
        return config_.chartPath;
    }

    const std::string& ConfigManager::GetMusicPath() const {
        return config_.musicPath;
    }

    void ConfigManager::SetChartPath(const std::string& path) {
        config_.chartPath = path;
    }

    void ConfigManager::SetMusicPath(const std::string& path) {
        config_.musicPath = path;
    }

    bool ConfigManager::LoadDefaults() {
        config_ = GlobalConfig{};
        return true;
    }

    bool ConfigManager::LoadFromFile(const std::string& /*filePath*/) {
        // TODO: 现在还用不上 以后再写
        return false;
    }

    bool ConfigManager::SaveToFile(const std::string& /*filePath*/) {
        // TODO: 现在还用不上 以后再写
        return false;
    }

}