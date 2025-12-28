/****************************************************************
* Project Name:  Clash_of_Clans
* File Name:     DraggableMapScene.cpp
* File Function: 主场景类
* Author:        刘相成、薛毓哲
* Update Date:   2025/12/28
* License:       MIT License
****************************************************************/
#include "DraggableMapScene.h"

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
#include "HUDLayer.h"
#include "InputController.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/MusicManager.h"
#include "Managers/ResourceCollectionManager.h"
#include "Managers/TroopInventory.h"
#include "Managers/UpgradeManager.h"
#include "MapController.h"
#include "ResourceManager.h"
#include "SceneUIController.h"
#include "ShopLayer.h"
#include "SocketClient.h"
#include "UI/ClanPanel.h"
#include "UI/PlayerListLayer.h"
#include "Unit/UnitTypes.h"
#include "ui/CocosGUI.h"

#include <ctime>

// Forward declaration for callback
class BaseUnit;

USING_NS_CC;
using namespace ui;

Scene* DraggableMapScene::createScene()
{
    return DraggableMapScene::create();
}

// ============================================================================
// 全局函数：创建主场景（供 BattleScene 等外部调用）
// ============================================================================

cocos2d::Scene* createDraggableMapScene()
{
    return DraggableMapScene::createScene();
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

    // 监听场景恢复事件（使用固定优先级，确保场景被push后仍能接收事件）
    auto listener = EventListenerCustom::create("scene_resume", [this](EventCustom* event) { this->onSceneResume(); });
    _eventDispatcher->addEventListenerWithFixedPriority(listener, 1);
    _sceneResumeListener = listener;

    // 延迟加载游戏状态
    this->scheduleOnce([this](float) { loadGameState(); }, 0.1f, "load_game_state");
    
    // 延迟检测并显示防守日志
    this->scheduleOnce([this](float) {
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
        CCLOG("Loading assigned map for account %s: %s", currentAccount->account.username.c_str(), assignedMap.c_str());
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
    
    // 退出建造模式回调
    _uiController->setOnExitBuildMode([this]() {
        if (_buildingManager && _buildingManager->isInBuildingMode())
        {
            _buildingManager->cancelPlacing();
            _uiController->hideConfirmButtons();
            _uiController->hideExitBuildModeButton();
            CCLOG("📱 通过UI按钮退出建造模式");
        }
    });
    
    // 防守日志按钮回调
    _uiController->setOnDefenseLogClicked([this]() {
        DefenseLogSystem::getInstance().showDefenseLogUI();
    });

    // 建筑管理器回调
    _buildingManager->setOnBuildingPlaced([this](BaseBuilding* building) { onBuildingPlaced(building); });
    _buildingManager->setOnBuildingClicked([this](BaseBuilding* building) { onBuildingClicked(building); });
    _buildingManager->setOnHint([this](const std::string& hint) { onBuildingHint(hint); });
    
    // 建造模式变化回调 - 当建造模式退出时隐藏退出按钮
    _buildingManager->setOnBuildModeChanged([this](bool isInBuildMode) {
        if (!isInBuildMode && _uiController)
        {
            _uiController->hideExitBuildModeButton();
            CCLOG("🏗️ 建造模式已退出，隐藏退出按钮");
        }
    });
}

void DraggableMapScene::setupUpgradeManagerCallbacks()
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    upgradeMgr->setOnAvailableBuilderChanged([this](int availableBuilders) {
        if (_hudLayer)
            _hudLayer->updateDisplay();
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
    _activeTouches[touch->getID()] = touchPos;

    if (_collectionMgr && _collectionMgr->handleTouch(touchPos))
        return true;

    if (_currentUpgradeUI && _currentUpgradeUI->isVisible())
    {
        Vec2 localPos = _currentUpgradeUI->convertTouchToNodeSpace(touch);
        Rect bbox = _currentUpgradeUI->getBoundingBox();
        bbox.origin = Vec2::ZERO;
        if (bbox.containsPoint(localPos))
        {
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
        _buildingManager->onBuildingTouchMoved(touchPos);
        return true;
    }

    if (_buildingManager && _buildingManager->isInBuildingMode())
    {
        if (!_buildingManager->isDraggingBuilding() && !_buildingManager->isWaitingConfirm())
        {
            _buildingManager->onTouchBegan(touchPos);
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
            return true;
        }
    }

    _clickedBuilding = nullptr;
    return true;
}

void DraggableMapScene::onTouchMoved(Touch* touch, Event* event)
{
    if (_activeTouches.find(touch->getID()) != _activeTouches.end())
    {
        _activeTouches[touch->getID()] = touch->getLocation();
    }

    // 多点触控缩放
    if (_activeTouches.size() >= 2)
    {
        _isPinching = true;
        
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
                float zoomFactor = currentDist / _prevPinchDistance;
                zoomFactor = std::max(0.9f, std::min(zoomFactor, 1.1f));
                
                Vec2 center = (p1 + p2) / 2;
                _mapController->zoomMap(zoomFactor, center);
                
                _prevPinchDistance = currentDist;
            }
        }
        
        _clickedBuilding = nullptr;
        _hasMoved = true;
        return;
    }
    else
    {
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
        return;
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
    // 预留键盘事件处理接口
    // 建造模式的退出通过屏幕底部的"退出放置"按钮实现
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
    }

    auto& client = SocketClient::getInstance();
    if (client.isConnected())
    {
        client.setOnUserListReceived([this](const std::string& data){
            CCLOG("[Socket] User list received, len=%zu", data.size());
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, data](){
                showPlayerListFromServerData(data);
            });
        });

        client.requestUserList();
    }
    else
    {
        showLocalPlayerList();
    }
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
    // 进入建造模式（退出按钮在 startPlacingBuilding 中统一显示）
    startPlacingBuilding(data);
}

void DraggableMapScene::onConfirmBuilding()
{
    if (_uiController)
        _uiController->hideConfirmButtons();

    if (_buildingManager && _buildingManager->isWaitingConfirm())
    {
        _buildingManager->confirmBuilding();
    }
    
    // 注意：退出按钮的隐藏由 BuildingManager::endPlacing() 的回调统一处理
    // 这样可以确保城墙连续放置模式下按钮保持显示
}

void DraggableMapScene::onCancelBuilding()
{
    if (_uiController)
        _uiController->hideConfirmButtons();

    if (_buildingManager && _buildingManager->isInBuildingMode())
    {
        _buildingManager->cancelBuilding();
    }
    
    // 注意：退出按钮的隐藏由 BuildingManager::endPlacing() 的回调统一处理
}

void DraggableMapScene::onMapChanged(const std::string& newMap)
{
    CCLOG("Map change requested: %s", newMap.c_str());

    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
        CCLOG("✅ Saved game state before map change");
    }

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
            barracks->setOnTrainingComplete([this](BaseUnit* unit) {
                _uiController->showHint("士兵训练完成！");
            });
        }
    }

    _uiController->hideConfirmButtons();
    
    // 注意：不在这里隐藏退出按钮
    // 退出按钮的隐藏由 BuildingManager::endPlacing() 触发后，
    // 在 onConfirmBuilding/onCancelBuilding 中处理
    // 这样可以确保城墙连续放置模式下按钮保持显示
}

void DraggableMapScene::onBuildingClicked(BaseBuilding* building)
{
    if (!building)
        return;
    
    if (_buildingManager)
        _buildingManager->showOccupiedGrids(false);
    
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
            CCLOG("Building upgraded: %s -> level %d", building->getDisplayName().c_str(), newLevel);
            if (_hudLayer)
                _hudLayer->updateDisplay();
        }
    });

    upgradeUI->setCloseCallback([this]() { 
        _currentUpgradeUI = nullptr;
        
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

    if (_buildingManager)
    {
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
        _collectionMgr->registerBuilding(building);
}

// ========== 网络 ==========

void DraggableMapScene::connectToServer()
{
    auto& sock = SocketClient::getInstance();

    sock.setOnConnected([](bool success) {
        if (success)
        {
            CCLOG("[Socket] Connected successfully");
            
            auto& accMgr = AccountManager::getInstance();
            auto currentAccount = accMgr.getCurrentAccount();
            if (currentAccount)
            {
                // 发送登录信息，包含部落ID
                SocketClient::getInstance().login(
                    currentAccount->account.userId, 
                    currentAccount->account.username, 
                    currentAccount->gameState.progress.trophies,
                    currentAccount->gameState.progress.clanId  // 发送部落ID
                );
                
                std::string mapData = accMgr.exportGameStateJson();
                SocketClient::getInstance().uploadMap(mapData);
            }
        }
        else
        {
            CCLOG("[Socket] Connection failed");
        }
    });

    const std::string host = "127.0.0.1";
    const int port = 8888;

    if (!sock.isConnected())
    {
        CCLOG("[Socket] Connecting to %s:%d...", host.c_str(), port);
        sock.connect(host, port);
    }
    else
    {
        CCLOG("[Socket] Already connected");
        auto& accMgr = AccountManager::getInstance();
        auto currentAccount = accMgr.getCurrentAccount();
        if (currentAccount)
        {
            std::string mapData = accMgr.exportGameStateJson();
            sock.uploadMap(mapData);
        }
    }
}

void DraggableMapScene::setupNetworkCallbacks()
{
    auto& sock = SocketClient::getInstance();

    sock.setOnAttackResult([this](const AttackResult& result){
        auto& accMgr = AccountManager::getInstance();
        const AccountInfo* cur = accMgr.getCurrentAccount();
        if (!cur) return;

        if (result.defender_id == cur->account.userId)
        {
            DefenseLog log;
            log.attackerId = result.attacker_id;
            log.attackerName = result.attacker_id;
            log.starsLost = result.stars_earned;
            log.goldLost = result.gold_looted;
            log.elixirLost = result.elixir_looted;
            log.trophyChange = result.trophy_change;
            log.timestamp = getCurrentTimestamp();
            log.isViewed = false;

            DefenseLogSystem::getInstance().addDefenseLog(log);
            
            CCLOG("Defense log added for defender: %s, attacked by: %s", 
                  result.defender_id.c_str(), result.attacker_id.c_str());

            if (DefenseLogSystem::getInstance().hasUnviewedLogs())
            {
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([](){
                    DefenseLogSystem::getInstance().showDefenseLogUI();
                });
            }
        }
    });

    sock.setOnUserListReceived([this](const std::string& data){
        CCLOG("[Socket] User list received, len=%zu", data.size());
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, data](){
            showPlayerListFromServerData(data);
        });
    });
}

std::string DraggableMapScene::getCurrentTimestamp()
{
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
    {
        _buildingManager->startPlacing(data);
        
        // 统一在此处显示退出建造模式按钮
        // 确保无论从 ShopLayer 还是其他入口进入建造模式都能显示
        if (_uiController)
        {
            _uiController->showExitBuildModeButton();
        }
    }
}

// ========== 生命周期 ==========

void DraggableMapScene::update(float dt)
{
    SocketClient::getInstance().processCallbacks();
}

DraggableMapScene::~DraggableMapScene()
{
    if (_sceneResumeListener)
    {
        _eventDispatcher->removeEventListener(_sceneResumeListener);
        _sceneResumeListener = nullptr;
    }
    
    UpgradeManager::getInstance()->setOnAvailableBuilderChanged(nullptr);
    
    SocketClient::getInstance().setOnAttackResult(nullptr);
    SocketClient::getInstance().setOnUserListReceived(nullptr);

    this->unscheduleAllCallbacks();
    _eventDispatcher->removeEventListenersForTarget(this);
    
    if (_currentUpgradeUI)
    {
        _currentUpgradeUI->removeFromParent();
        _currentUpgradeUI = nullptr;
    }

    if (_buildingManager && !_isAttackMode && !_isSwitchingAccount)
    {
        _buildingManager->saveCurrentState();
    }
}

void DraggableMapScene::onSceneResume()
{
    CCLOG("Scene resumed, refreshing ArmyCamp displays...");

    _activeTouches.clear();
    _isPinching        = false;
    _prevPinchDistance = 0.0f;

    MusicManager::getInstance().playMusic(MusicType::BATTLE_PREPARING);

    auto& client = SocketClient::getInstance();
    client.setOnPvpStart(nullptr);
    client.setOnPvpAction(nullptr);
    client.setOnPvpEnd(nullptr);
    client.setOnSpectateJoin(nullptr);

    setupNetworkCallbacks();
    setupUpgradeManagerCallbacks();

    if (_buildingManager)
    {
        _buildingManager->restoreArmyCampTroopDisplays();
        
        const auto& buildings = _buildingManager->getBuildings();
        for (auto* building : buildings)
        {
            auto resourceBuilding = dynamic_cast<ResourceBuilding*>(building);
            if (resourceBuilding && resourceBuilding->isProducer())
                ResourceCollectionManager::getInstance()->registerBuilding(resourceBuilding);
        }
    }

    if (_hudLayer)
    {
        _hudLayer->updateDisplay();
    }
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
    CCLOG("Account switch initiated...");
    
    if (_buildingManager)
    {
        _buildingManager->saveCurrentState();
    }

    std::string targetUserId = UserDefault::getInstance()->getStringForKey("switching_to_account", "");
    if (targetUserId.empty())
        return;

    _isSwitchingAccount = true;

    auto& accMgr = AccountManager::getInstance();
    if (!accMgr.switchAccount(targetUserId))
    {
        _isSwitchingAccount = false;
        return;
    }

    UserDefault::getInstance()->setStringForKey("switching_to_account", "");
    UserDefault::getInstance()->flush();

    DefenseLogSystem::getInstance().load();

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
        if (account.userId != currentAccount->account.userId)
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
    CCLOG("Starting attack on player: %s", targetUserId.c_str());

    auto& accMgr = AccountManager::getInstance();
    auto enemyGameData = accMgr.getPlayerGameData(targetUserId);

    if (enemyGameData.buildings.empty())
    {
        _uiController->showHint(StringUtils::format("玩家 %s 还没有建筑！", targetUserId.c_str()));
        return;
    }

    CCLOG("Loading battle scene (TH Level=%d, Buildings=%zu)", enemyGameData.townHallLevel, enemyGameData.buildings.size());
    auto battleScene = BattleScene::createWithEnemyData(enemyGameData, targetUserId);
    if (battleScene)
        Director::getInstance()->pushScene(TransitionFade::create(0.3f, battleScene));
    else
        _uiController->showHint("创建战斗场景失败！");
}
