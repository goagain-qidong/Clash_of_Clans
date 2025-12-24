/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingManager.cpp
 * File Function: 建筑管理器实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "BuildingManager.h"
#include "Managers/UpgradeManager.h"
#include "Managers/TroopInventory.h"
#include "Managers/BuildingLimitManager.h"
#include "Managers/OccupiedGridOverlay.h"
#include "ArmyBuilding.h"
#include "ArmyCampBuilding.h"
#include "BuildersHutBuilding.h"
#include "ResourceBuilding.h"
#include "TownHallBuilding.h"
#include "WallBuilding.h"
#include "DefenseBuilding.h"
#include "GameConfig.h" 
#include "BuildingCapacityManager.h" 
#include "UpgradeTimerUI.h"
#include "Managers/ResourceCollectionManager.h"
#include <map>

USING_NS_CC;
bool BuildingManager::init()
{
    if (!Node::init()) 
        return false;
    // 启用每帧更新
    scheduleUpdate();
    return true;
}
void BuildingManager::setup(cocos2d::Sprite* mapSprite, GridMap* gridMap)
{
    _mapSprite = mapSprite;
    _gridMap = gridMap;
    
    // 创建占用网格覆盖层
    if (_gridMap && !_occupiedGridOverlay)
    {
        _occupiedGridOverlay = OccupiedGridOverlay::create(_gridMap);
        if (_occupiedGridOverlay)
        {
            _occupiedGridOverlay->setVisible(true);
            _mapSprite->addChild(_occupiedGridOverlay, 500);
        }
    }
}
void BuildingManager::startPlacing(const BuildingData& buildingData)
{
    if (!_mapSprite || !_gridMap)
    {
        CCLOG("BuildingManager: Map or grid not set!");
        return;
    }
    _isBuildingMode = true;
    _isDraggingBuilding = false;
    _isWaitingConfirm = false;
    _selectedBuilding = buildingData;
    // 创建虚影精灵
    _ghostSprite = Sprite::create(buildingData.imageFile);
    if (_ghostSprite)
    {
        _ghostSprite->setOpacity(150);
        _ghostSprite->setAnchorPoint(Vec2(0.5f, 0.35f));
        _ghostSprite->setScale(buildingData.scaleFactor);
        _ghostSprite->setPosition(Vec2(-1000.0f, -1000.0f)); // 初始位置在屏幕外
        _mapSprite->addChild(_ghostSprite, 2000);
        showHint("点击地图开始放置建筑");
    }
}
void BuildingManager::cancelPlacing()
{
    endPlacing();
    showHint("已取消建造，点击地图重新选择位置");
}
bool BuildingManager::onTouchBegan(const cocos2d::Vec2& touchPos)
{
    if (!_isBuildingMode || _isDraggingBuilding || _isWaitingConfirm)
        return false;
    _isDraggingBuilding = true;
    if (_ghostSprite && _ghostSprite->getParent() && _gridMap)
    {
        Vec2 rawGridPos = _gridMap->getGridPosition(touchPos);
        int offsetX = static_cast<int>((_selectedBuilding.gridSize.width - 1.0f) / 2.0f);
        int offsetY = static_cast<int>((_selectedBuilding.gridSize.height - 1.0f) / 2.0f);
        Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
        Vec2 centerAlignedGridPos = rawGridPos - offset;
        Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
        _ghostSprite->setPosition(buildingPos);
        _ghostSprite->setVisible(true);
        bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);
        _gridMap->updateBuildingBase(centerAlignedGridPos, _selectedBuilding.gridSize, canBuild);
        showHint("拖动调整位置，松开鼠标后确认");
    }
    return true;
}

void BuildingManager::onTouchMoved(const cocos2d::Vec2& touchPos)
{
    if (!_isBuildingMode || !_isDraggingBuilding || !_ghostSprite || !_gridMap || !_ghostSprite->getParent())
        return;
    Vec2 rawGridPos = _gridMap->getGridPosition(touchPos);
    int offsetX = static_cast<int>((_selectedBuilding.gridSize.width - 1.0f) / 2.0f);
    int offsetY = static_cast<int>((_selectedBuilding.gridSize.height - 1.0f) / 2.0f);
    Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    Vec2 centerAlignedGridPos = rawGridPos - offset;
    bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);
    _gridMap->updateBuildingBase(centerAlignedGridPos, _selectedBuilding.gridSize, canBuild);
    Vec2 buildingPos = calculateBuildingPosition(centerAlignedGridPos);
    _ghostSprite->setPosition(buildingPos);
    _ghostSprite->setColor(canBuild ? Color3B::WHITE : Color3B(255, 100, 100));
}

void BuildingManager::onTouchEnded(const cocos2d::Vec2& touchPos)
{
    if (!_isBuildingMode || !_isDraggingBuilding || !_gridMap || !_ghostSprite || !_ghostSprite->getParent())
        return;
    Vec2 rawGridPos = _gridMap->getGridPosition(touchPos);
    int offsetX = static_cast<int>((_selectedBuilding.gridSize.width - 1.0f) / 2.0f);
    int offsetY = static_cast<int>((_selectedBuilding.gridSize.height - 1.0f) / 2.0f);
    Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    Vec2 centerAlignedGridPos = rawGridPos - offset;
    bool canBuild = _gridMap->checkArea(centerAlignedGridPos, _selectedBuilding.gridSize);
    if (canBuild)
    {
        // 位置合法：进入待确认状态
        _pendingGridPos = centerAlignedGridPos;
        _isDraggingBuilding = false;
        _isWaitingConfirm = true;
        showHint("点击按钮确认或取消建造");
    }
    else
    {
        // 位置非法：播放拒绝动画
        auto flashRed = TintTo::create(0.1f, 255, 0, 0);
        auto flashNormal = TintTo::create(0.1f, 255, 255, 255);
        auto shake = MoveBy::create(0.05f, Vec2(5.0f, 0.0f));
        auto shakeBack = MoveBy::create(0.05f, Vec2(-10.0f, 0.0f));
        auto shakeEnd = MoveBy::create(0.05f, Vec2(5.0f, 0.0f));
        auto sequence = Sequence::create(Spawn::create(flashRed, shake, nullptr),
                                         Spawn::create(flashNormal, shakeBack, nullptr), shakeEnd, nullptr);
        _ghostSprite->runAction(sequence);
        showHint("无法在此处建造！请重新选择位置");
    }
}
void BuildingManager::confirmBuilding()
{
    if (!_isWaitingConfirm)
        return;
    placeBuilding(_pendingGridPos);
}
void BuildingManager::cancelBuilding()
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
    showHint("已取消建造，点击地图重新选择位置");
}
cocos2d::Vec2 BuildingManager::getPendingBuildingWorldPos() const
{
    if (!_mapSprite || !_gridMap)
        return Vec2::ZERO;
    Vec2 buildingPos = calculateBuildingPosition(_pendingGridPos);
    return _mapSprite->convertToWorldSpace(buildingPos);
}
void BuildingManager::placeBuilding(const cocos2d::Vec2& gridPos)
{
    if (!_ghostSprite || !_isBuildingMode || _selectedBuilding.name.empty() || !_gridMap)
        return;
    bool canBuild = _gridMap->checkArea(gridPos, _selectedBuilding.gridSize);
    if (!canBuild)
    {
        showHint("无法在此处建造！区域被占用或越界");
        return;
    }
    
    // ==================== 检查建筑数量限制 ====================
    // 建筑名称映射到 BuildingLimitManager 的键
    std::string limitKey = _selectedBuilding.name;
    if (_selectedBuilding.name == "Town Hall" || _selectedBuilding.name == "大本营") {
        limitKey = "TownHall";
    }
    else if (_selectedBuilding.name == "Wall" || _selectedBuilding.name == "城墙") {
        limitKey = "Wall";
    }
    else if (_selectedBuilding.name == "Builder Hut" || _selectedBuilding.name == "建筑工人小屋") {
        limitKey = "BuildersHut";
    }
    else if (_selectedBuilding.name == "Cannon" || _selectedBuilding.name == "炮塔") {
        limitKey = "Cannon";
    }
    else if (_selectedBuilding.name == "Archer Tower" || _selectedBuilding.name == "箭塔" || _selectedBuilding.name == "ArcherTower") {
        limitKey = "ArcherTower";
    }
    else if (_selectedBuilding.name == "Gold Mine" || _selectedBuilding.name == "金矿" || _selectedBuilding.name == "GoldMine") {
        limitKey = "GoldMine";
    }
    else if (_selectedBuilding.name == "Elixir Collector" || _selectedBuilding.name == "圣水收集器" || _selectedBuilding.name == "ElixirCollector") {
        limitKey = "ElixirCollector";
    }
    else if (_selectedBuilding.name == "Gold Storage" || _selectedBuilding.name == "金币仓库" || _selectedBuilding.name == "GoldStorage") {
        limitKey = "GoldStorage";
    }
    else if (_selectedBuilding.name == "Elixir Storage" || _selectedBuilding.name == "圣水仓库" || _selectedBuilding.name == "ElixirStorage") {
        limitKey = "ElixirStorage";
    }
    else if (_selectedBuilding.name == "Barracks" || _selectedBuilding.name == "兵营") {
        limitKey = "Barracks";
    }
    else if (_selectedBuilding.name == "Army Camp" || _selectedBuilding.name == "军营" || _selectedBuilding.name == "ArmyCamp") {
        limitKey = "ArmyCamp";
    }
    
    // 检查是否可以建造
    auto* limitMgr = BuildingLimitManager::getInstance();
    if (!limitMgr->canBuild(limitKey))
    {
        int currentCount = limitMgr->getBuildingCount(limitKey);
        int maxCount = limitMgr->getLimit(limitKey);
        showHint(StringUtils::format("已达到建造上限！当前: %d/%d", currentCount, maxCount));
        
        endPlacing();
        return;
    }
    
    // ==================== 检查并扣除建造费用 ====================
    auto&        resMgr   = ResourceManager::getInstance();
    int          cost     = _selectedBuilding.cost;
    ResourceType costType = _selectedBuilding.costType;
    if (cost > 0 && !resMgr.consume(costType, cost))
    {
        // 正确处理所有资源类型的名称
        std::string resName;
        switch (costType)
        {
        case ResourceType::kGold:
            resName = "金币";
            break;
        case ResourceType::kElixir:
            resName = "圣水";
            break;
        case ResourceType::kGem:
            resName = "宝石";
            break;
        default:
            resName = "资源";
            break;
        }
        showHint(StringUtils::format("%s不足！需要 %d %s", resName.c_str(), cost, resName.c_str()));

        // 资源不足时，清理虚影和建造状态，防止卡住
        endPlacing();
        return;
    }

    // 1. 标记网格被占用
    _gridMap->markArea(gridPos, _selectedBuilding.gridSize, true);
    
    // 2. 创建建筑实体

    BaseBuilding* building = createBuildingEntity(_selectedBuilding);
    if (!building)
    {
        // 建造失败，清除网格占用并退还资源
        _gridMap->markArea(gridPos, _selectedBuilding.gridSize, false);
        if (cost > 0)
        {
            resMgr.addResource(costType, cost);
        }
        showHint("创建建筑失败！");
        return;
    }
    // 3. 设置建筑属性
    building->setGridPosition(gridPos);
    building->setGridSize(_selectedBuilding.gridSize);
    building->setAnchorPoint(Vec2(0.5f, 0.35f));
    
    // 强制使用 buildingData 的缩放值
    float targetScale = _selectedBuilding.scaleFactor;
    
    Vec2 buildingPos = calculateBuildingPosition(gridPos);
    building->setPosition(buildingPos);
    // 4. 设置动态 Z-Order (Y-Sorting)
    // 使用 10000 - Y 作为 Z-Order，确保始终为正数
    // 例如：Y=100 -> ZOrder=9900, Y=200 -> ZOrder=9800
    // ZOrder 越大越在前面，所以 Y 小的对象会在前面（靠屏幕上方）
    // 这符合 2.5D 游戏的深度逻辑
    building->setLocalZOrder(10000 - static_cast<int>(buildingPos.y));
    _mapSprite->addChild(building);
    // 5. 播放落地动画
    building->setScale(0.0f);
    auto scaleAction = EaseBackOut::create(ScaleTo::create(0.4f, targetScale));
    auto fadeIn = FadeIn::create(0.3f);
    building->runAction(Spawn::create(scaleAction, fadeIn, nullptr));
    // 6. 保存到建筑列表
    _buildings.pushBack(building);
    
    // 记录建筑到BuildingLimitManager
    limitMgr->recordBuilding(limitKey);
    
    // 为新建造的资源生产建筑创建收集UI
    auto* resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
    if (resourceBuilding && resourceBuilding->isProducer())
    {
        resourceBuilding->initCollectionUI();
    }
    
    auto* resBuilding = dynamic_cast<ResourceBuilding*>(building);
    if (resBuilding && resBuilding->isStorage())
    {
        // 注册新建筑 -> 这会自动触发 recalculateCapacity
        BuildingCapacityManager::getInstance().registerOrUpdateBuilding(resBuilding, true);
    }
    
    // 7. 为建筑添加点击监听器
    setupBuildingClickListener(building);
    
    // 8. 触发回调
    if (_onBuildingPlaced)
    {
        _onBuildingPlaced(building);
    }
    
    // 更新占用网格覆盖层（草坪图层）
    updateGrassLayer();
    
    // 9. 🆕 检查是否为城墙且可以继续放置
    bool isWall = (_selectedBuilding.name == "Wall" || _selectedBuilding.name == "城墙");
    bool canContinue = false;
    
    if (isWall)
    {
        // 检查是否还可以继续建造城墙
        if (limitMgr->canBuild("Wall"))
        {
            // 检查是否有足够资源
            if (resMgr.hasEnough(costType, cost))
            {
                canContinue = true;
                showHint("继续放置城墙，按ESC取消");
            }
            else
            {
                showHint(StringUtils::format("资源不足，无法继续建造城墙（已建造 %d/%d）",
                    limitMgr->getBuildingCount("Wall"),
                    limitMgr->getLimit("Wall")));
            }
        }
        else
        {
            showHint(StringUtils::format("已达城墙建造上限（%d/%d）",
                limitMgr->getBuildingCount("Wall"),
                limitMgr->getLimit("Wall")));
        }
    }
    
    // 10. 决定是否继续建造模式
    if (canContinue)
    {
        // 保持建造模式，重置状态以便继续放置
        _isDraggingBuilding = false;
        _isWaitingConfirm = false;
        _pendingGridPos = Vec2::ZERO;
        
        // 保持虚影精灵可见，但移到屏幕外
        if (_ghostSprite)
        {
            _ghostSprite->setPosition(Vec2(-1000.0f, -1000.0f));
            _ghostSprite->setVisible(false);
        }
        
        // 隐藏网格底座
        if (_gridMap)
        {
            _gridMap->hideBuildingBase();
        }
    }
    else
    {
        // 延迟退出建造模式
        auto delay = DelayTime::create(1.0f);
        auto callback = CallFunc::create([this]() { endPlacing(); });
        this->runAction(Sequence::create(delay, callback, nullptr));
    }
}
BaseBuilding* BuildingManager::createBuildingEntity(const BuildingData& buildingData)
{
    if (buildingData.name == "Town Hall" || buildingData.name == "大本营")
    {
        // 创建大本营实体
        return TownHallBuilding::create(1);
    }
    // ============================================================
    else if (buildingData.name == "Gold Mine" || buildingData.name == "金矿")
    {
        return ResourceBuilding::create(ResourceBuildingType::kGoldMine, 1);
    }
    else if (buildingData.name == "Elixir Collector" || buildingData.name == "圣水收集器")
    {
        return ResourceBuilding::create(ResourceBuildingType::kElixirCollector, 1);
    }
    else if (buildingData.name == "Gold Storage" || buildingData.name == "金币仓库")
    {
        return ResourceBuilding::create(ResourceBuildingType::kGoldStorage, 1);
    }
    else if (buildingData.name == "Elixir Storage" || buildingData.name == "圣水仓库")
    {
        return ResourceBuilding::create(ResourceBuildingType::kElixirStorage, 1);
    }
    else if (buildingData.name == "Barracks" || buildingData.name == "兵营")
    {
        return ArmyBuilding::create(1);
    }
    else if (buildingData.name == "Army Camp" || buildingData.name == "军营")
    {
        return ArmyCampBuilding::create(1);
    }
    else if (buildingData.name == "Wall" || buildingData.name == "城墙")
    {
        return WallBuilding::create(1);
    }
    else if (buildingData.name == "Builder Hut" || buildingData.name == "建筑工人小屋")
    {
        return BuildersHutBuilding::create(1);
    }
    // 防御建筑
    else if (buildingData.name == "ArcherTower" || buildingData.name == "Archer Tower" || buildingData.name == "箭塔")
    {
        return DefenseBuilding::create(DefenseType::kArcherTower, 1, buildingData.imageFile);
    }
    else if (buildingData.name == "Cannon" || buildingData.name == "加农炮" || buildingData.name == "炮塔")
    {
        return DefenseBuilding::create(DefenseType::kCannon, 1, buildingData.imageFile);
    }
    else if (buildingData.name == "BuilderHut" || buildingData.name == "BuildersHut")
    {
        return BuildersHutBuilding::create(1);
    }
    else
    {
        CCLOG("BuildingManager: Unknown building type: %s", buildingData.name.c_str());
        return nullptr;
    }
}
cocos2d::Vec2 BuildingManager::calculateBuildingPosition(const cocos2d::Vec2& gridPos) const
{
    if (!_gridMap)
        return Vec2::ZERO;
    Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
    Vec2 posEnd = _gridMap->getPositionFromGrid(
        gridPos + Vec2(_selectedBuilding.gridSize.width - 1, _selectedBuilding.gridSize.height - 1));
    Vec2 centerPos = (posStart + posEnd) / 2.0f;
    return centerPos;
}
void BuildingManager::endPlacing()
{
    _isBuildingMode = false;
    _isDraggingBuilding = false;
    _isWaitingConfirm = false;
    _pendingGridPos = Vec2::ZERO;
    _selectedBuilding = BuildingData("", "", Size::ZERO);
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
void BuildingManager::update(float dt)
{
    UpgradeManager::getInstance()->update(dt);

    // 更新建筑状态和升级UI
    for (auto* building : _buildings)
    {
        if (building)
        {
            building->tick(dt);

            if (building->isUpgrading())
            {
                auto* existingUI = building->getChildByName("upgradeTimerUI");
                if (!existingUI)
                {
                    auto* timerUI = UpgradeTimerUI::create(building);
                    if (timerUI)
                    {
                        timerUI->setName("upgradeTimerUI");
                        building->addChild(timerUI, 9999);
                    }
                }
            }
            // 注意：UpgradeTimerUI 内部检测到非升级状态会自动销毁，
            // 所以这里不需要 else { remove } 分支，
            // 但为了双重保险也可以加上。
        }
    }
}BaseBuilding* BuildingManager::getBuildingAtPosition(const cocos2d::Vec2& touchPos)
{
    if (!_mapSprite)
        return nullptr;
    // 从后往前遍历（后添加的在上层）
    for (auto it = _buildings.rbegin(); it != _buildings.rend(); ++it)
    {
        BaseBuilding* building = *it;
        if (!building || !building->isVisible())
            continue;
        // 将触摸点转换到建筑的本地坐标系
        Vec2 localPos = building->convertToNodeSpace(touchPos);
        Rect rect = Rect(Vec2::ZERO, building->getContentSize());
        if (rect.containsPoint(localPos))
        {
            return building;
        }
    }
    return nullptr;
}
void BuildingManager::showHint(const std::string& hint)
{
    if (_onHint)
    {
        _onHint(hint);
    }
}

void BuildingManager::setupBuildingClickListener(BaseBuilding* building)
{
    /**
     * 为建筑添加点击监听器
     * 注意：不再在这里添加触摸监听器，改为由场景触摸事件转发处理
     * 这样可以避免触摸事件优先级冲突
     */
    if (!building)
        return;

    // 移除旧的监听器（如果有）
    Director::getInstance()->getEventDispatcher()->removeEventListenersForTarget(building);
    
    // 不再添加触摸监听器，改为由 BuildingManager 的 getBuildingAtPosition 和场景转发处理
    // 触摸处理流程：
    // 1. 场景 onTouchBegan -> 调用 BuildingManager::getBuildingAtPosition
    // 2. 如果找到建筑 -> 触发 _onBuildingClicked 回调
    // 3. 场景决定是否启动移动模式或打开升级UI
}

// ==================== 建筑移动相关实现 ====================

void BuildingManager::startMovingBuilding(BaseBuilding* building)
{
    if (!building || !_gridMap || _isMovingBuilding)
        return;

    _isMovingBuilding = true;
    _movingBuilding = building;
    _buildingOriginalGridPos = building->getGridPosition();

    _gridMap->markArea(_buildingOriginalGridPos, building->getGridSize(), false);
    _gridMap->showWholeGrid(true);

    _movingGhostSprite = Sprite::createWithTexture(building->getTexture());
    if (_movingGhostSprite)
    {
        _movingGhostSprite->setOpacity(150);
        _movingGhostSprite->setAnchorPoint(building->getAnchorPoint());
        _movingGhostSprite->setScale(building->getScale());
        _movingGhostSprite->setPosition(building->getPosition());
        _mapSprite->addChild(_movingGhostSprite, 2000);
    }

    building->setVisible(false);
    showHint("拖动调整建筑位置，松开鼠标后确认");
}

void BuildingManager::cancelMovingBuilding()
{
    if (!_isMovingBuilding || !_movingBuilding)
        return;

    if (_gridMap)
    {
        _gridMap->markArea(_buildingOriginalGridPos, _movingBuilding->getGridSize(), true);
        _gridMap->showWholeGrid(false);
        _gridMap->hideBuildingBase();
    }

    _movingBuilding->setVisible(true);

    if (_movingGhostSprite)
    {
        _movingGhostSprite->removeFromParent();
        _movingGhostSprite = nullptr;
    }

    _isMovingBuilding = false;
    _movingBuilding = nullptr;
    _buildingOriginalGridPos = Vec2::ZERO;

    showHint("已取消移动");
}

void BuildingManager::onBuildingTouchMoved(const cocos2d::Vec2& touchPos)
{
    if (!_isMovingBuilding || !_movingBuilding || !_movingGhostSprite || !_gridMap)
        return;

    Vec2 rawGridPos = _gridMap->getGridPosition(touchPos);
    int offsetX = static_cast<int>((_movingBuilding->getGridSize().width - 1.0f) / 2.0f);
    int offsetY = static_cast<int>((_movingBuilding->getGridSize().height - 1.0f) / 2.0f);
    Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    Vec2 centerAlignedGridPos = rawGridPos - offset;

    bool canPlace = _gridMap->checkArea(centerAlignedGridPos, _movingBuilding->getGridSize());

    Vec2 buildingPos = calculateBuildingPositionForMoving(centerAlignedGridPos);
    _movingGhostSprite->setPosition(buildingPos);
    _movingGhostSprite->setColor(canPlace ? Color3B::WHITE : Color3B(255, 100, 100));

    _gridMap->updateBuildingBase(centerAlignedGridPos, _movingBuilding->getGridSize(), canPlace);
}

void BuildingManager::onBuildingTouchEnded(const cocos2d::Vec2& touchPos, BaseBuilding* building)
{
    if (!_isMovingBuilding || !building || !_gridMap || _movingBuilding != building)
        return;

    Vec2 rawGridPos = _gridMap->getGridPosition(touchPos);
    int offsetX = static_cast<int>((building->getGridSize().width - 1.0f) / 2.0f);
    int offsetY = static_cast<int>((building->getGridSize().height - 1.0f) / 2.0f);
    Vec2 offset = Vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    Vec2 newGridPos = rawGridPos - offset;

    bool canPlace = _gridMap->checkArea(newGridPos, building->getGridSize());

    if (canPlace)
    {
        building->setGridPosition(newGridPos);
        Vec2 newPos = calculateBuildingPositionForMoving(newGridPos);
        building->setPosition(newPos);
        building->setLocalZOrder(10000 - static_cast<int>(newPos.y));

        _gridMap->markArea(newGridPos, building->getGridSize(), true);

        showHint(StringUtils::format("%s 已移动到新位置", building->getDisplayName().c_str()));

        if (_onBuildingMoved)
            _onBuildingMoved(building, newGridPos);
        
        updateGrassLayer();
        confirmBuildingMove();
    }
    else
    {
        cancelMovingBuilding();
        showHint("无法在该位置放置建筑，已恢复原位置");
    }
}

void BuildingManager::confirmBuildingMove()
{
    /**
     * 确认建筑移动完成
     */
    if (!_isMovingBuilding || !_movingBuilding)
        return;

    // 显示建筑
    _movingBuilding->setVisible(true);

    // 移除幽灵精灵
    if (_movingGhostSprite)
    {
        _movingGhostSprite->removeFromParent();
        _movingGhostSprite = nullptr;
    }

    // 隐藏网格
    if (_gridMap)
    {
        _gridMap->showWholeGrid(false);
        _gridMap->hideBuildingBase();
    }

    _isMovingBuilding = false;
    _movingBuilding = nullptr;
}

void BuildingManager::showOccupiedGrids(bool autoFadeOut)
{
    if (!_occupiedGridOverlay)
        return;
    
    this->stopAllActions();
    _occupiedGridOverlay->showOccupiedGrids(_buildings);
}

void BuildingManager::hideOccupiedGrids()
{
    if (!_occupiedGridOverlay)
        return;
    _occupiedGridOverlay->fadeOutAndHide(0.5f);
}

void BuildingManager::updateGrassLayer()
{
    if (!_occupiedGridOverlay)
        return;
    _occupiedGridOverlay->updateGrassLayer(_buildings);
}

cocos2d::Vec2 BuildingManager::calculateBuildingPositionForMoving(const cocos2d::Vec2& gridPos) const
{
    /**
     * 计算移动建筑时的世界位置
     */
    if (!_gridMap || !_movingBuilding)
        return Vec2::ZERO;

    Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
    Vec2 posEnd = _gridMap->getPositionFromGrid(
        gridPos + Vec2(_movingBuilding->getGridSize().width - 1, _movingBuilding->getGridSize().height - 1));
    Vec2 centerPos = (posStart + posEnd) / 2.0f;
    return centerPos;
}

// ==================== Serialization / Multiplayer Support ====================

#include "AccountManager.h"

std::vector<BuildingSerialData> BuildingManager::serializeBuildings() const
{
    std::vector<BuildingSerialData> result;
    
    for (auto* building : _buildings)
    {
        if (!building)
            continue;
            
        BuildingSerialData data;
        data.name = building->getDisplayName();
        data.level = building->getLevel();
        data.gridX = building->getGridPosition().x;
        data.gridY = building->getGridPosition().y;
        data.gridWidth = building->getGridSize().width;
        data.gridHeight = building->getGridSize().height;
        
        result.push_back(data);
    }
    
    return result;
}

void BuildingManager::loadBuildingsFromData(const std::vector<BuildingSerialData>& buildingsData, bool isReadOnly)
{
    if (!_mapSprite || !_gridMap)
        return;
    
    _isReadOnlyMode = isReadOnly;
    clearAllBuildings(!isReadOnly);
    
    for (const auto& data : buildingsData)
    {
        BaseBuilding* building = createBuildingFromSerialData(data);
        
        if (!building)
        {
            CCLOG("⚠️ Failed to create building: %s", data.name.c_str());
            continue;
        }
        
        // 设置建筑属性
        Vec2 gridPos(data.gridX, data.gridY);
        Size gridSize(data.gridWidth, data.gridHeight);
        
        building->setGridPosition(gridPos);
        building->setGridSize(gridSize);
        building->setAnchorPoint(Vec2(0.5f, 0.35f));
        
        // 计算并设置位置
        Vec2 posStart = _gridMap->getPositionFromGrid(gridPos);
        Vec2 posEnd = _gridMap->getPositionFromGrid(gridPos + Vec2(gridSize.width - 1, gridSize.height - 1));
        Vec2 centerPos = (posStart + posEnd) / 2.0f;
        building->setPosition(centerPos);
        
        // 设置 Z-Order
        building->setLocalZOrder(10000 - static_cast<int>(centerPos.y));
        
        // 添加到地图
        _mapSprite->addChild(building);
        _buildings.pushBack(building);
        
        // 标记网格占用
        _gridMap->markArea(gridPos, gridSize, true);
        
        // 记录建筑到BuildingLimitManager（只在非只读模式下）
        if (!isReadOnly)
        {
            // 非只读模式：为资源建筑创建收集UI
            auto* resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
            if (resourceBuilding)
            {
                if (resourceBuilding->isProducer())
                {
                    resourceBuilding->initCollectionUI();
                }
                else if (resourceBuilding->isStorage())
                {
                    BuildingCapacityManager::getInstance().registerOrUpdateBuilding(resourceBuilding, true);
                }
            }
            
            // 移除等级后缀
            std::string rawName = data.name;
            size_t lvPos = rawName.find(" (Lv.");
            if (lvPos == std::string::npos)
            {
                lvPos = rawName.find(" Lv.");
            }
            if (lvPos != std::string::npos)
            {
                rawName = rawName.substr(0, lvPos);
            }
            
            // 建筑名称映射到 BuildingLimitManager 的键
            std::string limitKey = rawName;
            if (rawName.find("Town Hall") != std::string::npos || rawName.find("大本营") != std::string::npos) {
                limitKey = "TownHall";
            }
            else if (rawName.find("Wall") != std::string::npos || rawName.find("城墙") != std::string::npos) {
                limitKey = "Wall";
            }
            else if (rawName.find("Builder") != std::string::npos || rawName.find("建筑工人") != std::string::npos) {
                limitKey = "BuildersHut";
            }
            else if (rawName.find("Cannon") != std::string::npos || rawName.find("炮塔") != std::string::npos || rawName.find("加农炮") != std::string::npos) {
                limitKey = "Cannon";
            }
            else if (rawName.find("Archer Tower") != std::string::npos || rawName.find("箭塔") != std::string::npos) {
                limitKey = "ArcherTower";
            }
            else if (rawName.find("Gold Mine") != std::string::npos || rawName.find("金矿") != std::string::npos) {
                limitKey = "GoldMine";
            }
            else if (rawName.find("Elixir Collector") != std::string::npos || rawName.find("圣水收集器") != std::string::npos) {
                limitKey = "ElixirCollector";
            }
            else if (rawName.find("Gold Storage") != std::string::npos || rawName.find("金币仓库") != std::string::npos) {
                limitKey = "GoldStorage";
            }
            else if (rawName.find("Elixir Storage") != std::string::npos || rawName.find("圣水仓库") != std::string::npos) {
                limitKey = "ElixirStorage";
            }
            else if (rawName.find("Barracks") != std::string::npos || rawName.find("兵营") != std::string::npos) {
                limitKey = "Barracks";
            }
            else if (rawName.find("Army Camp") != std::string::npos || rawName.find("军营") != std::string::npos) {
                limitKey = "ArmyCamp";
            }
            
            BuildingLimitManager::getInstance()->recordBuilding(limitKey);
            
            setupBuildingClickListener(building);
        }
    }
    
    // 更新草坪图层
    if (!isReadOnly)
    {
        updateGrassLayer();
    }
}

void BuildingManager::clearAllBuildings(bool clearTroops)
{
    if (!_gridMap)
        return;
    
    // 清除网格占用
    for (auto* building : _buildings)
    {
        if (building)
        {
            _gridMap->markArea(building->getGridPosition(), building->getGridSize(), false);
        }
    }
    
    // 非只读模式下清除单例管理器状态
    if (!_isReadOnlyMode)
    {
        UpgradeManager::getInstance()->clearAllUpgradeTasks();
        
        ResourceCollectionManager::getInstance()->clearRegisteredBuildings();
        BuildingCapacityManager::getInstance().clearAllBuildings();
        BuildingLimitManager::getInstance()->reset();
    }
    
    if (clearTroops)
    {
        TroopInventory::getInstance().clearAll();
        auto& resMgr = ResourceManager::getInstance();
        resMgr.setResourceCapacity(ResourceType::kTroopPopulation, 0);
        resMgr.setResourceCount(ResourceType::kTroopPopulation, 0);
    }
    
    _buildings.clear();
}

void BuildingManager::saveCurrentState()
{
    auto& accMgr = AccountManager::getInstance();
    auto gameData = accMgr.getCurrentGameData();
    
    gameData.buildings = serializeBuildings();
    
    auto& resMgr = ResourceManager::getInstance();
    gameData.gold = resMgr.getResourceCount(ResourceType::kGold);
    gameData.elixir = resMgr.getResourceCount(ResourceType::kElixir);
    gameData.darkElixir = 0;
    gameData.gems = resMgr.getResourceCount(ResourceType::kGem);
    gameData.goldCapacity = resMgr.getResourceCapacity(ResourceType::kGold);
    gameData.elixirCapacity = resMgr.getResourceCapacity(ResourceType::kElixir);
    
    auto& troopInv = TroopInventory::getInstance();
    gameData.troopInventory = troopInv.toJson();
    
    for (auto* building : _buildings)
    {
        if (building && building->getBuildingType() == BuildingType::kTownHall)
        {
            gameData.townHallLevel = building->getLevel();
            break;
        }
    }
    
    accMgr.updateGameData(gameData);
}

void BuildingManager::loadCurrentAccountState()
{
    auto& accMgr = AccountManager::getInstance();
    auto gameData = accMgr.getCurrentGameData();
    auto& resMgr = ResourceManager::getInstance();

    // 1. 加载建筑
    loadBuildingsFromData(gameData.buildings, false);

    // 2. 恢复保存的容量
    if (gameData.goldCapacity > 0 || gameData.elixirCapacity > 0)
    {
        resMgr.setResourceCapacity(ResourceType::kGold, gameData.goldCapacity);
        resMgr.setResourceCapacity(ResourceType::kElixir, gameData.elixirCapacity);
    }
    else
    {
        BuildingCapacityManager::getInstance().recalculateCapacity();
    }

    // 3. 加载士兵库存
    auto& troopInv = TroopInventory::getInstance();
    if (!gameData.troopInventory.empty())
    {
        troopInv.fromJson(gameData.troopInventory);
        restoreArmyCampTroopDisplays();
    }
    else
    {
        troopInv.clearAll();
    }
    
    // 4. 加载资源数量
    resMgr.setResourceCount(ResourceType::kGold, gameData.gold);
    resMgr.setResourceCount(ResourceType::kElixir, gameData.elixir);
    resMgr.setResourceCount(ResourceType::kGem, gameData.gems);
}

bool BuildingManager::loadPlayerBase(const std::string& userId)
{
    auto& accMgr = AccountManager::getInstance();
    auto gameData = accMgr.getPlayerGameData(userId);
    
    if (gameData.buildings.empty())
        return false;
    
    loadBuildingsFromData(gameData.buildings, false);
    return true;
}

void BuildingManager::restoreArmyCampTroopDisplays()
{
    auto& troopInv = TroopInventory::getInstance();
    
    std::vector<ArmyCampBuilding*> armyCamps;
    for (auto* building : _buildings)
    {
        auto* armyCamp = dynamic_cast<ArmyCampBuilding*>(building);
        if (armyCamp)
            armyCamps.push_back(armyCamp);
    }
    
    if (armyCamps.empty())
        return;
    
    // 清空旧显示
    for (auto* armyCamp : armyCamps)
        armyCamp->clearTroopDisplays();
    
    const std::vector<UnitType> unitTypes = {
        UnitType::kBarbarian,
        UnitType::kArcher,
        UnitType::kGiant,
        UnitType::kGoblin,
        UnitType::kWallBreaker
    };
    
    int armyCampIndex = 0;
    
    for (auto unitType : unitTypes)
    {
        int count = troopInv.getTroopCount(unitType);
        
        for (int i = 0; i < count; ++i)
        {
            if (armyCampIndex >= static_cast<int>(armyCamps.size()))
                armyCampIndex = 0;
            
            armyCamps[armyCampIndex]->addTroopDisplay(unitType);
            
            if ((i + 1) % 5 == 0)
                armyCampIndex = (armyCampIndex + 1) % static_cast<int>(armyCamps.size());
        }
    }
}

BaseBuilding* BuildingManager::createBuildingFromSerialData(const BuildingSerialData& data)
{
    std::string name = data.name;
    int level = data.level;
    
    // 移除等级后缀
    size_t lvPos = name.find(" (Lv.");
    if (lvPos == std::string::npos)
        lvPos = name.find(" Lv.");
    if (lvPos != std::string::npos)
        name = name.substr(0, lvPos);
    
    // 移除括号残留
    size_t bracketPos = name.find(" (");
    if (bracketPos != std::string::npos)
        name = name.substr(0, bracketPos);
    
    // 根据名称创建建筑
    if (name.find("Town Hall") != std::string::npos || name.find("大本营") != std::string::npos)
    {
        return TownHallBuilding::create(level);
    }
    else if (name.find("Gold Mine") != std::string::npos || name.find("金矿") != std::string::npos)
    {
        return ResourceBuilding::create(ResourceBuildingType::kGoldMine, level);
    }
    else if (name.find("Elixir Collector") != std::string::npos || name.find("圣水收集器") != std::string::npos)
    {
        return ResourceBuilding::create(ResourceBuildingType::kElixirCollector, level);
    }
    else if (name.find("Gold Storage") != std::string::npos || name.find("金币仓库") != std::string::npos)
    {
        return ResourceBuilding::create(ResourceBuildingType::kGoldStorage, level);
    }
    else if (name.find("Elixir Storage") != std::string::npos || name.find("圣水仓库") != std::string::npos)
    {
        return ResourceBuilding::create(ResourceBuildingType::kElixirStorage, level);
    }
    else if (name.find("Barracks") != std::string::npos || name.find("兵营") != std::string::npos)
    {
        return ArmyBuilding::create(level);
    }
    else if (name.find("Army Camp") != std::string::npos || name.find("军营") != std::string::npos)
    {
        return ArmyCampBuilding::create(level);
    }
    else if (name.find("Wall") != std::string::npos || name.find("城墙") != std::string::npos)
    {
        return WallBuilding::create(level);
    }
    else if (name.find("Builder") != std::string::npos || name.find("建筑工人") != std::string::npos)
    {
        return BuildersHutBuilding::create(level);
    }
    else if (name.find("Archer Tower") != std::string::npos || name.find("箭塔") != std::string::npos)
    {
        std::string imagePath = StringUtils::format("buildings/ArcherTower/Archer_Tower%d.png", level);
        return DefenseBuilding::create(DefenseType::kArcherTower, level, imagePath);
    }
    else if (name.find("Cannon") != std::string::npos || name.find("加农炮") != std::string::npos)
    {
        std::string imagePath = StringUtils::format("buildings/Cannon_Static/Cannon%d.png", level);
        return DefenseBuilding::create(DefenseType::kCannon, level, imagePath);
    }
    else
    {
        return nullptr;
    }
}