#include "MapController.h"
#include "GridMap.h"

USING_NS_CC;

bool MapController::init()
{
    if (!Node::init())
    {
        return false;
    }
    
    _visibleSize = Director::getInstance()->getVisibleSize();
    
    // 默认地图配置
    _mapConfigs["map/Map1.png"] = {1.3f, Vec2(1406.0f, 2107.2f), 55.6f};
    _mapConfigs["map/Map2.png"] = {1.3f, Vec2(1402.0f, 2097.2f), 56.1f};
    _mapConfigs["map/Map3.png"] = {1.3f, Vec2(1403.0f, 2075.2f), 54.9f};
    
    return true;
}

void MapController::setMapNames(const std::vector<std::string>& mapNames)
{
    _mapNames = mapNames;
}

bool MapController::loadMap(const std::string& mapName)
{
    // 如果已加载同一张地图，直接返回
    if (_mapSprite && _currentMapName == mapName)
    {
        return true;
    }
    
    // 创建地图精灵
    auto newMapSprite = Sprite::create(mapName);
    if (!newMapSprite)
    {
        CCLOG("? Failed to load map: %s", mapName.c_str());
        return false;
    }
    
    // 移除旧地图
    if (_mapSprite)
    {
        _mapSprite->removeFromParent();
        _mapSprite = nullptr;
        _gridMap = nullptr;
    }
    
    // 添加新地图
    _mapSprite = newMapSprite;
    _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
    this->addChild(_mapSprite, 0);
    
    _currentMapName = mapName;
    
    // 应用地图配置
    auto it = _mapConfigs.find(mapName);
    if (it != _mapConfigs.end())
    {
        const auto& config = it->second;
        _currentScale = config.scale;
        _mapSprite->setScale(_currentScale);
        initializeGrid(config);
    }
    else
    {
        // 使用默认配置
        MapConfig defaultConfig = {1.3f, Vec2::ZERO, 55.6f};
        initializeGrid(defaultConfig);
    }
    
    updateBoundary();
    
    CCLOG("? Map loaded: %s", mapName.c_str());
    return true;
}

bool MapController::switchMap(const std::string& mapName)
{
    if (mapName == _currentMapName)
    {
        CCLOG("?? Already on map: %s", mapName.c_str());
        return false;
    }
    
    return loadMap(mapName);
}

void MapController::moveMap(const Vec2& delta)
{
    if (!_mapSprite)
        return;
    
    _mapSprite->setPosition(_mapSprite->getPosition() + delta);
    ensureMapInBoundary();
}

void MapController::zoomMap(float scaleFactor, const Vec2& pivotPoint)
{
    if (!_mapSprite)
        return;
    
    float newScale = _currentScale * scaleFactor;
    newScale = MAX(_minScale, MIN(_maxScale, newScale));
    
    if (newScale == _currentScale)
        return;
    
    if (pivotPoint != Vec2::ZERO)
    {
        // 以指定点为中心缩放
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
    ensureMapInBoundary();
}

void MapController::ensureMapInBoundary()
{
    if (!_mapSprite)
        return;
    
    Vec2 currentPos = _mapSprite->getPosition();
    Vec2 newPos = currentPos;
    
    if (currentPos.x < _mapBoundary.getMinX())
        newPos.x = _mapBoundary.getMinX();
    else if (currentPos.x > _mapBoundary.getMaxX())
        newPos.x = _mapBoundary.getMaxX();
    
    if (currentPos.y < _mapBoundary.getMinY())
        newPos.y = _mapBoundary.getMinY();
    else if (currentPos.y > _mapBoundary.getMaxY())
        newPos.y = _mapBoundary.getMaxY();
    
    if (newPos != currentPos)
    {
        _mapSprite->setPosition(newPos);
    }
}

void MapController::setMapConfig(const std::string& mapName, const MapConfig& config)
{
    _mapConfigs[mapName] = config;
}

void MapController::setScaleLimits(float minScale, float maxScale)
{
    _minScale = minScale;
    _maxScale = maxScale;
}

void MapController::updateBoundary()
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
}

void MapController::initializeGrid(const MapConfig& config)
{
    if (!_mapSprite)
        return;
    
    auto mapSize = _mapSprite->getContentSize();
    _gridMap = GridMap::create(mapSize, config.tileSize);
    _mapSprite->addChild(_gridMap, 999);
    
    if (config.startPixel != Vec2::ZERO)
    {
        _gridMap->setStartPixel(config.startPixel);
        _gridStartDefault = config.startPixel;
    }
    else
    {
        _gridStartDefault = _gridMap->getStartPixel();
    }
}
