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
    
    // ==================== 地图配置 ====================
    struct MapConfig
    {
        float scale;
        cocos2d::Vec2 startPixel;
        float tileSize;
    };
    
    // ==================== 地图操作 ====================
    
    /**
     * @brief 设置地图列表
     */
    void setMapNames(const std::vector<std::string>& mapNames);
    
    /**
     * @brief 加载地图
     * @param mapName 地图文件名
     * @return 是否成功加载
     */
    bool loadMap(const std::string& mapName);
    
    /**
     * @brief 切换地图
     * @param mapName 地图文件名
     * @return 是否成功切换
     */
    bool switchMap(const std::string& mapName);
    
    /**
     * @brief 平移地图
     */
    void moveMap(const cocos2d::Vec2& delta);
    
    /**
     * @brief 缩放地图
     * @param scaleFactor 缩放因子
     * @param pivotPoint 缩放中心点（世界坐标）
     */
    void zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint = cocos2d::Vec2::ZERO);
    
    /**
     * @brief 确保地图在边界内
     */
    void ensureMapInBoundary();
    
    // ==================== 访问器 ====================
    
    cocos2d::Sprite* getMapSprite() const { return _mapSprite; }
    GridMap* getGridMap() const { return _gridMap; }
    const std::string& getCurrentMapName() const { return _currentMapName; }
    float getCurrentScale() const { return _currentScale; }
    
    // ==================== 地图配置 ====================
    
    /**
     * @brief 设置地图配置
     */
    void setMapConfig(const std::string& mapName, const MapConfig& config);
    
    /**
     * @brief 设置缩放限制
     */
    void setScaleLimits(float minScale, float maxScale);
    
private:
    // ==================== 地图数据 ====================
    cocos2d::Sprite* _mapSprite = nullptr;
    GridMap* _gridMap = nullptr;
    
    std::string _currentMapName;
    std::vector<std::string> _mapNames;
    std::unordered_map<std::string, MapConfig> _mapConfigs;
    
    // ==================== 缩放和边界 ====================
    float _currentScale = 1.3f;
    float _minScale = 0.7f;
    float _maxScale = 2.5f;
    cocos2d::Rect _mapBoundary;
    cocos2d::Vec2 _gridStartDefault;
    
    cocos2d::Size _visibleSize;
    
    // ==================== 内部方法 ====================
    void updateBoundary();
    void initializeGrid(const MapConfig& config);
};

#endif // __MAP_CONTROLLER_H__
