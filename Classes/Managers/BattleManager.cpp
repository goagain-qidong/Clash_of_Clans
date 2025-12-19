/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleManager.cpp
 * File Function: 战斗逻辑实现 - 管理战斗流程和状态
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#include "BattleManager.h"
#include "Managers/TroopInventory.h"
#include "Managers/MusicManager.h"
#include "Managers/DefenseLogSystem.h"
#include "ResourceManager.h"
#include "PathFinder.h"
#include <ctime>

USING_NS_CC;

BattleManager::BattleManager()
{
}

BattleManager::~BattleManager()
{
}

void BattleManager::init(cocos2d::Node* mapLayer, const AccountGameData& enemyData, const std::string& enemyUserId, bool isReplay)
{
    _mapLayer = mapLayer;

    // 🔍 尝试 1: 直接转换
    _gridMap = dynamic_cast<GridMap*>(mapLayer);

    // 🔍 尝试 2: 如果 mapLayer 是容器，遍历子节点查找 GridMap
    if (!_gridMap && mapLayer)
    {
        for (auto child : mapLayer->getChildren())
        {
            _gridMap = dynamic_cast<GridMap*>(child);
            if (_gridMap) break;
        }
    }

    if (_gridMap) {
        CCLOG("✅ BattleManager: GridMap successfully linked!");
    }
    else {
        CCLOG("❌ ERROR: BattleManager could not find GridMap! Pathfinding will FAIL.");
    }

    _enemyGameData = enemyData;
    _enemyUserId = enemyUserId;
    _isReplayMode = isReplay;
    _state = BattleState::LOADING;

    _elapsedTime = 0.0f;
    _starsEarned = 0;
    _goldLooted = 0;
    _elixirLooted = 0;
    _destructionPercent = 0;
    _accumulatedTime = 0.0f;
    _currentFrame = 0;
    _deployedUnits.clear();
    _enemyBuildings.clear();
}

//
void BattleManager::setBuildings(const std::vector<BaseBuilding*>& buildings)
{
    _enemyBuildings = buildings;
    _totalBuildingHP = 0;
    _destroyedBuildingHP = 0;

    if (!_gridMap) CCLOG("❌ WARNING: setBuildings called but _gridMap is null!");

    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            _totalBuildingHP += building->getMaxHitpoints();

            // 🆕 标记障碍物
            if (_gridMap) {
                // 获取建筑的网格坐标和尺寸
                Vec2 gridPos = building->getGridPosition();
                Size gridSize = building->getGridSize();

                // 标记为占用 (不可通行)
                _gridMap->markArea(gridPos, gridSize, true);

                // 🔍 调试日志：确认位置是否正确
                // Vec2 worldPos = building->getPosition();
                // CCLOG("🏗️ Building at Grid(%.0f,%.0f) Size(%.0f,%.0f) marked as obstacle.", 
                //       gridPos.x, gridPos.y, gridSize.width, gridSize.height);
            }
        }
    }
}

// ------------------------------------------------------------------------------------
// 🆕 修正：startBattle 接受 TroopDeploymentMap
// ------------------------------------------------------------------------------------
void BattleManager::startBattle(const TroopDeploymentMap& deployment)
{
    _state = BattleState::READY;
    _elapsedTime = 0.0f;

    // 🎵 Play music
    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);

    // 🔴 启用所有建筑的战斗模式
    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            building->enableBattleMode();
        }
    }

    // 🆕 核心：用玩家选择的军队数量初始化本地军队计数
    _barbarianCount = 0;
    _archerCount = 0;
    _giantCount = 0;
    _goblinCount = 0;
    _wallBreakerCount = 0;

    for (const auto& pair : deployment)
    {
        switch (pair.first)
        {
        case UnitType::kBarbarian: _barbarianCount = pair.second; break;
        case UnitType::kArcher:    _archerCount = pair.second;    break;
        case UnitType::kGiant:     _giantCount = pair.second;     break;
        case UnitType::kGoblin:    _goblinCount = pair.second;    break;
        case UnitType::kWallBreaker: _wallBreakerCount = pair.second; break;
        }
    }

    CCLOG("📦 Deployed Troops: Barb=%d, Arch=%d, Giant=%d, Goblin=%d, WallBreaker=%d",
        _barbarianCount, _archerCount, _giantCount, _goblinCount, _wallBreakerCount);

    if (!_isReplayMode)
    {
        // 🚨 重要：扣除库存中的军队 (因为玩家已经确认部署)
        auto& troopInv = TroopInventory::getInstance();
        for (const auto& pair : deployment)
        {
            if (pair.second > 0)
            {
                // 注意：这里需要假设 consumeTroops 不会失败，因为 ArmySelectionUI 已经保证了数量有效
                troopInv.consumeTroops(pair.first, pair.second);
            }
        }

        // Start recording
        unsigned int seed = static_cast<unsigned int>(time(nullptr));
        srand(seed);
        ReplaySystem::getInstance().startRecording(_enemyUserId, _enemyGameData.toJson(), seed);
    }

    if (_onUIUpdate) _onUIUpdate();
}

void BattleManager::update(float dt)
{
    if (_state == BattleState::READY || _state == BattleState::FIGHTING)
    {
        _accumulatedTime += dt;
        
        while (_accumulatedTime >= FIXED_TIME_STEP)
        {
            fixedUpdate();
            _accumulatedTime -= FIXED_TIME_STEP;
        }
    }
}

void BattleManager::fixedUpdate()
{
    _currentFrame++;
    
    if (_isReplayMode)
    {
        ReplaySystem::getInstance().updateFrame(_currentFrame);
    }
    
    updateBattleState(FIXED_TIME_STEP);
}

void BattleManager::updateBattleState(float dt)
{
    _elapsedTime += dt;
    float remainingTime = _battleTime - _elapsedTime;

    if (remainingTime <= 0)
    {
        endBattle(false);
        return;
    }

    // Update Z-Order
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->IsDead())
        {
            int newZOrder = 10000 - static_cast<int>(unit->getPositionY());
            unit->setLocalZOrder(newZOrder);
        }
    }
    
    for (auto* building : _enemyBuildings)
    {
        if (building && !building->isDestroyed())
        {
            int newZOrder = 10000 - static_cast<int>(building->getPositionY());
            building->setLocalZOrder(newZOrder);
        }
    }

    // Update AI and Physics
    updateUnitAI(dt);
    
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->IsDead())
        {
            unit->tick(dt);
        }
    }
    
    for (auto* building : _enemyBuildings)
    {
        if (building && building->isDefenseBuilding())
        {
            auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(building);
            if (defenseBuilding)
            {
                defenseBuilding->tick(dt);
                defenseBuilding->detectEnemies(_deployedUnits);
            }
        }
    }
    
    // Calculate Destruction
    _destroyedBuildingHP = 0;
    int destroyedCount = 0;
    
    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            int lostHP = building->getMaxHitpoints() - building->getHitpoints();
            _destroyedBuildingHP += lostHP;
            
            if (building->isDestroyed())
            {
                destroyedCount++;
            }
        }
    }
    
    if (_totalBuildingHP > 0)
    {
        int newDestruction = (_destroyedBuildingHP * 100) / _totalBuildingHP;
        if (newDestruction != _destructionPercent)
        {
            _destructionPercent = std::min(newDestruction, 100);
            
            // Calculate Stars
            int stars = 0;
            if (_destructionPercent >= 33) stars = 1;
            if (_destructionPercent >= 67) stars = 2;
            if (_destructionPercent == 100) stars = 3;
            
            if (stars > _starsEarned)
            {
                _starsEarned = stars;
            }
            
            if (_onUIUpdate) _onUIUpdate();
        }
    }
    
    if (destroyedCount == _enemyBuildings.size() && !_enemyBuildings.empty())
    {
        CCLOG("🎉 All buildings destroyed!");
        endBattle(false);
    }
    else if (_onUIUpdate)
    {
        // Frequent UI update for timer
        _onUIUpdate(); 
    }
}

void BattleManager::deployUnit(UnitType type, const cocos2d::Vec2& position)
{
    // 如果是联网模式且不是攻击者（即防御者或观战者），禁止本地操作下兵
    if (_isNetworked && !_isAttacker)
    {
        return;
    }

    int* count = nullptr;
    switch (type)
    {
    case UnitType::kBarbarian: count = &_barbarianCount; break;
    case UnitType::kArcher: count = &_archerCount; break;
    case UnitType::kGiant: count = &_giantCount; break;
    case UnitType::kGoblin: count = &_goblinCount; break;
    case UnitType::kWallBreaker: count = &_wallBreakerCount; break;
    default: return;
    }

    // 逻辑修正：现在库存已经在 startBattle 中转移到 BattleManager 的本地变量了
    // deployUnit 只需要消耗 BattleManager 的本地计数

    if (*count <= 0) return; // 检查本地计数

    // 无论是回放还是实时模式，本地计数都应该减少
    (*count)--;
    if (_onTroopDeploy) _onTroopDeploy(type, *count);

    if (!_isReplayMode)
    {
        ReplaySystem::getInstance().recordDeployUnit(_currentFrame, type, position);

        // 🆕 发送网络包
        if (_isNetworked && _isAttacker && _onNetworkDeploy)
        {
            _onNetworkDeploy(type, position);
        }
    }

    spawnUnit(type, position);
}

void BattleManager::deployUnitRemote(UnitType type, const cocos2d::Vec2& position)
{
    // 远程下兵（防御者/观战者接收到的操作）
    // 不消耗本地库存，直接生成
    spawnUnit(type, position);
}

void BattleManager::spawnUnit(UnitType type, const cocos2d::Vec2& position)
{
    Unit* unit = Unit::create(type);
    if (!unit) return;

    unit->setPosition(position);
    unit->enableBattleMode();

    int zOrder = 10000 - static_cast<int>(position.y);
    if (_mapLayer) _mapLayer->addChild(unit, zOrder);
    _deployedUnits.push_back(unit);

    // 🆕 关键修正：只要放兵，且不是结束状态，就进入战斗状态，确保 tick 被调用
    if (_state == BattleState::READY || _state == BattleState::LOADING)
    {
        if (_state == BattleState::LOADING) {
            CCLOG("Auto-starting battle on unit deploy");
            // 如果在 loading 阶段就放兵，可能需要手动触发 startBattle 的部分逻辑
            // 但通常建议 UI 层面控制好。这里强制设为 FIGHTING 确保兵能动
        }
        _state = BattleState::FIGHTING;
        activateAllBuildings();
    }
}

//

void BattleManager::updateUnitAI(float dt)
{
    // 必须确保 _mapLayer 被正确识别为 GridMap
    GridMap* gridMap = dynamic_cast<GridMap*>(_mapLayer);

    for (auto it = _deployedUnits.begin(); it != _deployedUnits.end();)
    {
        Unit* unit = *it;
        if (!unit || unit->IsDead())
        {
            // 简单处理：跳过已死亡单位，等待下一帧清理或由容器管理
            ++it;
            continue;
        }

        BaseBuilding* target = unit->getTarget();

        // --------------------------------------------------------
        // 1. 如果没有目标，或者目标已被摧毁 -> 寻找新目标
        // --------------------------------------------------------
        if (!target || target->isDestroyed())
        {
            unit->clearTarget();

            BaseBuilding* bestTarget = nullptr;
            float minDistance = 999999.0f;
            Vec2 unitPos = unit->getPosition();

            // 辅助 Lambda：寻找符合条件的最近建筑
            auto findTargetWithFilter = [&](std::function<bool(BaseBuilding*)> filter) -> BaseBuilding* {
                BaseBuilding* closest = nullptr;
                float minDist = 999999.0f;
                for (auto* b : _enemyBuildings) {
                    if (b && !b->isDestroyed() && filter(b)) {
                        float dist = unitPos.distance(b->getPosition());
                        if (dist < minDist) {
                            minDist = dist;
                            closest = b;
                        }
                    }
                }
                return closest;
                };

            // 根据兵种偏好选择目标
            if (unit->GetType() == UnitType::kGiant) {
                // 巨人优先打防御
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return b->isDefenseBuilding(); });
            }
            else if (unit->GetType() == UnitType::kGoblin) {
                // 哥布林优先打资源
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return b->getBuildingType() == BuildingType::kResource; });
            }
            else if (unit->GetType() == UnitType::kWallBreaker) {
                // 炸弹人优先打墙
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return b->getBuildingType() == BuildingType::kWall; });
            }

            // 如果没有找到优先目标（或不是优先兵种），则攻击最近的任意建筑
            if (!bestTarget) {
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return true; });
            }

            // 锁定目标
            if (bestTarget) {
                unit->setTarget(bestTarget);
                target = bestTarget;
                // 找到新目标后，立即让单位停止当前动作，准备下一帧的寻路
                unit->StopMoving();
            }
        }

        // --------------------------------------------------------
        // 2. 执行攻击或移动逻辑
        // --------------------------------------------------------
        if (target && !target->isDestroyed())
        {
            // ✅ 修复点：在这里明确定义 targetPos，供后续逻辑使用
            Vec2 targetPos = target->getPosition();

            // 检查是否在攻击范围内
            if (unit->isInAttackRange(targetPos))
            {
                // 到达范围，停止移动
                if (unit->isMoving()) unit->StopMoving();

                unit->updateAttackCooldown(dt);
                if (unit->isAttackReady())
                {
                    // --- 炸弹人自爆逻辑 ---
                    if (unit->GetType() == UnitType::kWallBreaker) {
                        unit->Attack(false);
                        // 炸弹人造成极高伤害（这里简化为直接伤害目标，实际可能是范围AOE）
                        float damage = unit->getDamage() * 40.0f;
                        target->takeDamage(damage);
                        unit->Die(); // 自爆后死亡
                    }
                    // --- 普通单位攻击逻辑 ---
                    else {
                        unit->Attack(false);
                        target->takeDamage(unit->getDamage());
                        unit->resetAttackCooldown();
                    }

                    // 检查目标是否被摧毁
                    if (target->isDestroyed()) {
                        unit->clearTarget();
                        // 🆕 核心：建筑被摧毁后，释放占用的网格
                        if (gridMap) {
                            gridMap->markArea(target->getGridPosition(), target->getGridSize(), false);
                        }
                    }
                }
            }
            else
            {
                // 不在攻击范围内，需要移动

                // 优化：仅当单位当前静止（刚生成/刚打完/被阻挡）时才重新计算路径
                // 或者目标位置发生显著变化时才寻路。避免每帧调用 A*
                bool needsPathfinding = !unit->isMoving();

                if (needsPathfinding)
                {
                    if (gridMap) {
                        // 炸弹人通常要走向城墙，不需要“绕开”作为目标的城墙
                        // 但对于其他障碍物，炸弹人也需要绕。
                        // 这里简化：如果兵种是炸弹人，ignoreWalls = false (因为 GridMap 里所有建筑都是障碍)
                        // PathFinder 会计算到目标（墙）相邻格子的路径

                        std::vector<Vec2> path = PathFinder::getInstance().findPath(
                            gridMap,
                            unit->getPosition(),
                            targetPos, // ✅ 这里使用了上面定义的 targetPos
                            false // ignoreWalls: false 表示必须绕墙
                        );

                        // 让单位沿着路径点移动
                        unit->MoveToPath(path);
                    }
                    else {
                        // 如果没有地图数据，降级为直线移动
                        unit->MoveTo(targetPos);
                    }
                }
            }
        }

        ++it;
    }
}

void BattleManager::activateAllBuildings() 
{
    for (auto* building : _enemyBuildings)
    {
        if (building) // 只要是建筑就执行
        {
            // 尝试直接调用父类的 enableBattleMode
            // 前提是你已经按第2点将方法移到了 BaseBuilding
            building->enableBattleMode();
        }
    }
}

void BattleManager::endBattle(bool surrender)
{
    if (_state == BattleState::FINISHED) return;
    _state = BattleState::FINISHED;
    
    if (!_isReplayMode)
    {
        ReplaySystem::getInstance().recordEndBattle(_currentFrame);
    }
    
    calculateBattleResult();
    
    // ... existing code ...
    if (_starsEarned > 0)
        MusicManager::getInstance().playMusic(MusicType::BATTLE_WIN, false);
    else
        MusicManager::getInstance().playMusic(MusicType::BATTLE_LOSE, false);

    if (!_isReplayMode && !_isNetworked) // 🆕 联网模式下不保存本地结果
    {
        // 🆕 核心修改：战斗结束时，将剩余未使用的士兵返还到库存
        // startBattle 时已消耗了所有带入的士兵，这里只需把没用完的加回去
        auto& inventory = TroopInventory::getInstance();
        inventory.addTroops(UnitType::kBarbarian, _barbarianCount);
        inventory.addTroops(UnitType::kArcher, _archerCount);
        inventory.addTroops(UnitType::kGiant, _giantCount);
        inventory.addTroops(UnitType::kGoblin, _goblinCount);
        inventory.addTroops(UnitType::kWallBreaker, _wallBreakerCount);

        // 保存数据（此时 Inventory 已包含剩余士兵，会正确写入 JSON）
        AccountManager::getInstance().saveGameStateToFile();
        uploadBattleResult();
    }

    if (_onBattleEnd)
        _onBattleEnd();
}

void BattleManager::calculateBattleResult()
{
    int maxGold = _enemyGameData.gold;
    int maxElixir = _enemyGameData.elixir;
    float lootRate = 0.3f;
    
    _goldLooted = static_cast<int>(maxGold * (_destructionPercent / 100.0f) * lootRate);
    _elixirLooted = static_cast<int>(maxElixir * (_destructionPercent / 100.0f) * lootRate);
    
    auto& resMgr = ResourceManager::getInstance();
    resMgr.addResource(ResourceType::kGold, _goldLooted);
    resMgr.addResource(ResourceType::kElixir, _elixirLooted);
}

void BattleManager::uploadBattleResult()
{
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (!currentAccount) return;

    DefenseLog defenseLog;
    defenseLog.attackerId = currentAccount->userId;
    defenseLog.attackerName = currentAccount->username;
    defenseLog.starsLost = _starsEarned;
    defenseLog.goldLost = _goldLooted;
    defenseLog.elixirLost = _elixirLooted;
    defenseLog.trophyChange = -(_starsEarned * 10 - (3 - _starsEarned) * 3);
    defenseLog.timestamp = getCurrentTimestamp();
    defenseLog.isViewed = false;
    defenseLog.replayData = ReplaySystem::getInstance().stopRecording();

    std::string attackerUserId = currentAccount->userId;
    if (accMgr.switchAccount(_enemyUserId, true))
    {
        DefenseLogSystem::getInstance().load();
        DefenseLogSystem::getInstance().addDefenseLog(defenseLog);
        accMgr.switchAccount(attackerUserId, true);
        DefenseLogSystem::getInstance().load();
    }
}

std::string BattleManager::getCurrentTimestamp()
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

float BattleManager::getRemainingTime() const
{
    return std::max(0.0f, _battleTime - _elapsedTime);
}

int BattleManager::getTroopCount(UnitType type) const
{
    switch (type)
    {
        case UnitType::kBarbarian: return _barbarianCount;
        case UnitType::kArcher: return _archerCount;
        case UnitType::kGiant: return _giantCount;
        case UnitType::kGoblin: return _goblinCount;
        case UnitType::kWallBreaker: return _wallBreakerCount;
        default: return 0;
    }
}

void BattleManager::setNetworkMode(bool isNetworked, bool isAttacker)
{
    _isNetworked = isNetworked;
    _isAttacker = isAttacker;
}

void BattleManager::setNetworkDeployCallback(const std::function<void(UnitType, const cocos2d::Vec2&)>& callback)
{
    _onNetworkDeploy = callback;
}

void BattleManager::setBattleMode(BattleMode mode, const std::string& warId)
{
    _battleMode   = mode;
    _currentWarId = warId;
}

bool BattleManager::canDeployUnit() const
{
    switch (_battleMode)
    {
    case BattleMode::LOCAL:
    case BattleMode::PVP_ATTACK:
    case BattleMode::CLAN_WAR_ATTACK:
        return true;
    case BattleMode::PVP_DEFEND:
    case BattleMode::CLAN_WAR_DEFEND:
    case BattleMode::SPECTATE:
        return false;
    }
    return false;
}

int BattleManager::calculateStars() const
{
    int stars = 0;

    // 检查是否摧毁大本营
    bool townHallDestroyed = false;
    for (auto* building : _enemyBuildings)
    {
        if (building && building->getBuildingType() == BuildingType::kTownHall && building->isDestroyed())
        {
            townHallDestroyed = true;
            break;
        }
    }
    if (townHallDestroyed)
        stars++;

    float destruction = calculateDestructionRate();
    if (destruction >= 0.5f)
        stars++;

    if (destruction >= 1.0f)
        stars++;

    return stars;
}

float BattleManager::calculateDestructionRate() const
{
    if (_enemyBuildings.empty())
        return 0.0f;
    
    int totalBuildings     = static_cast<int>(_enemyBuildings.size());
    int destroyedBuildings = 0;

    for (auto* building : _enemyBuildings)
    {
        if (building && building->isDestroyed())
            destroyedBuildings++;
    }

    return totalBuildings > 0 ? static_cast<float>(destroyedBuildings) / totalBuildings : 0.0f;
}