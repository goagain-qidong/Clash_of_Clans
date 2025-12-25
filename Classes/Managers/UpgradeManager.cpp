/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     UpgradeManager.cpp
 * File Function: 建筑升级管理器实现
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "UpgradeManager.h"
#include "ResourceManager.h"
#include "Buildings/BaseBuilding.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

UpgradeManager* UpgradeManager::_instance = nullptr;

UpgradeManager* UpgradeManager::getInstance()
{
    if (!_instance)
    {
        _instance = new UpgradeManager();
    }
    return _instance;
}

void UpgradeManager::destroyInstance()
{
    if (_instance)
    {
        delete _instance;
        _instance = nullptr;
    }
}

UpgradeManager::UpgradeManager()
{
}

UpgradeManager::~UpgradeManager()
{
    // 清理所有升级任务
    _upgradeTasks.clear();
}

bool UpgradeManager::init()
{
    if (!Node::init())
        return false;
    
    scheduleUpdate();
    
    CCLOG("✅ UpgradeManager 初始化成功");
    return true;
}

bool UpgradeManager::canStartUpgrade(BaseBuilding* building, bool needBuilder)
{
    if (!building)
        return false;
    
    // 检查是否已在升级
    if (isUpgrading(building))
        return false;
    
    // ✅ 检查工人数量
    if (needBuilder && !_cheatModeEnabled)
    {
        int availableBuilders = getAvailableBuilders();
        if (availableBuilders <= 0)
        {
            CCLOG("❌ 无空闲工人！当前空闲：%d", availableBuilders);
            return false;
        }
    }
    
    return true;
}

bool UpgradeManager::startUpgrade(BaseBuilding* building, int cost, float time, bool needBuilder)
{
    if (!building)
    {
        CCLOG("❌ startUpgrade 失败：建筑指针为空");
        return false;
    }
    
    if (isUpgrading(building))
    {
        CCLOG("❌ %s 已在升级队列中", building->getDisplayName().c_str());
        return false;
    }
    
    // 🎮 作弊模式：升级时间为0
    if (_cheatModeEnabled)
    {
        time = 0.0f;
    }
    
    // ✅ 尝试分配工人（只在需要且非作弊模式时）
    if (needBuilder && !_cheatModeEnabled)
    {
        if (!allocateBuilder())
        {
            CCLOG("❌ 无法分配工人，升级失败");
            return false;
        }
    }
    
    // 创建升级任务
    UpgradeTask task(building, time, cost, needBuilder);
    _upgradeTasks.push_back(task);
    
    // 标记建筑为升级中
    building->setUpgrading(true);
    
    std::string displayName = building->getDisplayName();
    size_t lvPos = displayName.find(" Lv.");
    std::string buildingName = (lvPos != std::string::npos) 
        ? displayName.substr(0, lvPos) 
        : displayName;
    
    // ✅ 显示当前工人状态
    int availableBuilders = getAvailableBuilders();
    CCLOG("🔨 开始升级：%s Lv.%d → Lv.%d（升级时间：%.1f 秒，费用：%d，分配工人：%s，剩余工人：%d）", 
          buildingName.c_str(),
          building->getLevel(),
          building->getLevel() + 1,
          time, 
          cost,
          needBuilder ? "已分配" : "无需",
          availableBuilders);
    
    // ✅ 触发工人数量更新回调
    if (_onAvailableBuildersChanged)
    {
        _onAvailableBuildersChanged(availableBuilders);
    }
    
    return true;
}

bool UpgradeManager::cancelUpgrade(BaseBuilding* building)
{
    if (!building)
        return false;
    
    auto it = std::find_if(_upgradeTasks.begin(), _upgradeTasks.end(),
                           [building](const UpgradeTask& task) {
                               return task.building == building;
                           });
    
    if (it == _upgradeTasks.end())
    {
        CCLOG("❌ 未找到 %s 的升级任务", building->getDisplayName().c_str());
        return false;
    }
    
    // 退还部分资源（50%）
    int refund = it->cost / 2;
    ResourceManager::getInstance().addResource(building->getUpgradeCostType(), refund);
    
    // 释放工人
    if (it->useBuilder)
    {
        releaseBuilder();
    }
    
    // 移除任务
    building->setUpgrading(false);
    _upgradeTasks.erase(it);
    
    CCLOG("❌ 取消升级：%s，退还 %d 资源，释放工人", 
          building->getDisplayName().c_str(), refund);
    
    // ✅ 触发工人数量更新回调
    int availableBuilders = getAvailableBuilders();
    if (_onAvailableBuildersChanged)
    {
        _onAvailableBuildersChanged(availableBuilders);
    }
    
    return true;
}

bool UpgradeManager::finishUpgradeNow(BaseBuilding* building)
{
    if (!building)
        return false;
    
    auto it = std::find_if(_upgradeTasks.begin(), _upgradeTasks.end(),
                           [building](const UpgradeTask& task) {
                               return task.building == building;
                           });
    
    if (it == _upgradeTasks.end())
        return false;
    
    completeUpgrade(*it);
    _upgradeTasks.erase(it);
    
    return true;
}

bool UpgradeManager::isUpgrading(BaseBuilding* building) const
{
    if (!building)
        return false;
    
    for (const auto& task : _upgradeTasks)
    {
        if (task.building == building)
            return true;
    }
    
    return false;
}

UpgradeTask* UpgradeManager::getUpgradeTask(BaseBuilding* building) const
{
    if (!building)
        return nullptr;
    
    for (auto& task : const_cast<std::vector<UpgradeTask>&>(_upgradeTasks))
    {
        if (task.building == building)
            return &task;
    }
    
    return nullptr;
}

// ✅ 获取空闲工人数量
int UpgradeManager::getAvailableBuilders() const
{
    auto& resMgr = ResourceManager::getInstance();
    int totalBuilders = resMgr.getResourceCount(kBuilder);
    int usedBuilders = 0;
    
    // 计算正在使用的工人数（只计算需要工人的升级任务）
    for (const auto& task : _upgradeTasks)
    {
        if (task.useBuilder)
        {
            usedBuilders++;
        }
    }
    
    int available = totalBuilders - usedBuilders;
    
    return std::max(0, available);  // ✅ 确保不返回负数
}

// ✅ 分配工人：检查是否有空闲工人
bool UpgradeManager::allocateBuilder()
{
    int available = getAvailableBuilders();
    if (available <= 0)
    {
        CCLOG("❌ 无空闲工人可分配（总工人数：%d，使用中：%d）",
              ResourceManager::getInstance().getResourceCount(kBuilder),
              _upgradeTasks.size());
        return false;
    }
    
    CCLOG("✅ 分配工人成功（剩余空闲：%d）", available - 1);
    return true;
}

// ✅ 释放工人：升级完成时调用
void UpgradeManager::releaseBuilder()
{
    CCLOG("✅ 释放工人（当前空闲工人数将增加）");
}

void UpgradeManager::completeUpgrade(UpgradeTask& task)
{
    if (!task.building) return;

    // 🔴 关键修复1：不要调用 upgrade() (那是开始升级)，要调用 onUpgradeComplete() (这是结算升级)
    task.building->onUpgradeComplete();

    // 标记结束
    task.building->setUpgrading(false);

    // 此时不要急着通知工人变化，因为任务还在队列里，getAvailableBuilders() 算出来还是少的。
    // 我们在 update() 里移除任务后再通知。

    // 日志
    std::string displayName = task.building->getDisplayName();
    CCLOG("✅ 升级完成：%s", displayName.c_str());
}

void UpgradeManager::update(float dt)
{
    if (_upgradeTasks.empty()) return;

    auto it = _upgradeTasks.begin();
    while (it != _upgradeTasks.end())
    {
        it->elapsedTime += dt;

        // 检查是否完成
        if (it->elapsedTime >= it->totalTime)
        {
            // 1. 完成结算（等级+1，改变外观）
            completeUpgrade(*it);

            // 2. 🔴 关键修复2：先从队列移除任务
            it = _upgradeTasks.erase(it);

            // 3. 🔴 关键修复3：任务移除后，空闲工人数才变对，此时再通知 UI 刷新
            if (_onAvailableBuildersChanged)
            {
                _onAvailableBuildersChanged(getAvailableBuilders());
            }
        }
        else
        {
            ++it;
        }
    }
}

void UpgradeManager::clearAllUpgradeTasks()
{
    CCLOG("[UpgradeManager] Clearing all upgrade tasks (%d tasks)", (int)_upgradeTasks.size());
    _upgradeTasks.clear();
    
    // 通知UI更新工人数量
    if (_onAvailableBuildersChanged)
    {
        _onAvailableBuildersChanged(getAvailableBuilders());
    }
}

std::vector<UpgradeTaskData> UpgradeManager::serializeUpgradeTasks() const
{
    std::vector<UpgradeTaskData> result;
    
    for (const auto& task : _upgradeTasks)
    {
        if (!task.building)
            continue;
            
        UpgradeTaskData data;
        data.buildingName = task.building->getDisplayName();
        data.gridX = task.building->getGridPosition().x;
        data.gridY = task.building->getGridPosition().y;
        data.totalTime = task.totalTime;
        data.elapsedTime = task.elapsedTime;
        data.cost = task.cost;
        data.useBuilder = task.useBuilder;
        
        result.push_back(data);
        
        CCLOG("[UpgradeManager] Serialized upgrade task: %s at (%.0f, %.0f), progress: %.1f%%",
              data.buildingName.c_str(), data.gridX, data.gridY,
              (data.elapsedTime / data.totalTime) * 100.0f);
    }
    
    return result;
}

void UpgradeManager::restoreUpgradeTasks(const std::vector<UpgradeTaskData>& tasksData,
                                          const cocos2d::Vector<BaseBuilding*>& buildings)
{
    // 先清除旧任务
    _upgradeTasks.clear();
    
    for (const auto& data : tasksData)
    {
        // 根据名称和位置找到对应的建筑
        BaseBuilding* matchedBuilding = nullptr;
        
        for (auto* building : buildings)
        {
            if (!building)
                continue;
                
            // 匹配位置（允许小误差）
            auto gridPos = building->getGridPosition();
            if (std::abs(gridPos.x - data.gridX) < 0.5f &&
                std::abs(gridPos.y - data.gridY) < 0.5f)
            {
                matchedBuilding = building;
                break;
            }
        }
        
        if (matchedBuilding)
        {
            // 恢复升级任务
            UpgradeTask task(matchedBuilding, data.totalTime, data.cost, data.useBuilder);
            task.elapsedTime = data.elapsedTime;
            _upgradeTasks.push_back(task);
            
            // 标记建筑为升级中
            matchedBuilding->setUpgrading(true);
            
            CCLOG("[UpgradeManager] Restored upgrade task: %s, remaining: %.1fs",
                  matchedBuilding->getDisplayName().c_str(),
                  task.getRemainingTime());
        }
        else
        {
            CCLOG("[UpgradeManager] Warning: Could not find building for upgrade task at (%.0f, %.0f)",
                  data.gridX, data.gridY);
        }
    }
    
    // 通知UI更新工人数量
    if (_onAvailableBuildersChanged)
    {
        _onAvailableBuildersChanged(getAvailableBuilders());
    }
}