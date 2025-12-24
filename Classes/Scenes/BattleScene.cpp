/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleScene.cpp
 * File Function: 战斗场景
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "BattleScene.h"
#include "AccountManager.h"
#include "BuildingManager.h"
#include "Buildings/BaseBuilding.h"
#include "Buildings/DefenseBuilding.h"
#include "DraggableMapScene.h"
#include "GridMap.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/MusicManager.h"
#include "Managers/SocketClient.h"
#include "Managers/TroopInventory.h"
#include "ResourceManager.h"
#include <ctime>

USING_NS_CC;
using namespace ui;

// ==================== 创建场景 ====================

Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

BattleScene* BattleScene::createWithEnemyData(const AccountGameData& enemyData)
{
    return createWithEnemyData(enemyData, "Enemy");
}

BattleScene* BattleScene::createWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId)
{
    BattleScene* scene = new (std::nothrow) BattleScene();
    if (scene && scene->initWithEnemyData(enemyData, enemyUserId))
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

BattleScene* BattleScene::createWithReplayData(const std::string& replayDataStr)
{
    BattleScene* scene = new (std::nothrow) BattleScene();
    if (scene && scene->initWithReplayData(replayDataStr))
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

BattleScene::BattleScene()
{
    _battleManager = new BattleManager();
}

BattleScene::~BattleScene()
{
    CC_SAFE_DELETE(_battleManager);
}

// ==================== 初始化 ====================

bool BattleScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    setupMap();
    setupUI();

    if (_battleUI)
    {
        _battleUI->updateStatus("错误：未加载敌方基地数据！", Color4B::RED);
    }

    return true;
}

bool BattleScene::initWithEnemyData(const AccountGameData& enemyData)
{
    return initWithEnemyData(enemyData, "Enemy");
}

bool BattleScene::initWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId)
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    setupMap();
    setupUI();
    setupTouchListeners();

    // Initialize Manager
    if (_battleManager)
    {
        _battleManager->init(_mapSprite, enemyData, enemyUserId, false);

        // Setup callbacks
        _battleManager->setUIUpdateCallback([this]() {
            if (_battleUI && _battleManager)
            {
                _battleUI->updateTimer(static_cast<int>(_battleManager->getRemainingTime()));
                _battleUI->updateStars(_battleManager->getStars());
                _battleUI->updateDestruction(_battleManager->getDestructionPercent());
            }
        });

        _battleManager->setBattleEndCallback([this]() {
            if (_battleUI && _battleManager)
            {
                int trophyChange = _battleManager->getStars() * 10 - (3 - _battleManager->getStars()) * 3;
                _battleUI->showResultPanel(_battleManager->getStars(), _battleManager->getDestructionPercent(),
                                           _battleManager->getGoldLooted(), _battleManager->getElixirLooted(),
                                           trophyChange, _battleManager->isReplayMode());

                // 🆕 修复：确保所有PVP相关方（攻击者、防守者、观战者）都收到结束通知
                if (_isPvpMode)
                {
                    SocketClient::getInstance().endPvp();
                    CCLOG("🔚 Battle ended, PVP notification sent (isAttacker=%d)", _isAttacker);
                }
            }
        });

        _battleManager->setTroopDeployCallback([this](UnitType type, int count) {
            if (_battleUI && _battleManager)
            {
                _battleUI->updateTroopCounts(_battleManager->getTroopCount(UnitType::kBarbarian),
                                             _battleManager->getTroopCount(UnitType::kArcher),
                                             _battleManager->getTroopCount(UnitType::kGiant),
                                             _battleManager->getTroopCount(UnitType::kGoblin),
                                             _battleManager->getTroopCount(UnitType::kWallBreaker));
            }
        });
    }

    // Load Buildings
    if (_buildingManager && !enemyData.buildings.empty())
    {
        CCLOG("🏰 Loading enemy base with %zu buildings...", enemyData.buildings.size());
        _buildingManager->loadBuildingsFromData(enemyData.buildings, true);

        // Pass buildings to manager
        if (_battleManager)
        {
            const auto&                buildings = _buildingManager->getBuildings();
            std::vector<BaseBuilding*> buildingVec(buildings.begin(), buildings.end());
            _battleManager->setBuildings(buildingVec);
        }

        if (_battleUI)
        {
            _battleUI->updateStatus(
                StringUtils::format("攻击 %s 的村庄 (大本营 Lv.%d)", enemyUserId.c_str(), enemyData.townHallLevel),
                Color4B::GREEN);
        }
    }
    else
    {
        if (_battleUI)
            _battleUI->updateStatus("错误：无法加载敌方基地！", Color4B::RED);
        CCLOG("❌ Failed to load enemy base: no buildings data");
    }

    // 🎵 播放准备音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_PREPARING);

    this->scheduleOnce(
        [this](float dt) {
            if (_battleManager)
            {
                TroopDeploymentMap allTroops;
                auto&              inventory      = TroopInventory::getInstance();
                allTroops[UnitType::kBarbarian]   = inventory.getTroopCount(UnitType::kBarbarian);
                allTroops[UnitType::kArcher]      = inventory.getTroopCount(UnitType::kArcher);
                allTroops[UnitType::kGiant]       = inventory.getTroopCount(UnitType::kGiant);
                allTroops[UnitType::kGoblin]      = inventory.getTroopCount(UnitType::kGoblin);
                allTroops[UnitType::kWallBreaker] = inventory.getTroopCount(UnitType::kWallBreaker);

                _battleManager->startBattle(allTroops);
            }

            if (_battleUI)
            {
                // 🔧 修复：确保非攻击方（防守方/观战者）不显示兵种按钮
                bool canDeploy = !_isPvpMode || _isAttacker;

                if (canDeploy)
                {
                    _battleUI->updateStatus("部署你的士兵进行攻击！", Color4B::YELLOW);
                    _battleUI->showBattleHUD(true);
                    _battleUI->showTroopButtons(true);
                }
                else
                {
                    std::string statusMsg = _isPvpMode && !_isAttacker ? "正在被攻击中，无法操作" : "观战中，无法操作";
                    _battleUI->updateStatus(statusMsg, Color4B::RED);
                    _battleUI->showBattleHUD(true);
                    _battleUI->showTroopButtons(false);
                }

                // Initial troop counts update
                if (_battleManager)
                {
                    _battleUI->updateTroopCounts(_battleManager->getTroopCount(UnitType::kBarbarian),
                                                 _battleManager->getTroopCount(UnitType::kArcher),
                                                 _battleManager->getTroopCount(UnitType::kGiant),
                                                 _battleManager->getTroopCount(UnitType::kGoblin),
                                                 _battleManager->getTroopCount(UnitType::kWallBreaker));
                }
            }
        },
        1.0f, "start_battle_delay");

    scheduleUpdate();

    return true;
}

bool BattleScene::initWithReplayData(const std::string& replayDataStr)
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    auto& replaySystem = ReplaySystem::getInstance();
    replaySystem.loadReplay(replayDataStr);

    std::string enemyUserId = replaySystem.getReplayEnemyUserId();
    std::string enemyJson   = replaySystem.getReplayEnemyGameDataJson();

    if (enemyJson.empty())
    {
        CCLOG("❌ Replay data missing enemy game data!");
        return false;
    }

    AccountGameData enemyData = AccountGameData::fromJson(enemyJson);

    srand(replaySystem.getReplaySeed());

    setupMap();
    setupUI();
    setupTouchListeners();

    if (_battleManager)
    {
        _battleManager->init(_mapSprite, enemyData, enemyUserId, true);

        _battleManager->setUIUpdateCallback([this]() {
            if (_battleUI && _battleManager)
            {
                _battleUI->updateTimer(static_cast<int>(_battleManager->getRemainingTime()));
                _battleUI->updateStars(_battleManager->getStars());
                _battleUI->updateDestruction(_battleManager->getDestructionPercent());
            }
        });

        _battleManager->setBattleEndCallback([this]() {
            if (_battleUI && _battleManager)
            {
                int trophyChange = _battleManager->getStars() * 10 - (3 - _battleManager->getStars()) * 3;
                _battleUI->showResultPanel(_battleManager->getStars(), _battleManager->getDestructionPercent(),
                                           _battleManager->getGoldLooted(), _battleManager->getElixirLooted(),
                                           trophyChange, true);
            }
        });
    }

    if (_buildingManager && !enemyData.buildings.empty())
    {
        _buildingManager->loadBuildingsFromData(enemyData.buildings, true);

        if (_battleManager)
        {
            const auto&                buildings = _buildingManager->getBuildings();
            std::vector<BaseBuilding*> buildingVec(buildings.begin(), buildings.end());
            _battleManager->setBuildings(buildingVec);
        }
    }

    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);

    scheduleUpdate();

    replaySystem.setDeployUnitCallback([this](UnitType type, const Vec2& pos) {
        if (_battleManager)
            _battleManager->deployUnit(type, pos);
    });

    replaySystem.setEndBattleCallback([this]() {
        if (_battleManager)
            _battleManager->endBattle(false);
    });

    if (_battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->updateStatus("🔴 战斗回放中", Color4B::RED);
        _battleUI->setEndBattleButtonText("退出回放");
        _battleUI->setReplayMode(true);
        _battleUI->showBattleHUD(true);
    }

    if (_battleManager)
        _battleManager->startBattle(TroopDeploymentMap{});

    return true;
}

// ==================== 场景设置 ====================

void BattleScene::setupMap()
{
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);

    _mapSprite = Sprite::create("map/Map1.png");
    if (_mapSprite)
    {
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        _mapSprite->setScale(1.3f);
        this->addChild(_mapSprite, 0);

        auto mapSize = _mapSprite->getContentSize();
        _gridMap     = GridMap::create(mapSize, 55.6f);
        _gridMap->setStartPixel(Vec2(1406.0f, 2107.2f));
        _mapSprite->addChild(_gridMap, 999);

        _buildingManager = BuildingManager::create();
        this->addChild(_buildingManager);
        _buildingManager->setup(_mapSprite, _gridMap);

        updateBoundary();
    }
}

void BattleScene::disableAllBuildingsBattleMode()
{
    if (!_mapSprite)
        return;

    auto& children = _mapSprite->getChildren();
    for (auto child : children)
    {
        auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(child);
        if (defenseBuilding)
        {
            defenseBuilding->disableBattleMode();
        }
    }
}

void BattleScene::enableAllBuildingsBattleMode()
{
    if (!_buildingManager)
        return;

    auto buildingSprite = _mapSprite;
    if (!buildingSprite)
        return;

    auto& children = buildingSprite->getChildren();
    for (auto child : children)
    {
        auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(child);
        if (defenseBuilding)
        {
            defenseBuilding->enableBattleMode();
        }
    }
}

void BattleScene::setupUI()
{
    if (_buildingManager)
    {
        enableAllBuildingsBattleMode();
    }
    _battleUI = BattleUI::create();
    this->addChild(_battleUI, 100);

    _battleUI->setEndBattleCallback([this]() {
        if (_battleManager && _battleManager->isReplayMode())
        {
            returnToMainScene();
        }
        else if (_battleManager)
        {
            _battleManager->endBattle(true);
        }
    });

    _battleUI->setReturnCallback([this]() { returnToMainScene(); });

    _battleUI->setTroopSelectionCallback([this](UnitType type) { onTroopSelected(type); });
}

void BattleScene::onTroopSelected(UnitType type)
{
    _selectedUnitType = type;
    if (_battleUI)
        _battleUI->highlightTroopButton(type);
}

// ==================== 战斗逻辑 ====================

void BattleScene::update(float dt)
{
    SocketClient::getInstance().processCallbacks();

    float scaledDt = dt * _timeScale;
    if (_battleManager)
    {
        _battleManager->update(scaledDt);
    }
}

void BattleScene::toggleSpeed()
{
    if (_timeScale >= 4.0f)
    {
        _timeScale = 1.0f;
    }
    else
    {
        _timeScale *= 2.0f;
    }
}

// 🆕 PVP Implementation
void BattleScene::setPvpMode(bool isAttacker)
{
    _isPvpMode  = true;
    _isAttacker = isAttacker;

    if (_battleManager)
    {
        _battleManager->setNetworkMode(true, isAttacker);
    }

    if (!_isAttacker && _battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->updateStatus("Defending against attacker...", Color4B::RED);
    }

    // 🆕 Register callbacks immediately to catch packets arriving in the same frame
    auto& client = SocketClient::getInstance();
    client.setOnPvpAction([this](int unitType, float x, float y) {
        if (_battleManager)
        {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, unitType, x, y]() {
                if (_battleManager)
                    _battleManager->deployUnitRemote((UnitType)unitType, Vec2(x, y));
            });
        }
    });

    client.setOnPvpEnd([this](const std::string& result) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this]() {
            CCLOG("🔚 Received PVP_END notification");
            if (_battleManager && _battleManager->getState() != BattleManager::BattleState::FINISHED)
            {
                _battleManager->endBattle(false);
            }
        });
    });

    CCLOG("🎮 PVP Mode set: isAttacker=%d", isAttacker);
}

void BattleScene::setSpectateHistory(const std::vector<std::string>& history)
{
    _spectateHistory = history;
}

void BattleScene::onEnter()
{
    Scene::onEnter();

    // 🆕 Replay spectate history if available
    if (!_spectateHistory.empty() && _battleManager)
    {
        CCLOG("📜 Replaying %zu history actions...", _spectateHistory.size());
        for (const auto& action : _spectateHistory)
        {
            // Format: UnitType|X|Y
            try {
                std::istringstream iss(action);
                std::string token;
                std::getline(iss, token, '|');
                if (token.empty()) continue;
                int unitType = std::stoi(token);
                std::getline(iss, token, '|');
                if (token.empty()) continue;
                float x = std::stof(token);
                std::getline(iss, token, '|');
                if (token.empty()) continue;
                float y = std::stof(token);
                
                _battleManager->deployUnitRemote((UnitType)unitType, Vec2(x, y));
            } catch (...) {
                CCLOG("❌ Error parsing history action: %s", action.c_str());
            }
        }
        _spectateHistory.clear(); // Clear after replay
    }

    if (_isPvpMode)
    {
        // Callbacks are already registered in setPvpMode, but re-registering here ensures
        // they are restored if the scene is popped and re-entered.
        auto& client = SocketClient::getInstance();

        client.setOnPvpAction([this](int unitType, float x, float y) {
            if (_battleManager)
            {
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, unitType, x, y]() {
                    if (_battleManager)
                        _battleManager->deployUnitRemote((UnitType)unitType, Vec2(x, y));
                });
            }
        });

        client.setOnPvpEnd([this](const std::string& result) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this]() {
                CCLOG("🔚 Received PVP_END notification");
                if (_battleManager && _battleManager->getState() != BattleManager::BattleState::FINISHED)
                {
                    _battleManager->endBattle(false);
                }
            });
        });

        if (_battleManager && _isAttacker)
        {
            _battleManager->setNetworkDeployCallback([this](UnitType type, const Vec2& pos) {
                SocketClient::getInstance().sendPvpAction((int)type, pos.x, pos.y);
            });
        }
    }
}

void BattleScene::onExit()
{
    if (_isPvpMode)
    {
        auto& client = SocketClient::getInstance();

        // 🔧 修复：清除所有PVP回调，防止残留
        client.setOnPvpAction(nullptr);
        client.setOnPvpEnd(nullptr);

        if (_battleManager)
        {
            _battleManager->setNetworkDeployCallback(nullptr);
        }

        // 🔧 修复：确保通知服务器结束PVP会话
        if (_isAttacker)
        {
            client.endPvp();
            CCLOG("🔴 [BattleScene] PVP ended by attacker on scene exit");
        }
    }

    Scene::onExit();
}

// ==================== 触摸监听器设置 ====================

void BattleScene::setupTouchListeners()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (_battleManager && _battleManager->getState() == BattleManager::BattleState::FINISHED)
            return false;

        _activeTouches[touch->getID()] = touch->getLocation();

        _lastTouchPos = touch->getLocation();
        _isDragging   = false;
        return true;
    };

    touchListener->onTouchMoved = [this](Touch* touch, Event* event) {
        if (_activeTouches.find(touch->getID()) != _activeTouches.end())
        {
            _activeTouches[touch->getID()] = touch->getLocation();
        }

        if (_activeTouches.size() >= 2)
        {
            _isPinching = true;
            _isDragging = false;

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
                if (currentDist > 10.0f && _mapSprite)
                {
                    float zoomFactor = currentDist / _prevPinchDistance;
                    zoomFactor       = std::max(0.9f, std::min(zoomFactor, 1.1f));

                    float newScale = _mapSprite->getScale() * zoomFactor;
                    newScale       = std::max(0.9f, std::min(newScale, 2.0f));
                    _mapSprite->setScale(newScale);

                    updateBoundary();
                    ensureMapInBoundary();

                    _prevPinchDistance = currentDist;
                }
            }
            return;
        }
        else
        {
            _prevPinchDistance = 0.0f;
        }

        Vec2 currentPos = touch->getLocation();
        Vec2 delta      = currentPos - touch->getPreviousLocation();

        if (currentPos.distance(_lastTouchPos) > 10.0f)
        {
            _isDragging = true;
        }

        if (_mapSprite && _isDragging && !_isPinching)
        {
            Vec2 newPos = _mapSprite->getPosition() + delta;
            _mapSprite->setPosition(newPos);
            ensureMapInBoundary();
        }
    };

    touchListener->onTouchEnded = [this](Touch* touch, Event* event) {
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

        if (!_isDragging && _battleManager &&
            (_battleManager->getState() == BattleManager::BattleState::READY ||
             _battleManager->getState() == BattleManager::BattleState::FIGHTING))
        {
            if (_battleUI && _battleUI->hasSelectedUnit())
            {
                Vec2 touchPos    = touch->getLocation();
                Vec2 mapLocalPos = _mapSprite->convertToNodeSpace(touchPos);
                _battleManager->deployUnit(_selectedUnitType, mapLocalPos);
            }
        }
        _isDragging = false;
    };

    touchListener->onTouchCancelled = [this](Touch* touch, Event* event) {
        _activeTouches.erase(touch->getID());
        if (_activeTouches.size() < 2)
        {
            _prevPinchDistance = 0.0f;
            if (_activeTouches.empty())
                _isPinching = false;
        }
        _isDragging = false;
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

    auto mouseListener           = EventListenerMouse::create();
    mouseListener->onMouseScroll = [this](Event* event) {
        EventMouse* mouseEvent = static_cast<EventMouse*>(event);
        float       scrollY    = mouseEvent->getScrollY();

        if (_mapSprite)
        {
            float zoomFactor = scrollY < 0 ? 1.1f : 0.9f;
            float newScale   = _mapSprite->getScale() * zoomFactor;
            newScale         = std::max(0.9f, std::min(newScale, 2.0f));
            _mapSprite->setScale(newScale);

            updateBoundary();
            ensureMapInBoundary();
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void BattleScene::returnToMainScene()
{
    MusicManager::getInstance().stopMusic();
    disableAllBuildingsBattleMode();

    // 🔧 修复：在切换场景前清理PVP状态
    if (_isPvpMode && _isAttacker)
    {
        SocketClient::getInstance().endPvp();
        CCLOG("🔴 [BattleScene] PVP ended before returning to main scene");
    }

    auto director   = Director::getInstance();
    auto sceneCount = director->getRunningScene() != nullptr ? 1 : 0;

    if (sceneCount <= 1)
    {
        CCLOG("⚠️ 场景栈只有一个场景，创建新的主场景替换");
        auto newScene = DraggableMapScene::createScene();
        director->replaceScene(TransitionFade::create(0.3f, newScene));
    }
    else
    {
        CCLOG("✅ 弹出战斗场景，返回主场景");
        director->popScene();
        director->getScheduler()->performFunctionInCocosThread(
            []() { Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("scene_resume"); });
    }
}

void BattleScene::updateBoundary()
{
    if (!_mapSprite)
        return;
    auto  mapSize = _mapSprite->getContentSize() * _mapSprite->getScale();
    float minX    = _visibleSize.width - mapSize.width / 2;
    float maxX    = mapSize.width / 2;
    float minY    = _visibleSize.height - mapSize.height / 2;
    float maxY    = mapSize.height / 2;
    if (mapSize.width <= _visibleSize.width)
        minX = maxX = _visibleSize.width / 2;
    if (mapSize.height <= _visibleSize.height)
        minY = maxY = _visibleSize.height / 2;
    _mapBoundary = Rect(minX, minY, maxX - minX, maxY - minY);
}

void BattleScene::ensureMapInBoundary()
{
    if (!_mapSprite)
        return;
    Vec2 currentPos = _mapSprite->getPosition();
    Vec2 newPos     = currentPos;
    if (currentPos.x < _mapBoundary.getMinX())
        newPos.x = _mapBoundary.getMinX();
    else if (currentPos.x > _mapBoundary.getMaxX())
        newPos.x = _mapBoundary.getMaxX();
    if (currentPos.y < _mapBoundary.getMinY())
        newPos.y = _mapBoundary.getMinY();
    else if (currentPos.y > _mapBoundary.getMaxY())
        newPos.y = _mapBoundary.getMaxY();
    if (newPos != currentPos)
        _mapSprite->setPosition(newPos);
}