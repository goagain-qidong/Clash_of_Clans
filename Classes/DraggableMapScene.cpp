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

    _visibleSize = Director::getInstance()->getVisibleSize();
    _currentScale = 1.3f;
    _minScale = 1.0f;
    _maxScale = 2.5f;

    _mapNames = {"map/Map1.png", "map/Map2.png", "map/Map3.png"};
    _currentMapName = "map/Map1.png";
    _mapSprite = nullptr;
    _gridMap = nullptr;
    _lastTouchPos = Vec2::ZERO;
    _dragStartPos = Vec2::ZERO;

    _isBuildingMode = false;
    _isDraggingBuilding = false;
    _ghostSprite = nullptr;
    _selectedBuilding = BuildingData();

    _buildButton = nullptr;
    _mapButton = nullptr;
    _buildingListUI = nullptr;
    _mapList = nullptr;
    _isBuildingListVisible = false;
    _isMapListVisible = false;

    _heroManager = nullptr;

    initBuildingData();

    _mapConfigs.clear();
    _mapConfigs["map/Map1.png"] = {1.3f, Vec2(1406.0f, 2107.2f), 55.6f};
    _mapConfigs["map/Map2.png"] = {1.3f, Vec2(1402.0f, 2097.2f), 56.1f};
    _mapConfigs["map/Map3.png"] = {1.3f, Vec2(1403.0f, 2075.2f), 54.9f};

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

    return true;
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

void DraggableMapScene::initBuildingData()
{
    _buildingList.clear();
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
    _buildButton = Button::create();
    _buildButton->setTitleText("Build");
    _buildButton->setTitleFontSize(24);
    _buildButton->setContentSize(Size(100, 50));
    _buildButton->setPosition(Vec2(80, _visibleSize.height - 50));
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

    _mapButton = Button::create();
    _mapButton->setTitleText("Map");
    _mapButton->setTitleFontSize(24);
    _mapButton->setContentSize(Size(120, 60));
    _mapButton->setPosition(Vec2(_visibleSize.width - 80, _visibleSize.height - 50));
    _mapButton->addClickEventListener(CC_CALLBACK_1(DraggableMapScene::onMapButtonClicked, this));
    this->addChild(_mapButton, 10);

    _heroManager->setupHeroUI(this, _visibleSize);

    createBuildingSelection();
    createMapList();

    auto tipLabel = Label::createWithSystemFont(
        "Drag: Move Map  Scroll: Zoom  Buttons: Switch Map/Hero/Build\nClick Hero to Select, Click Ground to Move",
        "Arial", 14);
    tipLabel->setPosition(Vec2(_visibleSize.width / 2, 40));
    tipLabel->setTextColor(Color4B::YELLOW);
    tipLabel->setAlignment(TextHAlignment::CENTER);
    this->addChild(tipLabel, 10);

    auto mapNameLabel = Label::createWithSystemFont("Current: " + _currentMapName, "Arial", 18);
    mapNameLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height - 30));
    mapNameLabel->setTextColor(Color4B::GREEN);
    mapNameLabel->setName("mapNameLabel");
    this->addChild(mapNameLabel, 10);

    Vec2 uiBase = Vec2(500, 400);
    float btnSize = 40.0f;

    auto makeArrowBtn = [this, btnSize](const std::string& title, const Vec2& pos, const std::function<void()>& cb) {
        auto btn = ui::Button::create();
        btn->setTitleText(title);
        btn->setTitleFontSize(18);
        btn->setContentSize(Size(btnSize, btnSize));
        btn->setScale9Enabled(true);
        btn->setPosition(pos);
        btn->addClickEventListener([cb](Ref* sender) { cb(); });
        this->addChild(btn, 30);
        return btn;
    };

    makeArrowBtn("←", uiBase + Vec2(-50, 0), [this]() {
        if (!_gridMap)
            return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(-1, 0);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    makeArrowBtn("→", uiBase + Vec2(50, 0), [this]() {
        if (!_gridMap)
            return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(1, 0);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    makeArrowBtn("↑", uiBase + Vec2(0, 50), [this]() {
        if (!_gridMap)
            return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(0, 1);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    makeArrowBtn("↓", uiBase + Vec2(0, -50), [this]() {
        if (!_gridMap)
            return;
        Vec2 p = _gridMap->getStartPixel();
        p += Vec2(0, -1);
        _gridMap->setStartPixel(p);
        _gridMap->showWholeGrid(true);
        CCLOG("Grid start pixel: %.2f, %.2f", p.x, p.y);
    });

    makeArrowBtn("Reset", uiBase + Vec2(0, -110), [this]() {
        if (!_gridMap)
            return;
        _gridMap->setStartPixel(_gridStartDefault);
        _gridMap->showWholeGrid(true);
        Vec2 p = _gridMap->getStartPixel();
        CCLOG("Grid reset to default: %.2f, %.2f", p.x, p.y);
    });
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

        MapElement markerElement = {marker, localPos};
        MapElement labelElement = {label, localPos + Vec2(0, 20)};
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

    Vec2 oldPosition = _mapSprite->getPosition();

    if (pivotPoint != Vec2::ZERO)
    {
        Vec2 worldPos = pivotPoint;
        Vec2 localPos = _mapSprite->convertToNodeSpace(worldPos);

        Vec2 offsetBefore = localPos * _currentScale;
        Vec2 offsetAfter = localPos * newScale;
        Vec2 positionDelta = offsetAfter - offsetBefore;

        _mapSprite->setScale(newScale);
        _mapSprite->setPosition(oldPosition - positionDelta);
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

    if (_isBuildingMode && !_isDraggingBuilding)
    {
        _dragStartPos = _lastTouchPos;
        _isDraggingBuilding = true;

        if (_ghostSprite && _gridMap)
        {
            // 修改逻辑：计算中心偏移量，使得建筑中心对齐鼠标
            Vec2 rawGridPos = _gridMap->getGridPosition(_dragStartPos);
            Vec2 offset = Vec2((int)((_selectedBuilding.gridSize.width - 1) / 2),
                               (int)((_selectedBuilding.gridSize.height - 1) / 2));
            Vec2 centerAlignedGridPos = rawGridPos - offset;

            Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
            _ghostSprite->setPosition(buildingPos);
            _ghostSprite->setVisible(true);

            bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);
            _gridMap->updateBuildingBase(centerAlignedGridPos, _selectedBuilding.gridSize, canBuild);
            showBuildingHint("拖动调整位置，再次点击确认放置");
        }

        return true;
    }

    if (_isBuildingMode && _isDraggingBuilding)
    {
        // 放置逻辑同样需要应用偏移量
        Vec2 rawGridPos = _gridMap->getGridPosition(_lastTouchPos);
        Vec2 offset =
            Vec2((int)((_selectedBuilding.gridSize.width - 1) / 2), (int)((_selectedBuilding.gridSize.height - 1) / 2));
        Vec2 centerAlignedGridPos = rawGridPos - offset;

        placeBuilding(centerAlignedGridPos);
        _isDraggingBuilding = false;
        return true;
    }

    if (!_isBuildingMode && !_heroManager->getSelectedHeroName().empty())
    {
        _heroManager->handleHeroTouch(_lastTouchPos, _mapSprite, true);
        return true;
    }

    if (!_isBuildingMode)
    {
        _heroManager->handleHeroTouch(_lastTouchPos, _mapSprite, false);
        for (auto& hero : _heroManager->getPlacedHeroes())
        {
            if (hero && hero->containsTouch(_lastTouchPos, _mapSprite))
            {
                break;
            }
        }
    }

    return true;
}

void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    Vec2 currentTouchPos = touch->getLocation();

    if (_isBuildingMode && _isDraggingBuilding && _ghostSprite && _gridMap)
    {
        // 移动逻辑：计算中心偏移量，保持建筑跟手
        Vec2 rawGridPos = _gridMap->getGridPosition(currentTouchPos);
        Vec2 offset =
            Vec2((int)((_selectedBuilding.gridSize.width - 1) / 2), (int)((_selectedBuilding.gridSize.height - 1) / 2));
        Vec2 centerAlignedGridPos = rawGridPos - offset;

        bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);

        _gridMap->updateBuildingBase(centerAlignedGridPos, _selectedBuilding.gridSize, canBuild);

        Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
        _ghostSprite->setPosition(buildingPos);

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

    Vec2 delta = currentTouchPos - _lastTouchPos;
    moveMap(delta);
    _lastTouchPos = currentTouchPos;
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event) {}

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

    if (currentPos.x < _mapBoundary.getMinX())
    {
        newPos.x = _mapBoundary.getMinX();
    }
    else if (currentPos.x > _mapBoundary.getMaxX())
    {
        newPos.x = _mapBoundary.getMaxX();
    }

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

    // 修改：不再显示全图网格
    // _gridMap->showWholeGrid(true);

    _ghostSprite = Sprite::create(building.imageFile);
    if (_ghostSprite)
    {
        _ghostSprite->setOpacity(150);
        _ghostSprite->setAnchorPoint(Vec2(0.5f, 0.2f));
        _ghostSprite->setScale(building.scaleFactor);
        _ghostSprite->setPosition(Vec2(-1000, -1000));
        _mapSprite->addChild(_ghostSprite, 2000);
        showBuildingHint("点击地图开始放置建筑");
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

        auto flashRed = TintTo::create(0.1f, 255, 0, 0);
        auto flashNormal = TintTo::create(0.1f, 255, 255, 255);
        auto shake = MoveBy::create(0.05f, Vec2(5, 0));
        auto shakeBack = MoveBy::create(0.05f, Vec2(-10, 0));
        auto shakeEnd = MoveBy::create(0.05f, Vec2(5, 0));
        auto sequence = Sequence::create(Spawn::create(flashRed, shake, nullptr),
                                         Spawn::create(flashNormal, shakeBack, nullptr), shakeEnd, nullptr);
        _ghostSprite->runAction(sequence);

        showBuildingHint("无法在此处建造！区域被占用或越界");
        return;
    }

    _gridMap->markArea(gridPos, _selectedBuilding.gridSize, true);

    auto building = Sprite::create(_selectedBuilding.imageFile);
    building->setAnchorPoint(Vec2(0.5f, 0.2f));
    building->setScale(_selectedBuilding.scaleFactor);

    Vec2 buildingPos = calculateBuildingPosition(gridPos);
    building->setPosition(buildingPos);
    building->setLocalZOrder(10000 - buildingPos.y);

    _mapSprite->addChild(building);

    building->setScale(0.0f);
    auto scaleAction = EaseBackOut::create(ScaleTo::create(0.4f, _selectedBuilding.scaleFactor));
    auto fadeIn = FadeIn::create(0.3f);
    building->runAction(Spawn::create(scaleAction, fadeIn, nullptr));

    showBuildingHint(StringUtils::format("%s 建造完成！", _selectedBuilding.name.c_str()));

    CCLOG("Building placed: %s at grid (%.0f, %.0f)", _selectedBuilding.name.c_str(), gridPos.x, gridPos.y);

    auto delay = DelayTime::create(1.0f);
    auto callback = CallFunc::create([this]() { endPlacing(); });
    this->runAction(Sequence::create(delay, callback, nullptr));
}

void DraggableMapScene::cancelPlacing()
{
    if (_isDraggingBuilding)
    {
        _isDraggingBuilding = false;

        if (_ghostSprite)
        {
            _ghostSprite->setPosition(Vec2(-1000, -1000));
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
}