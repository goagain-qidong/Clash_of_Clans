/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MapController.h
 * File Function: 地图控制器 - 管理游戏中的地图切换、缩放和移动
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef __MAP_CONTROLLER_H__
#define __MAP_CONTROLLER_H__

#include "cocos2d.h"

#include <string>
#include <vector>
#include <unordered_map>

class GridMap;

/**
 * @class MapController
 * @brief 地图控制器 - 负责地图的加载、切换、缩放和边界管理
 *
 * 职责：
 * - 地图的加载和切换
 * - 地图的缩放和平移
 * - 地图边界检查
 * - 地图配置管理
 */
class MapController : public cocos2d::Node
{
public:
    CREATE_FUNC(MapController);

    virtual bool init() override;

    /**
     * @struct MapConfig
     * @brief 地图配置
     */
    struct MapConfig
    {
        float scale;                ///< 缩放比例
        cocos2d::Vec2 startPixel;   ///< 起始像素位置
        float tileSize;             ///< 瓦片大小
    };

    /**
     * @brief 设置地图列表
     * @param mapNames 地图文件名列表
     */
    void setMapNames(const std::vector<std::string>& mapNames);

    /**
     * @brief 加载地图
     * @param mapName 地图文件名
     * @return bool 是否成功加载
     */
    bool loadMap(const std::string& mapName);

    /**
     * @brief 切换地图
     * @param mapName 地图文件名
     * @return bool 是否成功切换
     */
    bool switchMap(const std::string& mapName);

    /**
     * @brief 平移地图
     * @param delta 位移量
     */
    void moveMap(const cocos2d::Vec2& delta);

    /**
     * @brief 缩放地图
     * @param scaleFactor 缩放因子
     * @param pivotPoint 缩放中心点
     */
    void zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint = cocos2d::Vec2::ZERO);

    /** @brief 确保地图在边界内 */
    void ensureMapInBoundary();

    /** @brief 获取地图精灵 */
    cocos2d::Sprite* getMapSprite() const { return _mapSprite; }

    /** @brief 获取网格地图 */
    GridMap* getGridMap() const { return _gridMap; }

    /** @brief 获取当前地图名称 */
    const std::string& getCurrentMapName() const { return _currentMapName; }

    /** @brief 获取当前缩放比例 */
    float getCurrentScale() const { return _currentScale; }

    /**
     * @brief 设置地图配置
     * @param mapName 地图名称
     * @param config 配置
     */
    void setMapConfig(const std::string& mapName, const MapConfig& config);

    /**
     * @brief 设置缩放限制
     * @param minScale 最小缩放
     * @param maxScale 最大缩放
     */
    void setScaleLimits(float minScale, float maxScale);

private:
    cocos2d::Sprite* _mapSprite = nullptr;  ///< 地图精灵
    GridMap* _gridMap = nullptr;            ///< 网格地图

    std::string _currentMapName;            ///< 当前地图名称
    std::vector<std::string> _mapNames;     ///< 地图名称列表
    std::unordered_map<std::string, MapConfig> _mapConfigs;  ///< 地图配置

    float _currentScale = 1.3f;   ///< 当前缩放比例
    float _minScale = 0.7f;       ///< 最小缩放
    float _maxScale = 2.5f;       ///< 最大缩放
    cocos2d::Rect _mapBoundary;   ///< 地图边界
    cocos2d::Vec2 _gridStartDefault;  ///< 网格起始位置

    cocos2d::Size _visibleSize;   ///< 可视区域大小

    void updateBoundary();                          ///< 更新边界
    void initializeGrid(const MapConfig& config);   ///< 初始化网格
};

#endif // __MAP_CONTROLLER_H__
