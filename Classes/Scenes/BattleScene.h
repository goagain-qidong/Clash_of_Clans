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

class BuildingManager;
class GridMap;
class BaseBuilding;

/**
 * @class BattleScene
 * @brief 战斗场景 - 异步多人游戏攻击场景
 *
 * 功能：
 * - 加载敌方基地布局
 * - 部署己方士兵进行攻击
 * - 计算战斗结果
 * - 上传战斗结果到服务器
 */
class BattleScene : public cocos2d::Scene {
public:
    /**
     * @brief 创建战斗场景
     * @return cocos2d::Scene* 场景指针
     */
    static cocos2d::Scene* createScene();

    /**
     * @brief 创建战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     * @return BattleScene* 场景指针
     */
    static BattleScene* createWithEnemyData(const AccountGameData& enemyData);

    /**
     * @brief 创建战斗场景（带敌方数据和ID）
     * @param enemyData 敌方玩家的基地数据
     * @param enemyUserId 敌方玩家ID
     * @return BattleScene* 场景指针
     */
    static BattleScene* createWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId);

    /**
     * @brief 创建战斗回放场景
     * @param replayDataStr 序列化的回放数据
     * @return BattleScene* 场景指针
     */
    static BattleScene* createWithReplayData(const std::string& replayDataStr);

    virtual bool init() override;

    /**
     * @brief 初始化战斗场景（带敌方数据）
     * @param enemyData 敌方玩家的基地数据
     * @return bool 是否成功
     */
    virtual bool initWithEnemyData(const AccountGameData& enemyData);

    /**
     * @brief 初始化战斗场景（带敌方数据和ID）
     * @param enemyData 敌方玩家的基地数据
     * @param enemyUserId 敌方玩家ID
     * @return bool 是否成功
     */
    virtual bool initWithEnemyData(const AccountGameData& enemyData, const std::string& enemyUserId);

    /**
     * @brief 初始化战斗回放场景
     * @param replayDataStr 回放数据
     * @return bool 是否成功
     */
    virtual bool initWithReplayData(const std::string& replayDataStr);

    virtual void update(float dt) override;
    virtual void onEnter() override;
    virtual void onExit() override;

    /**
     * @brief 设置PVP模式
     * @param isAttacker 是否为攻击者
     */
    void setPvpMode(bool isAttacker);
    void setSpectateHistory(const std::vector<std::string>& history); // 🆕 Set history for spectating

private:
    BattleScene();
    ~BattleScene();

    cocos2d::Size _visibleSize;               ///< 可视区域大小
    cocos2d::Sprite* _mapSprite = nullptr;    ///< 地图精灵
    GridMap* _gridMap = nullptr;              ///< 网格地图
    BuildingManager* _buildingManager = nullptr;  ///< 建筑管理器
    BattleUI* _battleUI = nullptr;            ///< 战斗UI
    BattleManager* _battleManager = nullptr;  ///< 战斗管理器

    cocos2d::Vec2 _lastTouchPos;  ///< 上次触摸位置
    bool _isDragging = false;     ///< 是否在拖拽
    float _timeScale = 1.0f;      ///< 时间缩放

    std::map<int, cocos2d::Vec2> _activeTouches;  ///< 活动触摸点
    bool _isPinching = false;         ///< 是否在缩放
    float _prevPinchDistance = 0.0f;  ///< 上次缩放距离

    UnitType _selectedUnitType = UnitType::kBarbarian;  ///< 选中的单位类型

    void setupMap();             ///< 设置地图
    void setupUI();              ///< 设置UI
    void setupTouchListeners();  ///< 设置触摸监听器

    void onTroopSelected(UnitType type);  ///< 选中部队
    void onTroopDeselected();             ///< 取消选中
    void returnToMainScene();             ///< 返回主场景
    void toggleSpeed();                   ///< 切换速度

    cocos2d::Rect _mapBoundary;   ///< 地图边界
    void updateBoundary();        ///< 更新边界
    void ensureMapInBoundary();   ///< 确保地图在边界内

    /** @brief 启用所有建筑的战斗模式 */
    void enableAllBuildingsBattleMode();

    /** @brief 禁用所有建筑的战斗模式 */
    void disableAllBuildingsBattleMode();
    
    // PVP State
    bool _isPvpMode = false;
    bool _isAttacker = false;
    std::vector<std::string> _spectateHistory; // 🆕 History buffer
};

#endif  // BATTLE_SCENE_H_