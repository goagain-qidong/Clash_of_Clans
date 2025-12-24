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
#include "json/document.h"

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

/**
 * @class MapConfigManager
 * @brief 地图配置管理器
 */
class MapConfigManager : public cocos2d::Node
{
public:
    /**
     * @struct MapConfig
     * @brief 地图配置数据
     */
    struct MapConfig {
        float scale = 1.0f;                            ///< 缩放比例
        cocos2d::Vec2 startPixel = cocos2d::Vec2::ZERO;  ///< 起始像素位置
        float tileSize = 64.0f;                        ///< 瓦片尺寸
    };

    /**
     * @brief 创建地图配置管理器
     * @param filename 配置文件名
     * @return MapConfigManager* 管理器指针
     */
    static MapConfigManager* create(const std::string& filename = "map_configs.json");

    /**
     * @brief 初始化
     * @param filename 配置文件名
     * @return bool 是否成功
     */
    virtual bool init(const std::string& filename);

    /** @brief 从文件加载配置 */
    bool loadFromFile();

    /** @brief 保存配置到文件 */
    bool saveToFile();

    /**
     * @brief 检查是否有指定地图的配置
     * @param mapName 地图名称
     * @return bool 是否存在
     */
    bool hasConfig(const std::string& mapName) const;

    /**
     * @brief 获取地图配置
     * @param mapName 地图名称
     * @return MapConfig 地图配置
     */
    MapConfig getConfig(const std::string& mapName) const;

    /**
     * @brief 设置地图配置
     * @param mapName 地图名称
     * @param cfg 配置数据
     */
    void setConfig(const std::string& mapName, const MapConfig& cfg);

    /**
     * @brief 设置可用地图列表
     * @param maps 地图名称列表
     */
    void setAvailableMaps(const std::vector<std::string>& maps);

    /** @brief 创建编辑器UI */
    void createEditorUI();

    /**
     * @brief 刷新指定地图的UI
     * @param mapName 地图名称
     */
    void refreshUIForMap(const std::string& mapName);

    /** @brief 设置配置变更回调 */
    void setOnConfigChanged(const std::function<void(const std::string&, const MapConfig&)>& cb) { _onConfigChanged = cb; }

private:
    std::string _filename;                               ///< 配置文件名
    std::unordered_map<std::string, MapConfig> _configs; ///< 配置映射
    std::vector<std::string> _availableMaps;             ///< 可用地图列表
    std::function<void(const std::string&, const MapConfig&)> _onConfigChanged;  ///< 配置变更回调

    cocos2d::ui::ListView* _mapListView = nullptr;  ///< 地图列表视图
    cocos2d::Label* _lblScale = nullptr;            ///< 缩放标签
    cocos2d::Label* _lblStart = nullptr;            ///< 起始位置标签
    cocos2d::Label* _lblTile = nullptr;             ///< 瓦片标签

    std::string _currentEditingMap;  ///< 当前编辑的地图
};