/****************************************************************
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleScene.h
 * File Function: 战斗场景
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef BATTLE_SCENE_H_
#define BATTLE_SCENE_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include "Buildings/DefenseBuilding.h"
#include "Managers/AccountManager.h"
#include "Managers/BattleManager.h"
#include "Managers/ReplaySystem.h"
#include "UI/BattleUI.h"
#include "Unit/UnitTypes.h"

#include <map>
#include <string>
#include <vector>

// Forward declarations
class BuildingManager;
class GridMap;
class BaseBuilding;

/**
 * @class BattleScene
 * @brief 战斗场景 - 异步多人游戏攻击场景
 * 
 * 功能：
 * 1. 加载敌方基地布局（从 AccountGameData）
 * 2. 部署己方士兵进行攻击
 * 3. 计算战斗结果（星数、掠夺资源）
 * 4. 上传战斗结果到服务器（可选）
 */
class BattleScene : public cocos2d::Scene {
public:
    /**
     * @brief 创建战斗场景
     */
    static cocos2d::Scene* createScene();
    
    /**
     * @brief 创建战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     */
    static BattleScene* createWithEnemyData(const AccountGameData& enemyData);

    /**
     * @brief 创建战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     * @param enemyUserId 敌方玩家ID
     */
    static BattleScene* createWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId);
    
    /**
     * @brief 创建战斗回放场景
     * @param replayDataStr 序列化的回放数据
     */
    static BattleScene* createWithReplayData(const std::string& replayDataStr);

    virtual bool init() override;
    
    /**
     * @brief 初始化战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     */
    virtual bool initWithEnemyData(const AccountGameData& enemyData);

    /**
     * @brief 初始化战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     * @param enemyUserId 敌方玩家ID
     */
    virtual bool initWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId);
    
    /**
     * @brief 初始化战斗回放场景
     */
    virtual bool initWithReplayData(const std::string& replayDataStr);

    virtual void update(float dt) override;
    virtual void onEnter() override;
    virtual void onExit() override;
    
    // 🆕 PVP Configuration
    void setPvpMode(bool isAttacker);
    void setSpectateHistory(const std::vector<std::string>& history); // 🆕 Set history for spectating

private:
    BattleScene();
    ~BattleScene();
    
    // ==================== 场景元素 ====================
    cocos2d::Size _visibleSize;
    cocos2d::Sprite* _mapSprite = nullptr;
    GridMap* _gridMap = nullptr;
    BuildingManager* _buildingManager = nullptr;
    BattleUI* _battleUI = nullptr;
    BattleManager* _battleManager = nullptr; // 🆕 BattleManager instance
    
    // ==================== 触摸控制相关 ====================
    cocos2d::Vec2 _lastTouchPos;
    bool _isDragging = false;
    float _timeScale = 1.0f;
    
    // 🆕 多点触控缩放
    std::map<int, cocos2d::Vec2> _activeTouches;
    bool _isPinching = false;
    float _prevPinchDistance = 0.0f;

    // ==================== 士兵部署数据 ====================
    UnitType _selectedUnitType = UnitType::kBarbarian;
    
    // ==================== 初始化方法 ====================
    void setupMap();
    void setupUI();
    void setupTouchListeners();
    
    // ==================== 交互逻辑 ====================
    void onTroopSelected(UnitType type);
    void onTroopDeselected();  // 🆕 取消选中处理
    void returnToMainScene();
    void toggleSpeed();

    // ==================== 地图控制 ====================
    cocos2d::Rect _mapBoundary;
    void updateBoundary();
    void ensureMapInBoundary();

    // ==================== 🆕 战斗模式血条管理 ====================
    /**
     * @brief 启用所有防御建筑的战斗模式和血条显示
     */
    void enableAllBuildingsBattleMode();

    /**
     * @brief 禁用所有防御建筑的战斗模式并重置血量
     */
    void disableAllBuildingsBattleMode();
    
    // PVP State
    bool _isPvpMode = false;
    bool _isAttacker = false;
    std::vector<std::string> _spectateHistory; // 🆕 History buffer
};

#endif  // BATTLE_SCENE_H_