/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleManager.cpp
 * File Function: 战斗逻辑实现 - 管理战斗流程和状态
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/

#include "BattleManager.h"

#include "Managers/DefenseLogSystem.h"
#include "Managers/MusicManager.h"
#include "Managers/TroopInventory.h"
#include "PathFinder.h"
#include "ResourceManager.h"
#include "Unit/UnitFactory.h"

#include <ctime>

USING_NS_CC;

BattleManager::BattleManager() {}

BattleManager::~BattleManager() {}

void BattleManager::init(cocos2d::Node* mapLayer, const AccountGameData& enemyData, const std::string& enemyUserId,
                         bool isReplay)
{
    _mapLayer = mapLayer;

    _gridMap = dynamic_cast<GridMap*>(mapLayer);

    if (!_gridMap && mapLayer)
    {
        for (auto child : mapLayer->getChildren())
        {
            _gridMap = dynamic_cast<GridMap*>(child);
            if (_gridMap)
                break;
        }
    }

    if (_gridMap)
    {
        CCLOG("✅ BattleManager: GridMap successfully linked!");
    }
    else
    {
        CCLOG("❌ ERROR: BattleManager could not find GridMap! Pathfinding will FAIL.");
    }

    _enemyGameData = enemyData;
    _enemyUserId   = enemyUserId;
    _isReplayMode  = isReplay;
    _state         = BattleState::LOADING;

    _elapsedTime        = 0.0f;
    _starsEarned        = 0;
    _goldLooted         = 0;
    _elixirLooted       = 0;
    _destructionPercent = 0;
    _accumulatedTime    = 0.0f;
    _currentFrame       = 0;

    // 🆕 重置战斗结束状态
    _endReason          = BattleEndReason::TIMEOUT;
    _townHallDestroyed  = false;
    _hasDeployedAnyUnit = false;

    _deployedUnits.clear();
    _enemyBuildings.clear();
}

void BattleManager::setBuildings(const std::vector<BaseBuilding*>& buildings)
{
    _enemyBuildings      = buildings;
    _totalBuildingHP     = 0;
    _destroyedBuildingHP = 0;

    if (!_gridMap)
        CCLOG("❌ WARNING: setBuildings called but _gridMap is null!");

    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            _totalBuildingHP += building->getMaxHitpoints();

            if (_gridMap)
            {
                Vec2 gridPos  = building->getGridPosition();
                Size gridSize = building->getGridSize();
                _gridMap->markArea(gridPos, gridSize, true);
            }
        }
    }
}

void BattleManager::startBattle(const TroopDeploymentMap& deployment)
{
    _state       = BattleState::READY;
    _elapsedTime = 0.0f;

    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);

    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            building->enableBattleMode();
        }
    }

    _barbarianCount   = 0;
    _archerCount      = 0;
    _giantCount       = 0;
    _goblinCount      = 0;
    _wallBreakerCount = 0;

    for (const auto& pair : deployment)
    {
        switch (pair.first)
        {
        case UnitType::kBarbarian:
            _barbarianCount = pair.second;
            break;
        case UnitType::kArcher:
            _archerCount = pair.second;
            break;
        case UnitType::kGiant:
            _giantCount = pair.second;
            break;
        case UnitType::kGoblin:
            _goblinCount = pair.second;
            break;
        case UnitType::kWallBreaker:
            _wallBreakerCount = pair.second;
            break;
        }
    }

    CCLOG("📦 Deployed Troops: Barb=%d, Arch=%d, Giant=%d, Goblin=%d, WallBreaker=%d", _barbarianCount, _archerCount,
          _giantCount, _goblinCount, _wallBreakerCount);

    if (!_isReplayMode)
    {
        auto& troopInv = TroopInventory::getInstance();
        for (const auto& pair : deployment)
        {
            if (pair.second > 0)
            {
                troopInv.consumeTroops(pair.first, pair.second);
            }
        }

        unsigned int seed = static_cast<unsigned int>(time(nullptr));
        srand(seed);
        ReplaySystem::getInstance().startRecording(_enemyUserId, _enemyGameData.toJson(), seed);
    }

    if (_onUIUpdate)
        _onUIUpdate();
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

    // Update Z-Order
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->isDead())
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
        if (unit && !unit->isDead())
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

    // 🆕 统一的星星和破坏率计算
    updateStarsAndDestruction();

    // 🆕 统一的战斗结束条件检查
    checkBattleEndConditions();

    if (_onUIUpdate)
        _onUIUpdate();
}

// 🆕 统一的星星和破坏率更新逻辑
void BattleManager::updateStarsAndDestruction()
{
    if (_enemyBuildings.empty())
    {
        _destructionPercent = 100;
        _starsEarned        = 3;
        return;
    }

    // 计算破坏的 HP
    _destroyedBuildingHP = 0;
    int  destroyedCount  = 0;
    bool townHallFound   = false;

    for (auto* building : _enemyBuildings)
    {
        if (!building)
            continue;

        int lostHP = building->getMaxHitpoints() - building->getHitpoints();
        _destroyedBuildingHP += lostHP;

        if (building->isDestroyed())
        {
            destroyedCount++;

            // 检测大本营是否被摧毁
            if (building->getBuildingType() == BuildingType::kTownHall)
            {
                if (!_townHallDestroyed)
                {
                    _townHallDestroyed = true;
                    CCLOG("⭐ Town Hall destroyed! +1 Star");
                }
            }
        }

        if (building->getBuildingType() == BuildingType::kTownHall)
        {
            townHallFound = true;
        }
    }

    // 计算破坏率 (基于 HP)
    if (_totalBuildingHP > 0)
    {
        _destructionPercent = std::min(100, (_destroyedBuildingHP * 100) / _totalBuildingHP);
    }

    // 🆕 统一的星星计算逻辑
    int newStars = 0;

    // 规则1: 摧毁大本营 = 1 星
    if (_townHallDestroyed)
    {
        newStars++;
    }

    // 规则2: 破坏率 >= 50% = 1 星
    if (_destructionPercent >= 50)
    {
        newStars++;
    }

    // 规则3: 破坏率 = 100% = 1 星 (总共最多3星)
    if (_destructionPercent >= 100)
    {
        newStars++;
    }

    // 只能增加星星，不能减少
    if (newStars > _starsEarned)
    {
        int gained   = newStars - _starsEarned;
        _starsEarned = newStars;
        CCLOG("⭐ Stars updated: %d (+%d), Destruction: %d%%", _starsEarned, gained, _destructionPercent);
    }
}

// 🆕 统一的战斗结束条件检查
void BattleManager::checkBattleEndConditions()
{
    if (_state == BattleState::FINISHED)
        return;

    float remainingTime = _battleTime - _elapsedTime;

    // 条件1: 时间耗尽
    if (remainingTime <= 0)
    {
        CCLOG("⏰ Battle ended: Time's up!");
        _endReason = BattleEndReason::TIMEOUT;
        endBattle(false);
        return;
    }

    // 条件2: 100% 破坏
    if (_destructionPercent >= 100)
    {
        CCLOG("🎉 Battle ended: All buildings destroyed!");
        _endReason = BattleEndReason::ALL_DESTROYED;
        endBattle(false);
        return;
    }

    // 条件3: 全军覆没 (已部署过单位，且所有单位死亡，且没有剩余可部署单位)
    if (_hasDeployedAnyUnit && checkAllUnitsDeadOrDeployed())
    {
        int aliveUnits      = countAliveUnits();
        int remainingTroops = getTotalRemainingTroops();

        if (aliveUnits == 0 && remainingTroops == 0)
        {
            CCLOG("💀 Battle ended: All units eliminated!");
            _endReason = BattleEndReason::ALL_UNITS_DEAD;
            endBattle(false);
            return;
        }
    }
}

// 检查是否所有单位都已死亡或已部署
bool BattleManager::checkAllUnitsDeadOrDeployed() const
{
    // 检查是否还有存活的已部署单位
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->isDead())
        {
            return false;  // 还有存活单位
        }
    }
    return true;
}

// 统计存活单位数量
int BattleManager::countAliveUnits() const
{
    int count = 0;
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->isDead())
        {
            count++;
        }
    }
    return count;
}

// 🆕 获取剩余可部署的总兵力
int BattleManager::getTotalRemainingTroops() const
{
    return _barbarianCount + _archerCount + _giantCount + _goblinCount + _wallBreakerCount;
}

void BattleManager::deployUnit(UnitType type, const cocos2d::Vec2& position)
{
    if (_isNetworked && !_isAttacker)
    {
        return;
    }

    int* count = nullptr;
    switch (type)
    {
    case UnitType::kBarbarian:
        count = &_barbarianCount;
        break;
    case UnitType::kArcher:
        count = &_archerCount;
        break;
    case UnitType::kGiant:
        count = &_giantCount;
        break;
    case UnitType::kGoblin:
        count = &_goblinCount;
        break;
    case UnitType::kWallBreaker:
        count = &_wallBreakerCount;
        break;
    default:
        return;
    }

    if (*count <= 0)
        return;

    (*count)--;
    _hasDeployedAnyUnit = true; // 🆕 标记已部署过单位

    if (_onTroopDeploy)
        _onTroopDeploy(type, *count);

    if (!_isReplayMode)
    {
        ReplaySystem::getInstance().recordDeployUnit(_currentFrame, type, position);

        if (_isNetworked && _isAttacker && _onNetworkDeploy)
        {
            _onNetworkDeploy(type, position);
        }
    }

    spawnUnit(type, position);
}

void BattleManager::deployUnitRemote(UnitType type, const cocos2d::Vec2& position)
{
    _hasDeployedAnyUnit = true; // 🆕 远程部署也标记
    spawnUnit(type, position);
}

void BattleManager::spawnUnit(UnitType type, const cocos2d::Vec2& position)
{
    BaseUnit* unit = UnitFactory::createUnit(type);
    if (!unit)
        return;

    unit->setPosition(position);
    unit->enableBattleMode();

    int zOrder = 10000 - static_cast<int>(position.y);
    if (_mapLayer)
        _mapLayer->addChild(unit, zOrder);
    _deployedUnits.push_back(unit);

    if (_state == BattleState::READY || _state == BattleState::LOADING)
    {
        if (_state == BattleState::LOADING)
        {
            CCLOG("Auto-starting battle on unit deploy");
        }
        _state = BattleState::FIGHTING;
        activateAllBuildings();
    }
}

void BattleManager::updateUnitAI(float dt)
{
    GridMap* gridMap = dynamic_cast<GridMap*>(_mapLayer);

    for (auto it = _deployedUnits.begin(); it != _deployedUnits.end();)
    {
        BaseUnit* unit = *it;
        if (!unit || unit->isDead())
        {
            ++it;
            continue;
        }

        BaseBuilding* target = unit->getTarget();

        if (!target || target->isDestroyed())
        {
            unit->clearTarget();

            BaseBuilding* bestTarget = nullptr;
            Vec2          unitPos    = unit->getPosition();

            auto findTargetWithFilter = [&](std::function<bool(BaseBuilding*)> filter) -> BaseBuilding* {
                BaseBuilding* closest = nullptr;
                float         minDist = 999999.0f;
                for (auto* b : _enemyBuildings)
                {
                    if (b && !b->isDestroyed() && filter(b))
                    {
                        float dist = unitPos.distance(b->getPosition());
                        if (dist < minDist)
                        {
                            minDist = dist;
                            closest = b;
                        }
                    }
                }
                return closest;
            };

            if (unit->getUnitType() == UnitType::kGiant)
            {
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return b->isDefenseBuilding(); });
            }
            else if (unit->getUnitType() == UnitType::kGoblin)
            {
                bestTarget = findTargetWithFilter(
                    [](BaseBuilding* b) { return b->getBuildingType() == BuildingType::kResource; });
            }
            else if (unit->getUnitType() == UnitType::kWallBreaker)
            {
                bestTarget =
                    findTargetWithFilter([](BaseBuilding* b) { return b->getBuildingType() == BuildingType::kWall; });
            }

            if (!bestTarget)
            {
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return true; });
            }

            if (bestTarget)
            {
                unit->setTarget(bestTarget);
                target = bestTarget;
                unit->stopMoving();
            }
        }

        if (target && !target->isDestroyed())
        {
            Vec2 targetPos = target->getPosition();

            if (unit->isInAttackRange(targetPos))
            {
                if (unit->isMoving())
                    unit->stopMoving();

                unit->updateAttackCooldown(dt);
                if (unit->isAttackReady())
                {
                    if (unit->getUnitType() == UnitType::kWallBreaker)
                    {
                        unit->attack(false);
                        float damage = unit->getDamage() * 40.0f;
                        target->takeDamage(static_cast<int>(damage));
                        unit->die();
                    }
                    else
                    {
                        unit->attack(false);
                        target->takeDamage(static_cast<int>(unit->getDamage()));
                        unit->resetAttackCooldown();
                    }

                    if (target->isDestroyed())
                    {
                        unit->clearTarget();
                        if (gridMap)
                        {
                            gridMap->markArea(target->getGridPosition(), target->getGridSize(), false);
                        }
                    }
                }
            }
            else
            {
                bool needsPathfinding = !unit->isMoving();

                if (needsPathfinding)
                {
                    if (gridMap)
                    {
                        std::vector<Vec2> path =
                            PathFinder::getInstance().findPath(gridMap, unit->getPosition(), targetPos, false);

                        unit->moveToPath(path);
                    }
                    else
                    {
                        unit->moveTo(targetPos);
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
        if (building)
        {
            building->enableBattleMode();
        }
    }
}

void BattleManager::endBattle(bool surrender)
{
    if (_state == BattleState::FINISHED)
        return;
    _state = BattleState::FINISHED;

    // 🆕 处理投降
    if (surrender)
    {
        _endReason = BattleEndReason::SURRENDER;
        CCLOG("🏳️ Battle ended: Player surrendered!");
    }

    if (!_isReplayMode)
    {
        ReplaySystem::getInstance().recordEndBattle(_currentFrame);
    }

    calculateBattleResult();

    // 🆕 改进的胜负音乐判定
    // 获得至少1星 或 破坏率>=50% 视为胜利
    bool isVictory = (_starsEarned > 0) || (_destructionPercent >= 50);

    if (isVictory)
        MusicManager::getInstance().playMusic(MusicType::BATTLE_WIN, false);
    else
        MusicManager::getInstance().playMusic(MusicType::BATTLE_LOSE, false);

    CCLOG("🏆 Battle Result: Stars=%d, Destruction=%d%%, Reason=%d, Victory=%s", _starsEarned, _destructionPercent,
          static_cast<int>(_endReason), isVictory ? "YES" : "NO");

    if (!_isReplayMode && !_isNetworked)
    {
        auto& inventory = TroopInventory::getInstance();
        inventory.addTroops(UnitType::kBarbarian, _barbarianCount);
        inventory.addTroops(UnitType::kArcher, _archerCount);
        inventory.addTroops(UnitType::kGiant, _giantCount);
        inventory.addTroops(UnitType::kGoblin, _goblinCount);
        inventory.addTroops(UnitType::kWallBreaker, _wallBreakerCount);

        AccountManager::getInstance().saveGameStateToFile();
        uploadBattleResult();
    }

    if (_onBattleEnd)
        _onBattleEnd();
}

void BattleManager::calculateBattleResult()
{
    int maxGold   = _enemyGameData.gold;
    int maxElixir = _enemyGameData.elixir;

    // 🆕 根据星星数量调整掠夺率
    float baseLootRate = 0.2f;
    float starBonus    = _starsEarned * 0.1f; // 每颗星 +10%
    float lootRate     = std::min(0.5f, baseLootRate + starBonus);

    _goldLooted   = static_cast<int>(maxGold * (_destructionPercent / 100.0f) * lootRate);
    _elixirLooted = static_cast<int>(maxElixir * (_destructionPercent / 100.0f) * lootRate);

    auto& resMgr = ResourceManager::getInstance();
    resMgr.addResource(ResourceType::kGold, _goldLooted);
    resMgr.addResource(ResourceType::kElixir, _elixirLooted);
}

void BattleManager::uploadBattleResult()
{
    auto&       accMgr         = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (!currentAccount)
        return;

    DefenseLog defenseLog;
    defenseLog.attackerId   = currentAccount->userId;
    defenseLog.attackerName = currentAccount->username;
    defenseLog.starsLost    = _starsEarned;
    defenseLog.goldLost     = _goldLooted;
    defenseLog.elixirLost   = _elixirLooted;
    defenseLog.trophyChange = -(_starsEarned * 10 - (3 - _starsEarned) * 3);
    defenseLog.timestamp    = getCurrentTimestamp();
    defenseLog.isViewed     = false;
    defenseLog.replayData   = ReplaySystem::getInstance().stopRecording();

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
    time_t    now = time(nullptr);
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
    case UnitType::kBarbarian:
        return _barbarianCount;
    case UnitType::kArcher:
        return _archerCount;
    case UnitType::kGiant:
        return _giantCount;
    case UnitType::kGoblin:
        return _goblinCount;
    case UnitType::kWallBreaker:
        return _wallBreakerCount;
    default:
        return 0;
    }
}

void BattleManager::setNetworkMode(bool isNetworked, bool isAttacker)
{
    _isNetworked = isNetworked;
    _isAttacker  = isAttacker;
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

    // 规则1: 摧毁大本营 = 1 星
    if (_townHallDestroyed)
    {
        stars++;
    }

    // 规则2: 破坏率 >= 50% = 1 星
    if (_destructionPercent >= 50)
    {
        stars++;
    }

    // 规则3: 破坏率 = 100% = 1 星
    if (_destructionPercent >= 100)
    {
        stars++;
    }

    return stars;
}

float BattleManager::calculateDestructionRate() const
{
    if (_totalBuildingHP <= 0)
        return 0.0f;

    return static_cast<float>(_destroyedBuildingHP) / static_cast<float>(_totalBuildingHP);
}