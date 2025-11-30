//2453619 薛毓哲

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "cocos2d.h"
#include <functional>
#include <map>

// 定义资源类型，使用 enum class (C++11)
enum class ResourceType {
    kGold,      // 金币
    kElixir,    // 圣水
    kGem,       // 宝石(就是充钱买的那个，将来肯定不设计充钱功能，如果这块要做就直接点击即送就行）
    kBuilder    // 建筑工(空闲工人数)
};

class ResourceManager {
public:
    // 获取单例实例
    static ResourceManager* GetInstance();

    // 初始化
    bool Init();

    // --- 核心操作 ---

    /**
     * 增加资源
     * @param type 资源类型
     * @param amount 增加的数量
     * @return 实际增加的数量 (因为可能会达到存储上限)
     */
    int AddResource(ResourceType type, int amount);

    /**
     * 消耗资源
     * @param type 资源类型
     * @param amount 消耗的数量
     * @return 如果成功扣除返回 true，余额不足返回 false
     */
    bool ConsumeResource(ResourceType type, int amount);

    // 检查资源是否足够
    bool HasEnough(ResourceType type, int amount) const;

    // 获取当前资源数量
    int GetResourceCount(ResourceType type) const;

    // 获取资源上限 (例如金库容量)
    int GetResourceCapacity(ResourceType type) const;

    // 设置资源上限 (当建造新的存储建筑时调用)
    void SetResourceCapacity(ResourceType type, int capacity);

    // --- 事件监听系统 (观察者模式) ---
    // 当资源变化时，UI需要知道。我们使用回调函数。
    using ResourceChangeCallback = std::function<void(ResourceType, int)>;
    void SetOnResourceChangeCallback(ResourceChangeCallback callback);

private:
    // 私有构造函数，确保单例
    ResourceManager();

    // 禁止拷贝和赋值
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // 存储当前资源数量
    std::map<ResourceType, int> resources_;

    // 存储资源上限
    std::map<ResourceType, int> capacities_;

    // 资源变化的回调函数
    ResourceChangeCallback on_resource_change_;
};

#endif // RESOURCE_MANAGER_H_