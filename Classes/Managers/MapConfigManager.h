/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MapConfigManager.h
 * File Function: 地图配置管理器头文件
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "json/document.h" // 引入 rapidjson
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

class MapConfigManager : public cocos2d::Node
{
public:
    struct MapConfig {
        float scale = 1.0f;
        cocos2d::Vec2 startPixel = cocos2d::Vec2::ZERO;
        float tileSize = 64.0f;
    };

    static MapConfigManager* create(const std::string& filename = "map_configs.json");
    virtual bool init(const std::string& filename);

    // I/O
    bool loadFromFile();
    bool saveToFile();

    // Access
    bool hasConfig(const std::string& mapName) const;
    MapConfig getConfig(const std::string& mapName) const;
    void setConfig(const std::string& mapName, const MapConfig& cfg);

    // Provide map list used by the editor UI (optional)
    void setAvailableMaps(const std::vector<std::string>& maps);

    // UI: create simple editor controls and attach to this node
    void createEditorUI();

    // helpers
    void refreshUIForMap(const std::string& mapName);

    // Callback when user edits and saves a config
    void setOnConfigChanged(const std::function<void(const std::string&, const MapConfig&)>& cb) { _onConfigChanged = cb; }

private:
    std::string _filename;
    std::unordered_map<std::string, MapConfig> _configs;
    std::vector<std::string> _availableMaps;
    std::function<void(const std::string&, const MapConfig&)> _onConfigChanged;

    // UI widgets
    cocos2d::ui::ListView* _mapListView = nullptr;
    cocos2d::Label* _lblScale = nullptr;
    cocos2d::Label* _lblStart = nullptr;
    cocos2d::Label* _lblTile = nullptr;

    std::string _currentEditingMap;
};