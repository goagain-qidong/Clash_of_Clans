#ifndef __DRAGGABLE_MAP_SCENE_H__
#define __DRAGGABLE_MAP_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(DraggableMapScene);

private:
    cocos2d::Sprite* _mapSprite;
    cocos2d::Vec2 _lastTouchPos;
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

    // 修改：地图元素现在存储相对于地图的本地坐标
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
    void updateMapElementsPosition();  // 新增：更新元素位置
};

#endif