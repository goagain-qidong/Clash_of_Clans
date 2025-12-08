/**
 * @file DraggableMapScene.cpp
 * @brief 主场景实现 - 重构后的精简版本
 */

#include "DraggableMapScene.h"
#include "MapController.h"
#include "SceneUIController.h"
#include "InputController.h"
#include "BuildingManager.h"
#include "AccountManager.h"
#include "BuildingUpgradeUI.h"
#include "BattleScene.h"
#include "HUDLayer.h"
#include "ShopLayer.h"
#include "ResourceManager.h"
#include "SocketClient.h"
#include "BuildingData.h"
#include "BaseBuilding.h"
#include "Buildings/ArmyBuilding.h"
#include "Unit/unit.h"

USING_NS_CC;

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
    
    initializeManagers();
    setupCallbacks();
    
    connectToServer();
    setupNetworkCallbacks();
    
    scheduleUpdate();
    
    // 延迟加载游戏状态
    this->scheduleOnce([this](float dt) {
        loadGameState();
    }, 0.1f, "load_game_state");

    return true;
}

void DraggableMapScene::initializeManagers()
{
    // ==================== 获取当前账号分配的地图 ====================
    std::string assignedMap = "map/Map1.png"; // 默认地图
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (currentAccount && !currentAccount->assignedMapName.empty()) {
        assignedMap = currentAccount->assignedMapName;
        CCLOG("✅ Loading assigned map for account %s: %s", 
              currentAccount->username.c_str(), assignedMap.c_str());
    }
    
    // ==================== 地图控制器 ====================
    _mapController = MapController::create();
    this->addChild(_mapController, 0);
    _mapController->setMapNames({assignedMap}); // 只设置当前账号的地图
    _mapController->loadMap(assignedMap);
    
    // ==================== 建筑管理器 ====================
    _buildingManager = BuildingManager::create();
    this->addChild(_buildingManager);
    _buildingManager->setup(_mapController->getMapSprite(), _mapController->getGridMap());
    
    // ==================== UI 控制器 ====================
    _uiController = SceneUIController::create();
    this->addChild(_uiController, 100);
    
    initBuildingData();
    
    // ==================== 输入控制器 ====================
    _inputController = InputController::create();
    this->addChild(_inputController);
    
    // ==================== HUD ====================
    _hudLayer = HUDLayer::create();
    this->addChild(_hudLayer, 100);
}

void DraggableMapScene::setupCallbacks()
{
    // ==================== 输入回调 ====================
    _inputController->setOnTouchBegan([this](Touch* touch, Event* event) {
        return onTouchBegan(touch, event);
    });
    
    _inputController->setOnTouchMoved([this](Touch* touch, Event* event) {
        onTouchMoved(touch, event);
    });
    
    _inputController->setOnTouchEnded([this](Touch* touch, Event* event) {
        onTouchEnded(touch, event);
    });
    
    _inputController->setOnMouseScroll([this](float scrollY, Vec2 mousePos) {
        onMouseScroll(scrollY, mousePos);
    });
    
    _inputController->setOnKeyPressed([this](EventKeyboard::KeyCode keyCode) {
        onKeyPressed(keyCode);
    });
    
    // ==================== UI 回调 ====================
    _uiController->setOnShopClicked([this]() {
        onShopClicked();
    });
    
    _uiController->setOnAttackClicked([this]() {
        onAttackClicked();
    });
    
    _uiController->setOnClanClicked([this]() {
        onClanClicked();
    });
    
    _uiController->setOnBuildingSelected([this](const BuildingData& data) {
        onBuildingSelected(data);
    });
    
    _uiController->setOnConfirmBuilding([this]() {
        onConfirmBuilding();
    });
    
    _uiController->setOnCancelBuilding([this]() {
        onCancelBuilding();
    });
    
    _uiController->setOnAccountSwitched([this]() {
        onAccountSwitched();
    });
    
    _uiController->setOnLogout([this]() {
        onLogout();
    });
    
    // ==================== 建筑管理器回调 ====================
    _buildingManager->setOnBuildingPlaced([this](BaseBuilding* building) {
        onBuildingPlaced(building);
    });
    
    _buildingManager->setOnBuildingClicked([this](BaseBuilding* building) {
        onBuildingClicked(building);
    });
    
    _buildingManager->setOnHint([this](const std::string& hint) {
        onBuildingHint(hint);
    });
}

void DraggableMapScene::initBuildingData()
{
    std::vector<BuildingData> buildingList;
    
    buildingList.push_back(BuildingData("TownHall", "buildings/BaseCamp/town-hall-1.png", Size(5, 5), 1.0f, 0, 0, ResourceType::kGold));
    buildingList.push_back(BuildingData("ArcherTower", "buildings/ArcherTower/Archer_Tower1.png", Size(3, 3), 0.8f, 1000, 60, ResourceType::kGold));
    buildingList.push_back(BuildingData("Cannon", "buildings/Cannon_Static/Cannon1.png", Size(3, 3), 0.8f, 500, 30, ResourceType::kGold));
    buildingList.push_back(BuildingData("Wall", "buildings/Wall/Wall1.png", Size(1, 1), 0.6f, 50, 0, ResourceType::kGold));
    buildingList.push_back(BuildingData("Barracks", "buildings/Barracks/Barracks1.png", Size(4, 4), 0.6f, 1500, 120, ResourceType::kElixir));
    buildingList.push_back(BuildingData("ArmyCamp", "buildings/ArmyCamp/Army_Camp1.png", Size(4, 4), 0.8f, 250, 0, ResourceType::kElixir));
    buildingList.push_back(BuildingData("GoldMine", "buildings/GoldMine/Gold_Mine1.png", Size(3, 3), 0.8f, 800, 45, ResourceType::kGold));
    buildingList.push_back(BuildingData("ElixirCollector", "buildings/ElixirCollector/Elixir_Collector1.png", Size(3, 3), 0.8f, 750, 40, ResourceType::kGold));
    buildingList.push_back(BuildingData("GoldStorage", "buildings/GoldStorage/Gold_Storage1.png", Size(3, 3), 0.8f, 1000, 30, ResourceType::kGold));
    buildingList.push_back(BuildingData("ElixirStorage", "buildings/ElixirStorage/Elixir_Storage1.png", Size(3, 3), 0.8f, 1000, 30, ResourceType::kGold));
    buildingList.push_back(BuildingData("BuilderHut", "buildings/BuildersHut/Builders_Hut1.png", Size(2, 2), 0.7f, 0, 0, ResourceType::kGold));
    
    _uiController->setBuildingList(buildingList);
}

void DraggableMapScene::loadGameState()
{
    if (_buildingManager)
    {
        _buildingManager->loadCurrentAccountState();
        CCLOG("? Game state loaded");
    }
}

// ==================== 输入处理 ====================

bool DraggableMapScene::onTouchBegan(Touch* touch, Event* event)
{
    Vec2 touchPos = touch->getLocation();
    
    // 【优先级1】升级UI
    if (_currentUpgradeUI && _currentUpgradeUI->isVisible())
    {
        Vec2 localPos = _currentUpgradeUI->convertTouchToNodeSpace(touch);
        Rect bbox = _currentUpgradeUI->getBoundingBox();
        bbox.origin = Vec2::ZERO;
        
        if (bbox.containsPoint(localPos))
        {
            return true;  // UI 内部处理
        }
        else
        {
            hideUpgradeUI();
            return false;
        }
    }
    
    // 【优先级2】建筑移动模式（高于建造模式）
    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        // 建筑移动模式下，不处理场景触摸
        return false;
    }
    
    // 【优先级3】建筑建造模式
    if (_buildingManager && _buildingManager->isInBuildingMode())
    {
        if (!_buildingManager->isDraggingBuilding() && !_buildingManager->isWaitingConfirm())
        {
            _buildingManager->onTouchBegan(touchPos);
            return true;
        }
    }
    
    // 【优先级4】建筑点击检测（新增）
    if (_buildingManager)
    {
        BaseBuilding* clickedBuilding = _buildingManager->getBuildingAtPosition(touchPos);
        if (clickedBuilding)
        {
            // 记录点击的建筑和触摸起点，用于后续判断是点击还是拖动
            _clickedBuilding = clickedBuilding;
            _touchBeganPos = touchPos;
            _touchBeganTime = Director::getInstance()->getTotalFrames() / 60.0f;
            _hasMoved = false;
            return true;
        }
    }
    
    // 【优先级5】地图操作（最低）
    _clickedBuilding = nullptr;
    return true;
}

void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    Vec2 currentPos = touch->getLocation();
    Vec2 previousPos = touch->getPreviousLocation();
    Vec2 delta = currentPos - previousPos;
    
    // 检测是否移动了足够距离
    if (_clickedBuilding)
    {
        float distance = currentPos.distance(_touchBeganPos);
        
        // 如果移动距离超过10像素，标记为已移动
        if (distance > 10.0f)
        {
            _hasMoved = true;
            
            // 如果移动超过30像素，进入建筑移动模式
            if (distance > 30.0f && _buildingManager && !_buildingManager->isMovingBuilding() && !_buildingManager->isInBuildingMode())
            {
                _buildingManager->startMovingBuilding(_clickedBuilding);
                _clickedBuilding = nullptr; // 清空，后续由 BuildingManager 处理
                return;
            }
        }
    }
    
    // 【优先级1】建筑移动模式
    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        _buildingManager->onBuildingTouchMoved(currentPos);
        return;
    }
    
    // 【优先级2】建筑建造模式
    if (_buildingManager && _buildingManager->isInBuildingMode() && _buildingManager->isDraggingBuilding())
    {
        _buildingManager->onTouchMoved(currentPos);
        return;
    }
    
    // 【优先级3】地图平移
    if (!_clickedBuilding || _hasMoved)
    {
        _mapController->moveMap(delta);
    }
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event)
{
    Vec2 touchPos = touch->getLocation();
    
    // 【优先级1】建筑移动模式
    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        // 建筑移动由 BuildingManager 内部处理
        BaseBuilding* movingBuilding = _buildingManager->getMovingBuilding();
        if (movingBuilding)
        {
            _buildingManager->onBuildingTouchEnded(touchPos, movingBuilding);
        }
        _clickedBuilding = nullptr;
        _hasMoved = false;
        return;
    }
    
    // 【优先级2】建筑建造模式
    if (_buildingManager && _buildingManager->isInBuildingMode() && _buildingManager->isDraggingBuilding())
    {
        _buildingManager->onTouchEnded(touchPos);
        
        if (_buildingManager->isWaitingConfirm())
        {
            Vec2 worldPos = _buildingManager->getPendingBuildingWorldPos();
            _uiController->showConfirmButtons(worldPos);
        }
        _clickedBuilding = nullptr;
        _hasMoved = false;
        return;
    }
    
    // 【优先级3】建筑点击（单击）
    if (_clickedBuilding && !_hasMoved && !_buildingManager->isInBuildingMode())
    {
        // 触发建筑点击回调
        onBuildingClicked(_clickedBuilding);
        _clickedBuilding = nullptr;
        _hasMoved = false;
        return;
    }
    
    // 重置状态
    _clickedBuilding = nullptr;
    _hasMoved = false;
}

void DraggableMapScene::onMouseScroll(float scrollY, Vec2 mousePos)
{
    float zoomFactor = scrollY > 0 ? 0.9f : 1.1f;
    _mapController->zoomMap(zoomFactor, mousePos);
}

void DraggableMapScene::onKeyPressed(EventKeyboard::KeyCode keyCode)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        if (_buildingManager && _buildingManager->isInBuildingMode())
        {
            _buildingManager->cancelPlacing();
            _uiController->hideConfirmButtons();
        }
    }
}

// ==================== UI 回调 ====================

void DraggableMapScene::onShopClicked()
{
    if (_buildingManager && _buildingManager->isInBuildingMode())
        return;
    
    auto shop = ShopLayer::create();
    this->addChild(shop, 200);
    shop->show();
}

void DraggableMapScene::onAttackClicked()
{
    // 保存当前状态
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved current base before attacking");
    }
    
    // 查找其他账号
    auto& accMgr = AccountManager::getInstance();
    const auto& allAccounts = accMgr.listAccounts();
    const auto* currentAccount = accMgr.getCurrentAccount();
    
    if (!currentAccount)
    {
        _uiController->showHint("错误：未登录账号！");
        return;
    }
    
    // 查找第一个不是当前账号的玩家
    std::string targetUserId = "";
    for (const auto& account : allAccounts)
    {
        if (account.userId != currentAccount->userId)
        {
            targetUserId = account.userId;
            break;
        }
    }
    
    if (targetUserId.empty())
    {
        _uiController->showHint("测试模式：请先创建第二个账号！");
        return;
    }
    
    // 加载目标玩家的基地数据
    auto enemyGameData = accMgr.getPlayerGameData(targetUserId);
    
    if (enemyGameData.buildings.empty())
    {
        _uiController->showHint(StringUtils::format("玩家 %s 还没有建筑！", targetUserId.c_str()));
        return;
    }
    
    // 进入战斗场景
    CCLOG("✅ Attacking player: %s (TH Level=%d, Buildings=%zu)", 
          targetUserId.c_str(), enemyGameData.townHallLevel, enemyGameData.buildings.size());
    
    auto battleScene = BattleScene::createWithEnemyData(enemyGameData);
    if (battleScene)
    {
        Director::getInstance()->pushScene(TransitionFade::create(0.3f, battleScene));
    }
    else
    {
        _uiController->showHint("创建战斗场景失败！");
    }
}

void DraggableMapScene::onClanClicked()
{
    _uiController->showHint("部落系统开发中，敬请期待！");
}

void DraggableMapScene::onBuildingSelected(const BuildingData& data)
{
    if (_buildingManager)
    {
        _buildingManager->startPlacing(data);
    }
}

void DraggableMapScene::onConfirmBuilding()
{
    if (_buildingManager)
    {
        _buildingManager->confirmBuilding();
    }
    _uiController->hideConfirmButtons();
}

void DraggableMapScene::onCancelBuilding()
{
    if (_buildingManager)
    {
        _buildingManager->cancelBuilding();
    }
    _uiController->hideConfirmButtons();
    _uiController->showHint("已取消建造，点击地图重新选择位置");
}

// ==================== 建筑回调 ====================

void DraggableMapScene::onBuildingPlaced(BaseBuilding* building)
{
    if (!building)
        return;
    
    CCLOG("Building placed: %s", building->getDisplayName().c_str());
    
    // 检查是否为兵营建筑
    if (building->getBuildingType() == BuildingType::kArmy)
    {
        auto barracks = dynamic_cast<ArmyBuilding*>(building);
        if (barracks)
        {
            barracks->setOnTrainingComplete([this, barracks](Unit* unit) {
                if (!unit) return;
                
                Vec2 barracksWorldPos = barracks->getParent()->convertToWorldSpace(barracks->getPosition());
                Vec2 spawnPos = barracksWorldPos;
                spawnPos.x += barracks->getContentSize().width * barracks->getScale() + 20;
                
                Vec2 spawnLocalPos = _mapController->getMapSprite()->convertToNodeSpace(spawnPos);
                unit->setPosition(spawnLocalPos);
                
                _mapController->getMapSprite()->addChild(unit, 100);
                unit->PlayAnimation(UnitAction::kIdle, UnitDirection::kRight);
                
                CCLOG("?? Unit training complete!");
                _uiController->showHint("士兵训练完成！");
            });
        }
    }
    
    _uiController->hideConfirmButtons();
}

void DraggableMapScene::onBuildingClicked(BaseBuilding* building)
{
    if (!building)
        return;
    
    showUpgradeUI(building);
}

void DraggableMapScene::onBuildingHint(const std::string& hint)
{
    _uiController->showHint(hint);
}

// ==================== 升级UI ====================

void DraggableMapScene::showUpgradeUI(BaseBuilding* building)
{
    hideUpgradeUI();
    
    auto upgradeUI = BuildingUpgradeUI::create(building);
    if (upgradeUI)
    {
        upgradeUI->setPositionNearBuilding(building);
        
        upgradeUI->setUpgradeCallback([this, building](bool success, int newLevel) {
            if (success)
            {
                _uiController->showHint(
                    StringUtils::format("%s 升级到 %d 级！", building->getDisplayName().c_str(), newLevel));
            }
            else
            {
                _uiController->showHint("资源不足，无法升级！");
            }
        });
        
        upgradeUI->setCloseCallback([this]() {
            _currentUpgradeUI = nullptr;
        });
        
        this->addChild(upgradeUI, 1000);
        upgradeUI->show();
        _currentUpgradeUI = upgradeUI;
    }
}

void DraggableMapScene::hideUpgradeUI()
{
    if (!_currentUpgradeUI)
        return;
    
    auto upgradeUI = dynamic_cast<BuildingUpgradeUI*>(_currentUpgradeUI);
    if (upgradeUI)
    {
        upgradeUI->hide();
    }
    else
    {
        _currentUpgradeUI->removeFromParent();
    }
    _currentUpgradeUI = nullptr;
}

void DraggableMapScene::closeUpgradeUI()
{
    hideUpgradeUI();
}

void DraggableMapScene::cleanupUpgradeUI()
{
    if (_currentUpgradeUI)
    {
        if (_currentUpgradeUI->getParent() == this)
        {
            _currentUpgradeUI->removeFromParent();
        }
        _currentUpgradeUI = nullptr;
    }
}

// ==================== 网络 ====================

void DraggableMapScene::connectToServer()
{
    auto& client = SocketClient::getInstance();
    
    if (!client.isConnected())
    {
        bool connected = client.connect("127.0.0.1", 8888);
        
        if (connected)
        {
            auto account = AccountManager::getInstance().getCurrentAccount();
            if (account)
            {
                client.login(account->userId, account->username, 1000);
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
            CCLOG("Login successful!");
        }
        else
        {
            CCLOG("Login failed: %s", msg.c_str());
        }
    });
    
    client.setOnDisconnected([]() {
        CCLOG("Disconnected from server!");
    });
}

// ==================== ShopLayer 接口 ====================

int DraggableMapScene::getTownHallLevel() const
{
    if (!_buildingManager)
        return 1;
    
    const auto& buildings = _buildingManager->getBuildings();
    for (auto* b : buildings)
    {
        if (b->getBuildingType() == BuildingType::kTownHall)
        {
            return b->getLevel();
        }
    }
    
    return 1;
}

int DraggableMapScene::getBuildingCount(const std::string& name) const
{
    if (!_buildingManager)
        return 0;
    
    int count = 0;
    const auto& buildings = _buildingManager->getBuildings();
    
    for (auto* b : buildings)
    {
        if (name == "Town Hall" || name == "大本营")
        {
            if (b->getBuildingType() == BuildingType::kTownHall)
            {
                count++;
            }
            continue;
        }
        
        std::string displayName = b->getDisplayName();
        if (displayName.find(name) != std::string::npos)
        {
            count++;
        }
    }
    
    return count;
}

void DraggableMapScene::startPlacingBuilding(const BuildingData& data)
{
    if (_buildingManager)
    {
        _buildingManager->startPlacing(data);
    }
}

// ==================== 生命周期 ====================

void DraggableMapScene::update(float dt)
{
    SocketClient::getInstance().processCallbacks();
}

DraggableMapScene::~DraggableMapScene()
{
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->removeFromParent();
        _currentUpgradeUI = nullptr;
    }
    
    if (_buildingManager && !_isAttackMode)
    {
        _buildingManager->saveCurrentState();
        CCLOG("? Game state auto-saved on scene destruction");
    }
}

// ==================== 多人游戏（未使用，保留接口） ====================

bool DraggableMapScene::switchToAttackMode(const std::string& targetUserId)
{
    // 预留接口
    return false;
}

void DraggableMapScene::returnToOwnBase()
{
    // 预留接口
}

void DraggableMapScene::onAccountSwitched()
{
    CCLOG("✅ Account switch initiated...");
    
    // 1. 保存当前账号的建筑状态和资源
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved current account state");
    }
    
    // 2. 获取目标账号ID
    std::string targetUserId = UserDefault::getInstance()->getStringForKey("switching_to_account", "");
    if (targetUserId.empty())
    {
        CCLOG("❌ No target account specified");
        return;
    }
    
    // 3. 切换账号
    auto& accMgr = AccountManager::getInstance();
    if (!accMgr.switchAccount(targetUserId))
    {
        CCLOG("❌ Failed to switch account");
        return;
    }
    
    CCLOG("✅ Account switched successfully, reloading scene...");
    
    // 4. 清除临时数据
    UserDefault::getInstance()->setStringForKey("switching_to_account", "");
    UserDefault::getInstance()->flush();
    
    // 5. 重新创建整个场景以确保彻底清理
    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.3f, newScene));
}

void DraggableMapScene::onLogout()
{
    // 保存当前状态
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
    }
    
    // 退出游戏或返回主菜单（暂时直接退出）
    Director::getInstance()->end();
    
    #if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
        exit(0);
    #endif
}
