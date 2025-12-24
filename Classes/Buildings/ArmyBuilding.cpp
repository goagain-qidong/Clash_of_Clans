/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.cpp
 * File Function: 军事建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ArmyBuilding.h"

#include "ArmyCampBuilding.h"
#include "GameConfig.h"
#include "Managers/TroopInventory.h"
#include "Unit/UnitFactory.h"

USING_NS_CC;

// ==================== 创建与初始化 ====================

ArmyBuilding* ArmyBuilding::create(int level)
{
    ArmyBuilding* building = new (std::nothrow) ArmyBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool ArmyBuilding::init(int level)
{
    // 使用 initWithType 统一初始化，配置数据由基类管理
    if (!initWithType(BuildingType::kArmy, level))
    {
        return false;
    }

    // 兵营特有的外观设置
    this->setAnchorPoint(Vec2(0.5f, 0.35f));

    // 初始化血条UI
    initHealthBarUI();

    CCLOG("⚔️ %s 初始化 HP: %d", getDisplayName().c_str(), getMaxHitpoints());
    return true;
}

// ==================== 训练系统属性 ====================

int ArmyBuilding::getTrainingCapacity() const
{
    // 每级增加训练容量
    return 20 + (_level - 1) * 5;
}

float ArmyBuilding::getTrainingSpeedBonus() const
{
    // 每级增加5%训练速度
    return (_level - 1) * 0.05f;
}

void ArmyBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    CCLOG("ArmyBuilding upgraded to level %d, capacity: %d", _level, getTrainingCapacity());
}

// ==================== 训练系统实现 ====================

bool ArmyBuilding::addTrainingTask(UnitType unitType)
{
    // 检查队列是否已满
    if (getQueueLength() >= getTrainingCapacity())
    {
        CCLOG("训练队列已满！容量：%d", getTrainingCapacity());
        return false;
    }

    // 🔧 修复：检查人口空间（根据兵种类型）
    auto& resMgr     = ResourceManager::getInstance();
    int   population = getUnitPopulation(unitType); // ✅ 获取正确的人口数

    if (!resMgr.hasTroopSpace(population))
    {
        CCLOG("人口不足！需要 %d 人口，当前：%d/%d", population, resMgr.getCurrentTroopCount(),
              resMgr.getMaxTroopCapacity());
        return false;
    }

    // 获取训练费用和时间
    int   cost     = getUnitTrainingCost(unitType);
    float baseTime = getUnitBaseTrainingTime(unitType);

    // 应用训练速度加成
    float actualTime = baseTime / (1.0f + getTrainingSpeedBonus());

    // 检查资源是否足够
    if (!resMgr.consume(ResourceType::kElixir, cost))
    {
        CCLOG("圣水不足！需要 %d 圣水", cost);
        return false;
    }

    // 添加到训练队列
    _trainingQueue.push(TrainingTask(unitType, actualTime, cost));

    // 获取兵种名称
    std::string unitName;
    switch (unitType)
    {
    case UnitType::kBarbarian:
        unitName = "野蛮人";
        break;
    case UnitType::kArcher:
        unitName = "弓箭手";
        break;
    case UnitType::kGiant:
        unitName = "巨人";
        break;
    case UnitType::kGoblin:
        unitName = "哥布林";
        break;
    case UnitType::kWallBreaker:
        unitName = "炸弹人";
        break;
    default:
        unitName = "未知兵种";
        break;
    }

    CCLOG("✅ 开始训练 %s，预计 %.1f 秒完成（队列：%d/%d）", unitName.c_str(), actualTime, getQueueLength(),
          getTrainingCapacity());

    return true;
}

void ArmyBuilding::cancelCurrentTask()
{
    if (_trainingQueue.empty())
        return;

    // 退还部分资源（50%）
    auto& task   = _trainingQueue.front();
    int   refund = task.cost / 2;
    ResourceManager::getInstance().addResource(ResourceType::kElixir, refund);

    // 移除任务
    _trainingQueue.pop();

    CCLOG("❌ 取消训练，退还 %d 圣水", refund);
}

void ArmyBuilding::clearTrainingQueue()
{
    // 退还所有资源的50%
    int totalRefund = 0;
    while (!_trainingQueue.empty())
    {
        totalRefund += _trainingQueue.front().cost / 2;
        _trainingQueue.pop();
    }

    if (totalRefund > 0)
    {
        ResourceManager::getInstance().addResource(ResourceType::kElixir, totalRefund);
        CCLOG("❌ 清空训练队列，退还 %d 圣水", totalRefund);
    }
}

float ArmyBuilding::getTrainingProgress() const
{
    if (_trainingQueue.empty())
        return 0.0f;

    const auto& task = _trainingQueue.front();
    return task.elapsedTime / task.trainingTime;
}

void ArmyBuilding::tick(float dt)
{
    BaseBuilding::tick(dt);

    // 如果队列为空，不处理
    if (_trainingQueue.empty())
        return;

    // 更新当前训练任务
    auto& task = _trainingQueue.front();
    task.elapsedTime += dt;

    // 检查是否完成
    if (task.elapsedTime >= task.trainingTime)
    {
        completeCurrentTask();
    }
}

void ArmyBuilding::completeCurrentTask()
{
    if (_trainingQueue.empty())
        return;

    auto task = _trainingQueue.front();
    _trainingQueue.pop();

    // 🆕 使用工厂类创建单位
    std::string unitName = UnitFactory::getUnitName(task.unitType);

    // 添加士兵到库存
    auto& troopInv   = TroopInventory::getInstance();
    int   addedCount = troopInv.addTroops(task.unitType, 1);

    if (addedCount > 0)
    {
        auto& resMgr = ResourceManager::getInstance();
        CCLOG("🎉 训练完成：%s！（剩余队列：%d，人口：%d/%d）", unitName.c_str(), getQueueLength(),
              resMgr.getCurrentTroopCount(), resMgr.getMaxTroopCapacity());

        notifyArmyCampsToDisplayTroop(task.unitType);

        if (_onTrainingComplete)
        {
            _onTrainingComplete(nullptr);
        }
    }
    else
    {
        CCLOG("⚠️ 人口已满，无法完成训练：%s", unitName.c_str());
        // 退还资源
        ResourceManager::getInstance().addResource(ResourceType::kElixir, task.cost);
    }
}

// ==================== 静态方法：获取兵种数据 ====================

float ArmyBuilding::getUnitBaseTrainingTime(UnitType type)
{
    // 基础训练时间（秒）- 官方数据
    switch (type)
    {
    case UnitType::kBarbarian:
        return 1.0f; // 野蛮人
    case UnitType::kArcher:
        return 1.0f; // 弓箭手
    case UnitType::kGoblin:
        return 1.0f; // 哥布林
    case UnitType::kGiant:
        return 1.0f; // 巨人
    case UnitType::kWallBreaker:
        return 1.0f; // 炸弹人
    default:
        return 1.0f;
    }
}

int ArmyBuilding::getUnitTrainingCost(UnitType type)
{
    // 训练费用（圣水）
    switch (type)
    {
    case UnitType::kBarbarian:
        return 25; // 野蛮人：25圣水
    case UnitType::kArcher:
        return 50; // 弓箭手：50圣水
    case UnitType::kGoblin:
        return 40; // 哥布林：40圣水
    case UnitType::kGiant:
        return 250; // 巨人：250圣水
    case UnitType::kWallBreaker:
        return 600; // 炸弹人：600圣水
    default:
        return 50;
    }
}

int ArmyBuilding::getUnitPopulation(UnitType type)
{
    // 兵种占用人口数
    switch (type)
    {
    case UnitType::kBarbarian:
        return 1; // 野蛮人：1人口
    case UnitType::kArcher:
        return 1; // 弓箭手：1人口
    case UnitType::kGoblin:
        return 1; // 哥布林：1人口
    case UnitType::kGiant:
        return 5; // 巨人：5人口 ✅ 修复
    case UnitType::kWallBreaker:
        return 2; // 炸弹人：2人口
    default:
        return 1;
    }
}

// ==================== 🆕 通知军营显示小兵 ====================

void ArmyBuilding::notifyArmyCampsToDisplayTroop(UnitType type)
{
    // 🔍 查找场景中的所有军营建筑
    // 注意：这需要访问 BuildingManager 或场景
    // 由于架构限制，这里暂时使用简化方案：
    // 通过父节点查找兄弟节点（同样是建筑）

    auto parent = this->getParent();
    if (!parent)
    {
        CCLOG("⚠️ ArmyBuilding: No parent node, cannot notify ArmyCamps");
        return;
    }

    // 遍历父节点的所有子节点，查找军营
    auto& children = parent->getChildren();
    bool  found    = false;

    for (auto child : children)
    {
        // 尝试转换为 ArmyCampBuilding
        auto* armyCamp = dynamic_cast<ArmyCampBuilding*>(child);
        if (armyCamp)
        {
            // 找到军营，添加小兵显示
            armyCamp->addTroopDisplay(type);
            found = true;
            CCLOG("✅ Notified ArmyCamp to display troop");
            break; // 只通知第一个军营（简化处理）
        }
    }

    if (!found)
    {
        CCLOG("⚠️ No ArmyCamp found to display troop");
    }
}