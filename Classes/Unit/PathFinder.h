#pragma once
#ifndef __PATH_FINDER_H__
#define __PATH_FINDER_H__

#include "GridMap.h"
#include "cocos2d.h"
#include <vector>

struct PathNode
{
    int       x, y;
    int       gCost;
    int       hCost;
    PathNode* parent;

    int  fCost() const { return gCost + hCost; }
    bool operator>(const PathNode& other) const { return fCost() > other.fCost(); }
    PathNode(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {}
};

class PathFinder
{
public:
    static PathFinder& getInstance();

    /**
     * @brief 核心 A* 寻路函数 (支持8方向+平滑处理)
     */
    std::vector<cocos2d::Vec2> findPath(GridMap* gridMap, const cocos2d::Vec2& startWorldUnit,
                                        const cocos2d::Vec2& endWorldTarget, bool ignoreWalls = false);

private:
    int  getDistance(const PathNode* nodeA, const PathNode* nodeB);
    bool isValid(int x, int y, int width, int height);

    // 🆕 检查两点之间是否有视线（无障碍）
    bool hasLineOfSight(GridMap* gridMap, const cocos2d::Vec2& start, const cocos2d::Vec2& end, bool ignoreWalls);

    // 🆕 路径平滑（弗洛伊德路径平滑算法简化版）
    std::vector<cocos2d::Vec2> smoothPath(GridMap* gridMap, const std::vector<cocos2d::Vec2>& rawPath,
                                          bool ignoreWalls);

    PathFinder()                             = default;
    ~PathFinder()                            = default;
    PathFinder(const PathFinder&)            = delete;
    PathFinder& operator=(const PathFinder&) = delete;
};

#endif // __PATH_FINDER_H__