/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceCollectionUI.h
 * File Function: 资源收集UI（显示可收集的资源图标和数量）
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#ifndef RESOURCE_COLLECTION_UI_H_
#define RESOURCE_COLLECTION_UI_H_

#include "cocos2d.h"
#include "../Managers/ResourceManager.h"

class ResourceBuilding;

/**
 * @class ResourceCollectionUI
 * @brief 资源收集UI组件
 */
class ResourceCollectionUI : public cocos2d::Node
{
public:
    /**
     * @brief 创建资源收集UI
     * @param building 关联的资源建筑
     * @return ResourceCollectionUI* UI指针
     */
    static ResourceCollectionUI* create(ResourceBuilding* building);

    /**
     * @brief 初始化
     * @param building 关联的资源建筑
     * @return bool 是否成功
     */
    virtual bool init(ResourceBuilding* building);

    /**
     * @brief 更新可收集状态
     * @param amount 可收集数量
     */
    void updateReadyStatus(int amount);

    /**
     * @brief 播放收集反馈动画
     * @param amount 收集数量
     */
    void playCollectionAnimation(int amount);

    /**
     * @brief 检查触摸点是否在收集区域
     * @param touchPos 触摸位置
     * @return bool 是否在区域内
     */
    bool checkTouchInside(const cocos2d::Vec2& touchPos);

    /** @brief 执行收集逻辑 */
    void performCollection();

    /** @brief 是否可以被点击 */
    bool isClickable() const { return _isReadyToCollect; }

private:
    ResourceBuilding* _building = nullptr;     ///< 关联的建筑
    cocos2d::Node* _iconContainer = nullptr;   ///< 图标容器
    cocos2d::Sprite* _resourceIcon = nullptr;  ///< 资源图标
    cocos2d::Label* _amountLabel = nullptr;    ///< 资源数量标签

    bool _isReadyToCollect = false;  ///< 是否可收集
};

#endif // RESOURCE_COLLECTION_UI_H_