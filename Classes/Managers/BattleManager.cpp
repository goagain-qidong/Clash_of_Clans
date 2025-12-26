/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleManager.cpp
 * File Function: 战斗逻辑实现 - 管理战斗流程和状态
 * Author:        赵崇治
 * Update Date:   2025/12/26
 * License:       MIT License
 ****************************************************************/

#include "BattleManager.h"
#include "AccountManager.h"
#include "Managers/DefenseLogSystem.h"
#include "Managers/DeploymentValidator.h"
#include "Managers/MusicManager.h"
#include "Managers/TroopInventory.h"
#include "PathFinder.h"
#include "ResourceManager.h"
#include "Unit/UnitFactory.h"

#include <ctime>

USING_NS_CC;

BattleManager::BattleManager() : _deploymentValidator(nullptr) {}

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
        CCLOG("✅ BattleManager: GridMap 已成功关联!");
    }
    else
    {
        CCLOG("❌ 错误: BattleManager 未找到 GridMap! 寻路将失败.");
    }

    _enemyGameData = enemyData;
    _enemyUserId   = enemyUserId;
    _isReplayMode  = isReplay;
    _state         = BattleState::LOADING;

    // 重置所有战斗数据
    _elapsedTime        = 0.0f;
    _readyPhaseElapsed  = 0.0f;
    _starsEarned        = 0;
    _goldLooted         = 0;
    _elixirLooted       = 0;
    _destructionPercent = 0;
    _accumulatedTime    = 0.0f;
    _currentFrame       = 0;

    // 重置战斗结束状态
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
        CCLOG("❌ 警告: setBuildings 调用时 _gridMap 为空!");

    CCLOG("📊 ============ 设置战斗建筑 ============");
    CCLOG("📊 建筑总数: %zu", buildings.size());

    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            int maxHP = building->getMaxHitpoints();
            int curHP = building->getHitpoints();
            
            // 确保血量合理，防止初始化问题
            if (maxHP <= 0)
            {
                CCLOG("⚠️ 警告：建筑 %s 的 maxHP 为 %d，使用默认值 100", 
                      building->getDisplayName().c_str(), maxHP);
                maxHP = 100;
            }
            
            // 战斗开始时强制将所有建筑血量重置为满血
            if (curHP != maxHP)
            {
                CCLOG("⚠️ 警告：建筑 %s 血量不一致 (%d/%d)，重置为满血", 
                      building->getDisplayName().c_str(), curHP, maxHP);
                building->repair(maxHP - curHP);
            }
            
            _totalBuildingHP += maxHP;

            CCLOG("📊 建筑: %s, 血量: %d/%d, 类型: %d", 
                  building->getDisplayName().c_str(), 
                  curHP, maxHP,
                  static_cast<int>(building->getBuildingType()));

            if (_gridMap)
            {
                Vec2 gridPos  = building->getGridPosition();
                Size gridSize = building->getGridSize();
                _gridMap->markArea(gridPos, gridSize, true);
            }
        }
    }

    // 初始化部署验证器
    if (_gridMap)
    {
        _deploymentValidator.reset(DeploymentValidator::Create(_gridMap));
        if (_deploymentValidator)
        {
            _deploymentValidator->SetBuildings(buildings);
            CCLOG("📍 部署验证器初始化完成");
        }
    }

    CCLOG("📊 总血量: %d", _totalBuildingHP);
    CCLOG("📊 ========================================");
}

void BattleManager::startBattle(const TroopDeploymentMap& deployment)
{
    // 防止重复调用时重置已进行中的战斗
    if (_state == BattleState::FIGHTING)
    {
        CCLOG("⚠️ 战斗已在进行中，跳过 startBattle");
        return;
    }

    // 进入准备阶段（准备倒计时开始，战斗计时器不启动）
    _state             = BattleState::READY;
    _elapsedTime       = 0.0f;
    _readyPhaseElapsed = 0.0f;

    CCLOG("🎮 进入战斗准备阶段，等待部署第一个单位（最长 %.0f 秒）...", _readyPhaseTime);

    // 初始化部队数量
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

    CCLOG("📦 可部署部队: 野蛮人=%d, 弓箭手=%d, 巨人=%d, 哥布林=%d, 炸弹人=%d", 
          _barbarianCount, _archerCount, _giantCount, _goblinCount, _wallBreakerCount);

    // 非回放模式下消耗部队并开始录制
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

void BattleManager::skipReadyPhase()
{
    if (_state != BattleState::READY && _state != BattleState::LOADING)
    {
        CCLOG("⚠️ skipReadyPhase: 当前状态不允许跳过准备阶段");
        return;
    }

    CCLOG("⏩ 跳过准备阶段，直接进入战斗状态（回放模式）");
    
    _state = BattleState::FIGHTING;
    _elapsedTime = 0.0f;
    _readyPhaseElapsed = _readyPhaseTime;  // 标记准备阶段已完成

    // 激活所有防御建筑
    activateAllBuildings();

    // 播放战斗音乐（回放模式在 BattleScene 中已处理）
}

void BattleManager::triggerBattleStart()
{
    if (_state != BattleState::READY)
    {
        CCLOG("⚠️ triggerBattleStart: 当前状态不是 READY，跳过");
        return;
    }

    CCLOG("⚔️ 战斗正式开始！计时器启动（准备阶段用时: %.1f秒）", _readyPhaseElapsed);
    
    _state = BattleState::FIGHTING;
    _elapsedTime = 0.0f;

    // 播放战斗音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);

    // 激活所有防御建筑
    activateAllBuildings();

    // 通知 UI 战斗已开始
    if (_onBattleStart)
    {
        _onBattleStart();
    }

    if (_onUIUpdate)
    {
        _onUIUpdate();
    }
}

void BattleManager::checkReadyPhaseTimeout()
{
    if (_state != BattleState::READY)
        return;

    // 检查准备阶段是否超时
    if (_readyPhaseElapsed >= _readyPhaseTime)
    {
        CCLOG("⏰ 准备阶段超时（%.0f秒），战斗结束（0星）", _readyPhaseTime);
        _endReason = BattleEndReason::TIMEOUT;
        endBattle(false);
    }
}

float BattleManager::getReadyPhaseRemainingTime() const
{
    if (_state != BattleState::READY)
    {
        return 0.0f;
    }
    return std::max(0.0f, _readyPhaseTime - _readyPhaseElapsed);
}

void BattleManager::update(float dt)
{
    // READY 状态：更新准备阶段倒计时
    if (_state == BattleState::READY)
    {
        _readyPhaseElapsed += dt;
        
        // 检查准备阶段超时
        checkReadyPhaseTimeout();
        
        // 更新 UI（显示准备阶段倒计时）
        if (_onUIUpdate)
            _onUIUpdate();
        return;
    }

    // FIGHTING 状态：正常更新战斗逻辑
    if (_state == BattleState::FIGHTING)
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
    // 仅在 FIGHTING 状态下累加战斗时间
    if (_state == BattleState::FIGHTING)
    {
        _elapsedTime += dt;
    }

    // 更新 Z-Order
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->isDead() && !unit->isPendingRemoval())
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

    // 清理已死亡且等待移除的单位
    _deployedUnits.erase(
        std::remove_if(_deployedUnits.begin(), _deployedUnits.end(),
            [](BaseUnit* unit) {
                if (unit == nullptr)
                {
                    return true;
                }
                // 检查是否已标记为等待移除
                if (unit->isPendingRemoval())
                {
                    return true;
                }
                return false;
            }),
        _deployedUnits.end());

    // 更新单位移动
    for (auto* unit : _deployedUnits)
    {
        if (unit && !unit->isDead())
        {
            unit->tickMovement(dt);
        }
    }

    // 更新 AI（包括攻击冷却和攻击逻辑）
    updateUnitAI(dt);

    // 更新防御建筑
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

    // 更新星星和破坏率
    updateStarsAndDestruction();

    // 检查战斗结束条件
    checkBattleEndConditions();

    if (_onUIUpdate)
        _onUIUpdate();
}

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

            // 检測大本营是否被摧毁
            if (building->getBuildingType() == BuildingType::kTownHall)
            {
                if (!_townHallDestroyed)
                {
                    _townHallDestroyed = true;
                    CCLOG("⭐ 大本营被摧毁! +1 星");
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

    // 统一的星星计算逻辑
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
        CCLOG("⭐ 星星更新: %d (+%d), 破坏率: %d%%", _starsEarned, gained, _destructionPercent);
    }
}

void BattleManager::checkBattleEndConditions()
{
    if (_state == BattleState::FINISHED)
        return;

    // 仅在 FIGHTING 状态下检查时间
    if (_state == BattleState::FIGHTING)
    {
        float remainingTime = _battleTime - _elapsedTime;

        // 条件1: 战斗时间耗尽
        if (remainingTime <= 0)
        {
            CCLOG("⏰ 战斗结束: 时间耗尽!");
            _endReason = BattleEndReason::TIMEOUT;
            endBattle(false);
            return;
        }
    }

    // 条件2: 100% 破坏
    if (_destructionPercent >= 100)
    {
        CCLOG("🎉 战斗结束: 全部建筑被摧毁!");
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
            CCLOG("💀 战斗结束: 全军覆没!");
            _endReason = BattleEndReason::ALL_UNITS_DEAD;
            endBattle(false);
            return;
        }
    }
}

bool BattleManager::checkAllUnitsDeadOrDeployed() const
{
    for (auto* unit : _deployedUnits)
    {
        // 跳过空指针和等待移除的单位
        if (unit && !unit->isDead() && !unit->isPendingRemoval())
        {
            return false;
        }
    }
    return true;
}

int BattleManager::countAliveUnits() const
{
    int count = 0;
    for (auto* unit : _deployedUnits)
    {
        // 只统计有效且未死亡且未等待移除的单位
        if (unit && !unit->isDead() && !unit->isPendingRemoval())
        {
            count++;
        }
    }
    return count;
}

int BattleManager::getTotalRemainingTroops() const
{
    return _barbarianCount + _archerCount + _giantCount + _goblinCount + _wallBreakerCount;
}

void BattleManager::deployUnit(UnitType type, const cocos2d::Vec2& position)
{
    // 网络模式下非攻击方不能部署
    if (_isNetworked && !_isAttacker)
    {
        return;
    }

    // Issue 2 Fix: 检查是否为有效的部队类型
    if (type == UnitType::kNone)
    {
        CCLOG("⚠️ deployUnit: 无效的部队类型 (kNone)");
        return;
    }

    // 获取对应部队计数器
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

    // 记录部署回调
    if (_onTroopDeploy)
        _onTroopDeploy(type, *count);

    // 非回放模式下记录操作
    if (!_isReplayMode)
    {
        ReplaySystem::getInstance().recordDeployUnit(_currentFrame, type, position);

        // 网络模式下发送操作
        if (_isNetworked && _isAttacker && _onNetworkDeploy)
        {
            _onNetworkDeploy(type, position);
        }
    }

    // 生成单位
    spawnUnit(type, position);
}

void BattleManager::deployUnitRemote(UnitType type, const cocos2d::Vec2& position)
{
    // 远程部署不进行位置验证，因为原始部署已在攻击方验证过
    // 这适用于网络同步、观战和回放场景
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

    // 首次部署单位时触发战斗正式开始
    if (!_hasDeployedAnyUnit)
    {
        _hasDeployedAnyUnit = true;
        
        // 如果当前是 READY 或 LOADING 状态，触发战斗开始
        if (_state == BattleState::READY || _state == BattleState::LOADING)
        {
            triggerBattleStart();
        }
    }

    CCLOG("🪖 部署单位: type=%d, pos=(%.1f,%.1f), 存活单位数=%zu", 
          static_cast<int>(type), position.x, position.y, _deployedUnits.size());
}

void BattleManager::updateUnitAI(float dt)
{
    GridMap* gridMap = dynamic_cast<GridMap*>(_mapLayer);

    for (auto it = _deployedUnits.begin(); it != _deployedUnits.end();)
    {
        BaseUnit* unit = *it;
        
        // 安全检查：跳过空指针、已死亡或等待移除的单位
        if (!unit || unit->isDead() || unit->isPendingRemoval())
        {
            ++it;
            continue;
        }

        BaseBuilding* target = unit->getTarget();

        // 需要寻找新目标
        if (!target || target->isDestroyed())
        {
            unit->clearTarget();

            BaseBuilding* bestTarget = nullptr;
            Vec2          unitPos    = unit->getPosition();

            // 目标查找辅助函数
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

            // 根据单位类型选择优先目标
            if (unit->getUnitType() == UnitType::kGiant)
            {
                // 巨人优先攻击防御建筑
                bestTarget = findTargetWithFilter([](BaseBuilding* b) { return b->isDefenseBuilding(); });
            }
            else if (unit->getUnitType() == UnitType::kGoblin)
            {
                // 哥布林优先攻击资源建筑
                bestTarget = findTargetWithFilter(
                    [](BaseBuilding* b) { return b->getBuildingType() == BuildingType::kResource; });
            }
            else if (unit->getUnitType() == UnitType::kWallBreaker)
            {
                // 炸弹人优先攻击城墙
                bestTarget =
                    findTargetWithFilter([](BaseBuilding* b) { return b->getBuildingType() == BuildingType::kWall; });
            }

            // 如果没有优先目标，选择最近的任意建筑
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

        // 执行攻击或移动逻辑
        if (target && !target->isDestroyed())
        {
            Vec2 targetPos = target->getPosition();

            if (unit->isInAttackRange(targetPos))
            {
                // 在攻击范围内
                if (unit->isMoving())
                    unit->stopMoving();

                // 🔴 修复：先更新攻击冷却，再检查是否可以攻击
                unit->updateAttackCooldown(dt);
                
                if (unit->isAttackReady())
                {
                    // 🔴 修复：在攻击前再次检查目标有效性，防止重复攻击已摧毁的建筑
                    if (target->isDestroyed())
                    {
                        unit->clearTarget();
                        ++it;
                        continue;
                    }
                    
                    if (unit->getUnitType() == UnitType::kWallBreaker)
                    {
                        // 炸弹人自爆攻击
                        unit->attack(false);
                        float damage = unit->getDamage() * 40.0f;
                        target->takeDamage(static_cast<int>(damage));
                        unit->die();
                        
                        CCLOG("💥 炸弹人自爆攻击: 伤害=%.1f, 目标血量=%d/%d",
                              damage, target->getHitpoints(), target->getMaxHitpoints());
                    }
                    else
                    {
                        // 普通攻击
                        unit->attack(false);
                        int damage = static_cast<int>(unit->getDamage());
                        target->takeDamage(damage);
                        unit->resetAttackCooldown();
                        
                        CCLOG("⚔️ %s 攻击 %s: 伤害=%d, 目标血量=%d/%d",
                              unit->getDisplayName().c_str(), target->getDisplayName().c_str(),
                              damage, target->getHitpoints(), target->getMaxHitpoints());
                    }

                    // 检查目标是否被摧毁
                    if (target->isDestroyed())
                    {
                        CCLOG("🔥 %s 被摧毁!", target->getDisplayName().c_str());
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
                // 不在攻击范围内，需要移动
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

    // 处理投降
    if (surrender)
    {
        _endReason = BattleEndReason::SURRENDER;
        CCLOG("🏳️ 战斗结束: 玩家投降!");
    }

    if (!_isReplayMode)
    {
        ReplaySystem::getInstance().recordEndBattle(_currentFrame);
    }

    calculateBattleResult();

    // 胜负判定：获得至少1星 或 破坏率>=50% 视为胜利
    bool isVictory = (_starsEarned > 0) || (_destructionPercent >= 50);

    if (isVictory)
        MusicManager::getInstance().playMusic(MusicType::BATTLE_WIN, false);
    else
        MusicManager::getInstance().playMusic(MusicType::BATTLE_LOSE, false);

    CCLOG("🏆 战斗结果: 星数=%d, 破坏率=%d%%, 原因=%d, 胜利=%s", 
          _starsEarned, _destructionPercent,
          static_cast<int>(_endReason), isVictory ? "是" : "否");

    // Issue 1 Fix: 非回放非网络模式下返还未使用的部队并正确保存
    if (!_isReplayMode && !_isNetworked)
    {
        returnUnusedTroops();
        uploadBattleResult();
    }

    if (_onBattleEnd)
        _onBattleEnd();
}

void BattleManager::returnUnusedTroops()
{
    auto& inventory = TroopInventory::getInstance();
    
    CCLOG("📦 返还未使用部队: 野蛮人=%d, 弓箭手=%d, 巨人=%d, 哥布林=%d, 炸弹人=%d",
          _barbarianCount, _archerCount, _giantCount, _goblinCount, _wallBreakerCount);

    // 检查是否有需要返还的部队
    int totalToReturn = _barbarianCount + _archerCount + _giantCount + 
                        _goblinCount + _wallBreakerCount;
    
    if (totalToReturn == 0)
    {
        CCLOG("📦 没有需要返还的部队");
        return;
    }

    // 获取当前库存并合并返还的部队
    // 注意：不使用 addTroops() 因为战斗开始时已经从库存扣除
    // 这里直接恢复库存数量
    std::map<UnitType, int> currentTroops = inventory.getAllTroops();
    
    if (_barbarianCount > 0)
    {
        currentTroops[UnitType::kBarbarian] += _barbarianCount;
    }
    if (_archerCount > 0)
    {
        currentTroops[UnitType::kArcher] += _archerCount;
    }
    if (_giantCount > 0)
    {
        currentTroops[UnitType::kGiant] += _giantCount;
    }
    if (_goblinCount > 0)
    {
        currentTroops[UnitType::kGoblin] += _goblinCount;
    }
    if (_wallBreakerCount > 0)
    {
        currentTroops[UnitType::kWallBreaker] += _wallBreakerCount;
    }
    
    // 使用 setAllTroops 直接设置库存，避免触发人口检查
    inventory.setAllTroops(currentTroops);

    // 保存到文件
    inventory.save();

    // 同步保存游戏状态
    AccountManager::getInstance().saveGameStateToFile();

    CCLOG("✅ 部队已返还并保存到文件（共 %d 个单位）", totalToReturn);
}

void BattleManager::calculateBattleResult()
{
    int maxGold   = _enemyGameData.gold;
    int maxElixir = _enemyGameData.elixir;

    // 根据星星数量调整掠夺率
    float baseLootRate = 0.2f;
    float starBonus    = _starsEarned * 0.1f;
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

    // 跳过自己攻击自己的情况
    if (_enemyUserId == currentAccount->account.userId)
        return;

    DefenseLog defenseLog;
    defenseLog.attackerId   = currentAccount->account.userId;
    defenseLog.attackerName = currentAccount->account.username;
    defenseLog.starsLost    = _starsEarned;
    defenseLog.goldLost     = _goldLooted;
    defenseLog.elixirLost   = _elixirLooted;
    defenseLog.trophyChange = -(_starsEarned * 10 - (3 - _starsEarned) * 3);
    defenseLog.timestamp    = getCurrentTimestamp();
    defenseLog.isViewed     = false;
    defenseLog.replayData   = ReplaySystem::getInstance().stopRecording();

    std::string attackerUserId = currentAccount->account.userId;
    std::string enemyUserId = _enemyUserId;
    
    //  修复：显式捕获变量，避免悬空引用
    Director::getInstance()->getScheduler()->performFunctionInCocosThread([defenseLog, attackerUserId, enemyUserId]() {
        auto& accMgrLocal = AccountManager::getInstance();
        if (accMgrLocal.switchAccount(enemyUserId, true))
        {
            DefenseLogSystem::getInstance().load();
            DefenseLogSystem::getInstance().addDefenseLog(defenseLog);
            accMgrLocal.switchAccount(attackerUserId, true);
            DefenseLogSystem::getInstance().load();
        }
    });
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
    // READY 状态返回满时间，FIGHTING 状态返回剩余时间
    if (_state == BattleState::READY || _state == BattleState::LOADING)
    {
        return _battleTime;
    }
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

    if (_townHallDestroyed)
    {
        stars++;
    }

    if (_destructionPercent >= 50)
    {
        stars++;
    }

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

// ============================================================================
// 时间同步（观战模式）
// ============================================================================

void BattleManager::setTimeOffset(int64_t elapsed_ms)
{
    // 将毫秒转换为秒
    float elapsed_seconds = static_cast<float>(elapsed_ms) / 1000.0f;
    
    // 确保不超过战斗总时间
    _elapsedTime = std::min(elapsed_seconds, _battleTime);
    
    // 计算对应的帧数
    _currentFrame = static_cast<unsigned int>(_elapsedTime / FIXED_TIME_STEP);
    
    // 如果有时间偏移，说明战斗已经在进行中
    if (elapsed_ms > 0 && _state == BattleState::READY)
    {
        _state = BattleState::FIGHTING;
        _hasDeployedAnyUnit = true;
        _readyPhaseElapsed = _readyPhaseTime;  // 标记准备阶段已完成
    }
    
    CCLOG("📺 [BattleManager] 设置时间偏移: %lldms -> %.2fs (帧: %u)", 
          static_cast<long long>(elapsed_ms), _elapsedTime, _currentFrame);
    
    if (_onUIUpdate)
    {
        _onUIUpdate();
    }
}

int64_t BattleManager::getElapsedTimeMs() const
{
    return static_cast<int64_t>(_elapsedTime * 1000.0f);
}