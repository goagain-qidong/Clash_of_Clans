/**
 * @file ResourceCollectionManager.h
 * @brief 资源收集管理器（处理资源建筑的收集交互）
 */
 /****************************************************************
    * Project Name:  Clash_of_Clans
    * File Name:     WallBuilding.cpp
    * File Function: 资源收集管理器类
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
 * @brief 管理资源建筑的收集交互（点击收集）
 */
class ResourceCollectionManager : public cocos2d::Node
{
public:
    void clearRegisteredBuildings();
    static ResourceCollectionManager* getInstance();
    virtual bool init() override;
    
    // 注册资源建筑以进行收集管理
    void registerBuilding(ResourceBuilding* building);
    
    // 处理触摸事件（在场景的触摸回调中调用）
    bool handleTouch(const cocos2d::Vec2& touchPos);
    
    // 清空所有注册的建筑
    void clear();
    
private:
    std::vector<ResourceBuilding*> _trackedBuildings;
    ResourceCollectionManager(); // 私有化构造函数
    static ResourceCollectionManager* _instance; // 静态单例指针

   
    // 获取建筑对应的收集UI
    ResourceCollectionUI* getCollectionUI(ResourceBuilding* building);
};

#endif // RESOURCE_COLLECTION_MANAGER_H_