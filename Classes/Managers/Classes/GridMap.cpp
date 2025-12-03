#include "GridMap.h"



USING_NS_CC;



GridMap* GridMap::create(const Size& mapSize, float tileSize)

{

    GridMap* ret = new (std::nothrow) GridMap();

    if (ret && ret->init(mapSize, tileSize)) {

        ret->autorelease();

        return ret;

    }

    delete ret;

    return nullptr;

}



bool GridMap::init(const Size& mapSize, float tileSize)

{

    if (!Node::init()) return false;

    _mapSize = mapSize;

    _tileSize = tileSize;



    _gridNode = DrawNode::create();

    this->addChild(_gridNode, 1);



    _baseNode = DrawNode::create();

    this->addChild(_baseNode, 2);



    // 初始化冲突地图

    _gridWidth = mapSize.width / tileSize; // 实际上应该根据地图形状细算，这里简化处理

    _gridHeight = mapSize.height / tileSize * 2; // ISO地图Y轴比较特殊，预留多一点空间防止越界



    // 初始化二维数组，全部为 false (空)

    _collisionMap.resize(_gridWidth, std::vector<bool>(_gridHeight, false));



    return true;

}



Vec2 GridMap::getPositionFromGrid(Vec2 gridPos)

{

    float halfW = _tileSize / 2.0f;

    float halfH = halfW * 0.7f;



    // 标准 ISO 公式 (上个回答修正后的版本)

    // 加上偏移量让 (0,0) 在地图上方中心

    float x = (gridPos.x - gridPos.y) * halfW + _mapSize.width / 2.0f;

    float y = _mapSize.height - (gridPos.x + gridPos.y) * halfH - halfH; // 这里的微调视具体贴图而定



    return Vec2(x, y);

}



Vec2 GridMap::getGridPosition(Vec2 worldPosition)

{

    Vec2 localPos = this->convertToNodeSpace(worldPosition);



    float halfW = _tileSize / 2.0f;

    float halfH = halfW * 0.7f;



    float dx = localPos.x - _mapSize.width / 2.0f;

    float dy = _mapSize.height - localPos.y - halfH; // 对应上面的Y轴反转逻辑



    float x = (dy / halfH + dx / halfW) / 2.0f;

    float y = (dy / halfH - dx / halfW) / 2.0f;



    return Vec2(round(x), round(y));

}



void GridMap::showWholeGrid(bool visible)

{

    _gridNode->clear();

    if (!visible) return;



    // --- 配置参数 ---

    int bigGridStep = 3; // 每 3x3 算一个大格



    // 颜色配置

    Color4F smallGridColor = Color4F(1.0f, 1.0f, 1.0f, 0.05f); // 小格子底色：极淡，几乎看不见，只作为底纹

    Color4F smallGridLineColor = Color4F(1.0f, 1.0f, 1.0f, 0.15f); // 小格子线：灰色，半透明

    Color4F bigGridLineColor = Color4F(1.0f, 1.0f, 1.0f, 0.35f); // 大格子线：亮白色，半透明



    // 基础尺寸计算

    float halfW = _tileSize / 2.0f;

    float halfH = halfW * 0.7f;



    // 假设地图最大网格数 (你可以存为成员变量 _maxGridX, _maxGridY)

    // 这里暂时用估算值，或者使用 init 里算的 _gridWidth/_gridHeight

    int maxX = _gridWidth;

    int maxY = _gridHeight;



    // ===========================================================

    // 第一步：画所有小网格 (作为底色填充)

    // ===========================================================

    for (int x = 0; x < maxX; x++) {

        for (int y = 0; y < maxY; y++) {

            Vec2 center = getPositionFromGrid(Vec2(x, y));



            Vec2 p[4];

            p[0] = Vec2(center.x, center.y + halfH);

            p[1] = Vec2(center.x + halfW, center.y);

            p[2] = Vec2(center.x, center.y - halfH);

            p[3] = Vec2(center.x - halfW, center.y);



            // 使用 drawSolidPoly 填充淡色，让地面看起来有质感

            _gridNode->drawSolidPoly(p, 4, smallGridColor);

            // 画小网格的“边界线”

            _gridNode->drawPoly(p, 4, true, smallGridLineColor);

        }

    }



    // ===========================================================

    // 第二步：单独画大网格的“边界线”

    // ===========================================================

    // 注意步长是 bigGridStep (3)

    for (int x = 0; x < maxX; x += bigGridStep) {

        for (int y = 0; y < maxY; y += bigGridStep) {



            // 我们要画一个囊括了 3x3 区域的大菱形框

            // 逻辑和 updateBuildingBase 一模一样



            // 1. 确定这个大块的有效宽/高 (处理边缘不足 3 格的情况)

            int currentW = (x + bigGridStep > maxX) ? (maxX - x) : bigGridStep;

            int currentH = (y + bigGridStep > maxY) ? (maxY - y) : bigGridStep;



            if (currentW <= 0 || currentH <= 0) continue;



            // 2. 计算四个关键顶点（包裹住这块区域）

            // Top:    (x, y) 的上顶点

            // Right:  (x + w - 1, y) 的右顶点

            // Bottom: (x + w - 1, y + h - 1) 的下顶点

            // Left:   (x, y + h - 1) 的左顶点



            Vec2 topGridCenter = getPositionFromGrid(Vec2(x, y));

            Vec2 rightGridCenter = getPositionFromGrid(Vec2(x + currentW - 1, y));

            Vec2 bottomGridCenter = getPositionFromGrid(Vec2(x + currentW - 1, y + currentH - 1));

            Vec2 leftGridCenter = getPositionFromGrid(Vec2(x, y + currentH - 1));



            Vec2 p[4];

            p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);       // 顶尖

            p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y);   // 右尖

            p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH); // 底尖

            p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);     // 左尖



            // 3. 画空心线框

            _gridNode->drawPoly(p, 4, true, bigGridLineColor);



            // 可选：如果你想让大网格的四个角有个小圆点装饰（更有科技感）

            // _gridNode->drawDot(p[0], 2, bigGridLineColor); 

        }

    }

}



void GridMap::updateBuildingBase(Vec2 gridPos, Size size, bool isValid)

{

    _baseNode->clear();



    // 1. 计算 3x3 (或 NxM) 区域形成的大菱形的四个顶点

    // ISO 坐标系下，一个矩形区域依然呈现为菱形



    // Top 顶点：对应 gridPos (假设 gridPos 是最上面的角/或者左上索引)

    // 让我们定义 gridPos 为该区域 X最小、Y最小 的格子 (ISO的 Top-Left 逻辑)



    // 我们需要四个关键点的“屏幕坐标”：

    // A: (x, y) 的 Top 点

    // B: (x + w, y) 的 Right 点

    // C: (x + w, y + h) 的 Bottom 点

    // D: (x, y + h) 的 Left 点



    // 注意：getPositionFromGrid 返回的是格子的“中心点”

    // 为了画包围盒，我们需要精细控制到格子的边缘



    float halfW = _tileSize / 2.0f;

    float halfH = halfW * 0.7f;



    // 获取四个角格子的中心点

    Vec2 topGridCenter = getPositionFromGrid(gridPos); // (x, y)

    Vec2 rightGridCenter = getPositionFromGrid(gridPos + Vec2(size.width - 1, 0)); // (x + w -1, y)

    Vec2 bottomGridCenter = getPositionFromGrid(gridPos + Vec2(size.width - 1, size.height - 1)); // (x+w-1, y+h-1)

    Vec2 leftGridCenter = getPositionFromGrid(gridPos + Vec2(0, size.height - 1)); // (x, y+h-1)



    // 向外延伸半个格子的距离以包裹边缘

    Vec2 p[4];

    p[0] = Vec2(topGridCenter.x, topGridCenter.y + halfH);     // 顶端

    p[1] = Vec2(rightGridCenter.x + halfW, rightGridCenter.y); // 右端

    p[2] = Vec2(bottomGridCenter.x, bottomGridCenter.y - halfH); // 底端

    p[3] = Vec2(leftGridCenter.x - halfW, leftGridCenter.y);   // 左端



    // 颜色配置

    Color4F color = isValid ? Color4F(0.0f, 1.0f, 0.0f, 0.5f) : Color4F(1.0f, 0.0f, 0.0f, 0.5f);

    Color4F borderColor = isValid ? Color4F::GREEN : Color4F::RED;



    _baseNode->drawSolidPoly(p, 4, color);

    _baseNode->drawPoly(p, 4, true, borderColor);

}



void GridMap::hideBuildingBase()

{

    _baseNode->clear();

}



bool GridMap::checkArea(Vec2 startGridPos, Size size)

{

    int startX = (int)startGridPos.x;

    int startY = (int)startGridPos.y;

    int w = (int)size.width;

    int h = (int)size.height;



    // 越界检查

    if (startX < 0 || startY < 0 || startX + w > _gridWidth || startY + h > _gridHeight) {

        return false;

    }



    // 遍历区域内所有小方格

    for (int x = startX; x < startX + w; x++) {

        for (int y = startY; y < startY + h; y++) {

            // 如果任何一个格子是 true (被占用)，则返回 false (不可造)

            if (_collisionMap[x][y]) {

                return false;

            }

        }

    }

    return true; // 所有格子都空闲

}



void GridMap::markArea(Vec2 startGridPos, Size size, bool occupied)

{

    int startX = (int)startGridPos.x;

    int startY = (int)startGridPos.y;

    int w = (int)size.width;

    int h = (int)size.height;



    if (startX < 0 || startY < 0 || startX + w > _gridWidth || startY + h > _gridHeight) return;



    for (int x = startX; x < startX + w; x++) {

        for (int y = startY; y < startY + h; y++) {

            _collisionMap[x][y] = occupied;

        }

    }

}