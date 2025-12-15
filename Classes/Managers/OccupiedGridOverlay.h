/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     OccupiedGridOverlay.h
 * File Function: 占用网格覆盖层 - 显示已有建筑占用的网格及其周围一格
 * Author:        AI Assistant
 * Update Date:   2025/01/15
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include <vector>

// 前向声明
class GridMap;
class BaseBuilding;

/**
 * @class OccupiedGridOverlay
 * @brief 显示已有建筑占用的网格及其周围一格的覆盖层
 */
class OccupiedGridOverlay : public cocos2d::Node
{
public:
    static OccupiedGridOverlay* create(GridMap* gridMap);
    virtual bool init(GridMap* gridMap);
    
    /**
     * @brief 显示所有已有建筑的占用网格（含周围一格）
     * @param buildings 建筑列表
     */
    void showOccupiedGrids(const cocos2d::Vector<BaseBuilding*>& buildings);
    
    /**
     * @brief 淡出并隐藏覆盖层
     * @param duration 淡出持续时间（默认0.5秒）
     */
    void fadeOutAndHide(float duration = 0.5f);
    
    /**
     * @brief 更新草坪图层（常态显示建筑占用的网格）
     * @param buildings 建筑列表
     */
    void updateGrassLayer(const cocos2d::Vector<BaseBuilding*>& buildings);
    
    /**
     * @brief 立即隐藏覆盖层
     */
    void hide();
    
private:
    GridMap* _gridMap = nullptr;
    cocos2d::DrawNode* _highlightDrawNode = nullptr;  // 高亮图层（淡白色）
    cocos2d::DrawNode* _grassDrawNode = nullptr;      // 草坪图层（淡绿色）
    
    /**
     * @brief 绘制单个网格到指定图层
     * @param drawNode 绘制节点
     * @param gridX 网格X坐标
     * @param gridY 网格Y坐标
     * @param color 填充颜色
     */
    void drawGrid(cocos2d::DrawNode* drawNode, int gridX, int gridY, const cocos2d::Color4F& color);
    
    /**
     * @brief 检查网格是否在有效范围内
     */
    bool isValidGrid(int x, int y) const;
};
