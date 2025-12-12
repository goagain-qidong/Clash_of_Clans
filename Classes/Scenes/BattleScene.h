#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "AccountManager.h"
#include "Unit/unit.h"
#include "Buildings/DefenseBuilding.h"
#include "Managers/ReplaySystem.h" // 🆕 添加 ReplaySystem 头文件
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
     * @brief 创建战斗场景（传统方式，保留兼容性）
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
    bool _isReplayMode = false; // 🆕 是否为回放模式
    
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
    
    // ==================== 战斗单位管理 ⭐ 新增 ====================
    std::vector<Unit*> _deployedUnits;           // 已部署的士兵
    std::vector<BaseBuilding*> _enemyBuildings;  // 敌方建筑列表
    int _totalBuildingHP = 0;                    // 总建筑血量
    int _destroyedBuildingHP = 0;                // 已摧毁建筑血量
    
    // ==================== UI 元素 ====================
    cocos2d::Label* _statusLabel = nullptr;
    cocos2d::Label* _timerLabel = nullptr;
    cocos2d::Label* _starsLabel = nullptr;
    cocos2d::Label* _destructionLabel = nullptr;
    cocos2d::ui::Button* _endBattleButton = nullptr;
    cocos2d::ui::Button* _returnButton = nullptr;
    cocos2d::ui::Button* _speedButton = nullptr; // 🆕 速度控制按钮
    
    // ✅ 新增：触摸控制相关
    cocos2d::Vec2 _lastTouchPos;
    bool _isDragging = false;
    float _timeScale = 1.0f; // 🆕 时间缩放比例
    
    // ==================== 士兵部署 UI ⭐ 新增 ====================
    cocos2d::ui::Button* _barbarianButton = nullptr;
    cocos2d::ui::Button* _archerButton = nullptr;
    cocos2d::ui::Button* _giantButton = nullptr;
    cocos2d::Label* _barbarianCountLabel = nullptr;
    cocos2d::Label* _archerCountLabel = nullptr;
    cocos2d::Label* _giantCountLabel = nullptr;
    
    int _barbarianCount = 20;  // 可用野蛮人数量
    int _archerCount = 20;     // 可用弓箭手数量
    int _giantCount = 5;       // 可用巨人数量
    
    UnitType _selectedUnitType = UnitType::kBarbarian;  // 当前选中的兵种
    
    // ==================== 初始化方法 ====================
    void setupMap();
    void setupUI();
    void setupTouchListeners();  // ✅ 新增
    void loadEnemyBase();
    void setupTroopButtons();  // ⭐ 新增：设置部署按钮
    
    // ==================== 士兵部署与管理 ⭐ 新增 ====================
    void deployUnit(UnitType type, const cocos2d::Vec2& position);
    void onTroopButtonClicked(UnitType type);
    void updateTroopCounts();
    void updateUnitAI(float dt);  // 更新所有士兵的 AI
    void activateDefenseBuildings();  // 激活防御建筑
    
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
    void toggleSpeed(); // 🆕 切换回放速度
    
    // ==================== 返回主场景 ====================
    void returnToMainScene();
    
    // ==================== 网络相关（可选） ====================
    void uploadBattleResult();
    
    // ==================== 🆕 辅助函数 ====================
    std::string getCurrentTimestamp();
};

#endif // __BATTLE_SCENE_H__