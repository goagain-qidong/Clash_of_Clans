#include "GridMap.h"

USING_NS_CC;

GridMap* GridMap::create(const Size& mapSize, float tileSize)
{
    GridMap* ret = new (std::nothrow) GridMap();
    if (ret && ret->init(mapSize, tileSize))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GridMap::init(const Size& mapSize, float tileSize)
{
    if (!Node::init())
        return false;
    _mapSize = mapSize;
    _tileSize = tileSize;

    _gridNode = DrawNode::create();
    this->addChild(_gridNode, 1);

    _baseNode = DrawNode::create();
    this->addChild(_baseNode, 2);

    // 长边与短边相同
    _gridWidth = static_cast<int>(round(mapSize.width / tileSize)) - 1;
    _gridHeight = _gridWidth;

    _collisionMap.resize(_gridWidth, std::vector<bool>(_gridHeight, false));

    _startPixel = Vec2(_mapSize.width / 2.0f, _mapSize.height + 30.0f - _tileSize * 0.5f);
    _gridVisible = false;

    return true;
}

Vec2 GridMap::getPositionFromGrid(Vec2 gridPos)
{
    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    float x = (gridPos.x - gridPos.y) * halfW + _startPixel.x;
    float y = _startPixel.y - (gridPos.x + gridPos.y) * halfH;

    return Vec2(x, y);
}

Vec2 GridMap::getGridPosition(Vec2 worldPosition)
{
    Vec2 localPos = this->convertToNodeSpace(worldPosition);

    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    float dx = localPos.x - _startPixel.x;
    float dy = _startPixel.y - localPos.y;

    float x = (dy / halfH + dx / halfW) / 2.0f;
    float y = (dy / halfH - dx / halfW) / 2.0f;

    int gridX = static_cast<int>(round(x));
    int gridY = static_cast<int>(round(y));

    gridX = MAX(0, MIN(_gridWidth - 1, gridX));
    gridY = MAX(0, MIN(_gridHeight - 1, gridY));

    return Vec2(gridX, gridY);
}

void GridMap::showWholeGrid(bool visible, const cocos2d::Size& currentBuildingSize)
{
    _gridVisible = visible;
    _gridNode->clear();
    if (!visible)
        return;

    // 根据建筑尺寸确定大网格步长，默认为3x3
    int bigGridStep = 3;
    if (currentBuildingSize.width > 0 && currentBuildingSize.height > 0)
    {
        bigGridStep = static_cast<int>(currentBuildingSize.width);
    }

    // 定义网格颜色：小网格填充、小网格边线、大网格边线
    Color4F smallGridColor = Color4F(1.0f, 1.0f, 1.0f, 0.03f);
    Color4F smallGridLineColor = Color4F(1.0f, 1.0f, 1.0f, 0.15f);
    Color4F bigGridLineColor = Color4F(1.0f, 1.0f, 1.0f, 0.35f);

    // 计算菱形网格的半宽和半高（0.79是isometric视角的比例系数）
    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.79f;

    int maxX = _gridWidth;
    int maxY = _gridHeight;

    // 第一步：绘制所有小网格
    for (int x = 0; x < maxX; x++)
    {
        for (int y = 0; y < maxY; y++)
        {
            Vec2 center = getPositionFromGrid(Vec2(static_cast<float>(x), static_cast<float>(y)));

            // 计算菱形的四个顶点：上、右、下、左
            Vec2 p[4];
            p[0] = Vec2(center.x, center.y + halfH);
            p[1] = Vec2(center.x + halfW, center.y);
            p[2] = Vec2(center.x, center.y - halfH);
            p[3] = Vec2(center.x - halfW, center.y);

            // 先填充菱形，再绘制边线
            _gridNode->drawSolidPoly(p, 4, smallGridColor);
            _gridNode->drawPoly(p, 4, true, smallGridLineColor);
        }
    }

    // 第二步：绘制大网格边框（用于标识建筑占用区域）
    for (int x = 0; x < maxX; x += bigGridStep)
    {
        for (int y = 0; y < maxY; y += bigGridStep)
        {
            // 处理边界情况：如果剩余网格不足一个完整的bigGridStep
            int currentW = (x + bigGridStep > maxX) ? (maxX - x) : bigGridStep;
            int currentH = (y + bigGridStep > maxY) ? (maxY - y) : bigGridStep;

            if (currentW <= 0 || currentH <= 0)
                continue;

            // 获取大网格区域的四个角对应的网格中心点
            Vec2 topGridCenter = getPositionFromGrid(Vec2(static_cast<float>(x), static_cast<float>(y)));
            Vec2 rightGridCenter = getPositionFromGrid(Vec2(static_cast<float>(x + currentW - 1), static_cast<float>(y)));
            Vec2 bottomGridCenter = getPositionFromGrid(Vec2(static_cast<float>(x + currentW - 1), static_cast<float>(y + currentH - 1)));
            Vec2 leftGridCenter = getPositionFromGrid(Vec2(static_cast<float>(x), static_cast<float>(y + currentH - 1)));

            // 连接四个角的外边缘顶点，形成大网格边框
            Vec2 p[4];
            p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);
            p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y);
            p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH);
            p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);

            _gridNode->drawPoly(p, 4, true, bigGridLineColor);
        }
    }
}

void GridMap::updateBuildingBase(Vec2 gridPos, Size size, bool isValid)
{
    _baseNode->clear();

    float halfW = _tileSize / 2.0f;
    float halfH = halfW * 0.75f;

    Color4F color;
    Color4F borderColor;

    if (isValid)
    {
        color = Color4F(0.0f, 1.0f, 0.0f, 0.3f);
        borderColor = Color4F(0.0f, 1.0f, 0.0f, 0.8f);
    }
    else
    {
        color = Color4F(1.0f, 0.0f, 0.0f, 0.3f);
        borderColor = Color4F(1.0f, 0.0f, 0.0f, 0.8f);
    }

    // 循环绘制建筑底下的每一个网格，只显示建筑脚下的网格
    for (int i = 0; i < static_cast<int>(size.width); i++)
    {
        for (int j = 0; j < static_cast<int>(size.height); j++)
        {
            Vec2 currentGridPos = gridPos + Vec2(static_cast<float>(i), static_cast<float>(j));
            Vec2 center = getPositionFromGrid(currentGridPos);

            Vec2 p[4];
            p[0] = Vec2(center.x, center.y + halfH);
            p[1] = Vec2(center.x + halfW, center.y);
            p[2] = Vec2(center.x, center.y - halfH);
            p[3] = Vec2(center.x - halfW, center.y);

            _baseNode->drawSolidPoly(p, 4, color);
            _baseNode->drawPoly(p, 4, true, borderColor);
        }
    }
}

void GridMap::hideBuildingBase()
{
    _baseNode->clear();
}

bool GridMap::checkArea(Vec2 startGridPos, Size size)
{
    int startX = static_cast<int>(startGridPos.x);
    int startY = static_cast<int>(startGridPos.y);
    int w = static_cast<int>(size.width);
    int h = static_cast<int>(size.height);

    if (startX < 0 || startY < 0 || startX + w > _gridWidth || startY + h > _gridHeight)
    {
        return false;
    }

    for (int x = startX; x < startX + w; x++)
    {
        for (int y = startY; y < startY + h; y++)
        {
            if (_collisionMap[x][y])
            {
                return false;
            }
        }
    }
    return true;
}



void GridMap::setStartPixel(const Vec2& pixel)
{
    _startPixel = pixel;
    if (_gridVisible)
        showWholeGrid(true);
}

Vec2 GridMap::getStartPixel() const
{
    return _startPixel;
}

void GridMap::setStartCorner(GridMap::Corner corner)
{
    Vec2 p;
    switch (corner)
    {
        case TOP_LEFT:
            p = Vec2(0 + _tileSize / 2.0f, _mapSize.height - _tileSize / 2.0f);
            break;
        case TOP_RIGHT:
            p = Vec2(_mapSize.width - _tileSize / 2.0f, _mapSize.height - _tileSize / 2.0f);
            break;
        case BOTTOM_LEFT:
            p = Vec2(0 + _tileSize / 2.0f, 0 + _tileSize / 2.0f);
            break;
        case BOTTOM_RIGHT:
            p = Vec2(_mapSize.width - _tileSize / 2.0f, 0 + _tileSize / 2.0f);
            break;
        case CENTER:
        default:
            p = Vec2(_mapSize.width / 2.0f, _mapSize.height / 2.0f);
            break;
    }

    p.y += _tileSize * 0.5f;

    setStartPixel(p);
}

bool GridMap::isBlocked(int x, int y) const
{
    if (x < 0 || y < 0 || x >= _gridWidth || y >= _gridHeight)
        return true;
    return _collisionMap[x][y];
}
void GridMap::markArea(cocos2d::Vec2 startGridPos, cocos2d::Size size, bool occupied)
{
    int startX = static_cast<int>(startGridPos.x);
    int startY = static_cast<int>(startGridPos.y);
    int w = static_cast<int>(size.width);
    int h = static_cast<int>(size.height);

    // 遍历建筑覆盖的每一个格子
    for (int x = startX; x < startX + w; x++)
    {
        for (int y = startY; y < startY + h; y++)
        {
            // 边界检查，防止数组越界
            if (x >= 0 && x < _gridWidth && y >= 0 && y < _gridHeight)
            {
                // _collisionMap 是你在 GridMap 中存储 true/false 的二维数组
                _collisionMap[x][y] = occupied;

                // 可选：如果是调试模式，把格子变红方便观察
                // if (occupied) drawDebugRect(x, y, Color4F::RED);
            }
        }
    }
}