/**
 * @file DraggableMapScene.h
 * @brief 主场景类 - 重构后的精简版本
 *
 * 职责：
 * - 场景初始化和生命周期管理
 * - 协调各个管理器之间的交互
 * - 处理游戏逻辑回调
 * - 管理升级UI
 */

#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"
#include <string>

// 前向声明
class MapController;
class SceneUIController;
class InputController;
class BuildingManager;
class BaseBuilding;
class HUDLayer;
struct BuildingData;

/**
 * @class DraggableMapScene
 * @brief 主游戏场景 - 精简后的主控制器
 */
class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual ~DraggableMapScene();
    virtual void update(float dt) override;

    CREATE_FUNC(DraggableMapScene);
    
    // 供 ShopLayer 调用
    int getTownHallLevel() const;
    int getBuildingCount(const std::string& name) const;
    void startPlacingBuilding(const BuildingData& data);
    
    // 供外部调用的升级UI接口
    void closeUpgradeUI();

private:
    // ==================== 管理器 ====================
    MapController* _mapController = nullptr;
    SceneUIController* _uiController = nullptr;
    InputController* _inputController = nullptr;
    BuildingManager* _buildingManager = nullptr;
    HUDLayer* _hudLayer = nullptr;
    
    // ==================== 游戏状态 ====================
    bool _isAttackMode = false;
    std::string _attackTargetUserId = "";
    cocos2d::Node* _currentUpgradeUI = nullptr;
    
    cocos2d::Size _visibleSize;
    
    // ==================== 建筑点击和拖动状态 ====================
    BaseBuilding* _clickedBuilding = nullptr;
    cocos2d::Vec2 _touchBeganPos = cocos2d::Vec2::ZERO;
    float _touchBeganTime = 0.0f;
    bool _hasMoved = false;
    
    // ==================== 初始化 ====================
    void initializeManagers();
    void setupCallbacks();
    void loadGameState();
    void initBuildingData();
    
    // ==================== 输入处理回调 ====================
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onMouseScroll(float scrollY, cocos2d::Vec2 mousePos);
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode);
    
    // ==================== UI 回调 ====================
    void onShopClicked();
    void onAttackClicked();
    void onClanClicked();
    void onBuildingSelected(const BuildingData& data);
    void onConfirmBuilding();
    void onCancelBuilding();
    void onAccountSwitched();
    void onLogout();
    
    // ==================== 建筑回调 ====================
    void onBuildingPlaced(BaseBuilding* building);
    void onBuildingClicked(BaseBuilding* building);
    void onBuildingHint(const std::string& hint);
    
    // ==================== 升级UI ====================
    void showUpgradeUI(BaseBuilding* building);
    void hideUpgradeUI();
    void cleanupUpgradeUI();

    // ==================== 多人游戏 ====================
    bool switchToAttackMode(const std::string& targetUserId);
    void returnToOwnBase();
    
    // ==================== 网络 ====================
    void connectToServer();
    void setupNetworkCallbacks();
};

#endif // __DRAGGABLE_MAP_SCENE_H__