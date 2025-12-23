/****************************************************************
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleManager.h
 * File Function: 战斗逻辑管理器
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef BATTLE_MANAGER_H_
#define BATTLE_MANAGER_H_

#include "AccountManager.h"
#include "Buildings/BaseBuilding.h"
#include "Buildings/DefenseBuilding.h"
#include "GridMap.h"
#include "Managers/ReplaySystem.h"
#include "PathFinder.h"
#include "Unit/BaseUnit.h"
#include "Unit/UnitTypes.h"
#include "cocos2d.h"

#include <functional>
#include <map>
#include <string>
#include <vector>
using TroopDeploymentMap = std::map<UnitType, int>;

enum class BattleMode
{
    LOCAL,           // 本地测试
    PVP_ATTACK,      // PVP攻击
    PVP_DEFEND,      // PVP防守
    CLAN_WAR_ATTACK, // 部落战攻击
    CLAN_WAR_DEFEND, // 部落战防守
    SPECTATE         // 观战模式
};

// 🆕 战斗结束原因
enum class BattleEndReason
{
    TIMEOUT,           // 时间耗尽
    ALL_DESTROYED,     // 全部摧毁
    ALL_UNITS_DEAD,    // 全军覆没
    SURRENDER,         // 主动投降
    NETWORK_DISCONNECT // 网络断开
};

class BattleManager
{
public:
    enum class BattleState
    {
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
    void startBattle(const TroopDeploymentMap& deployment);
    void endBattle(bool surrender);
    void deployUnit(UnitType type, const cocos2d::Vec2& position);

    // Getters
    BattleState     getState() const { return _state; }
    bool            isReplayMode() const { return _isReplayMode; }
    int             getStars() const { return _starsEarned; }
    int             getDestructionPercent() const { return _destructionPercent; }
    int             getGoldLooted() const { return _goldLooted; }
    int             getElixirLooted() const { return _elixirLooted; }
    float           getRemainingTime() const;
    BattleEndReason getEndReason() const { return _endReason; }                // 🆕
    bool            isTownHallDestroyed() const { return _townHallDestroyed; } // 🆕

    // Troop Counts (for UI)
    int getTroopCount(UnitType type) const;
    int getTotalRemainingTroops() const; // 🆕

    void  setBattleMode(BattleMode mode, const std::string& warId = "");
    bool  canDeployUnit() const;
    int   calculateStars() const;
    float calculateDestructionRate() const;

    // Callbacks
    void setUIUpdateCallback(const std::function<void()>& callback) { _onUIUpdate = callback; }
    void setBattleEndCallback(const std::function<void()>& callback) { _onBattleEnd = callback; }
    void setTroopDeployCallback(const std::function<void(UnitType, int)>& callback) { _onTroopDeploy = callback; }

    // 🆕 Network PVP
    void setNetworkMode(bool isNetworked, bool isAttacker);
    void deployUnitRemote(UnitType type, const cocos2d::Vec2& position);
    void setNetworkDeployCallback(const std::function<void(UnitType, const cocos2d::Vec2&)>& callback);

private:
    // Logic Methods
    void        fixedUpdate();
    void        updateBattleState(float dt);
    void        updateUnitAI(float dt);
    void        activateAllBuildings();
    void        calculateBattleResult();
    void        uploadBattleResult();
    std::string getCurrentTimestamp();

    // 🆕 胜负判定核心方法
    void updateStarsAndDestruction();
    void checkBattleEndConditions();
    bool checkAllUnitsDeadOrDeployed() const;
    int  countAliveUnits() const;

    GridMap* _gridMap = nullptr;
    void     updateTroopCounts();

    BattleMode  _battleMode = BattleMode::LOCAL;
    std::string _currentWarId;

    void spawnUnit(UnitType type, const cocos2d::Vec2& position);

    // Data
    cocos2d::Node* _mapLayer = nullptr;

    AccountGameData _enemyGameData;
    std::string     _enemyUserId;
    bool            _isReplayMode = false;
    BattleState     _state        = BattleState::LOADING;

    // Battle Stats
    float _battleTime         = 180.0f;
    float _elapsedTime        = 0.0f;
    int   _starsEarned        = 0;
    int   _goldLooted         = 0;
    int   _elixirLooted       = 0;
    int   _destructionPercent = 0;

    // 🆕 战斗结束状态
    BattleEndReason _endReason          = BattleEndReason::TIMEOUT;
    bool            _townHallDestroyed  = false;
    bool            _hasDeployedAnyUnit = false; // 是否曾部署过单位

    // Units & Buildings
    std::vector<BaseUnit*>     _deployedUnits;
    std::vector<BaseBuilding*> _enemyBuildings;
    int                        _totalBuildingHP     = 0;
    int                        _destroyedBuildingHP = 0;

    // Troop Inventory (Local copy for battle)
    int _barbarianCount   = 0;
    int _archerCount      = 0;
    int _giantCount       = 0;
    int _goblinCount      = 0;
    int _wallBreakerCount = 0;

    // Deterministic Update
    float        _accumulatedTime = 0.0f;
    unsigned int _currentFrame    = 0;
    const float  FIXED_TIME_STEP  = 1.0f / 60.0f;

    // Callbacks
    std::function<void()>              _onUIUpdate;
    std::function<void()>              _onBattleEnd;
    std::function<void(UnitType, int)> _onTroopDeploy;

    // Network
    bool                                                _isNetworked = false;
    bool                                                _isAttacker  = false;
    std::function<void(UnitType, const cocos2d::Vec2&)> _onNetworkDeploy;
};

#endif  // BATTLE_MANAGER_H_