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
#include "GridMap.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/MusicManager.h"
#include "Managers/SocketClient.h"
#include "Managers/TroopInventory.h"
#include "ResourceManager.h"
#include "Unit/UnitTypes.h"
#include <ctime>
#include <sstream> // 🆕 用于解析历史记录

USING_NS_CC;
using namespace ui;

// ==================== 创建场景 ====================

/**
 * @brief 创建默认战斗场景
 * @return Scene* 场景指针
 */
Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

/**
 * @brief 创建带有敌方数据的战斗场景
 * @param enemyData 敌方游戏数据
 * @return BattleScene* 战斗场景指针
 */
BattleScene* BattleScene::createWithEnemyData(const AccountGameData& enemyData)
{
    return createWithEnemyData(enemyData, "Enemy");
}

/**
 * @brief 创建带有敌方数据和ID的战斗场景
 * @param enemyData 敌方游戏数据
 * @param enemyUserId 敌方用户ID
 * @return BattleScene* 战斗场景指针
 */
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

/**
 * @brief 创建回放模式的战斗场景
 * @param replayDataStr 回放数据字符串
 * @return BattleScene* 战斗场景指针
 */
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

/**
 * @brief 初始化场景
 * @return bool 是否成功
 */
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

/**
 * @brief 使用敌方数据初始化
 * @param enemyData 敌方数据
 * @return bool 是否成功
 */
bool BattleScene::initWithEnemyData(const AccountGameData& enemyData)
{
    return initWithEnemyData(enemyData, "Enemy");
}

/**
 * @brief 使用敌方数据和ID初始化
 * @param enemyData 敌方数据
 * @param enemyUserId 敌方ID
 * @return bool 是否成功
 */
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

                // 🆕 PVP End
                if (_isPvpMode && _isAttacker)
                {
                    SocketClient::getInstance().endPvp();
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
            const auto& buildings = _buildingManager->getBuildings();
            // Convert list<BaseBuilding*> to vector<BaseBuilding*>
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

    // Delay start battle
    this->scheduleOnce(
        [this](float dt) {
            if (_battleManager)
            {
                // 获取当前玩家的兵力库存
                auto troops = TroopInventory::getInstance().getAllTroops();
                _battleManager->startBattle(troops);
            }

            if (_battleUI)
            {
                _battleUI->updateStatus("部署你的士兵进行攻击！", Color4B::YELLOW);
                _battleUI->showBattleHUD(true);
                _battleUI->showTroopButtons(true);
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

/**
 * @brief 使用回放数据初始化
 * @param replayDataStr 回放数据
 * @return bool 是否成功
 */
bool BattleScene::initWithReplayData(const std::string& replayDataStr)
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    // 加载回放数据
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

    // 设置随机种子
    srand(replaySystem.getReplaySeed());

    setupMap();
    setupUI();
    setupTouchListeners();

    // Initialize Manager
    if (_battleManager)
    {
        _battleManager->init(_mapSprite, enemyData, enemyUserId, true);

        // Setup callbacks (same as above)
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

    // Load Buildings
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

    // 🎵 回放模式直接播放战斗音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);

    scheduleUpdate();

    // 设置回放回调
    replaySystem.setDeployUnitCallback([this](UnitType type, const Vec2& pos) {
        if (_battleManager)
            _battleManager->deployUnit(type, pos);
    });

    replaySystem.setEndBattleCallback([this]() {
        if (_battleManager)
            _battleManager->endBattle(false);
    });

    // UI Setup for Replay
    if (_battleUI)
    {
        _battleUI->showTroopButtons(false);
        _battleUI->updateStatus("🔴 战斗回放中", Color4B::RED);
        _battleUI->setEndBattleButtonText("退出回放");
        _battleUI->setReplayMode(true);
        _battleUI->showBattleHUD(true);
    }

    // Start Battle immediately for replay
    if (_battleManager)
    {
        // 🆕 计算回放所需的兵力
        // 必须统计回放事件中使用的所有兵力，否则 BattleManager 会因为兵力为0而拒绝部署
        std::map<UnitType, int> neededTroops;
        const auto&             replayData = replaySystem.getCurrentReplayData();
        for (const auto& evt : replayData.events)
        {
            if (evt.type == ReplayEventType::DEPLOY_UNIT)
            {
                neededTroops[static_cast<UnitType>(evt.unitType)]++;
            }
        }

        _battleManager->startBattle(neededTroops);
    }

    return true;
}

// ==================== 场景设置 ====================

/**
 * @brief 设置地图和相关组件
 */
void BattleScene::setupMap()
{
    // 创建地图背景
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);

    // 创建地图精灵
    _mapSprite = Sprite::create("map/Map1.png"); // 使用默认地图
    if (_mapSprite)
    {
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        _mapSprite->setScale(1.3f);
        this->addChild(_mapSprite, 0);

        // 创建网格
        auto mapSize = _mapSprite->getContentSize();
        _gridMap     = GridMap::create(mapSize, 55.6f);
        _gridMap->setStartPixel(Vec2(1406.0f, 2107.2f));
        _mapSprite->addChild(_gridMap, 999);

        // 创建建筑管理器
        _buildingManager = BuildingManager::create();
        this->addChild(_buildingManager);
        _buildingManager->setup(_mapSprite, _gridMap);

        updateBoundary(); // 初始化边界
    }
}

/**
 * @brief 禁用所有建筑的战斗模式
 */
void BattleScene::disableAllBuildingsBattleMode()
{
    // 战斗结束时禁用战斗模式
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

/**
 * @brief 启用所有建筑的战斗模式
 */
void BattleScene::enableAllBuildingsBattleMode()
{
    // 遍历所有建筑
    if (!_buildingManager)
        return;

    // 需要在 BuildingManager 中实现此方法
    // 或者直接遍历场景中的建筑
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

/**
 * @brief 设置UI界面
 */
void BattleScene::setupUI()
{
    // 启用所有防御建筑的战斗模式
    if (_buildingManager)
    {
        // 这需要在 BuildingManager 中添加一个方法来启用所有建筑的战斗模式
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
            _battleManager->endBattle(true); // 投降
        }
    });

    _battleUI->setReturnCallback([this]() { returnToMainScene(); });

    _battleUI->setTroopSelectionCallback([this](UnitType type) { onTroopSelected(type); });
}

/**
 * @brief 处理兵种选择事件
 * @param type 选中的兵种类型
 */
void BattleScene::onTroopSelected(UnitType type)
{
    _selectedUnitType = type;
    if (_battleUI)
        _battleUI->highlightTroopButton(type);
}

// ==================== 战斗逻辑 ====================

/**
 * @brief 每帧更新
 * @param dt 时间间隔
 */
void BattleScene::update(float dt)
{
    // Process network callbacks in main thread
    SocketClient::getInstance().processCallbacks();

    float scaledDt = dt * _timeScale;
    if (_battleManager)
    {
        _battleManager->update(scaledDt);
    }
}

/**
 * @brief 切换游戏速度
 */
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
/**
 * @brief 设置PVP模式
 * @param isAttacker 是否为攻击方
 */
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
}

/**
 * @brief 设置观战历史记录
 * @param history 历史操作记录
 */
void BattleScene::setSpectateHistory(const std::vector<std::string>& history)
{
    if (!_battleManager)
        return;

    CCLOG("📺 Replaying spectate history: %zu actions", history.size());

    for (const auto& action : history)
    {
        // 格式解析: "type,x,y"
        std::vector<std::string> parts;
        std::stringstream        ss(action);
        std::string              item;
        while (std::getline(ss, item, ','))
        {
            parts.push_back(item);
        }

        if (parts.size() >= 3)
        {
            try
            {
                int   type = std::stoi(parts[0]);
                float x    = std::stof(parts[1]);
                float y    = std::stof(parts[2]);

                _battleManager->deployUnitRemote(static_cast<UnitType>(type), Vec2(x, y));
            }
            catch (const std::exception& e)
            {
                CCLOG("❌ Failed to parse history action: %s (%s)", action.c_str(), e.what());
            }
        }
    }
}

/**
 * @brief 场景进入回调
 */
void BattleScene::onEnter()
{
    Scene::onEnter();

    if (_isPvpMode)
    {
        auto& client = SocketClient::getInstance();

        // 接收远程操作
        client.setOnPvpAction([this](int unitType, float x, float y) {
            if (_battleManager)
            {
                // 确保在主线程执行
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, unitType, x, y]() {
                    if (_battleManager)
                        _battleManager->deployUnitRemote((UnitType)unitType, Vec2(x, y));
                });
            }
        });

        // 接收结束通知
        client.setOnPvpEnd([this](const std::string& result) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this]() {
                if (_battleManager)
                {
                    _battleManager->endBattle(false);
                }
            });
        });

        // 发送本地操作
        if (_battleManager && _isAttacker)
        {
            _battleManager->setNetworkDeployCallback([this](UnitType type, const Vec2& pos) {
                SocketClient::getInstance().sendPvpAction((int)type, pos.x, pos.y);
            });
        }
    }
}

/**
 * @brief 场景退出回调
 */
void BattleScene::onExit()
{
    if (_isPvpMode)
    {
        auto& client = SocketClient::getInstance();
        client.setOnPvpAction(nullptr);
        client.setOnPvpEnd(nullptr);

        if (_battleManager)
        {
            _battleManager->setNetworkDeployCallback(nullptr);
        }

        if (_isAttacker)
        {
            client.endPvp();
        }
    }

    Scene::onExit();
}

// ==================== 触摸监听器设置 ====================

/**
 * @brief 设置触摸监听器
 */
void BattleScene::setupTouchListeners()
{
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (_battleManager && _battleManager->getState() == BattleManager::BattleState::FINISHED)
            return false;

        _activeTouches[touch->getID()] = touch->getLocation(); // ✅ 提前记录触摸点

        _lastTouchPos = touch->getLocation();
        _isDragging   = false;
        return true;
    };

    touchListener->onTouchMoved = [this](Touch* touch, Event* event) {
        if (_activeTouches.find(touch->getID()) != _activeTouches.end())
        {
            _activeTouches[touch->getID()] = touch->getLocation();
        }

        // 🆕 多点触控缩放
        if (_activeTouches.size() >= 2)
        {
            _isPinching = true;
            _isDragging = false; // 取消拖动标记

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
                    // 限制单帧缩放
                    zoomFactor = std::max(0.9f, std::min(zoomFactor, 1.1f));

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
            Vec2 touchPos    = touch->getLocation();
            Vec2 mapLocalPos = _mapSprite->convertToNodeSpace(touchPos);
            _battleManager->deployUnit(_selectedUnitType, mapLocalPos);
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

/**
 * @brief 返回主场景
 */
void BattleScene::returnToMainScene()
{
    MusicManager::getInstance().stopMusic();
    // 禁用所有建筑的战斗模式
    disableAllBuildingsBattleMode();
    Director::getInstance()->popScene();
    Director::getInstance()->getScheduler()->performFunctionInCocosThread(
        []() { Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("scene_resume"); });
}

/**
 * @brief 更新地图边界
 */
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

/**
 * @brief 确保地图在边界内
 */
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