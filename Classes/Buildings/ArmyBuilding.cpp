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
#include <exception>
#include <stdexcept>

USING_NS_CC;

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
    try {
        if (!initWithType(BuildingType::kArmy, level))
            return false;

        this->setAnchorPoint(Vec2(0.5f, 0.35f));
        initHealthBarUI();
        
        return true;
    }
    catch (const std::exception& e) {
        CCLOG("❌ ArmyBuilding::init 异常: %s", e.what());
        return false;
    }
}

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
}

bool ArmyBuilding::addTrainingTask(UnitType unitType)
{
    try {
        if (getQueueLength() >= getTrainingCapacity())
            return false;

        auto& resMgr = ResourceManager::getInstance();
        int population = getUnitPopulation(unitType);

        if (!resMgr.hasTroopSpace(population))
            return false;

        int cost = getUnitTrainingCost(unitType);
        float baseTime = getUnitBaseTrainingTime(unitType);
        float actualTime = baseTime / (1.0f + getTrainingSpeedBonus());

        if (!resMgr.consume(ResourceType::kElixir, cost))
            return false;

        _trainingQueue.push(TrainingTask(unitType, actualTime, cost));
        return true;
    }
    catch (const std::exception& e) {
        CCLOG("❌ ArmyBuilding::addTrainingTask 异常: %s", e.what());
        return false;
    }
}

void ArmyBuilding::cancelCurrentTask()
{
    if (_trainingQueue.empty())
        return;

    // 退还部分资源（50%）
    auto& task = _trainingQueue.front();
    int refund = task.cost / 2;
    ResourceManager::getInstance().addResource(ResourceType::kElixir, refund);
    _trainingQueue.pop();
}

void ArmyBuilding::clearTrainingQueue()
{
    int totalRefund = 0;
    while (!_trainingQueue.empty())
    {
        totalRefund += _trainingQueue.front().cost / 2;
        _trainingQueue.pop();
    }

    if (totalRefund > 0)
    {
        ResourceManager::getInstance().addResource(ResourceType::kElixir, totalRefund);
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
    try {
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
    catch (const std::exception& e) {
        CCLOG("❌ ArmyBuilding::tick 异常: %s", e.what());
    }
}

void ArmyBuilding::completeCurrentTask()
{
    try {
        if (_trainingQueue.empty())
            return;

        auto task = _trainingQueue.front();
        _trainingQueue.pop();

        std::string unitName = UnitFactory::getUnitName(task.unitType);
        auto& troopInv = TroopInventory::getInstance();
        int addedCount = troopInv.addTroops(task.unitType, 1);

        if (addedCount > 0)
        {
            notifyArmyCampsToDisplayTroop(task.unitType);
            if (_onTrainingComplete)
            {
                _onTrainingComplete(nullptr);
            }
        }
        else
        {
            // 人口已满，退还资源
            ResourceManager::getInstance().addResource(ResourceType::kElixir, task.cost);
        }
    }
    catch (const std::exception& e) {
        CCLOG("❌ ArmyBuilding::completeCurrentTask 异常: %s", e.what());
    }
}

float ArmyBuilding::getUnitBaseTrainingTime(UnitType type)
{
    switch (type)
    {
    case UnitType::kBarbarian: return 1.0f;
    case UnitType::kArcher:    return 1.0f;
    case UnitType::kGoblin:    return 1.0f;
    case UnitType::kGiant:     return 1.0f;
    case UnitType::kWallBreaker: return 1.0f;
    default: return 1.0f;
    }
}

int ArmyBuilding::getUnitTrainingCost(UnitType type)
{
    switch (type)
    {
    case UnitType::kBarbarian: return 25;
    case UnitType::kArcher:    return 50;
    case UnitType::kGoblin:    return 40;
    case UnitType::kGiant:     return 250;
    case UnitType::kWallBreaker: return 600;
    default: return 50;
    }
}

int ArmyBuilding::getUnitPopulation(UnitType type)
{
    switch (type)
    {
    case UnitType::kBarbarian: return 1;
    case UnitType::kArcher:    return 1;
    case UnitType::kGoblin:    return 1;
    case UnitType::kGiant:     return 5;
    case UnitType::kWallBreaker: return 2;
    default: return 1;
    }
}

void ArmyBuilding::notifyArmyCampsToDisplayTroop(UnitType type)
{
    auto parent = this->getParent();
    if (!parent)
        return;

    // 遍历父节点的所有子节点，查找军营
    auto& children = parent->getChildren();
    for (auto child : children)
    {
        auto* armyCamp = dynamic_cast<ArmyCampBuilding*>(child);
        if (armyCamp)
        {
            armyCamp->addTroopDisplay(type);
            break;
        }
    }
}