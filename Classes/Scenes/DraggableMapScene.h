/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DraggableMapScene.h
 * File Function: 主场景类
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"

#include <string>
#include <map>

class MapController;
class SceneUIController;
class InputController;
class BuildingManager;
class BaseBuilding;
class HUDLayer;
class ResourceCollectionManager;
struct BuildingData;

/**
 * @class DraggableMapScene
 * @brief 主游戏场景 - 精简后的主控制器
 *
 * 职责：
 * - 场景初始化和生命周期管理
 * - 协调各个管理器之间的交互
 * - 处理游戏逻辑回调
 * - 管理升级UI
 */
class DraggableMapScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建场景
     * @return cocos2d::Scene* 场景指针
     */
    static cocos2d::Scene* createScene();

    virtual bool init() override;
    virtual ~DraggableMapScene();
    virtual void update(float dt) override;
    virtual void onEnter() override;

    CREATE_FUNC(DraggableMapScene);

    /** @brief 获取大本营等级 */
    int getTownHallLevel() const;

    /**
     * @brief 获取建筑数量
     * @param name 建筑名称
     * @return int 数量
     */
    int getBuildingCount(const std::string& name) const;

    /**
     * @brief 开始放置建筑
     * @param data 建筑数据
     */
    void startPlacingBuilding(const BuildingData& data);

    /** @brief 关闭升级UI */
    void closeUpgradeUI();

    /** @brief 获取建筑管理器 */
    BuildingManager* getBuildingManager() const { return _buildingManager; }

private:
    MapController* _mapController = nullptr;        ///< 地图控制器
    SceneUIController* _uiController = nullptr;     ///< UI控制器
    InputController* _inputController = nullptr;    ///< 输入控制器
    BuildingManager* _buildingManager = nullptr;    ///< 建筑管理器
    HUDLayer* _hudLayer = nullptr;                  ///< HUD层
    ResourceCollectionManager* _collectionMgr = nullptr;  ///< 资源收集管理器

    bool _isAttackMode = false;          ///< 是否为攻击模式
    std::string _attackTargetUserId = "";  ///< 攻击目标用户ID
    cocos2d::Node* _currentUpgradeUI = nullptr;  ///< 当前升级UI

    cocos2d::Size _visibleSize;  ///< 可视区域大小

    BaseBuilding* _clickedBuilding = nullptr;        ///< 点击的建筑
    cocos2d::Vec2 _touchBeganPos = cocos2d::Vec2::ZERO;  ///< 触摸开始位置
    float _touchBeganTime = 0.0f;  ///< 触摸开始时间
    bool _hasMoved = false;        ///< 是否移动过

    std::map<int, cocos2d::Vec2> _activeTouches;  ///< 活动触摸点
    bool _isPinching = false;         ///< 是否在缩放
    float _prevPinchDistance = 0.0f;  ///< 上次缩放距离

    // 🆕 新增标志位：防止切换账号时析构函数错误保存数据
    bool _isSwitchingAccount = false;
    
    // 场景恢复事件监听器
    cocos2d::EventListenerCustom* _sceneResumeListener = nullptr;

    void initializeManagers();           ///< 初始化管理器
    void setupCallbacks();               ///< 设置回调
    void setupUpgradeManagerCallbacks(); ///< 设置升级管理器回调
    void loadGameState();                ///< 加载游戏状态
    void initBuildingData();             ///< 初始化建筑数据

    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);
    void onMouseScroll(float scrollY, cocos2d::Vec2 mousePos);
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode);

    void onShopClicked();       ///< 商店点击
    void onAttackClicked();     ///< 攻击点击
    void onClanClicked();       ///< 部落点击
    void onBuildingSelected(const BuildingData& data);  ///< 建筑选中
    void onConfirmBuilding();   ///< 确认建造
    void onCancelBuilding();    ///< 取消建造
    void onAccountSwitched();   ///< 账户切换
    void onLogout();            ///< 登出
    void onMapChanged(const std::string& newMap);  ///< 地图切换

    void onBuildingPlaced(BaseBuilding* building);   ///< 建筑放置
    void onBuildingClicked(BaseBuilding* building);  ///< 建筑点击
    void onBuildingHint(const std::string& hint);    ///< 建筑提示

    void showUpgradeUI(BaseBuilding* building);  ///< 显示升级UI
    void hideUpgradeUI();      ///< 隐藏升级UI
    void cleanupUpgradeUI();   ///< 清理升级UI

    void registerResourceBuilding(class ResourceBuilding* building);  ///< 注册资源建筑

    void onSceneResume();  ///< 场景恢复

    bool switchToAttackMode(const std::string& targetUserId);  ///< 切换到攻击模式
    void returnToOwnBase();    ///< 返回自己基地

    void connectToServer();       ///< 连接服务器
    void setupNetworkCallbacks(); ///< 设置网络回调

    void showLocalPlayerList();   ///< 显示本地玩家列表
    void showPlayerListFromServerData(const std::string& serverData);  ///< 显示服务器玩家列表
    void startAttack(const std::string& targetUserId);  ///< 开始攻击
    std::string getCurrentTimestamp();  ///< 获取当前时间戳
};

#endif // __DRAGGABLE_MAP_SCENE_H__