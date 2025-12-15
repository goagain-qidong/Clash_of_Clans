/**
 * @file DraggableMapScene.cpp
 * @brief 主场景实现 - 重构后的精简版本
 */

#include "AccountManager.h"
#include "BaseBuilding.h"
#include "BattleScene.h"
#include "BuildingCapacityManager.h"
#include "BuildingData.h"
#include "BuildingManager.h"
#include "Buildings/ArmyBuilding.h"
#include "Buildings/ArmyCampBuilding.h"
#include "Buildings/ResourceBuilding.h"
#include "BuildingUpgradeUI.h"
#include "DraggableMapScene.h"
#include "HUDLayer.h"
#include "InputController.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/ResourceCollectionManager.h"
#include "Managers/TroopInventory.h"
#include "Managers/UpgradeManager.h"
#include "MapController.h"
#include "ResourceManager.h"
#include "SceneUIController.h"
#include "ShopLayer.h"
#include "SocketClient.h"
#include "UI/ArmySelectionUI.h"
#include "UI/PlayerListLayer.h"
#include "UI/ClanPanel.h" // 🆕 Include ClanPanel
#include "Unit/unit.h"
#include "ui/CocosGUI.h"
#include "Managers/MusicManager.h" // ✅ 新增
#include <ctime>

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

    // 单例节点加入场景
    auto* capacityMgr = &BuildingCapacityManager::getInstance();
    if (capacityMgr->getParent())
        capacityMgr->removeFromParent();
    this->addChild(capacityMgr, 0);

    _collectionMgr = ResourceCollectionManager::getInstance();
    if (_collectionMgr->getParent())
        _collectionMgr->removeFromParent();
    this->addChild(_collectionMgr, 0);

    _visibleSize = Director::getInstance()->getVisibleSize();

    initializeManagers();
    setupCallbacks();
    setupUpgradeManagerCallbacks();

    connectToServer();
    setupNetworkCallbacks();

    scheduleUpdate();

    // 监听场景恢复事件
    auto listener = EventListenerCustom::create("scene_resume", [this](EventCustom* event) { this->onSceneResume(); });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    // HUD 层（已在 initializeManagers 中创建为成员 _hudLayer）
    if (_hudLayer)
    {
        // 绑定 UpgradeManager 回调到成员 HUD 层，避免创建重复 HUD
        UpgradeManager::getInstance()->setOnAvailableBuilderChanged([this](int available) {
            if (_hudLayer)
                _hudLayer->updateDisplay();
        });
    }

    // 🆕 延迟加载游戏状态，确保所有 manager 初始化完成
    this->scheduleOnce([this](float) { loadGameState(); }, 0.1f, "load_game_state");
    
    // 🆕 延迟检测并显示防守日志（确保场景完全加载后）
    this->scheduleOnce([this](float) {
        // 确保日志已加载
        DefenseLogSystem::getInstance().load();
        
        if (DefenseLogSystem::getInstance().hasUnviewedLogs())
        {
            DefenseLogSystem::getInstance().showDefenseLogUI();
            CCLOG("🔔 Displaying unviewed defense logs on scene init");
        }
    }, 0.5f, "check_defense_logs_on_init");

    return true;
}

void DraggableMapScene::onEnter()
{
    Scene::onEnter();
    // 🎵 播放背景音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_PREPARING);
}

void DraggableMapScene::initializeManagers()
{
    // 获取账号分配地图
    std::string assignedMap = "map/Map1.png";
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (currentAccount && !currentAccount->assignedMapName.empty())
    {
        assignedMap = currentAccount->assignedMapName;
        CCLOG("✅ Loading assigned map for account %s: %s", currentAccount->username.c_str(), assignedMap.c_str());
    }

    _mapController = MapController::create();
    this->addChild(_mapController, 0);
    _mapController->setMapNames({assignedMap});
    _mapController->loadMap(assignedMap);

    _buildingManager = BuildingManager::create();
    this->addChild(_buildingManager);
    _buildingManager->setup(_mapController->getMapSprite(), _mapController->getGridMap());

    _uiController = SceneUIController::create();
    this->addChild(_uiController, 100);

    initBuildingData();

    _inputController = InputController::create();
    this->addChild(_inputController);

    _hudLayer = HUDLayer::create();
    this->addChild(_hudLayer, 100);
}

void DraggableMapScene::setupCallbacks()
{
    // 输入回调
    _inputController->setOnTouchBegan([this](Touch* touch, Event* event) { return onTouchBegan(touch, event); });
    _inputController->setOnTouchMoved([this](Touch* touch, Event* event) { onTouchMoved(touch, event); });
    _inputController->setOnTouchEnded([this](Touch* touch, Event* event) { onTouchEnded(touch, event); });
    _inputController->setOnMouseScroll([this](float scrollY, Vec2 mousePos) { onMouseScroll(scrollY, mousePos); });
    _inputController->setOnKeyPressed([this](EventKeyboard::KeyCode keyCode) { onKeyPressed(keyCode); });

    // UI 回调
    _uiController->setOnShopClicked([this]() { onShopClicked(); });
    _uiController->setOnAttackClicked([this]() { onAttackClicked(); });
    _uiController->setOnClanClicked([this]() { onClanClicked(); });
    _uiController->setOnBuildingSelected([this](const BuildingData& data) { onBuildingSelected(data); });
    _uiController->setOnConfirmBuilding([this]() { onConfirmBuilding(); });
    _uiController->setOnCancelBuilding([this]() { onCancelBuilding(); });
    _uiController->setOnAccountSwitched([this]() { onAccountSwitched(); });
    _uiController->setOnLogout([this]() { onLogout(); });
    _uiController->setOnMapChanged([this](const std::string& newMap) { onMapChanged(newMap); });
    
    // 🆕 防守日志按钮回调
    _uiController->setOnDefenseLogClicked([this]() {
        DefenseLogSystem::getInstance().showDefenseLogUI();
    });

    // 建筑管理器回调
    _buildingManager->setOnBuildingPlaced([this](BaseBuilding* building) { onBuildingPlaced(building); });
    _buildingManager->setOnBuildingClicked([this](BaseBuilding* building) { onBuildingClicked(building); });
    _buildingManager->setOnHint([this](const std::string& hint) { onBuildingHint(hint); });
}

void DraggableMapScene::setupUpgradeManagerCallbacks()
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    upgradeMgr->setOnAvailableBuilderChanged([this](int availableBuilders) {
        if (_hudLayer)
            _hudLayer->updateDisplay();
        CCLOG("👷 工人数量已更新：可用=%d", availableBuilders);
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
        CCLOG("Game state loaded");
    }
}

// ========== 输入处理 ==========

bool DraggableMapScene::onTouchBegan(Touch* touch, Event* event)
{
    Vec2 touchPos = touch->getLocation();
    _activeTouches[touch->getID()] = touchPos; // ✅ 记录触摸点，支持多点触控

    if (_collectionMgr && _collectionMgr->handleTouch(touchPos))
    {
        CCLOG("✅ 资源收集：触摸已处理");
        return true;
    }

    if (_currentUpgradeUI && _currentUpgradeUI->isVisible())
    {
        Vec2 localPos = _currentUpgradeUI->convertTouchToNodeSpace(touch);
        Rect bbox = _currentUpgradeUI->getBoundingBox();
        bbox.origin = Vec2::ZERO;
        if (bbox.containsPoint(localPos))
        {
            // _activeTouches[touch->getID()] = touchPos; // 已在开头记录
            return true;
        }
        else
        {
            hideUpgradeUI();
            return false;
        }
    }

    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        // 🔴 修复：移动建筑模式下，开始拖动幽灵精灵
        Vec2 touchPos = touch->getLocation();
        _buildingManager->onBuildingTouchMoved(touchPos);
        return true;
    }

    if (_buildingManager && _buildingManager->isInBuildingMode())
    {
        if (!_buildingManager->isDraggingBuilding() && !_buildingManager->isWaitingConfirm())
        {
            _buildingManager->onTouchBegan(touchPos);
            // _activeTouches[touch->getID()] = touchPos; // 已在开头记录
            return true;
        }
    }

    if (_buildingManager)
    {
        BaseBuilding* clickedBuilding = _buildingManager->getBuildingAtPosition(touchPos);
        if (clickedBuilding)
        {
            _clickedBuilding = clickedBuilding;
            _touchBeganPos = touchPos;
            _touchBeganTime = Director::getInstance()->getTotalFrames() / 60.0f;
            _hasMoved = false;
            // _activeTouches[touch->getID()] = touchPos; // 已在开头记录
            return true;
        }
    }

    _clickedBuilding = nullptr;
    // _activeTouches[touch->getID()] = touchPos; // 已在开头记录
    return true;
}

void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    if (_activeTouches.find(touch->getID()) != _activeTouches.end())
    {
        _activeTouches[touch->getID()] = touch->getLocation();
    }

    // 🆕 多点触控缩放
    if (_activeTouches.size() >= 2)
    {
        _isPinching = true;
        
        // 获取前两个触摸点
        auto it = _activeTouches.begin();
        Vec2 p1 = it->second;
        it++;
        Vec2 p2 = it->second;
        
        float currentDist = p1.distance(p2);
        
        if (_prevPinchDistance <= 0.0f)
        {
            _prevPinchDistance = currentDist;
        }
        else
        {
            if (currentDist > 10.0f)
            {
                // 计算缩放因子
                float zoomFactor = currentDist / _prevPinchDistance;
                // 限制单帧缩放幅度，防止跳变
                zoomFactor = std::max(0.9f, std::min(zoomFactor, 1.1f));
                
                Vec2 center = (p1 + p2) / 2;
                _mapController->zoomMap(zoomFactor, center);
                
                _prevPinchDistance = currentDist;
            }
        }
        
        // 缩放时取消点击和拖动状态
        _clickedBuilding = nullptr;
        _hasMoved = true; // 防止触发点击
        return;
    }
    else
    {
        // 如果手指减少，重置缩放状态，但保持 _isPinching 为 true 直到所有手指抬起？
        // 或者允许单指继续拖动。
        // 这里简单处理：如果不是多指，就重置距离，允许拖动
        _prevPinchDistance = 0.0f;
    }

    Vec2 currentPos = touch->getLocation();
    Vec2 previousPos = touch->getPreviousLocation();
    Vec2 delta = currentPos - previousPos;

    if (_clickedBuilding)
    {
        float distance = currentPos.distance(_touchBeganPos);
        if (distance > 10.0f)
        {
            _hasMoved = true;
            // 🔴 已禁用：长按拖动建筑功能（现在通过建筑详情页的"移动"按钮触发）
            // if (distance > 30.0f && _buildingManager && !_buildingManager->isMovingBuilding() && !_buildingManager->isInBuildingMode())
            // {
            //     _buildingManager->startMovingBuilding(_clickedBuilding);
            //     _clickedBuilding = nullptr;
            //     return;
            // }
        }
    }

    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        _buildingManager->onBuildingTouchMoved(currentPos);
        return;
    }

    if (_buildingManager && _buildingManager->isInBuildingMode() && _buildingManager->isDraggingBuilding())
    {
        _buildingManager->onTouchMoved(currentPos);
        return;
    }

    if ((!_clickedBuilding || _hasMoved) && !_isPinching)
    {
        _mapController->moveMap(delta);
    }
}

void DraggableMapScene::onTouchEnded(Touch* touch, Event* event)
{
    _activeTouches.erase(touch->getID());
    
    if (_activeTouches.size() < 2)
    {
        _prevPinchDistance = 0.0f;
    }
    
    if (_isPinching)
    {
        if (_activeTouches.empty())
        {
            _isPinching = false;
        }
        return; // 缩放操作结束，不处理点击
    }

    Vec2 worldPos = touch->getLocation();

    if (ResourceCollectionManager::getInstance()->handleTouch(worldPos))
    {
        return;
    }

    Vec2 touchPos = worldPos;

    if (_buildingManager && _buildingManager->isMovingBuilding())
    {
        BaseBuilding* movingBuilding = _buildingManager->getMovingBuilding();
        if (movingBuilding)
            _buildingManager->onBuildingTouchEnded(touchPos, movingBuilding);
        _clickedBuilding = nullptr;
        _hasMoved = false;
        return;
    }

    if (_buildingManager && _buildingManager->isInBuildingMode() && _buildingManager->isDraggingBuilding())
    {
        _buildingManager->onTouchEnded(touchPos);
        if (_buildingManager->isWaitingConfirm())
        {
            Vec2 worldPos2 = _buildingManager->getPendingBuildingWorldPos();
            _uiController->showConfirmButtons(worldPos2);
        }
        _clickedBuilding = nullptr;
        _hasMoved = false;
        return;
    }

    if (_clickedBuilding && !_hasMoved && !_buildingManager->isInBuildingMode())
    {
        onBuildingClicked(_clickedBuilding);
        _clickedBuilding = nullptr;
        _hasMoved = false;
        return;
    }

    _clickedBuilding = nullptr;
    _hasMoved = false;
}

// 🆕 添加 onTouchCancelled 处理
void DraggableMapScene::onTouchCancelled(Touch* touch, Event* event)
{
    _activeTouches.erase(touch->getID());
    if (_activeTouches.size() < 2)
    {
        _prevPinchDistance = 0.0f;
        if (_activeTouches.empty()) _isPinching = false;
    }
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

// ========== UI 回调 ==========

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
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved current base before attacking");
    }

    auto armyUI = ArmySelectionUI::create();
    if (!armyUI)
    {
        _uiController->showHint("创建军队选择UI失败！");
        return;
    }

    this->addChild(armyUI, 200);

    armyUI->setOnConfirmed([this]() {
        auto& client = SocketClient::getInstance();
        if (client.isConnected())
        {
            client.requestUserList();
        }
        else
        {
            showLocalPlayerList();
        }
    });

    armyUI->setOnCancelled([this]() {
        CCLOG("❌ 取消攻击");
        _uiController->showHint("已取消攻击");
    });

    armyUI->show();
}

void DraggableMapScene::onClanClicked()
{
    auto clanPanel = ClanPanel::create();
    if (clanPanel)
    {
        this->addChild(clanPanel, 200);
        clanPanel->show();
    }
}

void DraggableMapScene::onBuildingSelected(const BuildingData& data)
{
    // Start placing the selected building
    startPlacingBuilding(data);
}

void DraggableMapScene::onConfirmBuilding()
{
    // Confirm building placement: hide confirm buttons and notify building manager if available
    if (_uiController)
        _uiController->hideConfirmButtons();

    // Notify BuildingManager to confirm placement if it's waiting for confirmation
    if (_buildingManager && _buildingManager->isWaitingConfirm())
    {
        _buildingManager->confirmBuilding();
    }
}

void DraggableMapScene::onCancelBuilding()
{
    // Hide UI confirm buttons
    if (_uiController)
        _uiController->hideConfirmButtons();

    // Notify BuildingManager to cancel placement if it's active
    if (_buildingManager && _buildingManager->isInBuildingMode())
    {
        // Prefer cancelBuilding() if implemented; fallback to cancelPlacing() if available
        // Try cancelBuilding first
        _buildingManager->cancelBuilding();
    }
}

void DraggableMapScene::onMapChanged(const std::string& newMap)
{
    CCLOG("Map change requested: %s", newMap.c_str());
    // Reload scene to apply new map selection
    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, newScene));
}

// ========== 建筑回调 ==========

void DraggableMapScene::onBuildingPlaced(BaseBuilding* building)
{
    if (!building)
        return;

    CCLOG("Building placed: %s", building->getDisplayName().c_str());

    auto resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
    if (resourceBuilding && resourceBuilding->isProducer())
        registerResourceBuilding(resourceBuilding);

    if (building->getBuildingType() == BuildingType::kArmy)
    {
        auto barracks = dynamic_cast<ArmyBuilding*>(building);
        if (barracks)
        {
            // 🔴 方案A优化：训练完成时只显示提示，不在地图上创建独立 Unit
            // 小兵会自动显示在军营中（由 ArmyBuilding::notifyArmyCampsToDisplayTroop 处理）
            barracks->setOnTrainingComplete([this](Unit* unit) {
                // unit 参数现在总是 nullptr，不需要检查
                // 只显示提示信息
                CCLOG("🎉 Unit training complete!");
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
    
    // 🆕 显示占用网格覆盖层（淡入效果，不自动淡出）
    if (_buildingManager)
    {
        _buildingManager->showOccupiedGrids(false); // false表示不自动淡出
    }
    
    showUpgradeUI(building);
}

void DraggableMapScene::onBuildingHint(const std::string& hint)
{
    _uiController->showHint(hint);
}

// ========== 升级 UI ==========

void DraggableMapScene::showUpgradeUI(BaseBuilding* building)
{
    hideUpgradeUI();

    auto upgradeUI = BuildingUpgradeUI::create(building);
    if (!upgradeUI)
        return;

    upgradeUI->setPositionNearBuilding(building);

    upgradeUI->setUpgradeCallback([this, building](bool success, int newLevel) {
        if (success)
        {
            // 默认行为：刷新 HUD 等（具体逻辑可按需扩展）
            CCLOG("Building upgraded: %s -> level %d", building->getDisplayName().c_str(), newLevel);
            if (_hudLayer)
                _hudLayer->updateDisplay();
        }
    });

    // 🔴 修复：在关闭回调中添加淡出网格的逻辑
    upgradeUI->setCloseCallback([this]() { 
        _currentUpgradeUI = nullptr;
        
        // 淡出占用网格覆盖层
        if (_buildingManager)
        {
            _buildingManager->hideOccupiedGrids();
        }
    });

    this->addChild(upgradeUI, 1000);
    upgradeUI->show();
    _currentUpgradeUI = upgradeUI;
}

void DraggableMapScene::hideUpgradeUI()
{
    if (!_currentUpgradeUI)
        return;

    // 先淡出占用网格覆盖层，再隐藏UI
    if (_buildingManager)
    {
        // 延迟淡出，让用户看到效果
        auto delay = DelayTime::create(0.2f);
        auto call  = CallFunc::create([this]() {
            if (_buildingManager)
                _buildingManager->hideOccupiedGrids();
        });
        this->runAction(Sequence::create(delay, call, nullptr));
    }
    
    auto upgradeUI = dynamic_cast<BuildingUpgradeUI*>(_currentUpgradeUI);
    if (upgradeUI)
        upgradeUI->hide();
    else if (_currentUpgradeUI->getParent() == this)
        _currentUpgradeUI->removeFromParent();
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
            _currentUpgradeUI->removeFromParent();
        _currentUpgradeUI = nullptr;
    }
}

// ========== 资源建筑注册 ==========

void DraggableMapScene::registerResourceBuilding(ResourceBuilding* building)
{
    if (_collectionMgr && building)
    {
        _collectionMgr->registerBuilding(building);
        CCLOG("✅ 注册资源建筑收集：%s", building->getDisplayName().c_str());
    }
}

// ========== 网络 ==========

void DraggableMapScene::connectToServer()
{
    auto& sock = SocketClient::getInstance();

    // 设置连接回调
    sock.setOnConnected([](bool success) {
        if (success)
        {
            CCLOG("[Socket] ✅ 连接成功！");
            
            // 自动登录并上传地图
            auto& accMgr = AccountManager::getInstance();
            auto currentAccount = accMgr.getCurrentAccount();
            if (currentAccount)
            {
                // 登录
                SocketClient::getInstance().login(currentAccount->userId, currentAccount->username, currentAccount->gameData.trophies);
                CCLOG("[Socket] 📤 Sent login: %s", currentAccount->userId.c_str());
                
                // 上传地图数据
                std::string mapData = currentAccount->gameData.toJson();
                SocketClient::getInstance().uploadMap(mapData);
                CCLOG("[Socket] 📤 Uploaded map data (size: %zu bytes)", mapData.size());
            }
        }
        else
        {
            CCLOG("[Socket] ❌ 连接失败");
        }
    });

    // 尝试连接到服务器（使用正确的端口）
    const std::string host = "127.0.0.1";
    const int port = 8888; // 🔴 修正：使用服务器默认端口 8888

    if (!sock.isConnected())
    {
        CCLOG("[Socket] 🔌 正在连接到服务器 %s:%d...", host.c_str(), port);
        sock.connect(host, port);
    }
    else
    {
        CCLOG("[Socket] ✅ 已连接到服务器");
        // 如果已连接，直接上传地图
        auto& accMgr = AccountManager::getInstance();
        auto currentAccount = accMgr.getCurrentAccount();
        if (currentAccount)
        {
            std::string mapData = currentAccount->gameData.toJson();
            sock.uploadMap(mapData);
            CCLOG("[Socket] 📤 Re-uploaded map data");
        }
    }
}

void DraggableMapScene::setupNetworkCallbacks()
{
    auto& sock = SocketClient::getInstance();

    // When an attack result is received from the server, add a DefenseLog if the current account was the defender
    sock.setOnAttackResult([this](const AttackResult& result){
        auto& accMgr = AccountManager::getInstance();
        const AccountInfo* cur = accMgr.getCurrentAccount();
        if (!cur) return;

        // If this client is the defender, record a defense log
        if (result.defenderId == cur->userId)
        {
            DefenseLog log;
            log.attackerId = result.attackerId;
            // we don't always have attacker name from network; fallback to id
            log.attackerName = result.attackerId;
            log.starsLost = result.starsEarned;
            log.goldLost = result.goldLooted;
            log.elixirLost = result.elixirLooted;
            log.trophyChange = result.trophyChange;
            log.timestamp = getCurrentTimestamp();
            log.isViewed = false;

            DefenseLogSystem::getInstance().addDefenseLog(log);
            
            CCLOG("🛡️ Defense log added for defender: %s, attacked by: %s", 
                  result.defenderId.c_str(), result.attackerId.c_str());

            // 🔧 修复内存泄漏：使用 performFunctionInCocosThread 避免循环引用
            if (DefenseLogSystem::getInstance().hasUnviewedLogs())
            {
                // 直接在主线程显示，不使用 scheduleOnce 捕获 this
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([](){
                    DefenseLogSystem::getInstance().showDefenseLogUI();
                    CCLOG("🔔 Displaying defense log UI after receiving attack result");
                });
            }
        }
    });

    // Optionally handle user list (map userId->name) if server returns additional info
    sock.setOnUserListReceived([this](const std::string& data){
        CCLOG("[Socket] User list received, len=%zu", data.size());
        // Ensure UI update runs on main thread
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, data](){
            showPlayerListFromServerData(data);
        });
    });
}

std::string DraggableMapScene::getCurrentTimestamp()
{
    // simple ISO-like timestamp
    time_t now = time(nullptr);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return std::string(buf);
}

// ========== ShopLayer 接口 ==========

int DraggableMapScene::getTownHallLevel() const
{
    if (!_buildingManager)
        return 1;
    const auto& buildings = _buildingManager->getBuildings();
    for (auto* b : buildings)
    {
        if (b->getBuildingType() == BuildingType::kTownHall)
            return b->getLevel();
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
                count++;
            continue;
        }
        std::string displayName = b->getDisplayName();
        if (displayName.find(name) != std::string::npos)
            count++;
    }
    return count;
}

void DraggableMapScene::startPlacingBuilding(const BuildingData& data)
{
    if (_buildingManager)
        _buildingManager->startPlacing(data);
}

// ========== 生命周期 ==========

void DraggableMapScene::update(float dt)
{
    SocketClient::getInstance().processCallbacks();
}

DraggableMapScene::~DraggableMapScene()
{
    // Clear network callbacks to prevent crash
    SocketClient::getInstance().setOnAttackResult(nullptr);
    SocketClient::getInstance().setOnUserListReceived(nullptr);

    // 🔧 修复内存泄漏：清理所有 schedule 回调
    this->unscheduleAllCallbacks();
    
    // 🔧 修复内存泄漏：移除所有事件监听器
    _eventDispatcher->removeEventListenersForTarget(this);
    
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->removeFromParent();
        _currentUpgradeUI = nullptr;
    }

    if (_buildingManager && !_isAttackMode)
    {
        _buildingManager->saveCurrentState();
        CCLOG("💾 Game state auto-saved on scene destruction");
    }
    
    CCLOG("🗑️ DraggableMapScene destroyed, all callbacks cleaned");
}

void DraggableMapScene::onSceneResume()
{
    CCLOG("🔄 Scene resumed, refreshing ArmyCamp displays...");
    
    // 重置触摸状态
    _activeTouches.clear();
    _isPinching = false;
    _prevPinchDistance = 0.0f;
    
    // 🎵 恢复背景音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_PREPARING);
    
    // 重新加载士兵库存
    TroopInventory::getInstance().load();
    
    if (_buildingManager)
    {
        const auto& buildings = _buildingManager->getBuildings();
        for (auto* building : buildings)
        {
            // 刷新军营的小兵显示
            auto armyCamp = dynamic_cast<ArmyCampBuilding*>(building);
            if (armyCamp)
            {
                armyCamp->refreshDisplayFromInventory();
                CCLOG("✅ Refreshed ArmyCamp display from inventory");
            }
            
            // 重新注册资源建筑
            auto resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
            if (resourceBuilding && resourceBuilding->isProducer())
                ResourceCollectionManager::getInstance()->registerBuilding(resourceBuilding);
        }
    }
    
    // 刷新HUD显示
    if (_hudLayer)
    {
        _hudLayer->updateDisplay();
        CCLOG("✅ Refreshed HUD display");
    }

    CCLOG("✅ Scene resume complete");
}

// ========== 多人游戏（保留接口） ==========

bool DraggableMapScene::switchToAttackMode(const std::string& targetUserId)
{
    return false;
}

void DraggableMapScene::returnToOwnBase()
{
}

void DraggableMapScene::onAccountSwitched()
{
    CCLOG("✅ Account switch initiated...");
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved current account state");
    }

    std::string targetUserId = UserDefault::getInstance()->getStringForKey("switching_to_account", "");
    if (targetUserId.empty())
    {
        CCLOG("❌ No target account specified");
        return;
    }

    auto& accMgr = AccountManager::getInstance();
    if (!accMgr.switchAccount(targetUserId))
    {
        CCLOG("❌ Failed to switch account");
        return;
    }

    CCLOG("✅ Account switched successfully, reloading scene...");
    UserDefault::getInstance()->setStringForKey("switching_to_account", "");
    UserDefault::getInstance()->flush();

    // 🆕 加载新账号的防守日志
    DefenseLogSystem::getInstance().load();
    
    // 🆕 检查是否有未查看的日志
    if (DefenseLogSystem::getInstance().hasUnviewedLogs())
    {
        CCLOG("🔔 Found unviewed defense logs for account: %s", targetUserId.c_str());
    }

    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.3f, newScene));
}

void DraggableMapScene::onLogout()
{
    if (_buildingManager)
        _buildingManager->saveCurrentState();
    Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

// ========== 本地玩家列表显示 ==========

void DraggableMapScene::showLocalPlayerList()
{
    auto& accMgr = AccountManager::getInstance();
    const auto& allAccounts = accMgr.listAccounts();
    const auto* currentAccount = accMgr.getCurrentAccount();

    if (!currentAccount)
    {
        _uiController->showHint("错误：未登录账号！");
        return;
    }

    std::vector<PlayerInfo> players;
    for (const auto& account : allAccounts)
    {
        if (account.userId != currentAccount->userId)
        {
            auto gameData = accMgr.getPlayerGameData(account.userId);
            PlayerInfo player;
            player.userId = account.userId;
            player.username = account.username;
            player.townHallLevel = gameData.townHallLevel;
            player.trophies = 1000;
            player.gold = gameData.gold;
            player.elixir = gameData.elixir;
            players.push_back(player);
        }
    }

    if (players.empty())
    {
        _uiController->showHint("暂无可攻击的玩家！(请确保其他玩家在线且使用不同的账号)");
        return;
    }

    auto playerListLayer = PlayerListLayer::create(players);
    if (playerListLayer)
    {
        this->addChild(playerListLayer, 300);
        playerListLayer->setOnPlayerSelected([this](const std::string& targetUserId) { startAttack(targetUserId); });
        playerListLayer->show();
    }
}

void DraggableMapScene::showPlayerListFromServerData(const std::string& serverData)
{
    std::vector<PlayerInfo> players;
    std::istringstream iss(serverData);
    std::string playerStr;

    while (std::getline(iss, playerStr, '|'))
    {
        std::istringstream ps(playerStr);
        std::string token;
        PlayerInfo player;
        std::getline(ps, player.userId, ',');
        std::getline(ps, player.username, ',');
        std::getline(ps, token, ',');
        if (!token.empty()) player.townHallLevel = std::stoi(token);
        std::getline(ps, token, ',');
        if (!token.empty()) player.gold = std::stoi(token);
        std::getline(ps, token, ',');
        if (!token.empty()) player.elixir = std::stoi(token);
        players.push_back(player);
    }

    if (players.empty())
    {
        _uiController->showHint("暂无可攻击的玩家！");
        return;
    }

    auto playerListLayer = PlayerListLayer::create(players);
    if (playerListLayer)
    {
        this->addChild(playerListLayer, 300);
        playerListLayer->setOnPlayerSelected([this](const std::string& targetUserId) { startAttack(targetUserId); });
        playerListLayer->show();
    }
}

void DraggableMapScene::startAttack(const std::string& targetUserId)
{
    CCLOG("⚔️ 开始攻击玩家: %s", targetUserId.c_str());

    auto& accMgr = AccountManager::getInstance();
    auto enemyGameData = accMgr.getPlayerGameData(targetUserId);

    if (enemyGameData.buildings.empty())
    {
        _uiController->showHint(StringUtils::format("玩家 %s 还没有建筑！", targetUserId.c_str()));
        return;
    }

    CCLOG("✅ 加载成功，进入战斗场景 (TH Level=%d, Buildings=%zu)", enemyGameData.townHallLevel, enemyGameData.buildings.size());
    auto battleScene = BattleScene::createWithEnemyData(enemyGameData, targetUserId);
    if (battleScene)
        Director::getInstance()->pushScene(TransitionFade::create(0.3f, battleScene));
    else
        _uiController->showHint("创建战斗场景失败！");
}
