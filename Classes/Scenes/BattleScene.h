/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleScene.h
 * File Function: 战斗场景
 * Author:        赵崇治
 * Update Date:   2025/12/26
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "Buildings/DefenseBuilding.h"
#include "Managers/BattleManager.h"
#include "Managers/GameDataModels.h"
#include "Managers/ReplaySystem.h"
#include "UI/BattleUI.h"
#include "Unit/UnitTypes.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

// 前向声明
class BuildingManager;
class GridMap;
class BaseBuilding;

/**
 * @class BattleScene
 * @brief 战斗场景 - 异步多人游戏攻击场景
 *
 * 功能：
 * 1. 加载敌方基地布局
 * 2. 部署己方士兵进行攻击
 * 3. 计算战斗结果（星数、掠夺资源）
 * 4. 支持PVP模式和观战模式
 */
class BattleScene : public cocos2d::Scene {
 public:
    /**
     * @brief 创建战斗场景
     */
    static cocos2d::Scene* createScene();

    CREATE_FUNC(BattleScene);

    /**
     * @brief 创建战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     */
    static BattleScene* createWithEnemyData(const GameStateData& enemyData);

    /**
     * @brief 创建战斗场景（带敌方数据和用户ID）
     * @param enemyData 敌方玩家的基地数据
     * @param enemyUserId 敌方玩家ID
     */
    static BattleScene* createWithEnemyData(const GameStateData& enemyData, 
                                            const std::string& enemyUserId);

    /**
     * @brief 创建战斗回放场景
     * @param replayDataStr 序列化的回放数据
     */
    static BattleScene* createWithReplayData(const std::string& replayDataStr);

    virtual bool init() override;
    virtual bool initWithEnemyData(const GameStateData& enemyData);
    virtual bool initWithEnemyData(const GameStateData& enemyData, 
                                   const std::string& enemyUserId);
    virtual bool initWithReplayData(const std::string& replayDataStr);

    virtual void update(float dt) override;
    virtual void onEnter() override;
    virtual void onExit() override;

    /**
     * @brief 设置PVP模式
     * @param isAttacker 是否为攻击方
     */
    void setPvpMode(bool isAttacker);

    /**
     * @brief 设置观战模式
     * @param attackerId 攻击方ID
     * @param defenderId 防守方ID
     * @param elapsedMs 已经过的时间（毫秒）
     * @param history 历史操作记录
     */
    void setSpectateMode(const std::string& attackerId, 
                         const std::string& defenderId,
                         int64_t elapsedMs,
                         const std::vector<std::string>& history);

    /**
     * @brief 设置观战历史记录（向后兼容）
     * @param history 历史操作记录
     */
    void setSpectateHistory(const std::vector<std::string>& history);

    /**
     * @brief 标记场景是否通过 pushScene 进入
     * @param pushed 是否为 push 进入
     * @note 用于 returnToMainScene 决定使用 popScene 还是 replaceScene
     */
    void setPushedScene(bool pushed) { _isPushedScene = pushed; }

 private:
    BattleScene();
    ~BattleScene();

    // ==================== 场景元素 ====================
    cocos2d::Size    _visibleSize;
    cocos2d::Sprite* _mapSprite       = nullptr;
    GridMap*         _gridMap         = nullptr;
    BuildingManager* _buildingManager = nullptr;
    BattleUI*        _battleUI        = nullptr;
    BattleManager*   _battleManager   = nullptr;

    // ==================== 触摸控制相关 ====================
    cocos2d::Vec2 _lastTouchPos;
    bool          _isDragging = false;
    float         _timeScale  = 1.0f;

    // 多点触控缩放
    std::map<int, cocos2d::Vec2> _activeTouches;
    bool                         _isPinching        = false;
    float                        _prevPinchDistance = 0.0f;

    // ==================== 士兵部署数据 ====================
    UnitType _selectedUnitType = UnitType::kNone;  ///< 默认无选中单位

    // ==================== 初始化方法 ====================
    void setupMap();
    void setupUI();
    void setupTouchListeners();

    // ==================== 交互逻辑 ====================
    void onTroopSelected(UnitType type);
    void onTroopDeselected();
    void returnToMainScene();
    void toggleSpeed();

    // ==================== 地图控制 ====================
    cocos2d::Rect _mapBoundary;
    void          updateBoundary();
    void          ensureMapInBoundary();

    // ==================== 战斗模式血条管理 ====================
    void enableAllBuildingsBattleMode();
    void disableAllBuildingsBattleMode();

    // ==================== 观战历史回放 ====================
    void replaySpectateHistory();
    
    // 🔧 新增：检查观战同步结束条件
    void checkSpectateEndCondition();

    // ==================== 部署区域可视化 ====================
    bool _deployOverlayShown = false;  ///< 部署覆盖层是否已显示
    
    /**
     * @brief 显示部署限制区域覆盖层
     * @note 在准备阶段显示，战斗开始后淡出
     */
    void showDeployRestrictionOverlay();
    
    /**
     * @brief 隐藏部署限制区域覆盖层（带淡出效果）
     */
    void hideDeployRestrictionOverlay();

    // ==================== PVP/观战状态 ====================
    bool        _isPvpMode      = false;    ///< 是否为PVP模式
    bool        _isAttacker     = false;    ///< 是否为攻击方
    bool        _isSpectateMode = false;    ///< 是否为观战模式
    std::string _spectateAttackerId;        ///< 观战时的攻击方ID
    std::string _spectateDefenderId;        ///< 观战时的防守方ID
    int64_t     _spectateElapsedMs = 0;     ///< 观战时已经过的时间
    std::vector<std::string> _spectateHistory;  ///< 观战历史操作
    bool        _historyReplayed = false;   ///< 历史是否已回放
    size_t      _spectateHistoryIndex = 0;  ///< 已处理的历史操作索引（用于跳过重复）

    // 🔧 新增：用于防止重复部署的同步机制
    bool _spectateHistoryProcessed = false;  // 历史操作是否已完全处理
    std::set<std::string> _processedActionSet;  // 已处理的操作集合（用于去重）
    std::vector<std::tuple<int, float, float>> _pendingRemoteActions;  // 缓存的远程操作

    // 🔧 新增：观战同步结束机制
    bool   _spectatePendingEnd = false;       ///< 是否收到结束信号但等待同步
    size_t _spectateExpectedActionCount = 0;  ///< 服务器端总操作数
    size_t _spectateReceivedActionCount = 0;  ///< 已接收的操作数
    float  _spectatePendingEndTimer = 0.0f;   ///< 等待同步超时计时器
    static constexpr float kSpectateEndTimeout = 5.0f;  ///< 观战同步超时时间（秒）

    // ==================== 场景栈标记 ====================
    bool        _isPushedScene = true;      ///< 是否通过 pushScene 进入（默认为 true）

    // ==================== 部署验证 ====================
    /**
     * @brief 检查位置是否可以部署单位
     * @param mapLocalPos 地图本地坐标
     * @return bool 是否可以部署
     * @note 单位只能在建筑物周围一圈网格之外部署
     */
    bool canDeployAtPosition(const cocos2d::Vec2& mapLocalPos) const;

    /**
     * @brief 显示部署失败的可视化反馈
     * @param worldPos 世界坐标位置
     * @param mapLocalPos 地图本地坐标
     * @param reason 失败原因
     */
    void showDeployFailedFeedback(const cocos2d::Vec2& worldPos, 
                                  const cocos2d::Vec2& mapLocalPos,
                                  const std::string& reason);
};

#endif  // __BATTLE_SCENE_H__