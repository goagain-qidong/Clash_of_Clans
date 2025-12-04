// DraggableMapScene.cpp - 完整版本
#include "DraggableMapScene.h"

#include "BattleScene.h"
#include "Managers/AccountManager.h"
#include "Managers/SocketClient.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Managers/ResourceManager.h"

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

    _visibleSize = Director::getInstance()->getVisibleSize();
    _currentScale = 1.3f;
    _minScale = 0.7f;
    _maxScale = 2.5f;

    _mapNames = { "map/Map1.png", "map/Map2.png", "map/Map3.png" };
    _currentMapName = "map/Map1.png";
    _mapSprite = nullptr;
    _gridMap = nullptr;
    _lastTouchPos = Vec2::ZERO;
    _dragStartPos = Vec2::ZERO;

    _isBuildingMode = false;
    _isDraggingBuilding = false;
    _ghostSprite = nullptr;
    _selectedBuilding = BuildingData();
    _isWaitingConfirm = false;
    _pendingGridPos = Vec2::ZERO;

    _buildButton = nullptr;
    _mapButton = nullptr;
    _buildingListUI = nullptr;
    _mapList = nullptr;
    _isBuildingListVisible = false;
    _isMapListVisible = false;

    _confirmButton = nullptr;
    _cancelButton = nullptr;

    _heroManager = nullptr;

    // 初始化大本营系统
    _currentUpgradeUI = nullptr;
    _resourceUI = nullptr;

    initBuildingData();

    // 地图配置
    _mapConfigs.clear();
    _mapConfigs["map/Map1.png"] = { 1.3f, Vec2(1406.0f, 2107.2f), 55.6f };
    _mapConfigs["map/Map2.png"] = { 1.3f, Vec2(1402.0f, 2097.2f), 56.1f };
    _mapConfigs["map/Map3.png"] = { 1.3f, Vec2(1403.0f, 2075.2f), 54.9f };

    _heroManager = HeroManager::create();
    this->addChild(_heroManager);

    setupMap();
    setupUI();
    setupTouchListener();

    auto keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            if (_isBuildingMode)
            {
                this->cancelPlacing();
            }
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    setupMouseListener();

    connectToServer();
    setupNetworkCallbacks();

    // 添加每帧更新以处理网络回调
    scheduleUpdate();

    // 设置资源显示
    setupResourceDisplay();

    return true;
}

void DraggableMapScene::update(float dt)
{
    SocketClient::getInstance().processCallbacks();
}

void DraggableMapScene::setupResourceDisplay()
{
    _resourceUI = ResourceDisplayUI::create();
    if (_resourceUI)
    {
        // 设置资源显示在左上角
        _resourceUI->setPositionAtTopLeft();
        this->addChild(_resourceUI, 100);

        // 调整位置，确保在屏幕内
        auto pos = _resourceUI->getPosition();
        CCLOG("Resource UI positioned at: (%.1f, %.1f)", pos.x, pos.y);
    }
    else
    {
        CCLOG("ERROR: Failed to create ResourceDisplayUI!");
    }
}

void DraggableMapScene::initBuildingData()
{
    _buildingList.clear();

    // 大本营
    BuildingData townHall("大本营", "BaseCamp/town-hall-1.png", Size(5, 5), 0.6f, 0, 0);
    _buildingList.push_back(townHall);

    _buildingList.push_back(BuildingData("箭塔", "Tower.png", Size(3, 3), 0.8f, 1000, 60));
    _buildingList.push_back(BuildingData("炮塔", "Cannon.png", Size(2, 2), 1.0f, 500, 30));
    _buildingList.push_back(BuildingData("兵营", "Barracks.png", Size(4, 4), 0.6f, 1500, 120));
    _buildingList.push_back(BuildingData("金矿", "GoldMine.png", Size(3, 3), 0.8f, 800, 45));
    _buildingList.push_back(BuildingData("圣水收集器", "ElixirCollector.png", Size(3, 3), 0.8f, 750, 40));
}

void DraggableMapScene::setupMap()
{
    _mapSprite = Sprite::create(_currentMapName);
    if (_mapSprite)
    {
        auto mapSize = _mapSprite->getContentSize();
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        this->addChild(_mapSprite, 0);

        float tile = 55.6f;
        Vec2 startPixel = Vec2::ZERO;
        float mapScale = _currentScale;
        auto it = _mapConfigs.find(_currentMapName);
        if (it != _mapConfigs.end())
        {
            tile = it->second.tileSize;
            startPixel = it->second.startPixel;
            mapScale = it->second.scale;
        }

        _currentScale = mapScale;
        _mapSprite->setScale(_currentScale);

        _gridMap = GridMap::create(mapSize, tile);
        _mapSprite->addChild(_gridMap, 999);

        if (_gridMap && startPixel != Vec2::ZERO)
        {
            _gridMap->setStartPixel(startPixel);
            _gridStartDefault = startPixel;
        }
        else if (_gridMap)
        {
            _gridStartDefault = _gridMap->getStartPixel();
        }

        updateBoundary();
        createSampleMapElements();
    }
    else
    {
        CCLOG("Error: Failed to load map image %s", _currentMapName.c_str());
        auto errorLabel = Label::createWithSystemFont("Failed to load " + _currentMapName, "Arial", 32);
        errorLabel->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        errorLabel->setTextColor(Color4B::RED);
        this->addChild(errorLabel);
    }

    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);
}

void DraggableMapScene::setupUI()
{
    // 先创建资源显示
    setupResourceDisplay();

    // 计算Build按钮的位置 - 与资源对齐（资源在X=30位置）
    float resourceXPos = 30;  // 资源显示的X坐标
    float buildButtonY = _visibleSize.height - 230;  // Build按钮的Y坐标

    CCLOG("Screen height: %.1f, Build button Y: %.1f", _visibleSize.height, buildButtonY);

    // ==================== Build按钮（与资源对齐）====================
    _buildButton = Button::create();
    _buildButton->setTitleText("Build");
    _buildButton->setTitleFontSize(24);
    _buildButton->setContentSize(Size(100, 50));
    _buildButton->setPosition(Vec2(resourceXPos + 70, buildButtonY));  // 资源X=30，按钮宽100，中心对齐
    _buildButton->addClickEventListener([this](Ref* sender) {
        if (_isBuildingMode)
        {
            this->cancelPlacing();
        }
        else
        {
            this->toggleBuildingSelection();
        }
        });

    this->addChild(_buildButton, 10);

    // ==================== Grid Dev按钮（放在Build下面）====================
    auto toggleDebugBtn = Button::create();
    toggleDebugBtn->setTitleText("Grid Dev");
    toggleDebugBtn->setTitleFontSize(20);
    toggleDebugBtn->setTitleColor(Color3B::MAGENTA);
    toggleDebugBtn->setContentSize(Size(100, 40));
    toggleDebugBtn->setPosition(Vec2(resourceXPos + 70, buildButtonY - 60));  // Build按钮下面

    this->addChild(toggleDebugBtn, 10);

    // ==================== 当前地图名称（放在最上面的中间）====================
    auto mapNameLabel = Label::createWithSystemFont("Current: " + _currentMapName, "Arial", 20);
    mapNameLabel->setPosition(Vec2(_visibleSize.width / 2.0f, _visibleSize.height - 30));  // 最上面的中间
    mapNameLabel->setTextColor(Color4B(0, 255, 255, 255));  // 青色：RGB(0, 255, 255)
    mapNameLabel->setName("mapNameLabel");
    this->addChild(mapNameLabel, 10);

    // ==================== Map按钮（右上角）====================
    _mapButton = Button::create();
    _mapButton->setTitleText("Map");
    _mapButton->setTitleFontSize(24);
    _mapButton->setContentSize(Size(120, 60));
    _mapButton->setPosition(Vec2(_visibleSize.width - 80, _visibleSize.height - 80));  // 右上角，地图名称下面
    _mapButton->addClickEventListener(CC_CALLBACK_1(DraggableMapScene::onMapButtonClicked, this));

    this->addChild(_mapButton, 10);

    // 战斗按钮
    _battleButton = Button::create();
    _battleButton->setTitleText("Attack!");
    _battleButton->setTitleFontSize(24);
    _battleButton->setPosition(Vec2(100, 100));
    _battleButton->addClickEventListener(CC_CALLBACK_1(DraggableMapScene::onBattleButtonClicked, this));
    this->addChild(_battleButton, 20);

    // 部落按钮
    _clanButton = Button::create();
    _clanButton->setTitleText("Clan");
    _clanButton->setTitleFontSize(24);
    _clanButton->setPosition(Vec2(_visibleSize.width - 50, 100));
    _clanButton->addClickEventListener(CC_CALLBACK_1(DraggableMapScene::onClanButtonClicked, this));
    this->addChild(_clanButton, 20);

    // [英雄UI]：初始化英雄管理器相关的UI (头像、状态栏等)
    // ==================== 英雄UI ====================
    _heroManager->setupHeroUI(this, _visibleSize);

    // ==================== 初始化其他UI ====================
    createBuildingSelection();
    createMapList();

    // ==================== 操作指南 ====================
    auto tipLabel = Label::createWithSystemFont(
        "Drag: Move Map  Scroll: Zoom  Buttons: Switch Map/Hero/Build\nClick Hero to Select, Click Ground to Move",
        "Arial", 14);
    tipLabel->setPosition(Vec2(_visibleSize.width / 2.0f, 40.0f));
    tipLabel->setTextColor(Color4B::YELLOW);
    tipLabel->setAlignment(TextHAlignment::CENTER);
    this->addChild(tipLabel, 10);

    // ==================== 调试层 ====================
    auto debugLayer = Node::create();
    debugLayer->setPosition(Vec2::ZERO);
    debugLayer->setVisible(false);
    this->addChild(debugLayer, 30);

    // 创建调试按钮（方向按钮等）
    Vec2 uiBase = Vec2(500.0f, 400.0f);
    float btnSize = 40.0f;

    auto makeArrowBtn = [this, debugLayer, btnSize](const std::string& title, const Vec2& pos,
        const std::function<void()>& cb) {
            auto btn = ui::Button::create();
            btn->setTitleText(title);
            btn->setTitleFontSize(18);
            btn->setContentSize(Size(btnSize, btnSize));
            btn->setScale9Enabled(true);
            btn->setPosition(pos);
            btn->addClickEventListener([cb](Ref* sender) { cb(); });
            debugLayer->addChild(btn);
        };

    // 方向按钮（示例）
    makeArrowBtn("←", uiBase + Vec2(-50, 0), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(-1.0f, 0.0f);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        });

    makeArrowBtn("→", uiBase + Vec2(50, 0), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(1.0f, 0.0f);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        });

    makeArrowBtn("↑", uiBase + Vec2(0, 50), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(0.0f, 1.0f);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        });

    makeArrowBtn("↓", uiBase + Vec2(0, -50), [this]() {
        if (!_gridMap) return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(0.0f, -1.0f);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        });

    makeArrowBtn("Reset", uiBase + Vec2(0, -110), [this]() {
        if (!_gridMap) return;
        _gridMap->setStartPixel(_gridStartDefault);
        _gridMap->showWholeGrid(true);
        });

    // Grid Dev按钮点击事件
    toggleDebugBtn->addClickEventListener([debugLayer, this](Ref* sender) {
        bool isVisible = debugLayer->isVisible();
        debugLayer->setVisible(!isVisible);

        if (_gridMap)
        {
            _gridMap->showWholeGrid(!isVisible);
        }

        CCLOG("Debug Mode: %s", !isVisible ? "ON" : "OFF");
        });

    CCLOG("UI setup complete. Build button at (%.1f, %.1f)", resourceXPos + 70, buildButtonY);
}
void DraggableMapScene::toggleBuildingSelection()
{
    _isBuildingListVisible = !_isBuildingListVisible;
    _buildingListUI->setVisible(_isBuildingListVisible);

    if (_isBuildingListVisible)
    {
        if (_isMapListVisible)
        {
            toggleMapList();
        }
        if (_heroManager->isHeroListVisible())
        {
            _heroManager->hideHeroList();
        }
    }
}

void DraggableMapScene::createBuildingSelection()
{
    _buildingListUI = ListView::create();
    _buildingListUI->setContentSize(Size(300, 200));
    _buildingListUI->setPosition(Vec2(160, _visibleSize.height - 250));
    _buildingListUI->setBackGroundColor(Color3B(60, 60, 80));
    _buildingListUI->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    _buildingListUI->setOpacity(220);
    _buildingListUI->setVisible(false);
    _buildingListUI->setScrollBarEnabled(true);
    _buildingListUI->setBounceEnabled(true);

    for (const auto& building : _buildingList)
    {
        auto item = Layout::create();
        item->setContentSize(Size(280, 60));
        item->setTouchEnabled(true);

        auto buildingSprite = Sprite::create(building.imageFile);
        if (buildingSprite)
        {
            buildingSprite->setScale(0.3f);
            buildingSprite->setPosition(Vec2(40, 30));
            buildingSprite->setName("sprite");
            item->addChild(buildingSprite);
        }

        auto nameLabel = Label::createWithSystemFont(building.name, "Arial", 16);
        nameLabel->setPosition(Vec2(120, 40));
        nameLabel->setTextColor(Color4B::YELLOW);
        nameLabel->setName("name");
        item->addChild(nameLabel);

        std::string sizeText =
            StringUtils::format("%dx%d", (int)building.gridSize.width, (int)building.gridSize.height);
        auto sizeLabel = Label::createWithSystemFont(sizeText, "Arial", 14);
        sizeLabel->setPosition(Vec2(120, 20));
        sizeLabel->setTextColor(Color4B::GREEN);
        item->addChild(sizeLabel);

        std::string costText = StringUtils::format("Cost: %d", (int)building.cost);
        auto costLabel = Label::createWithSystemFont(costText, "Arial", 12);
        costLabel->setPosition(Vec2(220, 40));
        costLabel->setTextColor(Color4B::WHITE);
        item->addChild(costLabel);

        auto itemBg = LayerColor::create(Color4B(40, 40, 60, 255));
        itemBg->setContentSize(Size(280, 60));
        itemBg->setPosition(Vec2::ZERO);
        item->addChild(itemBg, -1);

        item->addClickEventListener([this, building](Ref* sender) { this->onBuildingItemClicked(sender, building); });

        _buildingListUI->pushBackCustomItem(item);
    }

    this->addChild(_buildingListUI, 20);
}

void DraggableMapScene::onBuildingItemClicked(cocos2d::Ref* sender, const BuildingData& building)
{
    CCLOG("Selected building: %s, Size: %.0fx%.0f", building.name.c_str(), building.gridSize.width,
        building.gridSize.height);

    startPlacingBuilding(building);
    toggleBuildingSelection();
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

    for (const auto& mapName : _mapNames)
    {
        auto item = Layout::create();
        item->setContentSize(Size(140, 40));
        item->setTouchEnabled(true);

        auto label = Label::createWithSystemFont(mapName, "Arial", 16);
        label->setPosition(Vec2(70, 20));
        label->setTextColor(Color4B::WHITE);
        label->setName("label");
        item->addChild(label);

        item->addClickEventListener([this, mapName](Ref* sender) { this->onMapItemClicked(sender); });

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

    if (_isMapListVisible && _heroManager->isHeroListVisible())
    {
        _heroManager->hideHeroList();
    }
}

void DraggableMapScene::onMapItemClicked(cocos2d::Ref* sender)
{
    auto item = static_cast<Layout*>(sender);
    auto label = static_cast<Label*>(item->getChildByName("label"));
    std::string selectedMapName = label->getString();

    CCLOG("Selected map: %s", selectedMapName.c_str());

    switchMap(selectedMapName);
    toggleMapList();
}

void DraggableMapScene::switchMap(const std::string& mapName)
{
    if (mapName == _currentMapName)
        return;

    if (_isBuildingMode)
    {
        cancelPlacing();
    }
    cleanupUpgradeUI();
    // 隐藏升级界面
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->hide();
        _currentUpgradeUI = nullptr;
    }

    saveMapElementsState();

    if (_mapSprite)
    {
        this->removeChild(_mapSprite);
        _mapSprite = nullptr;
        _gridMap = nullptr;
    }

    _currentMapName = mapName;
    _mapSprite = Sprite::create(_currentMapName);

    if (_mapSprite)
    {
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);

        float tile = 55.6f;
        Vec2 startPixel = Vec2::ZERO;
        float mapScale = _currentScale;
        auto it = _mapConfigs.find(_currentMapName);
        if (it != _mapConfigs.end())
        {
            tile = it->second.tileSize;
            startPixel = it->second.startPixel;
            mapScale = it->second.scale;
        }

        _currentScale = mapScale;
        _mapSprite->setScale(_currentScale);
        this->addChild(_mapSprite, 0);

        auto mapSize = _mapSprite->getContentSize();
        _gridMap = GridMap::create(mapSize, tile);
        _mapSprite->addChild(_gridMap, 999);

        if (_gridMap && startPixel != Vec2::ZERO)
        {
            _gridMap->setStartPixel(startPixel);
            _gridStartDefault = startPixel;
        }
        else if (_gridMap)
        {
            _gridStartDefault = _gridMap->getStartPixel();
        }

        updateBoundary();
        restoreMapElementsState();

        _heroManager->onMapSwitched(_mapSprite);
        _heroManager->updateHeroesScale(_currentScale);

        auto mapNameLabel = static_cast<Label*>(this->getChildByName("mapNameLabel"));
        if (mapNameLabel)
        {
            mapNameLabel->setString("Current: " + _currentMapName);
        }

        CCLOG("Map switched successfully to %s", mapName.c_str());
    }
    else
    {
        CCLOG("Error: Failed to load new map %s", mapName.c_str());

        _currentMapName = "Map7.png";
        _mapSprite = Sprite::create(_currentMapName);
        if (_mapSprite)
        {
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
    for (auto& element : _mapElements)
    {
        if (element.node)
        {
            element.localPosition = element.node->getPosition();
            element.node->retain();
        }
    }
}

void DraggableMapScene::restoreMapElementsState()
{
    for (auto& element : _mapElements)
    {
        if (element.node && element.node->getParent() == nullptr)
        {
            _mapSprite->addChild(element.node, 1);
            element.node->setPosition(element.localPosition);
        }
        element.node->release();
    }
}

void DraggableMapScene::createSampleMapElements()
{
    _mapElements.clear();

    if (!_mapSprite)
        return;

    auto createMarker = [this](const Vec2& worldPosition, const Color4B& color, const std::string& text) {
        Vec2 localPos = _mapSprite->convertToNodeSpace(worldPosition);

        auto marker = DrawNode::create();
        marker->drawDot(Vec2::ZERO, 10, Color4F(color));
        marker->setPosition(localPos);
        _mapSprite->addChild(marker, 1);

        auto label = Label::createWithSystemFont(text, "Arial", 16);
        label->setPosition(localPos + Vec2(0, 20));
        label->setTextColor(Color4B::WHITE);
        _mapSprite->addChild(label, 1);

        MapElement markerElement = { marker, localPos };
        MapElement labelElement = { label, localPos + Vec2(0, 20) };
        _mapElements.push_back(markerElement);
        _mapElements.push_back(labelElement);
        };

    createMarker(Vec2(_visibleSize.width * 0.3f, _visibleSize.height * 0.7f), Color4B::RED, "Point A");
    createMarker(Vec2(_visibleSize.width * 0.7f, _visibleSize.height * 0.5f), Color4B::GREEN, "Point B");
    createMarker(Vec2(_visibleSize.width * 0.5f, _visibleSize.height * 0.3f), Color4B::BLUE, "Point C");

    CCLOG("Created %zd map elements", _mapElements.size());
}

void DraggableMapScene::updateMapElementsPosition()
{
    if (!_mapSprite)
        return;

    for (auto& element : _mapElements)
    {
        if (element.node && element.node->getParent() == _mapSprite)
        {
            element.node->setPosition(element.localPosition);
        }
    }
}

void DraggableMapScene::setupTouchListener()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

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
        if (!_mapSprite)
            return;

        EventMouse* mouseEvent = dynamic_cast<EventMouse*>(event);
        if (mouseEvent)
        {
            float scrollY = mouseEvent->getScrollY();

            float zoomFactor = 1.0f;
            if (scrollY < 0)
            {
                zoomFactor = 1.1f;
            }
            else if (scrollY > 0)
            {
                zoomFactor = 0.9f;
            }
            else
            {
                return;
            }

            Vec2 mousePos = Vec2(mouseEvent->getCursorX(), mouseEvent->getCursorY());
            zoomMap(zoomFactor, mousePos);
            CCLOG("Mouse scroll: %.1f, Scale: %.2f", scrollY, _currentScale);
        }
        };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void DraggableMapScene::zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint)
{
    if (!_mapSprite)
        return;

    float newScale = _currentScale * scaleFactor;
    newScale = MAX(_minScale, MIN(_maxScale, newScale));

    if (newScale == _currentScale)
    {
        return;
    }

    if (pivotPoint != Vec2::ZERO)
    {
        Vec2 localPos = _mapSprite->convertToNodeSpace(pivotPoint);
        Vec2 worldPosBefore = _mapSprite->convertToWorldSpace(localPos);
        _mapSprite->setScale(newScale);
        Vec2 worldPosAfter = _mapSprite->convertToWorldSpace(localPos);
        Vec2 positionDelta = worldPosBefore - worldPosAfter;
        _mapSprite->setPosition(_mapSprite->getPosition() + positionDelta);
    }
    else
    {
        _mapSprite->setScale(newScale);
    }

    _currentScale = newScale;
    updateBoundary();
    updateMapElementsPosition();
    _heroManager->updateHeroesScale(_currentScale);
    ensureMapInBoundary();
}

void DraggableMapScene::updateBoundary()
{
    if (!_mapSprite)
        return;

    auto mapSize = _mapSprite->getContentSize() * _currentScale;

    float minX = _visibleSize.width - mapSize.width / 2;
    float maxX = mapSize.width / 2;
    float minY = _visibleSize.height - mapSize.height / 2;
    float maxY = mapSize.height / 2;

    if (mapSize.width <= _visibleSize.width)
    {
        minX = maxX = _visibleSize.width / 2;
    }
    if (mapSize.height <= _visibleSize.height)
    {
        minY = maxY = _visibleSize.height / 2;
    }

    _mapBoundary = Rect(minX, minY, maxX - minX, maxY - minY);

    CCLOG("Boundary updated - Scale: %.2f, Boundary: minX=%.1f, maxX=%.1f", _currentScale, minX, maxX);
}

bool DraggableMapScene::onTouchBegan(Touch* touch, Event* event)
{
    _lastTouchPos = touch->getLocation();

    // 1. 如果有升级界面显示，检查是否点击在它上面
    if (_currentUpgradeUI && _currentUpgradeUI->isVisible())
    {
        Vec2 localPos = _currentUpgradeUI->convertTouchToNodeSpace(touch);
        Rect bbox = _currentUpgradeUI->getBoundingBox();
        bbox.origin = Vec2::ZERO;

        if (bbox.containsPoint(localPos)) {
            CCLOG("Touch on upgrade UI, let UI handle it");
            return true;  // 让UI自己处理
        }
        else {
            CCLOG("Touch outside upgrade UI, hiding it");
            _currentUpgradeUI->hide();
            _currentUpgradeUI = nullptr;
            return false;  // 返回false，让其他元素可以接收触摸
        }
    }

    // 2. 处理建筑放置模式下的点击逻辑
    if (_isBuildingMode && !_isDraggingBuilding && !_isWaitingConfirm)
    {
        _dragStartPos = _lastTouchPos;
        _isDraggingBuilding = true;

        if (_ghostSprite && _gridMap)
        {
            // 将触摸点转换为网格坐标
            Vec2 rawGridPos = _gridMap->getGridPosition(_dragStartPos);

            // 计算网格中心偏移量，确保建筑占据的网格以触摸点为中心
            int offsetX = static_cast<int>((_selectedBuilding.gridSize.width - 1.0f) / 2.0f);
            int offsetY = static_cast<int>((_selectedBuilding.gridSize.height - 1.0f) / 2.0f);
            Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));

            Vec2 centerAlignedGridPos = rawGridPos - offset;

            // 计算实际像素坐标并更新虚影位置
            Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
            _ghostSprite->setPosition(buildingPos);
            _ghostSprite->setVisible(true);

            // 检查当前位置是否合法并更新底座颜色提示
            bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);
            _gridMap->updateBuildingBase(centerAlignedGridPos, _selectedBuilding.gridSize, canBuild);
            showBuildingHint("拖动调整位置，松开鼠标后确认");
        }

        return true;
    }

    // 3. 处理选中英雄时的逻辑
    if (!_isBuildingMode && !_heroManager->getSelectedHeroName().empty())
    {
        _heroManager->handleHeroTouch(_lastTouchPos, _mapSprite, true);
        return true;
    }

    // 4. 处理普通模式下的英雄点击检测
    if (!_isBuildingMode)
    {
        _heroManager->handleHeroTouch(_lastTouchPos, _mapSprite, false);
        for (auto& hero : _heroManager->getPlacedHeroes())
        {
            if (hero && hero->containsTouch(_lastTouchPos, _mapSprite))
            {
                break;  // 命中英雄后中断，避免穿透
            }
        }
    }

    return true;
}
void DraggableMapScene::closeUpgradeUI()
{
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->hide();
        _currentUpgradeUI = nullptr;
    }
}
void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    Vec2 currentTouchPos = touch->getLocation();

    // 1. 建筑拖拽逻辑
    if (_isBuildingMode && _isDraggingBuilding && _ghostSprite && _gridMap)
    {
        Vec2 rawGridPos = _gridMap->getGridPosition(currentTouchPos);

        // 计算中心偏移
        int offsetX = static_cast<int>((_selectedBuilding.gridSize.width - 1.0f) / 2.0f);
        int offsetY = static_cast<int>((_selectedBuilding.gridSize.height - 1.0f) / 2.0f);
        Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));

        Vec2 centerAlignedGridPos = rawGridPos - offset;

        // 实时检测建造区域是否合法
        bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);

        _gridMap->updateBuildingBase(centerAlignedGridPos, _selectedBuilding.gridSize, canBuild);

        Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
        _ghostSprite->setPosition(buildingPos);

        // 根据能否建造改变虚影颜色
        if (canBuild)
        {
            _ghostSprite->setColor(Color3B::WHITE);
        }
        else
        {
            _ghostSprite->setColor(Color3B(255, 100, 100));
        }

        return;
    }

    // 2. 地图拖拽逻辑
    Vec2 delta = currentTouchPos - _lastTouchPos;
    moveMap(delta);
    _lastTouchPos = currentTouchPos;
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event)
{
    // 处理建筑拖拽结束
    if (_isBuildingMode && _isDraggingBuilding && _gridMap && _ghostSprite)
    {
        Vec2 currentTouchPos = touch->getLocation();
        Vec2 rawGridPos = _gridMap->getGridPosition(currentTouchPos);

        // 计算偏移
        int offsetX = static_cast<int>((_selectedBuilding.gridSize.width - 1.0f) / 2.0f);
        int offsetY = static_cast<int>((_selectedBuilding.gridSize.height - 1.0f) / 2.0f);
        Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));

        Vec2 centerAlignedGridPos = rawGridPos - offset;

        bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);

        if (canBuild)
        {
            // 位置合法：进入"待确认"状态
            _pendingGridPos = centerAlignedGridPos;
            _isDraggingBuilding = false;
            _isWaitingConfirm = true;

            Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
            Vec2 worldPos = _mapSprite->convertToWorldSpace(buildingPos);

            showConfirmButtons(worldPos);
            showBuildingHint("点击按钮确认或取消建造");
        }
        else
        {
            // 位置非法：播放红色闪烁和震动动画
            auto flashRed = TintTo::create(0.1f, 255, 0, 0);
            auto flashNormal = TintTo::create(0.1f, 255, 255, 255);
            auto shake = MoveBy::create(0.05f, Vec2(5.0f, 0.0f));
            auto shakeBack = MoveBy::create(0.05f, Vec2(-10.0f, 0.0f));
            auto shakeEnd = MoveBy::create(0.05f, Vec2(5.0f, 0.0f));

            auto sequence = Sequence::create(Spawn::create(flashRed, shake, nullptr),
                Spawn::create(flashNormal, shakeBack, nullptr), shakeEnd, nullptr);
            _ghostSprite->runAction(sequence);

            showBuildingHint("无法在此处建造！请重新选择位置");
        }

        return;
    }
}

void DraggableMapScene::onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event)
{
    this->onTouchEnded(touch, event);
}

void DraggableMapScene::moveMap(const cocos2d::Vec2& delta)
{
    if (!_mapSprite)
        return;

    _mapSprite->setPosition(_mapSprite->getPosition() + delta);
    ensureMapInBoundary();
}

void DraggableMapScene::ensureMapInBoundary()
{
    if (!_mapSprite)
        return;

    cocos2d::Vec2 currentPos = _mapSprite->getPosition();
    cocos2d::Vec2 newPos = currentPos;

    // 限制 X 轴范围
    if (currentPos.x < _mapBoundary.getMinX())
    {
        newPos.x = _mapBoundary.getMinX();
    }
    else if (currentPos.x > _mapBoundary.getMaxX())
    {
        newPos.x = _mapBoundary.getMaxX();
    }

    // 限制 Y 轴范围
    if (currentPos.y < _mapBoundary.getMinY())
    {
        newPos.y = _mapBoundary.getMinY();
    }
    else if (currentPos.y > _mapBoundary.getMaxY())
    {
        newPos.y = _mapBoundary.getMaxY();
    }

    if (newPos != currentPos)
    {
        _mapSprite->setPosition(newPos);
    }
}

void DraggableMapScene::startPlacingBuilding(const BuildingData& building)
{
    if (!_mapSprite || !_gridMap)
        return;

    _isBuildingMode = true;
    _isDraggingBuilding = false;
    _selectedBuilding = building;

    _ghostSprite = Sprite::create(building.imageFile);
    if (_ghostSprite)
    {
        _ghostSprite->setOpacity(150);
        _ghostSprite->setAnchorPoint(Vec2(0.5f, 0.2f));
        _ghostSprite->setScale(building.scaleFactor);
        _ghostSprite->setPosition(Vec2(-1000.0f, -1000.0f));
        _mapSprite->addChild(_ghostSprite, 2000);
        showBuildingHint("点击地图开始放置建筑");
        // 如果是大本营，设置正确的锚点
        if (building.name == "大本营") {
            _ghostSprite->setAnchorPoint(Vec2(0.5f, 0.2f));
        }
    }

}

void DraggableMapScene::placeBuilding(Vec2 gridPos)
{
    if (!_ghostSprite || !_isBuildingMode || _selectedBuilding.name.empty() || !_gridMap)
        return;

    bool canBuild = _gridMap->checkArea(gridPos, _selectedBuilding.gridSize);

    if (!canBuild)
    {
        CCLOG("Cannot build here! Area occupied or out of bounds.");

        // 播放拒绝动画
        auto flashRed = TintTo::create(0.1f, 255, 0, 0);
        auto flashNormal = TintTo::create(0.1f, 255, 255, 255);
        auto shake = MoveBy::create(0.05f, Vec2(5.0f, 0.0f));
        auto shakeBack = MoveBy::create(0.05f, Vec2(-10.0f, 0.0f));
        auto shakeEnd = MoveBy::create(0.05f, Vec2(5.0f, 0.0f));
        auto sequence = Sequence::create(Spawn::create(flashRed, shake, nullptr),
            Spawn::create(flashNormal, shakeBack, nullptr), shakeEnd, nullptr);
        _ghostSprite->runAction(sequence);

        showBuildingHint("无法在此处建造！区域被占用或越界");
        return;
    }

    // 1. 标记网格被占用
    _gridMap->markArea(gridPos, _selectedBuilding.gridSize, true);

    // 2. 创建建筑
    Node* building = nullptr;

    // 在 placeBuilding() 函数中找到创建大本营的部分，修改为：
    if (_selectedBuilding.name == "大本营")
    {
        // 创建大本营
        auto townHall = TownHallBuilding::create(1);
        building = townHall;

        if (townHall)
        {
            // 移除可能存在的默认监听器
            Director::getInstance()->getEventDispatcher()->removeEventListenersForTarget(townHall);

            // 添加自定义触摸监听器 - 修复这里
            auto listener = EventListenerTouchOneByOne::create();
            listener->setSwallowTouches(true);
            listener->onTouchBegan = [townHall](Touch* touch, Event* event) {
                if (!townHall->isVisible()) return false;

                Vec2 touchInNode = townHall->convertTouchToNodeSpace(touch);
                Rect rect = townHall->getBoundingBox();

                // 调整触摸检测区域，因为大本营的锚点在 (0.5, 0.2)
                rect.origin = Vec2::ZERO;

                bool contains = rect.containsPoint(touchInNode);

                if (contains)
                {
                    CCLOG("TownHall touched! Position in node: (%.1f, %.1f)", touchInNode.x, touchInNode.y);
                    return true;
                }
                return false;
                };

            listener->onTouchEnded = [this, townHall](Touch* touch, Event* event) {
                CCLOG("TownHall touch ended, calling onTownHallClicked");
                this->onTownHallClicked(townHall);
                };

            Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, townHall);

            CCLOG("TownHall touch listener added successfully");
        }
    }
    else
    {
        // 普通建筑
        building = Sprite::create(_selectedBuilding.imageFile);
        if (building)
        {
            building->setName(_selectedBuilding.name);
        }
    }

    if (!building)
    {
        showBuildingHint("创建建筑失败！");
        return;
    }

    building->setAnchorPoint(Vec2(0.5f, 0.2f));
    building->setScale(_selectedBuilding.scaleFactor);

    Vec2 buildingPos = calculateBuildingPosition(gridPos);
    building->setPosition(buildingPos);

    // 3. 设置动态 Z-Order (Y-Sorting)
    building->setLocalZOrder(10000 - static_cast<int>(buildingPos.y));

    _mapSprite->addChild(building);

    // 4. 播放落地动画
    building->setScale(0.0f);
    auto scaleAction = EaseBackOut::create(ScaleTo::create(0.4f, _selectedBuilding.scaleFactor));
    auto fadeIn = FadeIn::create(0.3f);
    building->runAction(Spawn::create(scaleAction, fadeIn, nullptr));

    // 5. 保存建筑信息
    PlacedBuildingInfo info;
    info.size = _selectedBuilding.gridSize;
    info.gridPos = gridPos;
    info.node = building;
    _placedBuildings.push_back(info);

    showBuildingHint(StringUtils::format("%s 建造完成！", _selectedBuilding.name.c_str()));

    CCLOG("Building placed: %s at grid (%.0f, %.0f)", _selectedBuilding.name.c_str(), gridPos.x, gridPos.y);

    // 6. 延迟退出建造模式
    auto delay = DelayTime::create(1.0f);
    auto callback = CallFunc::create([this]() { endPlacing(); });
    this->runAction(Sequence::create(delay, callback, nullptr));
}

void DraggableMapScene::onTownHallClicked(TownHallBuilding* townHall)
{
    if (!townHall) {
        CCLOG("ERROR: onTownHallClicked called with null townHall!");
        return;
    }

    CCLOG("TownHall clicked! Level: %d, Position: (%.1f, %.1f)",
        townHall->getLevel(), townHall->getPosition().x, townHall->getPosition().y);

    // 如果已经有升级界面显示，先隐藏
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->hide();
        _currentUpgradeUI = nullptr;
        return;  // 如果已经显示，点击后隐藏，不再创建新的
    }

    // 创建新的升级界面
    _currentUpgradeUI = TownHallUpgradeUI::create(townHall);
    if (_currentUpgradeUI)
    {
        CCLOG("Creating upgrade UI...");
        _currentUpgradeUI->setPositionNearBuilding(townHall);

        // 设置升级回调
        _currentUpgradeUI->setUpgradeCallback([this](bool success, int newLevel) {
            if (success)
            {
                showBuildingHint("大本营升级到 " + std::to_string(newLevel) + " 级！");
            }
            else
            {
                showBuildingHint("金币不足，无法升级！");
            }

            // 升级完成后隐藏UI
            if (_currentUpgradeUI)
            {
                _currentUpgradeUI->hide();
                _currentUpgradeUI = nullptr;
            }
            });

        this->addChild(_currentUpgradeUI, 1000);
        _currentUpgradeUI->show();
        CCLOG("Upgrade UI shown");
    }
    else
    {
        CCLOG("ERROR: Failed to create upgrade UI!");
    }
}

void DraggableMapScene::cancelPlacing()
{
    if (_isWaitingConfirm)
    {
        onCancelBuilding();
    }
    else if (_isDraggingBuilding)
    {
        _isDraggingBuilding = false;

        if (_ghostSprite)
        {
            _ghostSprite->setPosition(Vec2(-1000.0f, -1000.0f));
        }

        if (_gridMap)
        {
            _gridMap->hideBuildingBase();
        }

        showBuildingHint("点击地图开始放置建筑");
    }
    else
    {
        endPlacing();
    }
}

void DraggableMapScene::endPlacing()
{
    _isBuildingMode = false;
    _isDraggingBuilding = false;
    _isWaitingConfirm = false;
    _pendingGridPos = Vec2::ZERO;
    _selectedBuilding = BuildingData("", "", Size::ZERO);

    auto hint = this->getChildByName("buildingHint");
    if (hint)
    {
        hint->removeFromParent();
    }

    if (_gridMap)
    {
        _gridMap->showWholeGrid(false);
        _gridMap->hideBuildingBase();
    }

    if (_ghostSprite)
    {
        _ghostSprite->removeFromParent();
        _ghostSprite = nullptr;
    }

    hideConfirmButtons();
}

void DraggableMapScene::showConfirmButtons(const cocos2d::Vec2& buildingWorldPos)
{
    hideConfirmButtons();

    float buttonSize = 45.0f;
    float offsetX = 60.0f;
    float offsetY = 80.0f;

    // 创建确认按钮
    _confirmButton = ui::Button::create();
    _confirmButton->setTitleText("✓");
    _confirmButton->setTitleFontSize(30);
    _confirmButton->setTitleColor(Color3B::WHITE);
    _confirmButton->setContentSize(Size(buttonSize, buttonSize));
    _confirmButton->setPosition(Vec2(buildingWorldPos.x + offsetX, buildingWorldPos.y + offsetY));

    auto confirmBg = LayerColor::create(Color4B(0, 200, 0, 200), buttonSize, buttonSize);
    confirmBg->setPosition(Vec2(-buttonSize / 2, -buttonSize / 2));
    _confirmButton->addChild(confirmBg, -1);

    _confirmButton->addClickEventListener([this](Ref* sender) { this->onConfirmBuilding(); });
    this->addChild(_confirmButton, 10000);

    // 创建取消按钮
    _cancelButton = ui::Button::create();
    _cancelButton->setTitleText("✗");
    _cancelButton->setTitleFontSize(30);
    _cancelButton->setTitleColor(Color3B::WHITE);
    _cancelButton->setContentSize(Size(buttonSize, buttonSize));
    _cancelButton->setPosition(Vec2(buildingWorldPos.x - offsetX, buildingWorldPos.y + offsetY));

    auto cancelBg = LayerColor::create(Color4B(200, 0, 0, 200), buttonSize, buttonSize);
    cancelBg->setPosition(Vec2(-buttonSize / 2, -buttonSize / 2));
    _cancelButton->addChild(cancelBg, -1);

    _cancelButton->addClickEventListener([this](Ref* sender) { this->onCancelBuilding(); });
    this->addChild(_cancelButton, 10000);

    // 添加缩放动画
    auto scaleIn = ScaleTo::create(0.2f, 1.0f);
    _confirmButton->setScale(0.0f);
    _confirmButton->runAction(EaseBackOut::create(scaleIn->clone()));
    _cancelButton->setScale(0.0f);
    _cancelButton->runAction(EaseBackOut::create(scaleIn->clone()));
}

void DraggableMapScene::hideConfirmButtons()
{
    if (_confirmButton)
    {
        _confirmButton->removeFromParent();
        _confirmButton = nullptr;
    }

    if (_cancelButton)
    {
        _cancelButton->removeFromParent();
        _cancelButton = nullptr;
    }
}

void DraggableMapScene::onConfirmBuilding()
{
    if (!_isWaitingConfirm || !_ghostSprite || !_gridMap)
        return;

    placeBuilding(_pendingGridPos);
}

void DraggableMapScene::onCancelBuilding()
{
    if (!_isWaitingConfirm)
        return;

    _isWaitingConfirm = false;
    _isDraggingBuilding = false;
    _pendingGridPos = Vec2::ZERO;

    if (_ghostSprite)
    {
        _ghostSprite->setPosition(Vec2(-1000, -1000));
    }

    if (_gridMap)
    {
        _gridMap->hideBuildingBase();
    }

    hideConfirmButtons();
    showBuildingHint("已取消建造，点击地图重新选择位置");
}

void DraggableMapScene::connectToServer()
{
    auto& client = SocketClient::getInstance();

    if (!client.isConnected())
    {
        // 连接到服务器（本地测试用 127.0.0.1）
        bool connected = client.connect("127.0.0.1", 8888);

        if (connected)
        {
            // 登录
            auto account = AccountManager::getInstance().getCurrentAccount();
            if (account)
            {
                client.login(account->userId, account->username, 1000);  // 初始奖杯1000
            }
        }
    }
}

void DraggableMapScene::setupNetworkCallbacks()
{
    auto& client = SocketClient::getInstance();

    client.setOnLoginResult([](bool success, const std::string& msg) {
        if (success)
        {
            cocos2d::log("Login successful!");
        }
        else
        {
            cocos2d::log("Login failed: %s", msg.c_str());
        }
    });

    client.setOnDisconnected([]() { cocos2d::log("Disconnected from server!"); });
}

void DraggableMapScene::onBattleButtonClicked(Ref* sender)
{
    if (!SocketClient::getInstance().isConnected())
    {
        // 显示未连接提示
        auto label = Label::createWithSystemFont("未连接到服务器!", "Arial", 24);
        label->setPosition(_visibleSize / 2);
        label->setTextColor(Color4B::RED);
        this->addChild(label, 100);

        label->runAction(Sequence::create(DelayTime::create(2.0f), RemoveSelf::create(), nullptr));
        return;
    }

    auto scene = BattleScene::createScene();
    Director::getInstance()->pushScene(TransitionFade::create(0.3f, scene));
}

void DraggableMapScene::onClanButtonClicked(Ref* sender)
{
    // TODO: 打开部落界面
    cocos2d::log("Clan button clicked");
}


void DraggableMapScene::showBuildingHint(const std::string& hint)
{
    auto oldHint = this->getChildByName("buildingHint");
    if (oldHint)
    {
        oldHint->removeFromParent();
    }

    auto hintLabel = Label::createWithSystemFont(hint, "Arial", 18);
    hintLabel->setPosition(Vec2(_visibleSize.width / 2, 100));
    hintLabel->setTextColor(Color4B::YELLOW);
    hintLabel->setName("buildingHint");
    this->addChild(hintLabel, 30);
}

cocos2d::Vec2 DraggableMapScene::calculateBuildingPosition(const cocos2d::Vec2& gridPos)
{
    if (!_gridMap)
    {
        return Vec2::ZERO;
    }

    Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
    Vec2 posEnd = _gridMap->getPositionFromGrid(
        gridPos + Vec2(_selectedBuilding.gridSize.width - 1, _selectedBuilding.gridSize.height - 1));
    Vec2 centerPos = (posStart + posEnd) / 2.0f;

    return centerPos;
}

bool DraggableMapScene::getClosestAdjacentFreeCell(const PlacedBuildingInfo& bld, const cocos2d::Vec2& fromGrid, cocos2d::Vec2& outTargetGrid) const
{
    // 简化实现，返回建筑旁边的第一个空闲格子
    outTargetGrid = bld.gridPos + Vec2(bld.size.width, 0);
    return true;
}

void DraggableMapScene::commandSelectedHeroAttackNearest()
{
    // 暂不实现
}
void DraggableMapScene::cleanupUpgradeUI()
{
    if (_currentUpgradeUI)
    {
        // 如果升级界面还在场景中，移除它
        if (_currentUpgradeUI->getParent() == this)
        {
            _currentUpgradeUI->removeFromParent();
        }
        _currentUpgradeUI = nullptr;
    }
}
DraggableMapScene::~DraggableMapScene()
{
    // 清理升级界面
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->removeFromParent();
        _currentUpgradeUI = nullptr;
    }
}