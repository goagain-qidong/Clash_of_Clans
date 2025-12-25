/**
 * @file ResourceCollectionManager.cpp
 * @brief 资源收集管理器实现
 */
 /****************************************************************
    * Project Name:  Clash_of_Clans
    * File Name:     WallBuilding.cpp
    * File Function: 资源收集管理器类
    * Author:        刘相成
    * Update Date:   2025/12/09
    * License:       MIT License
    ****************************************************************/
#include "ResourceCollectionManager.h"
#include "../Buildings/ResourceBuilding.h"
#include "../UI/ResourceCollectionUI.h"

USING_NS_CC;



bool ResourceCollectionManager::init()
{
    if (!Node::init())
        return false;
    // ⬇️⬇️⬇️ 关键修复：添加空的触摸监听器并吞噬触摸 ⬇️⬇️⬇️
    auto listener = cocos2d::EventListenerTouchOneByOne::create();
    // 优先级设置为 1，确保它比地图拖拽（通常优先级较低）先收到事件。
    // 如果地图拖拽监听器优先级是 1，这里应该设为 0 或更低的值。
    // 我们在此演示一个高优先级的设置 (Priority = 1)
    listener->setSwallowTouches(false); // 不吞噬，确保事件能继续传递给地图

    listener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event* event) {
        // 只有当点击了可收集的资源时才返回 true，并处理
        if (this->handleTouch(touch->getLocation())) {
            // 如果成功收集，返回 true，但我们不希望事件被吞噬，让它继续传递给 onTouchEnded
        }
        return true; // 总是返回 true，确保 onTouchEnded 能够触发
        };

    listener->onTouchEnded = [this](cocos2d::Touch* touch, cocos2d::Event* event) {
        // 实际的收集逻辑应在 onTouchEnded 中完成
        // 我们在 GameScene::onTouchEnded 中处理了，这里只是为了确保 onTouchBegan 被触发。
        };

    // 绑定监听器，使用 SceneGraphPriority
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    // ⬆️⬆️⬆️ 关键修复结束 ⬆️⬆️⬆️
    _trackedBuildings.clear();
    return true;
}

void ResourceCollectionManager::registerBuilding(ResourceBuilding* building)
{
    if (!building)
        return;
    
    // 检查是否已注册
    for (auto* b : _trackedBuildings)
    {
        if (b == building)
            return;
    }
    
    _trackedBuildings.push_back(building);
    CCLOG("✅ 注册资源建筑收集：%s", building->getDisplayName().c_str());
}

// ✅ 新增：注销资源建筑
void ResourceCollectionManager::unregisterBuilding(ResourceBuilding* building)
{
    if (!building)
        return;
    
    auto it = std::find(_trackedBuildings.begin(), _trackedBuildings.end(), building);
    if (it != _trackedBuildings.end())
    {
        _trackedBuildings.erase(it);
        CCLOG("🗑️ 注销资源建筑收集：%s", building->getDisplayName().c_str());
    }
}

bool ResourceCollectionManager::handleTouch(const cocos2d::Vec2& touchPos)
{
    // 遍历所有注册的建筑，查看是否有可收集的资源被点击
    for (auto it = _trackedBuildings.begin(); it != _trackedBuildings.end(); )
    {
        auto* building = *it;
        
        // 安全检查：验证建筑指针是否有效
        if (!building || 
            building->getReferenceCount() <= 0 || 
            building->getReferenceCount() > 10000)
        {
            CCLOG("[ResourceCollectionManager] Removing invalid building pointer");
            it = _trackedBuildings.erase(it);
            continue;
        }
        
        if (!building->getParent() || !building->isVisible())
        {
            ++it;
            continue;
        }
        
        auto collectionUI = getCollectionUI(building);
        if (!collectionUI)
        {
            ++it;
            continue;
        }
        
        // 检查触摸是否在收集区域内
        if (collectionUI->checkTouchInside(touchPos))
        {
            // 执行收集
            collectionUI->performCollection();
            return true;
        }
        
        ++it;
    }
    
    return false;
}

ResourceCollectionUI* ResourceCollectionManager::getCollectionUI(ResourceBuilding* building)
{
    if (!building)
        return nullptr;
    
    return building->getChildByName<ResourceCollectionUI*>("collectionUI");
}

void ResourceCollectionManager::clear()
{
    _trackedBuildings.clear();
}
ResourceCollectionManager* ResourceCollectionManager::_instance = nullptr;

ResourceCollectionManager* ResourceCollectionManager::getInstance()
{
    if (!_instance)
    {
        // 改造为单例创建模式
        _instance = new (std::nothrow) ResourceCollectionManager();
        if (_instance && _instance->init())
        {
            _instance->autorelease();
            _instance->retain(); // 确保它不会被自动释放（Node单例的常见做法）
            // ⚠️ 警告：作为 Node 的单例，你需要确保它在场景中被 addChild 一次，否则它的触摸监听可能不工作
        }
        else
        {
            CC_SAFE_DELETE(_instance);
        }
    }
    return _instance;
}
// 🔴 关键修改：构造函数私有化
ResourceCollectionManager::ResourceCollectionManager()
{
    // 构造函数逻辑
}
void ResourceCollectionManager::clearRegisteredBuildings()
{
    // 清空内部存储的建筑列表
    _trackedBuildings.clear();
    CCLOG("🗑️ ResourceCollectionManager 成功清空所有已注册的资源建筑。");
}