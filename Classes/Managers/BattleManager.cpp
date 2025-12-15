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
    _enemyGameData = enemyData;
    _enemyUserId = enemyUserId;
    _isReplayMode = isReplay;
    _state = BattleState::LOADING;
    
    // Reset stats
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

void BattleManager::setBuildings(const std::vector<BaseBuilding*>& buildings)
{
    _enemyBuildings = buildings;
    
    _totalBuildingHP = 0;
    _destroyedBuildingHP = 0;
    
    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            _totalBuildingHP += building->getMaxHitpoints();
        }
    }
    
    CCLOG("📊 BattleManager: Loaded %zu buildings, Total HP: %d", _enemyBuildings.size(), _totalBuildingHP);
}

void BattleManager::startBattle()
{
    _state = BattleState::READY;
    _elapsedTime = 0.0f;
    
    // 🎵 Play music
    MusicManager::getInstance().playMusic(MusicType::BATTLE_GOING);
    
    // 🔴 修复：启用所有建筑的战斗模式
    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            building->enableBattleMode();
            CCLOG("⚔️ 启用 %s 战斗模式", building->getDisplayName().c_str());
        }
    }
    
    if (!_isReplayMode)
    {
        // Load troops from inventory
        auto& troopInv = TroopInventory::getInstance();
        _barbarianCount = troopInv.getTroopCount(UnitType::kBarbarian);
        _archerCount = troopInv.getTroopCount(UnitType::kArcher);
        _giantCount = troopInv.getTroopCount(UnitType::kGiant);
        
        CCLOG("📦 Available Troops: Barb=%d, Arch=%d, Giant=%d", 
              _barbarianCount, _archerCount, _giantCount);
        
        // Start recording
        unsigned int seed = static_cast<unsigned int>(time(nullptr));
        srand(seed);
        ReplaySystem::getInstance().startRecording(_enemyUserId, _enemyGameData.toJson(), seed);
    }
    else
    {
        // Replay mode setup handled by ReplaySystem callbacks in Scene usually, 
        // but here we just ensure state is correct.
        // ReplaySystem::getInstance().loadReplay(...) should have been called before init.
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
        default: return;
    }
    
    if (!_isReplayMode)
    {
        // 联网模式下，攻击者下兵也需要消耗（或者无限兵力？这里假设消耗）
        if (*count <= 0) return;
        
        auto& troopInv = TroopInventory::getInstance();
        if (!troopInv.consumeTroops(type, 1)) return;
        
        (*count)--;
        if (_onTroopDeploy) _onTroopDeploy(type, *count);
        
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
    if (!unit)
    {
        // if (!_isReplayMode) TroopInventory::getInstance().addTroops(type, 1); // Revert? Too complex for now
        return;
    }
    
    unit->setPosition(position);
    int zOrder = 10000 - static_cast<int>(position.y);
    if (_mapLayer) _mapLayer->addChild(unit, zOrder);
    _deployedUnits.push_back(unit);
    
    if (_state == BattleState::READY)
    {
        _state = BattleState::FIGHTING;
        activateAllBuildings();
    }
}

void BattleManager::updateUnitAI(float dt)
{
    for (auto it = _deployedUnits.begin(); it != _deployedUnits.end();)
    {
        Unit* unit = *it;
        if (!unit || unit->IsDead())
        {
            it = _deployedUnits.erase(it);
            continue;
        }
        
        BaseBuilding* target = unit->getTarget();
        if (!target || target->isDestroyed())
        {
            // Find new target
            BaseBuilding* closestBuilding = nullptr;
            BaseBuilding* closestDefenseBuilding = nullptr;
            BaseBuilding* closestResourceBuilding = nullptr;
            float closestDistance = 99999.0f;
            float closestDefenseDistance = 99999.0f;
            float closestResourceDistance = 99999.0f;
            
            Vec2 unitWorldPos = unit->getParent()->convertToWorldSpace(unit->getPosition());
            
            for (auto* building : _enemyBuildings)
            {
                if (!building || building->isDestroyed()) continue;
                
                Vec2 buildingWorldPos = building->getParent()->convertToWorldSpace(building->getPosition());
                float distance = unitWorldPos.distance(buildingWorldPos);
                
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestBuilding = building;
                }
                
                if (unit->GetType() == UnitType::kGiant && building->isDefenseBuilding())
                {
                    if (distance < closestDefenseDistance)
                    {
                        closestDefenseDistance = distance;
                        closestDefenseBuilding = building;
                    }
                }
                
                if (unit->GetType() == UnitType::kGoblin && building->getBuildingType() == BuildingType::kResource)
                {
                    if (distance < closestResourceDistance)
                    {
                        closestResourceDistance = distance;
                        closestResourceBuilding = building;
                    }
                }
            }
            
            BaseBuilding* selectedTarget = nullptr;
            if (unit->GetType() == UnitType::kGiant)
                selectedTarget = closestDefenseBuilding ? closestDefenseBuilding : closestBuilding;
            else if (unit->GetType() == UnitType::kGoblin)
                selectedTarget = closestResourceBuilding ? closestResourceBuilding : closestBuilding;
            else
                selectedTarget = closestBuilding;
            
            if (selectedTarget)
            {
                unit->setTarget(selectedTarget);
                target = selectedTarget;
            }
        }
        
        if (target && !target->isDestroyed())
        {
            Vec2 targetPos = target->getPosition();
            if (unit->isInAttackRange(targetPos))
            {
                unit->updateAttackCooldown(dt);
                if (unit->isAttackReady())
                {
                    unit->Attack(false);
                    target->takeDamage(unit->getDamage());
                    unit->resetAttackCooldown();
                    if (target->isDestroyed()) unit->clearTarget();
                }
            }
            else
            {
                bool needsNewPath = !unit->isMoving();
                if (unit->isMoving())
                {
                    float distToCurrentTarget = unit->getTargetPosition().distance(targetPos);
                    if (distToCurrentTarget > 10.0f) needsNewPath = true;
                }
                if (needsNewPath) unit->MoveTo(targetPos);
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
    
    if (_starsEarned > 0)
        MusicManager::getInstance().playMusic(MusicType::BATTLE_WIN, false);
    else
        MusicManager::getInstance().playMusic(MusicType::BATTLE_LOSE, false);
        
    if (!_isReplayMode && !_isNetworked) // 🆕 联网模式下不保存本地结果
    {
        AccountManager::getInstance().saveGameStateToFile();
        uploadBattleResult();
    }
    
    if (_onBattleEnd) _onBattleEnd();
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