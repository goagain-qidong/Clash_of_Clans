#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "GridMap.h" // <--- 1. 引入头文件
#include "HeroManager.h"

class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(DraggableMapScene);
    void showWholeGrid(bool visible);

private:
    cocos2d::Size _mapSize;
    float _tileSize;
    cocos2d::DrawNode* _gridNode; // 用于画全屏淡网格
    cocos2d::DrawNode* _baseNode; // 用于画当前鼠标下的绿色底座

    GridMap* _gridMap; // <--- 2. 新增网格对象指针

    // --- 建造模式相关 ---
    bool _isBuildingMode;           // 是否处于建造状态
    cocos2d::Sprite* _ghostSprite;  // 跟随鼠标的半透明建筑

    // 开始建造（点击UI按钮触发）
    void startPlacingBuilding();
    // 确认建造（点击地图触发）
    void placeBuilding(cocos2d::Vec2 gridPos);
    // 取消建造
    void cancelPlacing();
    // 结束建造（统一的退出建造模式接口）
    void endPlacing();

    cocos2d::Vec2 _lastTouchPos;
    cocos2d::Sprite* _mapSprite;
    cocos2d::Vec2 _velocity;
    cocos2d::Rect _mapBoundary;
    cocos2d::Size _visibleSize;

    // 缩放相关变量
    float _currentScale;
    float _minScale;
    float _maxScale;

    // 地图相关
    std::string _currentMapName;
    std::vector<std::string> _mapNames;

    // UI元素
    cocos2d::ui::Button* _mapButton;
    cocos2d::ui::ListView* _mapList;
    bool _isMapListVisible;

    // 英雄管理器
    HeroManager* _heroManager;

    // 地图元素现在存储相对于地图的本地坐标
    struct MapElement {
        cocos2d::Node* node;
        cocos2d::Vec2 localPosition;  // 相对于地图的本地坐标
    };
    std::vector<MapElement> _mapElements;

    void setupMap();
    void setupUI();
    void setupTouchListener();
    void setupMouseListener();

    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    void onMouseScroll(cocos2d::Event* event);

    void moveMap(const cocos2d::Vec2& delta);
    void ensureMapInBoundary();

    void zoomMap(float scaleFactor, const cocos2d::Vec2& pivotPoint = cocos2d::Vec2::ZERO);
    void updateBoundary();

    // 地图切换功能
    void switchMap(const std::string& mapName);
    void onMapButtonClicked(cocos2d::Ref* sender);
    void createMapList();
    void toggleMapList();
    void onMapItemClicked(cocos2d::Ref* sender);

    // 地图元素管理
    void saveMapElementsState();
    void restoreMapElementsState();
    void createSampleMapElements();
    void updateMapElementsPosition();
};

#endif