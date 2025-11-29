#include "DraggableMapScene.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

USING_NS_CC;
using namespace ui;

Scene* DraggableMapScene::createScene()
{
    return DraggableMapScene::create();
}

bool DraggableMapScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    _isBuildingMode = false;
    _ghostSprite = nullptr;

    _visibleSize = Director::getInstance()->getVisibleSize();

    // 初始化缩放参数
    _currentScale = 1.3f;
    _minScale = 1.0f;
    _maxScale = 2.5f;

    // 初始化地图列表
    _mapNames = { "202501.png", "202502.png", "202503.png", "202504.png",
                 "202505.png", "202506.png", "202507.png", "202508.png" };
    _currentMapName = "202507.png"; // 默认地图

    _isMapListVisible = false;

    // 创建英雄管理器
    _heroManager = HeroManager::create();
    this->addChild(_heroManager);

    setupMap();
    setupUI();
    setupTouchListener();
    setupMouseListener();

    return true;
}

// 添加缺失的 setupMap 方法
void DraggableMapScene::setupMap()
{
    _mapSprite = Sprite::create(_currentMapName);
    if (_mapSprite) {
        auto mapSize = _mapSprite->getContentSize();
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        this->addChild(_mapSprite, 0);

        // --------------------------------------------------------
        // 【修改】这里传入小格子的尺寸！
        // 假设原本 100 是大格子，现在我们把大格子切成 3x3
        // 那么小格子大约是 33.3f
        // --------------------------------------------------------
        _gridMap = GridMap::create(mapSize, 33.3f);
        _mapSprite->addChild(_gridMap, 999);

        updateBoundary();
        createSampleMapElements();
    }
    else {
        CCLOG("Error: Failed to load map image %s", _currentMapName.c_str());

        auto errorLabel = Label::createWithSystemFont(
            "Failed to load " + _currentMapName, "Arial", 32);
        errorLabel->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        errorLabel->setTextColor(Color4B::RED);
        this->addChild(errorLabel);
    }

    // 添加背景色
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);
}

void DraggableMapScene::setupUI()
{
    auto buildBtn = Button::create();
    buildBtn->setTitleText("Build");
    buildBtn->setTitleFontSize(40);
    buildBtn->setContentSize(Size(80, 40));
    // 放在左下角
    buildBtn->setPosition(Vec2(1000, 1000));
    buildBtn->addClickEventListener([this](Ref* sender) {
        if (_isBuildingMode) {
            this->cancelPlacing();
        }
        else {
            this->startPlacingBuilding();
        }
        });
    this->addChild(buildBtn, 10);

    // 地图切换按钮
    auto buildBtn = Button::create();
    buildBtn->setTitleText("Build");
    buildBtn->setTitleFontSize(40);
    buildBtn->setContentSize(Size(80, 40));
    // 放在左下角
    buildBtn->setPosition(Vec2(1000, 1000));
    buildBtn->addClickEventListener([this](Ref* sender) {
        if (_isBuildingMode) {
            this->cancelPlacing();
        }
        else {
            this->startPlacingBuilding();
        }
        });
    this->addChild(buildBtn, 10);


    auto buildBtn = Button::create();
    buildBtn->setTitleText("Build");
    buildBtn->setTitleFontSize(40);
    buildBtn->setContentSize(Size(80, 40));
    // 放在左下角
    buildBtn->setPosition(Vec2(1000, 1000));
    buildBtn->addClickEventListener([this](Ref* sender) {
        if (_isBuildingMode) {
            this->cancelPlacing();
        }
        else {
            this->startPlacingBuilding();
        }
        });
    this->addChild(buildBtn, 10);

    // 地图切换按钮
    _mapButton = Button::create();
    _mapButton->setTitleText("Map");
    _mapButton->setTitleFontSize(24);
    _mapButton->setContentSize(Size(120, 60));  // 这里应该是 _mapButton，不是 _heroButton
    _mapButton->setPosition(Vec2(_visibleSize.width - 80, _visibleSize.height - 50));
    _mapButton->addClickEventListener(CC_CALLBACK_1(DraggableMapScene::onMapButtonClicked, this));
    this->addChild(_mapButton, 10);

    // 设置英雄UI
    _heroManager->setupHeroUI(this, _visibleSize);

    // 创建地图列表（初始隐藏）
    createMapList();

    // 操作提示
    auto tipLabel = Label::createWithSystemFont(
        "Drag: Move Map  Scroll: Zoom  Buttons: Switch Map/Hero\nClick Hero to Select, Click Ground to Move",
        "Arial", 14);
    tipLabel->setPosition(Vec2(_visibleSize.width / 2, 40));
    tipLabel->setTextColor(Color4B::YELLOW);
    tipLabel->setAlignment(TextHAlignment::CENTER);
    this->addChild(tipLabel, 10);

    // 当前地图名称显示
    auto mapNameLabel = Label::createWithSystemFont("Current: " + _currentMapName, "Arial", 18);
    mapNameLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height - 30));
    mapNameLabel->setTextColor(Color4B::GREEN);
    mapNameLabel->setName("mapNameLabel");
    this->addChild(mapNameLabel, 10);
}

void DraggableMapScene::createMapList()
{
    _mapList = ListView::create();
    _mapList->setContentSize(Size(150, 200));
    _mapList->setPosition(Vec2(_visibleSize.width - 160, _visibleSize.height - 240));
    _mapList->setBackGroundColor(Color3B(80, 80, 80));
    _mapList->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    _mapList->setOpacity(200);
    _mapList->setVisible(false);
    _mapList->setScrollBarEnabled(true);

    for (const auto& mapName : _mapNames) {
        auto item = Layout::create();
        item->setContentSize(Size(140, 40));
        item->setTouchEnabled(true);

        auto label = Label::createWithSystemFont(mapName, "Arial", 16);
        label->setPosition(Vec2(70, 20));
        label->setTextColor(Color4B::WHITE);
        label->setName("label");
        item->addChild(label);

        // 添加点击事件
        item->addClickEventListener([this, mapName](Ref* sender) {
            this->onMapItemClicked(sender);
            });

        _mapList->pushBackCustomItem(item);
    }

    this->addChild(_mapList, 20);
}

void DraggableMapScene::onMapButtonClicked(cocos2d::Ref* sender)
{
    toggleMapList();
}

void DraggableMapScene::toggleMapList()
{
    _isMapListVisible = !_isMapListVisible;
    _mapList->setVisible(_isMapListVisible);

    // 如果打开地图列表，关闭英雄列表
    if (_isMapListVisible && _heroManager->isHeroListVisible()) {
        _heroManager->hideHeroList();
    }
}

void DraggableMapScene::onMapItemClicked(cocos2d::Ref* sender)
{
    auto item = static_cast<Layout*>(sender);
    auto label = static_cast<Label*>(item->getChildByName("label"));
    std::string selectedMapName = label->getString();

    CCLOG("Selected map: %s", selectedMapName.c_str());

    // 切换地图
    switchMap(selectedMapName);

    // 隐藏地图列表
    toggleMapList();
}

void DraggableMapScene::switchMap(const std::string& mapName)
{
    if (mapName == _currentMapName) return;

    // ... (保存状态、移除旧地图代码保持不变) ...
    saveMapElementsState();

    if (_mapSprite) {
        this->removeChild(_mapSprite);
        _mapSprite = nullptr;
        _gridMap = nullptr; // 指针置空
    }

    // 3. 创建新地图
    _currentMapName = mapName;
    _mapSprite = Sprite::create(_currentMapName);

    if (_mapSprite) {
        // 设置位置和缩放
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        _mapSprite->setScale(_currentScale);
        this->addChild(_mapSprite, 0);

        // --------------------------------------------------------
        // 【新增代码】给新地图添加网格
        // --------------------------------------------------------
        auto mapSize = _mapSprite->getContentSize();
        _gridMap = GridMap::create(mapSize, 100.0f);
        _mapSprite->addChild(_gridMap, 999);
        // --------------------------------------------------------

        // 更新边界
        updateBoundary();

        // 恢复地图元素
        restoreMapElementsState();

        // 5. 通知英雄管理器地图已切换
        _heroManager->onMapSwitched(_mapSprite);
        _heroManager->updateHeroesScale(_currentScale);  // 立即更新英雄缩放

        // 6. 更新UI显示
        auto mapNameLabel = static_cast<Label*>(this->getChildByName("mapNameLabel"));
        if (mapNameLabel) {
            mapNameLabel->setString("Current: " + _currentMapName);
        }

        CCLOG("Map switched successfully to %s", mapName.c_str());
    }
    else {
        CCLOG("Error: Failed to load new map %s", mapName.c_str());

        // 恢复旧地图名称
        _currentMapName = "202507.png"; // 回退到默认地图
        _mapSprite = Sprite::create(_currentMapName);
        if (_mapSprite) {
            _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
            _mapSprite->setScale(_currentScale);
            this->addChild(_mapSprite, 0);
            updateBoundary();
            restoreMapElementsState();
        }
    }
}

void DraggableMapScene::saveMapElementsState()
{
    // 保存元素的本地坐标
    for (auto& element : _mapElements) {
        if (element.node) {
            element.localPosition = element.node->getPosition();
            element.node->retain(); // 保持引用，防止被自动释放
        }
    }
}

void DraggableMapScene::restoreMapElementsState()
{
    // 恢复地图元素到新地图上
    for (auto& element : _mapElements) {
        if (element.node && element.node->getParent() == nullptr) {
            _mapSprite->addChild(element.node, 1); // 添加到新地图上
            element.node->setPosition(element.localPosition);
        }
        element.node->release(); // 释放之前保留的引用
    }
}

void DraggableMapScene::createSampleMapElements()
{
    // 清除旧元素
    _mapElements.clear();

    if (!_mapSprite) return;

    // 创建一些示例元素（标记点等）
    auto createMarker = [this](const Vec2& worldPosition, const Color4B& color, const std::string& text) {
        // 将世界坐标转换为地图本地坐标
        Vec2 localPos = _mapSprite->convertToNodeSpace(worldPosition);

        // 标记点
        auto marker = DrawNode::create();
        marker->drawDot(Vec2::ZERO, 10, Color4F(color));
        marker->setPosition(localPos);
        _mapSprite->addChild(marker, 1);  // 添加到地图上，而不是场景上

        // 文字标签
        auto label = Label::createWithSystemFont(text, "Arial", 16);
        label->setPosition(localPos + Vec2(0, 20));
        label->setTextColor(Color4B::WHITE);
        _mapSprite->addChild(label, 1);  // 添加到地图上

        // 保存元素信息
        MapElement markerElement = { marker, localPos };
        MapElement labelElement = { label, localPos + Vec2(0, 20) };
        _mapElements.push_back(markerElement);
        _mapElements.push_back(labelElement);
        };

    // 在世界坐标中创建几个示例标记
    createMarker(Vec2(_visibleSize.width * 0.3f, _visibleSize.height * 0.7f), Color4B::RED, "Point A");
    createMarker(Vec2(_visibleSize.width * 0.7f, _visibleSize.height * 0.5f), Color4B::GREEN, "Point B");
    createMarker(Vec2(_visibleSize.width * 0.5f, _visibleSize.height * 0.3f), Color4B::BLUE, "Point C");

    CCLOG("Created %zd map elements", _mapElements.size());
}

void DraggableMapScene::updateMapElementsPosition()
{
    if (!_mapSprite) return;

    for (auto& element : _mapElements) {
        if (element.node && element.node->getParent() == _mapSprite) {
            // 保持相对于地图的本地位置不变
            element.node->setPosition(element.localPosition);
        }
    }
}

void DraggableMapScene::setupTouchListener()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

    // 使用传统方法绑定
    touchListener->onTouchBegan = CC_CALLBACK_2(DraggableMapScene::onTouchBegan, this);
    touchListener->onTouchMoved = CC_CALLBACK_2(DraggableMapScene::onTouchMoved, this);
    touchListener->onTouchEnded = CC_CALLBACK_2(DraggableMapScene::onTouchEnded, this);
    touchListener->onTouchCancelled = CC_CALLBACK_2(DraggableMapScene::onTouchCancelled, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

void DraggableMapScene::setupMouseListener()
{
    auto mouseListener = EventListenerMouse::create();

    mouseListener->onMouseScroll = [this](Event* event) {
        if (!_mapSprite) return;

        EventMouse* mouseEvent = dynamic_cast<EventMouse*>(event);
        if (mouseEvent) {
            float scrollY = mouseEvent->getScrollY();  // 获取滚轮增量

        // 计算缩放因子（滚轮向下为正，向上为负）
        float zoomFactor = 1.0f;
        if (scrollY < 0) {
            zoomFactor = 1.1f;
        }
        else if (scrollY > 0) {
            zoomFactor = 0.9f;
        }
        else {
            return;  // 没有滚动
        }

            // 获取鼠标当前位置作为缩放中心点
            Vec2 mousePos = Vec2(mouseEvent->getCursorX(), mouseEvent->getCursorY());

            // 执行缩放
            zoomMap(zoomFactor, mousePos);

            CCLOG("Mouse scroll: %.1f, Scale: %.2f", scrollY, _currentScale);
        }
        };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void DraggableMapScene::zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint)
{
    if (!_mapSprite) return;

    // 计算新缩放比例
    float newScale = _currentScale * scaleFactor;

    // 限制缩放范围
    newScale = MAX(_minScale, MIN(_maxScale, newScale));

    if (newScale == _currentScale) {
        return;  // 缩放比例没有变化
    }

    // 保存缩放前的状态
    Vec2 oldPosition = _mapSprite->getPosition();
    Vec2 oldAnchor = _mapSprite->getAnchorPoint();

    // 如果指定了中心点，计算基于该点的缩放
    if (pivotPoint != Vec2::ZERO) {
        // 将中心点转换到地图的本地坐标系
        Vec2 worldPos = pivotPoint;
        Vec2 localPos = _mapSprite->convertToNodeSpace(worldPos);

        // 计算缩放前后的位置变化
        Vec2 offsetBefore = localPos * _currentScale;
        Vec2 offsetAfter = localPos * newScale;
        Vec2 positionDelta = offsetAfter - offsetBefore;

        // 应用缩放和位置调整
        _mapSprite->setScale(newScale);
        _mapSprite->setPosition(oldPosition - positionDelta);
    }
    else {
        // 没有指定中心点，简单缩放
        _mapSprite->setScale(newScale);
    }

    // 更新当前缩放比例
    _currentScale = newScale;

    // 更新边界（缩放后边界会变化）
    updateBoundary();

    // 更新地图元素位置
    updateMapElementsPosition();

    // 更新英雄缩放
    _heroManager->updateHeroesScale(_currentScale);

    // 确保地图在边界内
    ensureMapInBoundary();
}

void DraggableMapScene::updateBoundary()
{
    if (!_mapSprite) return;

    auto mapSize = _mapSprite->getContentSize() * _currentScale;

    // 计算拖动边界
    float minX = _visibleSize.width - mapSize.width / 2;
    float maxX = mapSize.width / 2;
    float minY = _visibleSize.height - mapSize.height / 2;
    float maxY = mapSize.height / 2;

    // 如果地图比屏幕小，则应该居中不能拖动
    if (mapSize.width <= _visibleSize.width) {
        minX = maxX = _visibleSize.width / 2;
    }
    if (mapSize.height <= _visibleSize.height) {
        minY = maxY = _visibleSize.height / 2;
    }

    _mapBoundary = Rect(minX, minY, maxX - minX, maxY - minY);

    CCLOG("Boundary updated - Scale: %.2f, Boundary: minX=%.1f, maxX=%.1f",
        _currentScale, minX, maxX);
}

bool DraggableMapScene::onTouchBegan(Touch* touch, Event* event)
{
    _lastTouchPos = touch->getLocation();

    if (_isBuildingMode) {
        // 建造模式下，点击屏幕 = 尝试放置
        // 1. 将屏幕坐标转为地图内部坐标 (关键一步！)
        Vec2 mapPos = _mapSprite->convertToNodeSpace(touch->getLocation());
        // 2. 获取格子坐标
        Vec2 gridPos = _gridMap->getGridPosition(mapPos);

        // 3. 执行放置
        placeBuilding(gridPos);

        return true; // 吞噬触摸，不让它穿透到底下的逻辑
    }

    return true;
}

void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    Vec2 currentTouchPos = touch->getLocation();

    if (_isBuildingMode && _ghostSprite) {
        // 1. 获取小格子坐标
        Vec2 gridPos = _gridMap->getGridPosition(currentTouchPos);

        // 2. 假设这个塔是 3x3 的大建筑
        Size buildingSize = Size(3, 3);

        // 3. 冲突检测
        bool canBuild = _gridMap->checkArea(gridPos, buildingSize);

        // 4. 更新底座（现在会画一个覆盖 3x3 区域的大菱形）
        _gridMap->updateBuildingBase(gridPos, buildingSize, canBuild);

        // 5. 更新幻影位置
        // 注意：BuildingBase 是画在格子的正中间，
        // 但如果我们的锚点是 (0.5, 0.5)，sprite 应该位于这个 3x3 区域的中心。
        // getPositionFromGrid 返回的是 (x,y) 这个单一小格子的中心。
        // 这里的中心点计算稍微复杂一点：

        Vec2 centerGridPos = gridPos + Vec2(1.0f, 1.0f); // 3x3 的中心是偏移 1,1 (0,1,2 中间是 1)
        // 或者更精确地，计算几何中心：
        // Vec2 p1 = getPositionFromGrid(gridPos);
        // Vec2 p2 = getPositionFromGrid(gridPos + Vec2(2,2));
        // Vec2 visualCenter = (p1 + p2) / 2; 

        // 简单起见，我们对齐到 gridPos 这个小格子的位置，
        // 但因为塔很大，看起来可能会偏。
        // 完美的做法是计算 3x3 区域的像素中心：
        Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
        Vec2 posEnd = _gridMap->getPositionFromGrid(gridPos + Vec2(2, 2));
        Vec2 centerPos = (posStart + posEnd) / 2.0f;

        _ghostSprite->setPosition(centerPos);

        // 保存状态供 Ended 使用
        _ghostSprite->setUserData((void*)(canBuild ? 1 : 0)); // 简单hack存状态

        return;
    }

    // 普通拖拽地图逻辑
    Vec2 delta = currentTouchPos - _lastTouchPos;
    moveMap(delta);
    _lastTouchPos = currentTouchPos;
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event)
{
    if (_isBuildingMode) {
        Vec2 touchPos = touch->getLocation();
        Vec2 gridPos = _gridMap->getGridPosition(touchPos);
        Size buildingSize = Size(3, 3); // 定义建筑大小

        // 再次检查（防止多点触控或其他边缘情况）
        if (_gridMap->checkArea(gridPos, buildingSize)) {
            placeBuilding(gridPos);
        }
        else {
            // 播放一个错误音效或红色闪烁
            CCLOG("Cannot build here!");
        }
    }
}

void DraggableMapScene::onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event)
{
    // 触摸取消处理
    this->onTouchEnded(touch, event);
}

void DraggableMapScene::moveMap(const cocos2d::Vec2& delta)
{
    if (!_mapSprite) return;

    _mapSprite->setPosition(_mapSprite->getPosition() + delta);
    ensureMapInBoundary();
}

void DraggableMapScene::ensureMapInBoundary()
{
    if (!_mapSprite) return;

    cocos2d::Vec2 currentPos = _mapSprite->getPosition();
    cocos2d::Vec2 newPos = currentPos;

    if (currentPos.x < _mapBoundary.getMinX()) {
        newPos.x = _mapBoundary.getMinX();
    }
    else if (currentPos.x > _mapBoundary.getMaxX()) {
        newPos.x = _mapBoundary.getMaxX();
    }

    if (currentPos.y < _mapBoundary.getMinY()) {
        newPos.y = _mapBoundary.getMinY();
    }
    else if (currentPos.y > _mapBoundary.getMaxY()) {
        newPos.y = _mapBoundary.getMaxY();
    }

    if (newPos != currentPos) {
        _mapSprite->setPosition(newPos);
    }
}

void DraggableMapScene::startPlacingBuilding()
{
    if (!_mapSprite || !_gridMap) return;

    _isBuildingMode = true;

    // 1. 【新增】开启全屏网格显示
    _gridMap->showWholeGrid(true);

    // 1. 创建幻影 (Tower.png)
    _ghostSprite = Sprite::create("Tower.png");
    if (_ghostSprite) {
        _ghostSprite->setOpacity(150); // 半透明

        // 【关键】设置锚点 (0.5, 0.2) 让脚底对齐网格中心
        _ghostSprite->setAnchorPoint(Vec2(0.5f, 0.15f));

        // 加到地图上！这样缩放地图时，幻影大小也会跟着变
        _mapSprite->addChild(_ghostSprite, 2000);

        // 2. 添加攻击范围圈 (画在幻影内部)
        auto rangeCircle = DrawNode::create();
        rangeCircle->drawDot(Vec2(_ghostSprite->getContentSize().width / 2,
            _ghostSprite->getContentSize().height * 0.2f), // 圆心在脚底
            300, Color4F(1, 1, 1, 0.2f)); // 半径300
        _ghostSprite->addChild(rangeCircle, -1);
    }
}

void DraggableMapScene::placeBuilding(Vec2 gridPos)
{
    if (!_ghostSprite && !_isBuildingMode) return;

    Size buildingSize = Size(3, 3);

    // 1. 标记占用
    _gridMap->markArea(gridPos, buildingSize, true);

    auto building = Sprite::create("Tower.png");
    building->setAnchorPoint(Vec2(0.5f, 0.2f));

    // 计算 3x3 区域中心
    Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
    Vec2 posEnd = _gridMap->getPositionFromGrid(gridPos + Vec2(2, 2));
    Vec2 centerPos = (posStart + posEnd) / 2.0f;

    building->setPosition(centerPos);

    // Z-Order: 基于 ISO Y 轴排序。通常使用最下方的点的 Y
    building->setLocalZOrder(10000 - centerPos.y);

    _mapSprite->addChild(building);

    // 动画
    building->setScale(0.0f);
    building->runAction(EaseBackOut::create(ScaleTo::create(0.4f, 1.0f)));

    endPlacing();
}

void DraggableMapScene::cancelPlacing()
{
    // 直接统一退出建造模式
    endPlacing();
}

// 统一结束建造，确保所有清理都在同一处执行
void DraggableMapScene::endPlacing()
{
    _isBuildingMode = false;

    if (_gridMap) {
        _gridMap->showWholeGrid(false);
        _gridMap->hideBuildingBase();
    }

    if (_ghostSprite) {
        _ghostSprite->removeFromParent();
        _ghostSprite = nullptr;
    }
}