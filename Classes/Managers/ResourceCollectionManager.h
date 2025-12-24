/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceCollectionManager.h
 * File Function: 资源收集管理器（处理资源建筑的收集交互）
 * Author:        刘相成
 * Update Date:   2025/12/09
 * License:       MIT License
 ****************************************************************/
#ifndef RESOURCE_COLLECTION_MANAGER_H_
#define RESOURCE_COLLECTION_MANAGER_H_

#include "cocos2d.h"
#include "ResourceManager.h"

#include <vector>

class ResourceBuilding;
class ResourceCollectionUI;

/**
 * @class ResourceCollectionManager
 * @brief 管理资源建筑的收集交互（单例）
 */
class ResourceCollectionManager : public cocos2d::Node
{
public:
    /**
     * @brief 获取单例实例
     * @return ResourceCollectionManager* 单例指针
     */
    static ResourceCollectionManager* getInstance();

    virtual bool init() override;

    /** @brief 清空已注册的建筑 */
    void clearRegisteredBuildings();

    /**
     * @brief 注册资源建筑
     * @param building 资源建筑
     */
    void registerBuilding(ResourceBuilding* building);

    /**
     * @brief 注销资源建筑
     * @param building 资源建筑
     */
    void unregisterBuilding(ResourceBuilding* building);

    /**
     * @brief 处理触摸事件
     * @param touchPos 触摸位置
     * @return bool 是否处理了触摸
     */
    bool handleTouch(const cocos2d::Vec2& touchPos);

    /** @brief 清空所有注册的建筑 */
    void clear();

private:
    ResourceCollectionManager();

    static ResourceCollectionManager* _instance;  ///< 单例实例
    std::vector<ResourceBuilding*> _trackedBuildings;  ///< 跟踪的建筑列表

    /**
     * @brief 获取建筑对应的收集UI
     * @param building 资源建筑
     * @return ResourceCollectionUI* 收集UI
     */
    ResourceCollectionUI* getCollectionUI(ResourceBuilding* building);
};

#endif // RESOURCE_COLLECTION_MANAGER_H_