// 2453619 薛毓哲





#include "ResourceManager.h"



// 静态变量初始化

static ResourceManager* s_instance = nullptr;



ResourceManager* ResourceManager::GetInstance() 

{

    if (s_instance == nullptr) //初始是nullptr，那么我们新建一个实例去访问资源数据，此后更改也将在这个实例上

    {

        s_instance = new ResourceManager();

        s_instance->Init();

    }

    return s_instance;

}



ResourceManager::ResourceManager() : on_resource_change_(nullptr) {

    // 构造函数

}





bool ResourceManager::Init() {

    // 初始化默认值

    resources_[ResourceType::kGold] = 1000;    // 初始金币

    resources_[ResourceType::kElixir] = 1000;  // 初始圣水

    resources_[ResourceType::kGem] = 500;      // 初始宝石

    resources_[ResourceType::kBuilder] = 2;    // 初始2个建筑工



    // 初始化容量 (默认容量，后续应由大本营/存储罐决定)

    capacities_[ResourceType::kGold] = 10000;

    capacities_[ResourceType::kElixir] = 10000;

    capacities_[ResourceType::kGem] = 999999;     // 宝石通常无上限

    capacities_[ResourceType::kBuilder] = 5;      // 建筑工上限



    return true;

}





int ResourceManager::AddResource(ResourceType type, int amount) {

    if (amount <= 0) 

        return 0;



    int current = resources_[type];

    int capacity = capacities_[type];



    // 计算实际能加多少 (不能超过上限)

    int actual_add = amount;

    if (current + amount > capacity) 

    {

        actual_add = capacity - current;

    }



    resources_[type] += actual_add;



    // 通知 UI 更新

    if (on_resource_change_) 

    {

        on_resource_change_(type, resources_[type]);

    }



    // 可以在控制台打印日志方便调试

    cocos2d::log("Resource Added: Type %d, Amount %d, Current %d",static_cast<int>(type), actual_add, resources_[type]);



    return actual_add;

}



bool ResourceManager::ConsumeResource(ResourceType type, int amount) {

    if (amount <= 0) 

        return true; // 消耗0总是成功的



    if (resources_[type] >= amount) 

    {

        resources_[type] -= amount;



        // 通知 UI 更新

        if (on_resource_change_) 

        {

            on_resource_change_(type, resources_[type]);

        }



        cocos2d::log("Resource Consumed: Type %d, Amount %d, Remaining %d",static_cast<int>(type), amount, resources_[type]);

        return true;

    }



    cocos2d::log("Not enough resource! Type %d, Need %d, Have %d",static_cast<int>(type), amount, resources_[type]);



    return false;

}



bool ResourceManager::HasEnough(ResourceType type, int amount) const {



    // map 的 find 方法查找 key，避免插入新键

    auto it = resources_.find(type);

    if (it != resources_.end()) 

    {

        return it->second >= amount;

    }

    return false;

}



int ResourceManager::GetResourceCount(ResourceType type) const {

    auto it = resources_.find(type);

    return (it != resources_.end()) ? it->second : 0;

}



int ResourceManager::GetResourceCapacity(ResourceType type) const {

    auto it = capacities_.find(type);

    return (it != resources_.end()) ? it->second : 0;

}



void ResourceManager::SetResourceCapacity(ResourceType type, int capacity) {

    capacities_[type] = capacity;

    // 如果当前资源超过了新上限（比如拆除了储金罐），需要截断吗？

    // 通常游戏逻辑是暂时保留溢出部分，但无法新增。这里简单处理为截断。

    if (resources_[type] > capacity) {

        resources_[type] = capacity;

        if (on_resource_change_) {

            on_resource_change_(type, resources_[type]);

        }

    }

}



void ResourceManager::SetOnResourceChangeCallback(ResourceChangeCallback callback) {

    on_resource_change_ = callback;

}