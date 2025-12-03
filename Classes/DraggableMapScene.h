#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "GridMap.h"
#include "HeroManager.h"
#include "BuildingData.h"
#include <unordered_map>
#include <string>
#include <vector>

class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;

    DraggableMapScene() = default;
    ~DraggableMapScene() = default;

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

    cocos2d::ui::Button* _buildButton;
    cocos2d::ui::Button* _mapButton;
    cocos2d::ui::ListView* _buildingListUI;
    cocos2d::ui::ListView* _mapList;
    bool _isBuildingListVisible;
    bool _isMapListVisible;

    HeroManager* _heroManager;

    std::string _currentMapName;
    std::vector<std::string> _mapNames;
    std::vector<BuildingData> _buildingList;
    std::unordered_map<std::string, MapConfig> _mapConfigs;

    struct MapElement {
        cocos2d::Node* node;
        cocos2d::Vec2 localPosition;
    };
    std::vector<MapElement> _mapElements;

    // --- Combat/Pathing ---
    struct PlacedBuildingInfo {
        cocos2d::Size size;       // grid size
        cocos2d::Vec2 gridPos;    // top-left grid
        cocos2d::Node* node;      // visual node
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

    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    void startPlacingBuilding(const BuildingData& building);
    void placeBuilding(cocos2d::Vec2 gridPos);
    void cancelPlacing();
    void endPlacing();

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

    // Compute the closest walkable grid cell adjacent to a placed building
    bool getClosestAdjacentFreeCell(const PlacedBuildingInfo& bld, const cocos2d::Vec2& fromGrid, cocos2d::Vec2& outTargetGrid) const;

    // Order the selected hero to attack nearest building by pathfinding
    void commandSelectedHeroAttackNearest();
};

#endif