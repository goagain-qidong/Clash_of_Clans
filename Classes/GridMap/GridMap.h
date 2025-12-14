/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     GridMap.h
 * File Function: 等距菱形网格地图类，支持网格显示、建筑放置、碰撞检测等功能
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include <vector>

/**
 * @class GridMap
 * @brief 等距菱形网格地图类，支持网格显示、建筑放置、碰撞检测等功能
 * 
 * 该类使用等距投影（Isometric）方式绘制菱形网格，支持：
 * - 世界坐标与网格坐标的相互转换
 * - 网格可视化显示
 * - 建筑放置区域预览
 * - 碰撞检测和区域占用管理
 */
class GridMap : public cocos2d::Node
{
private:
    cocos2d::Size _mapSize;                          ///< 地图的像素尺寸
    float _tileSize;                                  ///< 单个网格的宽度（像素）
    cocos2d::DrawNode* _gridNode;                     ///< 用于绘制网格线的节点
    cocos2d::DrawNode* _baseNode;                     ///< 用于绘制建筑底座预览的节点

    std::vector<std::vector<bool>> _collisionMap;     ///< 碰撞地图，true表示该网格被占用
    int _gridWidth;                                   ///< 网格宽度（网格单位）
    int _gridHeight;                                  ///< 网格高度（网格单位）

    cocos2d::Vec2 _startPixel;                        ///< 网格(0,0)对应的像素坐标
    bool _gridVisible;                                ///< 网格是否可见

public:
    /**
     * @enum Corner
     * @brief 网格起始角点枚举
     */
    enum Corner { 
        TOP_LEFT = 0,    ///< 左上角
        TOP_RIGHT,       ///< 右上角
        BOTTOM_LEFT,     ///< 左下角
        BOTTOM_RIGHT,    ///< 右下角
        CENTER           ///< 中心点
    };

    /**
     * @brief 创建GridMap实例
     * @param mapSize 地图的像素尺寸
     * @param tileSize 单个网格的宽度（像素）
     * @return 创建成功返回GridMap指针，失败返回nullptr
     */
    static GridMap* create(const cocos2d::Size& mapSize, float tileSize);
    
    /**
     * @brief 初始化GridMap
     * @param mapSize 地图的像素尺寸
     * @param tileSize 单个网格的宽度（像素）
     * @return 初始化成功返回true，失败返回false
     */
    virtual bool init(const cocos2d::Size& mapSize, float tileSize);

    /**
     * @brief 设置网格(0,0)对应的像素坐标
     * @param pixel 起始像素坐标
     */
    void setStartPixel(const cocos2d::Vec2& pixel);
    
    /**
     * @brief 获取网格(0,0)对应的像素坐标
     * @return 起始像素坐标
     */
    cocos2d::Vec2 getStartPixel() const;
    
    /**
     * @brief 通过指定角点设置网格起始位置
     * @param corner 角点类型（TOP_LEFT, TOP_RIGHT等）
     */
    void setStartCorner(Corner corner);

    /**
     * @brief 将世界坐标转换为网格坐标
     * @param worldPosition 世界坐标
     * @return 对应的网格坐标（已限制在有效范围内）
     */
    cocos2d::Vec2 getGridPosition(cocos2d::Vec2 worldPosition);
    
    /**
     * @brief 将网格坐标转换为世界坐标
     * @param gridPos 网格坐标
     * @return 对应的世界坐标（网格中心点）
     */
    cocos2d::Vec2 getPositionFromGrid(cocos2d::Vec2 gridPos);

    /**
     * @brief 更新建筑底座预览显示
     * @param gridPos 建筑的起始网格坐标
     * @param size 建筑占用的网格尺寸
     * @param isValid 是否为有效放置位置（绿色/红色显示）
     */
    void updateBuildingBase(cocos2d::Vec2 gridPos, cocos2d::Size size, bool isValid);
    
    /**
     * @brief 隐藏建筑底座预览
     */
    void hideBuildingBase();

    /**
     * @brief 检查指定区域是否可放置建筑
     * @param startGridPos 起始网格坐标
     * @param size 区域尺寸（网格单位）
     * @return 区域可用返回true，否则返回false
     */
    bool checkArea(cocos2d::Vec2 startGridPos, cocos2d::Size size);
    
    /**
     * @brief 显示或隐藏整个网格
     * @param visible 是否显示
     * @param currentBuildingSize 当前建筑尺寸，用于绘制大网格分组（默认为ZERO）
     */
    void showWholeGrid(bool visible, const cocos2d::Size& currentBuildingSize = cocos2d::Size::ZERO);
    
    /**
     * @brief 标记指定区域为占用或空闲状态
     * @param startGridPos 起始网格坐标
     * @param size 区域尺寸（网格单位）
     * @param occupied true表示占用，false表示空闲
     */
    void markArea(cocos2d::Vec2 startGridPos, cocos2d::Size size, bool occupied);

    // --- Pathfinding helpers ---
    
    /**
     * @brief 获取网格宽度
     * @return 网格宽度（网格单位）
     */
    inline int getGridWidth() const { return _gridWidth; }
    
    /**
     * @brief 获取网格高度
     * @return 网格高度（网格单位）
     */
    inline int getGridHeight() const { return _gridHeight; }
    
    /**
     * @brief 获取单个网格的尺寸
     * @return 网格宽度（像素）
     */
    inline float getTileSize() const { return _tileSize; }
    
    /**
     * @brief 检查指定网格是否被阻挡
     * @param x 网格X坐标
     * @param y 网格Y坐标
     * @return 被阻挡或超出范围返回true，否则返回false
     */
    bool isBlocked(int x, int y) const;
};