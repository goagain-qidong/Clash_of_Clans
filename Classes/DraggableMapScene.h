// DraggableMapScene.h - 简化版本
#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "GridMap.h"
#include "HeroManager.h"
#include "BuildingData.h"
#include "TownHallSystem.h"  // 只包含这个头文件
#include <unordered_map>
#include <string>
#include <vector>

class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual ~DraggableMapScene();
    void closeUpgradeUI();  // 关闭升级界面
    void cleanupUpgradeUI();
    CREATE_FUNC(DraggableMapScene);

private:
    struct MapConfig {
        float scale;
        cocos2d::Vec2 startPixel;
        float tileSize;
    };

    cocos2d::Size _visibleSize;
    float _currentScale;
    float _minScale;
    float _maxScale;

    cocos2d::Sprite* _mapSprite;
    GridMap* _gridMap;
    cocos2d::Rect _mapBoundary;
    cocos2d::Vec2 _lastTouchPos;
    cocos2d::Vec2 _dragStartPos;
    cocos2d::Vec2 _gridStartDefault;

    bool _isBuildingMode;
    bool _isDraggingBuilding;
    cocos2d::Sprite* _ghostSprite;
    BuildingData _selectedBuilding;
    cocos2d::Vec2 _pendingGridPos;
    bool _isWaitingConfirm;

    cocos2d::ui::Button* _buildButton;
    cocos2d::ui::Button* _mapButton;
    cocos2d::ui::ListView* _buildingListUI;
    cocos2d::ui::ListView* _mapList;
    bool _isBuildingListVisible;
    bool _isMapListVisible;

    cocos2d::ui::Button* _confirmButton;
    cocos2d::ui::Button* _cancelButton;

    HeroManager* _heroManager;

    // 使用新系统的类
    TownHallUpgradeUI* _currentUpgradeUI;
    ResourceDisplayUI* _resourceUI;

    std::string _currentMapName;
    std::vector<std::string> _mapNames;
    std::vector<BuildingData> _buildingList;
    std::unordered_map<std::string, MapConfig> _mapConfigs;

    struct MapElement {
        cocos2d::Node* node;
        cocos2d::Vec2 localPosition;
    };
    std::vector<MapElement> _mapElements;

    struct PlacedBuildingInfo {
        cocos2d::Size size;
        cocos2d::Vec2 gridPos;
        cocos2d::Node* node;
    };

    std::vector<PlacedBuildingInfo> _placedBuildings;

    void showBuildingHint(const std::string& hint);
    cocos2d::Vec2 calculateBuildingPosition(const cocos2d::Vec2& gridPos);

    void setupMap();
    void setupUI();
    void setupTouchListener();
    void setupMouseListener();
    void initBuildingData();
    void createBuildingSelection();
    void createMapList();
    void setupNetworkCallbacks();

    void setupResourceDisplay();

    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    void startPlacingBuilding(const BuildingData& building);
    void placeBuilding(cocos2d::Vec2 gridPos);
    void cancelPlacing();
    void endPlacing();

    void showConfirmButtons(const cocos2d::Vec2& buildingWorldPos);
    void hideConfirmButtons();
    void onConfirmBuilding();
    void onCancelBuilding();

    void toggleBuildingSelection();
    void onBuildingItemClicked(cocos2d::Ref* sender, const BuildingData& building);
    void toggleMapList();
    void onMapButtonClicked(cocos2d::Ref* sender);
    void onMapItemClicked(cocos2d::Ref* sender);
    void switchMap(const std::string& mapName);

    void moveMap(const cocos2d::Vec2& delta);
    void ensureMapInBoundary();
    void zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint = cocos2d::Vec2::ZERO);
    void updateBoundary();

    void saveMapElementsState();
    void restoreMapElementsState();
    void createSampleMapElements();
    void updateMapElementsPosition();

    // 大本营点击处理
    void onTownHallClicked(TownHallBuilding* townHall);

    bool getClosestAdjacentFreeCell(const PlacedBuildingInfo& bld, const cocos2d::Vec2& fromGrid, cocos2d::Vec2& outTargetGrid) const;
    void commandSelectedHeroAttackNearest();

    cocos2d::ui::Button* _battleButton;
    cocos2d::ui::Button* _clanButton;

    void update(float dt);

    void onBattleButtonClicked(cocos2d::Ref* sender);
    void onClanButtonClicked(cocos2d::Ref* sender);
    void connectToServer();
};

#endif