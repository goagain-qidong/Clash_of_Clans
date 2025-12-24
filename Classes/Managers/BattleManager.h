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

/**
 * @enum BattleMode
 * @brief 战斗模式枚举
 */
enum class BattleMode
{
    LOCAL,           ///< 本地测试
    PVP_ATTACK,      ///< PVP攻击
    PVP_DEFEND,      ///< PVP防守
    CLAN_WAR_ATTACK, ///< 部落战攻击
    CLAN_WAR_DEFEND, ///< 部落战防守
    SPECTATE         ///< 观战模式
};

/**
 * @enum BattleEndReason
 * @brief 战斗结束原因枚举
 */
enum class BattleEndReason
{
    TIMEOUT,           ///< 时间耗尽
    ALL_DESTROYED,     ///< 全部摧毁
    ALL_UNITS_DEAD,    ///< 全军覆没
    SURRENDER,         ///< 主动投降
    NETWORK_DISCONNECT ///< 网络断开
};

/**
 * @class BattleManager
 * @brief 战斗逻辑管理器 - 处理战斗中的所有逻辑
 */
class BattleManager
{
public:
    /**
     * @enum BattleState
     * @brief 战斗状态枚举
     */
    enum class BattleState
    {
        LOADING,   ///< 加载中
        READY,     ///< 准备就绪
        FIGHTING,  ///< 战斗中
        FINISHED   ///< 已结束
    };

    BattleManager();
    ~BattleManager();

    /**
     * @brief 初始化战斗管理器
     * @param mapLayer 地图层
     * @param enemyData 敌方数据
     * @param enemyUserId 敌方用户ID
     * @param isReplay 是否为回放模式
     */
    void init(cocos2d::Node* mapLayer, const AccountGameData& enemyData, const std::string& enemyUserId, bool isReplay);

    /**
     * @brief 设置建筑列表
     * @param buildings 建筑列表
     */
    void setBuildings(const std::vector<BaseBuilding*>& buildings);

    /**
     * @brief 每帧更新
     * @param dt 帧时间间隔
     */
    void update(float dt);

    /**
     * @brief 开始战斗
     * @param deployment 部署的部队
     */
    void startBattle(const TroopDeploymentMap& deployment);

    /**
     * @brief 结束战斗
     * @param surrender 是否投降
     */
    void endBattle(bool surrender);

    /**
     * @brief 部署单位
     * @param type 单位类型
     * @param position 部署位置
     */
    void deployUnit(UnitType type, const cocos2d::Vec2& position);

    /** @brief 获取战斗状态 */
    BattleState getState() const { return _state; }

    /** @brief 是否为回放模式 */
    bool isReplayMode() const { return _isReplayMode; }

    /** @brief 获取获得的星星数 */
    int getStars() const { return _starsEarned; }

    /** @brief 获取摧毁百分比 */
    int getDestructionPercent() const { return _destructionPercent; }

    /** @brief 获取掠夺的金币 */
    int getGoldLooted() const { return _goldLooted; }

    /** @brief 获取掠夺的圣水 */
    int getElixirLooted() const { return _elixirLooted; }

    /** @brief 获取剩余时间 */
    float getRemainingTime() const;

    /** @brief 获取战斗结束原因 */
    BattleEndReason getEndReason() const { return _endReason; }

    /** @brief 大本营是否被摧毁 */
    bool isTownHallDestroyed() const { return _townHallDestroyed; }

    /**
     * @brief 获取指定类型部队数量
     * @param type 单位类型
     * @return int 部队数量
     */
    int getTroopCount(UnitType type) const;

    /** @brief 获取剩余总部队数 */
    int getTotalRemainingTroops() const;

    /**
     * @brief 设置战斗模式
     * @param mode 战斗模式
     * @param warId 战争ID
     */
    void setBattleMode(BattleMode mode, const std::string& warId = "");

    /** @brief 是否可以部署单位 */
    bool canDeployUnit() const;

    /** @brief 计算星星数 */
    int calculateStars() const;

    /** @brief 计算摧毁率 */
    float calculateDestructionRate() const;

    /** @brief 设置UI更新回调 */
    void setUIUpdateCallback(const std::function<void()>& callback) { _onUIUpdate = callback; }

    /** @brief 设置战斗结束回调 */
    void setBattleEndCallback(const std::function<void()>& callback) { _onBattleEnd = callback; }

    /** @brief 设置部队部署回调 */
    void setTroopDeployCallback(const std::function<void(UnitType, int)>& callback) { _onTroopDeploy = callback; }

    /**
     * @brief 设置网络模式
     * @param isNetworked 是否为网络模式
     * @param isAttacker 是否为攻击者
     */
    void setNetworkMode(bool isNetworked, bool isAttacker);

    /**
     * @brief 远程部署单位
     * @param type 单位类型
     * @param position 部署位置
     */
    void deployUnitRemote(UnitType type, const cocos2d::Vec2& position);

    /** @brief 设置网络部署回调 */
    void setNetworkDeployCallback(const std::function<void(UnitType, const cocos2d::Vec2&)>& callback);

private:
    void fixedUpdate();
    void updateBattleState(float dt);
    void updateUnitAI(float dt);
    void activateAllBuildings();
    void calculateBattleResult();
    void uploadBattleResult();
    std::string getCurrentTimestamp();

    void updateStarsAndDestruction();
    void checkBattleEndConditions();
    bool checkAllUnitsDeadOrDeployed() const;
    int countAliveUnits() const;

    GridMap* _gridMap = nullptr;
    void updateTroopCounts();

    BattleMode _battleMode = BattleMode::LOCAL;
    std::string _currentWarId;

    void spawnUnit(UnitType type, const cocos2d::Vec2& position);

    cocos2d::Node* _mapLayer = nullptr;        ///< 地图层
    AccountGameData _enemyGameData;            ///< 敌方游戏数据
    std::string _enemyUserId;                  ///< 敌方用户ID
    bool _isReplayMode = false;                ///< 是否为回放模式
    BattleState _state = BattleState::LOADING; ///< 战斗状态

    float _battleTime = 180.0f;       ///< 战斗总时间
    float _elapsedTime = 0.0f;        ///< 已用时间
    int _starsEarned = 0;             ///< 获得星星数
    int _goldLooted = 0;              ///< 掠夺金币
    int _elixirLooted = 0;            ///< 掠夺圣水
    int _destructionPercent = 0;      ///< 摧毁百分比

    BattleEndReason _endReason = BattleEndReason::TIMEOUT;  ///< 战斗结束原因
    bool _townHallDestroyed = false;   ///< 大本营是否被摧毁
    bool _hasDeployedAnyUnit = false;  ///< 是否曾部署过单位

    std::vector<BaseUnit*> _deployedUnits;       ///< 已部署的单位
    std::vector<BaseBuilding*> _enemyBuildings;  ///< 敌方建筑
    int _totalBuildingHP = 0;      ///< 总建筑血量
    int _destroyedBuildingHP = 0;  ///< 已摧毁建筑血量

    int _barbarianCount = 0;    ///< 野蛮人数量
    int _archerCount = 0;       ///< 弓箭手数量
    int _giantCount = 0;        ///< 巨人数量
    int _goblinCount = 0;       ///< 哥布林数量
    int _wallBreakerCount = 0;  ///< 炸弹人数量

    float _accumulatedTime = 0.0f;   ///< 累积时间
    unsigned int _currentFrame = 0;  ///< 当前帧
    const float FIXED_TIME_STEP = 1.0f / 60.0f;  ///< 固定时间步长

    std::function<void()> _onUIUpdate;               ///< UI更新回调
    std::function<void()> _onBattleEnd;              ///< 战斗结束回调
    std::function<void(UnitType, int)> _onTroopDeploy;  ///< 部队部署回调

    bool _isNetworked = false;  ///< 是否为网络模式
    bool _isAttacker = false;   ///< 是否为攻击者
    std::function<void(UnitType, const cocos2d::Vec2&)> _onNetworkDeploy;  ///< 网络部署回调
};

#endif  // BATTLE_MANAGER_H_