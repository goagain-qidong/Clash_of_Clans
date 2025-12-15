#include "PathFinder.h"
#include <algorithm>
#include <cmath>
#include <queue>

USING_NS_CC;

PathFinder& PathFinder::getInstance()
{
    static PathFinder instance;
    return instance;
}

int PathFinder::getDistance(const PathNode* nodeA, const PathNode* nodeB)
{
    // 切比雪夫距离 (适合8方向) 或 欧几里得距离估算
    int dstX = std::abs(nodeA->x - nodeB->x);
    int dstY = std::abs(nodeA->y - nodeB->y);

    // 对角线移动优化：min(dx, dy) 步走斜线(14)，剩余走直线(10)
    if (dstX > dstY)
        return 14 * dstY + 10 * (dstX - dstY);
    return 14 * dstX + 10 * (dstY - dstX);
}

bool PathFinder::isValid(int x, int y, int width, int height)
{
    return x >= 0 && x < width && y >= 0 && y < height;
}

// 射线检测：检查两点之间是否有阻挡
bool PathFinder::hasLineOfSight(GridMap* gridMap, const Vec2& start, const Vec2& end, bool ignoreWalls)
{
    if (ignoreWalls)
        return true; // 炸弹人无视阻挡

    float dist = start.distance(end);
    if (dist < 1.0f)
        return true;

    Vec2 dir = (end - start).getNormalized();

    // 步长设为格子的一半，防止漏检
    float stepSize = gridMap->getTileSize() * 0.5f;
    int   steps    = static_cast<int>(dist / stepSize);

    for (int i = 1; i < steps; ++i)
    { // 从1开始，跳过起点
        Vec2 checkPos = start + dir * (i * stepSize);
        Vec2 gridPos  = gridMap->getGridPosition(checkPos);

        // 如果中间有点被阻挡，则视线不通
        if (gridMap->isBlocked((int)gridPos.x, (int)gridPos.y))
        {
            return false;
        }
    }
    return true;
}

// 路径平滑算法
std::vector<Vec2> PathFinder::smoothPath(GridMap* gridMap, const std::vector<Vec2>& rawPath, bool ignoreWalls)
{
    if (rawPath.size() <= 2)
        return rawPath;

    std::vector<Vec2> smoothedPath;
    smoothedPath.push_back(rawPath[0]); // 起点肯定要

    int currentIdx = 0;
    while (currentIdx < rawPath.size() - 1)
    {
        // 贪婪尝试：从当前点尽可能往后找，看能直达的最远点是哪个
        int nextIdx = currentIdx + 1;

        // 往后遍历，直到找不到直达路径或者到达终点
        for (int i = rawPath.size() - 1; i > currentIdx + 1; --i)
        {
            if (hasLineOfSight(gridMap, rawPath[currentIdx], rawPath[i], ignoreWalls))
            {
                nextIdx = i;
                break; // 找到了最远的直达点
            }
        }

        smoothedPath.push_back(rawPath[nextIdx]);
        currentIdx = nextIdx;
    }

    return smoothedPath;
}

std::vector<Vec2> PathFinder::findPath(GridMap* gridMap, const Vec2& startWorldUnit, const Vec2& endWorldTarget,
                                       bool ignoreWalls)
{
    std::vector<Vec2> path;
    if (!gridMap)
        return path;

    Vec2 startGrid = gridMap->getGridPosition(startWorldUnit);
    Vec2 endGrid   = gridMap->getGridPosition(endWorldTarget);

    int width  = gridMap->getGridWidth();
    int height = gridMap->getGridHeight();

    if (!isValid((int)startGrid.x, (int)startGrid.y, width, height) ||
        !isValid((int)endGrid.x, (int)endGrid.y, width, height))
    {
        return path;
    }

    auto cmp = [](PathNode* a, PathNode* b) { return a->fCost() > b->fCost(); };
    std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(cmp)> openSet(cmp);

    std::vector<std::vector<bool>>      closedSet(width, std::vector<bool>(height, false));
    std::vector<std::vector<PathNode*>> allNodes(width, std::vector<PathNode*>(height, nullptr));

    PathNode* startNode                          = new PathNode((int)startGrid.x, (int)startGrid.y);
    allNodes[(int)startGrid.x][(int)startGrid.y] = startNode;
    openSet.push(startNode);

    bool      pathFound  = false;
    PathNode* targetNode = nullptr;

    while (!openSet.empty())
    {
        PathNode* currentNode = openSet.top();
        openSet.pop();

        if (closedSet[currentNode->x][currentNode->y])
            continue;
        closedSet[currentNode->x][currentNode->y] = true;

        if (currentNode->x == (int)endGrid.x && currentNode->y == (int)endGrid.y)
        {
            pathFound  = true;
            targetNode = currentNode;
            break;
        }

        // 🆕 8方向移动：上下左右 + 对角线
        int dx[]    = {0, 1, 0, -1, 1, 1, -1, -1};
        int dy[]    = {1, 0, -1, 0, 1, -1, 1, -1};
        int costs[] = {10, 10, 10, 10, 14, 14, 14, 14}; // 直线10，斜线14

        for (int i = 0; i < 8; i++)
        {
            int nx = currentNode->x + dx[i];
            int ny = currentNode->y + dy[i];

            if (!isValid(nx, ny, width, height) || closedSet[nx][ny])
                continue;

            // 碰撞检测
            bool isTargetPos = (nx == (int)endGrid.x && ny == (int)endGrid.y);
            if (!ignoreWalls && !isTargetPos && gridMap->isBlocked(nx, ny))
            {
                // 对角线移动时的额外检查：防止“穿墙角”
                // 如果是斜走(i>=4)，且两个相邻的直线格子都是墙，则不能穿过
                if (i >= 4)
                {
                    if (gridMap->isBlocked(currentNode->x + dx[i], currentNode->y) ||
                        gridMap->isBlocked(currentNode->x, currentNode->y + dy[i]))
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }

            int       newCost  = currentNode->gCost + costs[i];
            PathNode* neighbor = allNodes[nx][ny];

            if (neighbor == nullptr)
            {
                neighbor         = new PathNode(nx, ny);
                allNodes[nx][ny] = neighbor;
                neighbor->gCost  = newCost;
                neighbor->hCost  = getDistance(neighbor, new PathNode((int)endGrid.x, (int)endGrid.y));
                neighbor->parent = currentNode;
                openSet.push(neighbor);
            }
            else if (newCost < neighbor->gCost)
            {
                neighbor->gCost  = newCost;
                neighbor->parent = currentNode;
                openSet.push(neighbor);
            }
        }
    }

    if (pathFound && targetNode)
    {
        PathNode*         current = targetNode;
        std::vector<Vec2> rawPath;
        while (current != nullptr)
        {
            rawPath.push_back(gridMap->getPositionFromGrid(Vec2(current->x, current->y)));
            current = current->parent;
        }
        std::reverse(rawPath.begin(), rawPath.end());

        // 🆕 关键：执行路径平滑
        // 如果单位在地图外，rawPath[0] 是边界点。
        // smoothPath 会检查 "startWorldUnit" 到 "rawPath[i]" 的连线
        // 所以我们需要把 startWorldUnit 插到路径最前面，再平滑
        if (!rawPath.empty())
        {
            std::vector<Vec2> fullPath;
            fullPath.push_back(startWorldUnit); // 真正的起点
            fullPath.insert(fullPath.end(), rawPath.begin(), rawPath.end());

            path = smoothPath(gridMap, fullPath, ignoreWalls);
        }
    }

    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            if (allNodes[i][j])
                delete allNodes[i][j];
        }
    }

    return path;
}