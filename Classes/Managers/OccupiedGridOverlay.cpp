/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     OccupiedGridOverlay.cpp
 * File Function: 占用网格覆盖层实现
 * Author:        赵崇治
 * Update Date:   2025/12/15
 * License:       MIT License
 ****************************************************************/
#include "OccupiedGridOverlay.h"
#include "GridMap.h"
#include "Buildings/BaseBuilding.h"
#include <set>
#include <exception>
#include <stdexcept>

USING_NS_CC;

OccupiedGridOverlay* OccupiedGridOverlay::create(GridMap* gridMap)
{
    OccupiedGridOverlay* ret = new (std::nothrow) OccupiedGridOverlay();
    if (ret && ret->init(gridMap))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool OccupiedGridOverlay::init(GridMap* gridMap)
{
    try {
        if (!Node::init())
        {
            return false;
        }
        
        _gridMap = gridMap;
        
        // 创建草坪图层（底层，淡绿色）
        _grassDrawNode = DrawNode::create();
        this->addChild(_grassDrawNode, 0);
        
        // 创建高亮图层（上层，淡白色）
        _highlightDrawNode = DrawNode::create();
        this->addChild(_highlightDrawNode, 1);
        
        return true;
    }
    catch (const std::exception& e) {
        CCLOG("❌ OccupiedGridOverlay::init 异常: %s", e.what());
        return false;
    }
}

void OccupiedGridOverlay::showOccupiedGrids(const cocos2d::Vector<BaseBuilding*>& buildings)
{
    try {
        if (!_gridMap || !_highlightDrawNode)
            return;
        
        // 停止之前的动作
        this->stopAllActions();
        
        // 清除高亮图层
        _highlightDrawNode->clear();
        
        // 使用集合去重，避免重复绘制
        std::set<std::pair<int, int>> occupiedGrids;
        
        // 遍历所有建筑，收集占用的网格及其周围一格
        for (auto* building : buildings)
        {
            if (!building)
                continue;
            
            Vec2 gridPos = building->getGridPosition();
            Size gridSize = building->getGridSize();
            
            int startX = static_cast<int>(gridPos.x);
            int startY = static_cast<int>(gridPos.y);
            int width = static_cast<int>(gridSize.width);
            int height = static_cast<int>(gridSize.height);
            
            // 建筑占用的网格及其周围一格
            for (int x = startX - 1; x < startX + width + 1; ++x)
            {
                for (int y = startY - 1; y < startY + height + 1; ++y)
                {
                    if (isValidGrid(x, y))
                    {
                        occupiedGrids.insert(std::make_pair(x, y));
                    }
                }
            }
        }
        
        // 绘制所有占用的网格（淡白色）
        Color4F highlightColor(1.0f, 1.0f, 1.0f, 0.15f);
        for (const auto& grid : occupiedGrids)
        {
            drawGrid(_highlightDrawNode, grid.first, grid.second, highlightColor);
        }
        
        // 淡入效果
        _highlightDrawNode->setOpacity(0);
        _highlightDrawNode->setVisible(true);
        auto fadeIn = FadeIn::create(0.3f);
        _highlightDrawNode->runAction(fadeIn);
    }
    catch (const std::exception& e) {
        CCLOG("❌ OccupiedGridOverlay::showOccupiedGrids 异常: %s", e.what());
    }
}

void OccupiedGridOverlay::fadeOutAndHide(float duration)
{
    if (!_highlightDrawNode)
        return;
    
    // 停止之前的动作
    this->stopAllActions();
    
    // 淡出效果
    auto fadeOut = FadeOut::create(duration);
    auto clear = CallFunc::create([this]() {
        if (_highlightDrawNode)
        {
            _highlightDrawNode->clear();
            _highlightDrawNode->setVisible(false);
        }
    });
    
    _highlightDrawNode->runAction(Sequence::create(fadeOut, clear, nullptr));
}

void OccupiedGridOverlay::updateGrassLayer(const cocos2d::Vector<BaseBuilding*>& buildings)
{
    if (!_gridMap || !_grassDrawNode)
        return;
    
    // 清除草坪图层
    _grassDrawNode->clear();
    
    // 使用集合去重，避免重复绘制
    std::set<std::pair<int, int>> buildingGrids;
    
    // 遍历所有建筑，收集占用的网格（不包含周围一格）
    for (auto* building : buildings)
    {
        if (!building)
            continue;
        
        Vec2 gridPos = building->getGridPosition();
        Size gridSize = building->getGridSize();
        
        int startX = static_cast<int>(gridPos.x);
        int startY = static_cast<int>(gridPos.y);
        int width = static_cast<int>(gridSize.width);
        int height = static_cast<int>(gridSize.height);
        
        // 仅收集建筑实际占用的网格
        for (int x = startX; x < startX + width; ++x)
        {
            for (int y = startY; y < startY + height; ++y)
            {
                if (isValidGrid(x, y))
                {
                    buildingGrids.insert(std::make_pair(x, y));
                }
            }
        }
    }
    
    // 绘制草坪（淡绿色）
    Color4F grassColor(0.4f, 0.8f, 0.4f, 0.15f); // 淡绿色半透明
    for (const auto& grid : buildingGrids)
    {
        drawGrid(_grassDrawNode, grid.first, grid.second, grassColor);
    }
    
    // 草坪图层始终可见
    _grassDrawNode->setOpacity(255);
    _grassDrawNode->setVisible(true);
}

void OccupiedGridOverlay::hide()
{
    // 停止所有动作
    this->stopAllActions();
    
    if (_highlightDrawNode)
    {
        _highlightDrawNode->clear();
        _highlightDrawNode->setVisible(false);
    }
    
    if (_grassDrawNode)
    {
        _grassDrawNode->clear();
        _grassDrawNode->setVisible(false);
    }
}

void OccupiedGridOverlay::drawGrid(cocos2d::DrawNode* drawNode, int gridX, int gridY, const cocos2d::Color4F& color)
{
    if (!_gridMap || !drawNode)
        return;
    
    // 获取网格中心位置
    Vec2 center = _gridMap->getPositionFromGrid(Vec2(gridX, gridY));
    
    // 计算菱形网格的四个顶点
    float tileSize = _gridMap->getTileSize();
    float halfW = tileSize / 2.0f;
    float halfH = halfW * 0.75f; // 等距投影比例
    
    Vec2 vertices[4];
    vertices[0] = Vec2(center.x, center.y + halfH);        // 上
    vertices[1] = Vec2(center.x + halfW, center.y);         // 右
    vertices[2] = Vec2(center.x, center.y - halfH);        // 下
    vertices[3] = Vec2(center.x - halfW, center.y);         // 左
    
    // 绘制填充的菱形（无边界）
    drawNode->drawSolidPoly(vertices, 4, color);
}

bool OccupiedGridOverlay::isValidGrid(int x, int y) const
{
    if (!_gridMap)
        return false;
    
    return (x >= 0 && x < _gridMap->getGridWidth() && 
            y >= 0 && y < _gridMap->getGridHeight());
}
