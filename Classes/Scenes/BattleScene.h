#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "AccountManager.h"  // 包含完整定义
#include <string>

// Forward declarations
class BuildingManager;
class GridMap;

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
     * @brief 创建战斗场景（传统方式，保留兼容性）
     */
    static cocos2d::Scene* createScene();
    
    /**
     * @brief 创建战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     */
    static BattleScene* createWithEnemyData(const AccountGameData& enemyData);
    
    virtual bool init() override;
    
    /**
     * @brief 初始化战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     */
    virtual bool initWithEnemyData(const AccountGameData& enemyData);
    
    virtual void update(float dt) override;
    
private:
    BattleScene() = default;
    ~BattleScene() = default;
    
    // ==================== 战斗状态 ====================
    enum class BattleState {
        LOADING,        // 加载敌方基地
        READY,          // 准备部署士兵
        FIGHTING,       // 战斗中
        FINISHED        // 战斗结束
    };
    
    BattleState _state = BattleState::LOADING;
    
    // ==================== 敌方数据 ====================
    AccountGameData _enemyGameData;
    std::string _enemyUserId;
    int _enemyTownHallLevel = 1;
    
    // ==================== 战斗数据 ====================
    float _battleTime = 180.0f;          // 3分钟战斗时间
    float _elapsedTime = 0.0f;
    int _starsEarned = 0;                // 获得星数 (0-3)
    int _goldLooted = 0;                 // 掠夺金币
    int _elixirLooted = 0;               // 掠夺圣水
    int _destructionPercent = 0;         // 摧毁百分比 (0-100)
    
    // ==================== 场景元素 ====================
    cocos2d::Size _visibleSize;
    cocos2d::Sprite* _mapSprite = nullptr;
    GridMap* _gridMap = nullptr;
    BuildingManager* _buildingManager = nullptr;
    
    // ==================== UI 元素 ====================
    cocos2d::Label* _statusLabel = nullptr;
    cocos2d::Label* _timerLabel = nullptr;
    cocos2d::Label* _starsLabel = nullptr;
    cocos2d::Label* _destructionLabel = nullptr;
    cocos2d::ui::Button* _endBattleButton = nullptr;
    cocos2d::ui::Button* _returnButton = nullptr;
    
    // ==================== 初始化方法 ====================
    void setupMap();
    void setupUI();
    void loadEnemyBase();
    
    // ==================== 战斗逻辑 ====================
    void startBattle();
    void endBattle(bool surrender = false);
    void updateBattleState(float dt);
    void calculateBattleResult();
    
    // ==================== UI 更新 ====================
    void updateTimer();
    void updateStars(int stars);
    void updateDestruction(int percent);
    void showBattleResult();
    
    // ==================== 返回主场景 ====================
    void returnToMainScene();
    
    // ==================== 网络相关（可选） ====================
    void uploadBattleResult();
};

#endif // __BATTLE_SCENE_H__