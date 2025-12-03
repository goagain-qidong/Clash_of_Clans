#pragma once

#include "cocos2d.h"
#include <vector>

class GridMap : public cocos2d::Node
{
private:
    cocos2d::Size _mapSize;
    float _tileSize;
    cocos2d::DrawNode* _gridNode;
    cocos2d::DrawNode* _baseNode;

    std::vector<std::vector<bool>> _collisionMap;
    int _gridWidth;
    int _gridHeight;

    cocos2d::Vec2 _startPixel;
    bool _gridVisible;

public:
    enum Corner { TOP_LEFT = 0, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER };

    static GridMap* create(const cocos2d::Size& mapSize, float tileSize);
    virtual bool init(const cocos2d::Size& mapSize, float tileSize);

    void setStartPixel(const cocos2d::Vec2& pixel);
    cocos2d::Vec2 getStartPixel() const;
    void setStartCorner(Corner corner);

    cocos2d::Vec2 getGridPosition(cocos2d::Vec2 worldPosition);
    cocos2d::Vec2 getPositionFromGrid(cocos2d::Vec2 gridPos);

    void updateBuildingBase(cocos2d::Vec2 gridPos, cocos2d::Size size, bool isValid);
    void hideBuildingBase();

    bool checkArea(cocos2d::Vec2 startGridPos, cocos2d::Size size);
    void showWholeGrid(bool visible, const cocos2d::Size& currentBuildingSize = cocos2d::Size::ZERO);
    void markArea(cocos2d::Vec2 startGridPos, cocos2d::Size size, bool occupied);

    // --- Pathfinding helpers ---
    inline int getGridWidth() const { return _gridWidth; }
    inline int getGridHeight() const { return _gridHeight; }
    inline float getTileSize() const { return _tileSize; }
    bool isBlocked(int x, int y) const;
};