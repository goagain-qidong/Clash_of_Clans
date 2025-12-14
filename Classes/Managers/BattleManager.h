/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleManager.h
 * File Function: 战斗逻辑管理器
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#pragma once
#ifndef __BATTLE_MANAGER_H__
#define __BATTLE_MANAGER_H__

#include "cocos2d.h"
#include "AccountManager.h"
#include "Unit/unit.h"
#include "Buildings/BaseBuilding.h"
#include "Buildings/DefenseBuilding.h"
#include "Managers/ReplaySystem.h"
#include <vector>
#include <string>
#include <functional>

class BattleManager {
public:
    enum class BattleState {
        LOADING,
        READY,
        FIGHTING,
        FINISHED
    };

    BattleManager();
    ~BattleManager();

    // Initialization
    void init(cocos2d::Node* mapLayer, const AccountGameData& enemyData, const std::string& enemyUserId, bool isReplay);
    void setBuildings(const std::vector<BaseBuilding*>& buildings);
    
    // Update Loop
    void update(float dt);
    
    // Actions
    void startBattle();
    void endBattle(bool surrender);
    void deployUnit(UnitType type, const cocos2d::Vec2& position);
    
    // Getters
    BattleState getState() const { return _state; }
    bool isReplayMode() const { return _isReplayMode; }
    int getStars() const { return _starsEarned; }
    int getDestructionPercent() const { return _destructionPercent; }
    int getGoldLooted() const { return _goldLooted; }
    int getElixirLooted() const { return _elixirLooted; }
    float getRemainingTime() const;
    
    // Troop Counts (for UI)
    int getTroopCount(UnitType type) const;

    // Callbacks
    void setUIUpdateCallback(const std::function<void()>& callback) { _onUIUpdate = callback; }
    void setBattleEndCallback(const std::function<void()>& callback) { _onBattleEnd = callback; }
    void setTroopDeployCallback(const std::function<void(UnitType, int)>& callback) { _onTroopDeploy = callback; }

private:
    // Logic Methods
    void fixedUpdate();
    void updateBattleState(float dt);
    void updateUnitAI(float dt);
    void activateAllBuildings();
    void calculateBattleResult();
    void uploadBattleResult();
    std::string getCurrentTimestamp();
    void updateTroopCounts();

    // Data
    cocos2d::Node* _mapLayer = nullptr;
    AccountGameData _enemyGameData;
    std::string _enemyUserId;
    bool _isReplayMode = false;
    BattleState _state = BattleState::LOADING;

    // Battle Stats
    float _battleTime = 180.0f;
    float _elapsedTime = 0.0f;
    int _starsEarned = 0;
    int _goldLooted = 0;
    int _elixirLooted = 0;
    int _destructionPercent = 0;

    // Units & Buildings
    std::vector<Unit*> _deployedUnits;
    std::vector<BaseBuilding*> _enemyBuildings;
    int _totalBuildingHP = 0;
    int _destroyedBuildingHP = 0;

    // Troop Inventory (Local copy for battle)
    int _barbarianCount = 0;
    int _archerCount = 0;
    int _giantCount = 0;

    // Deterministic Update
    float _accumulatedTime = 0.0f;
    unsigned int _currentFrame = 0;
    const float FIXED_TIME_STEP = 1.0f / 60.0f;

    // Callbacks
    std::function<void()> _onUIUpdate;
    std::function<void()> _onBattleEnd;
    std::function<void(UnitType, int)> _onTroopDeploy;
};

#endif // __BATTLE_MANAGER_H__