/**
 * @file DraggableMapScene.h
 * @brief 主场景类 - 场景层
 *
 * 职责分工：
 * 1. 场景初始化：地图、UI、管理器的设置
 * 2. 输入处理：触摸/鼠标事件的接收和转发
 * 3. UI管理：建造UI、确认UI、升级UI的显示/隐藏
 * 4. 建筑交互：点击建筑后的升级UI呼出
 * 5. 网络通信：服务器连接和回调处理
 *
 * 不应做的事：
 * - 不应处理建筑的创建和放置细节（由 BuildingManager 处理）
 * - 不应处理建筑升级逻辑（由 BaseBuilding 处理）
 * - 不应处理网格计算（由 GridMap 处理）
 */

#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include <string>
#include <unordered_map>
#include <vector>

#include "BuildingData.h"
#include "GridMap.h"
#include "HeroManager.h"
#include "Managers/TownHallSystem.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

// 前向声明
class BuildingManager;
class BaseBuilding;
class BuildingUpgradeUI;

/**
 * @class DraggableMapScene
 * @brief 主游戏场景 - 负责地图显示、UI管理和输入转发
 */
class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual ~DraggableMapScene();
    virtual void update(float dt) override;

    CREATE_FUNC(DraggableMapScene);
    // 供商店调用
    int getTownHallLevel() const;
    int getBuildingCount(const std::string& name) const;
    void startPlacingBuilding(const BuildingData& data); // 暴露建造接口
    void openShop(); // 打开商店
private:
    // ==================== 数据结构 ====================
    struct MapConfig
    {
        float scale;
        cocos2d::Vec2 startPixel;
        float tileSize;
    };

    struct MapElement
    {
        cocos2d::Node* node;
        cocos2d::Vec2 localPosition;
    };

    // ==================== 场景基础属性 ====================
    cocos2d::Size _visibleSize;
    float _currentScale;
    float _minScale;
    float _maxScale;

    cocos2d::Sprite* _mapSprite;
    GridMap* _gridMap;
    cocos2d::Rect _mapBoundary;
    cocos2d::Vec2 _lastTouchPos;
    cocos2d::Vec2 _gridStartDefault;

    // ==================== 管理器引用 ====================
    BuildingManager* _buildingManager;
    HeroManager* _heroManager;

    // ==================== UI元素 ====================
    cocos2d::ui::Button* _buildButton;
    cocos2d::ui::Button* _mapButton;
    cocos2d::ui::Button* _battleButton;
    cocos2d::ui::Button* _clanButton;
    cocos2d::ui::ListView* _buildingListUI;
    cocos2d::ui::ListView* _mapList;
    bool _isBuildingListVisible;
    bool _isMapListVisible;

    cocos2d::ui::Button* _confirmButton;
    cocos2d::ui::Button* _cancelButton;

    Node* _currentUpgradeUI;
    ResourceDisplayUI* _resourceUI;

    // ==================== 地图与建筑数据 ====================
    std::string _currentMapName;
    std::vector<std::string> _mapNames;
    std::vector<BuildingData> _buildingList;
    std::unordered_map<std::string, MapConfig> _mapConfigs;
    std::vector<MapElement> _mapElements;

    // ==================== 【初始化】场景和UI的设置 ====================
    void setupMap();
    void setupUI();
    void setupTouchListener();
    void setupMouseListener();
    void setupNetworkCallbacks();
    void setupResourceDisplay();
    void initBuildingData();
    void createBuildingSelection();
    void createMapList();

    // ==================== 【输入处理】触摸和鼠标事件 ====================
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    // ==================== 【建造UI】建筑建造的确认流程 ====================
    void showConfirmButtons(const cocos2d::Vec2& buildingWorldPos);
    void hideConfirmButtons();
    void onConfirmBuilding();
    void onCancelBuilding();

    // ==================== 【菜单交互】按钮点击和列表操作 ====================
    void toggleBuildingSelection();
    void onBuildingItemClicked(cocos2d::Ref* sender, const BuildingData& building);
    void toggleMapList();
    void onMapButtonClicked(cocos2d::Ref* sender);
    void onMapItemClicked(cocos2d::Ref* sender);
    void onBattleButtonClicked(cocos2d::Ref* sender);
    void onClanButtonClicked(cocos2d::Ref* sender);

    // ==================== 【地图操作】缩放、平移、切换地图 ====================
    void moveMap(const cocos2d::Vec2& delta);
    void ensureMapInBoundary();
    void zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint = cocos2d::Vec2::ZERO);
    void updateBoundary();
    void switchMap(const std::string& mapName);

    // ==================== 【地图元素】保存和恢复地图状态 ====================
    void saveMapElementsState();
    void restoreMapElementsState();
    void createSampleMapElements();
    void updateMapElementsPosition();

    // ==================== 【建筑交互】建筑放置和升级UI ====================
    void onBuildingPlaced(BaseBuilding* building);
    void onBuildingClicked(BaseBuilding* building);
    void hideUpgradeUI();
    void closeUpgradeUI();  // 公开方法，作为 hideUpgradeUI 的别名

    // ==================== 【辅助】提示信息和清理 ====================
    void showBuildingHint(const std::string& hint);
    void cleanupUpgradeUI();

    // ==================== 【网络】服务器连接 ====================
    void connectToServer();
};

#endif