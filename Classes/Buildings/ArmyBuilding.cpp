/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.cpp
 * File Function: 军事建筑类实现
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#include "ArmyBuilding.h"
#include "GameConfig.h" // 如果需要引用配置
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
float ArmyBuilding::getUpgradeTime() const
{
    // 无论多少级，统一 10 秒
    return 10.0f;
}
ArmyBuilding* ArmyBuilding::create(int level, const std::string& imageFile)
{
    ArmyBuilding* building = new (std::nothrow) ArmyBuilding();
    if (building && building->init(level, imageFile))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool ArmyBuilding::init(int level)
{
    if (!BaseBuilding::init(level, getImageForLevel(level)))
    {
        return false;
    }
    return true;
}

bool ArmyBuilding::init(int level, const std::string& imageFile)
{
    if (!BaseBuilding::init(level, imageFile))
    {
        return false;
    }
    
    // 保存自定义图片路径模板
    // 例如: "buildings/ArcherTower/Archer_Tower1.png" -> "buildings/ArcherTower/Archer_Tower"
    _customImagePath = imageFile;
    
    // 移除文件名中的等级数字和扩展名
    size_t lastSlash = _customImagePath.find_last_of('/');
    size_t lastDot = _customImagePath.find_last_of('.');
    
    if (lastSlash != std::string::npos && lastDot != std::string::npos)
    {
        std::string fileName = _customImagePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
        
        // 移除末尾的数字（如 "Archer_Tower1" -> "Archer_Tower"）
        size_t i = fileName.length();
        while (i > 0 && std::isdigit(fileName[i - 1]))
        {
            i--;
        }
        
        if (i < fileName.length())
        {
            fileName = fileName.substr(0, i);
        }
        
        // 重新组合路径模板
        _customImagePath = _customImagePath.substr(0, lastSlash + 1) + fileName;
        
        // 提取建筑名称（用于显示）
        // "Archer_Tower" -> "箭塔", "Cannon" -> "炮塔"
        if (fileName.find("Archer") != std::string::npos)
        {
            _customName = "箭塔";
        }
        else if (fileName.find("Cannon") != std::string::npos)
        {
            _customName = "炮塔";
        }
        else
        {
            _customName = "防御建筑";
        }
    }
    
    return true;
}

std::string ArmyBuilding::getDisplayName() const
{
    // 如果有自定义名称，使用自定义名称
    if (!_customName.empty())
    {
        return _customName + " Lv." + std::to_string(_level);
    }
    // 默认返回兵营
    return "兵营 Lv." + std::to_string(_level);
}

int ArmyBuilding::getUpgradeCost() const
{
    // 升级费用随等级递增
    static const int costs[] = {0, 1000, 2000, 4000, 8000, 15000, 30000, 60000, 120000, 200000};
    int idx = std::min(_level, getMaxLevel());
    return costs[idx];
}

std::string ArmyBuilding::getBuildingDescription() const
{
    return StringUtils::format("训练容量: %d\n训练速度: +%.0f%%", getTrainingCapacity(), getTrainingSpeedBonus() * 100);
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
    CCLOG("ArmyBuilding upgraded to level %d, capacity: %d", _level, getTrainingCapacity());
}
std::string ArmyBuilding::getImageForLevel(int level) const
{
    // 如果有自定义图片路径模板，使用它
    if (!_customImagePath.empty())
    {
        return _customImagePath + std::to_string(level) + ".png";
    }
    
    // 否则使用默认的兵营图片（支持1-18级）
    if (level < 1 || level > 18)
        level = 1;
    
    return "buildings/Barracks/Barracks" + std::to_string(level) + ".png";
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
    auto& resMgr = ResourceManager::getInstance();
    int population = getUnitPopulation(unitType);  // ✅ 获取正确的人口数
    
    if (!resMgr.hasTroopSpace(population))
    {
        CCLOG("人口不足！需要 %d 人口，当前：%d/%d", 
              population,
              resMgr.getCurrentTroopCount(), 
              resMgr.getMaxTroopCapacity());
        return false;
    }
    
    // 获取训练费用和时间
    int cost = getUnitTrainingCost(unitType);
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
    case UnitType::kBarbarian: unitName = "野蛮人"; break;
    case UnitType::kArcher: unitName = "弓箭手"; break;
    case UnitType::kGiant: unitName = "巨人"; break;
    case UnitType::kGoblin: unitName = "哥布林"; break;
    case UnitType::kWallBreaker: unitName = "炸弹人"; break;
    default: unitName = "未知兵种"; break;
    }
    
    CCLOG("✅ 开始训练 %s，预计 %.1f 秒完成（队列：%d/%d）",
          unitName.c_str(), actualTime, getQueueLength(), getTrainingCapacity());
    
    return true;
}

void ArmyBuilding::cancelCurrentTask()
{
    if (_trainingQueue.empty())
        return;
    
    // 退还部分资源（50%）
    auto& task = _trainingQueue.front();
    int refund = task.cost / 2;
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
    
    // 🔧 修复：增加正确的人口计数
    int population = getUnitPopulation(task.unitType);  // ✅ 获取兵种人口
    ResourceManager::getInstance().addTroops(population);
    
    // 创建训练好的单位
    Unit* unit = Unit::create(task.unitType);
    
    // 获取兵种名称
    std::string unitName;
    switch (task.unitType)
    {
    case UnitType::kBarbarian: unitName = "野蛮人"; break;
    case UnitType::kArcher: unitName = "弓箭手"; break;
    case UnitType::kGiant: unitName = "巨人"; break;
    case UnitType::kGoblin: unitName = "哥布林"; break;
    case UnitType::kWallBreaker: unitName = "炸弹人"; break;
    default: unitName = "未知兵种"; break;
    }
    
    auto& resMgr = ResourceManager::getInstance();
    CCLOG("🎉 训练完成：%s（占用 %d 人口）！（剩余队列：%d，人口：%d/%d）", 
          unitName.c_str(), population, getQueueLength(),
          resMgr.getCurrentTroopCount(), resMgr.getMaxTroopCapacity());
    
    // 触发回调
    if (_onTrainingComplete && unit)
    {
        _onTrainingComplete(unit);
    }
}

// ==================== 静态方法：获取兵种数据 ====================

float ArmyBuilding::getUnitBaseTrainingTime(UnitType type)
{
    // 基础训练时间（秒）- 官方数据
    switch (type)
    {
    case UnitType::kBarbarian:
        return 20.0f;   // 野蛮人：20秒
    case UnitType::kArcher:
        return 25.0f;   // 弓箭手：25秒
    case UnitType::kGoblin:
        return 30.0f;   // 哥布林：30秒
    case UnitType::kGiant:
        return 120.0f;  // 巨人：2分钟
    case UnitType::kWallBreaker:
        return 180.0f;  // 炸弹人：3分钟
    default:
        return 20.0f;
    }
}

int ArmyBuilding::getUnitTrainingCost(UnitType type)
{
    // 训练费用（圣水）
    switch (type)
    {
    case UnitType::kBarbarian:
        return 25;     // 野蛮人：25圣水
    case UnitType::kArcher:
        return 50;     // 弓箭手：50圣水
    case UnitType::kGoblin:
        return 40;     // 哥布林：40圣水
    case UnitType::kGiant:
        return 250;    // 巨人：250圣水
    case UnitType::kWallBreaker:
        return 600;    // 炸弹人：600圣水
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
        return 1;      // 野蛮人：1人口
    case UnitType::kArcher:
        return 1;      // 弓箭手：1人口
    case UnitType::kGoblin:
        return 1;      // 哥布林：1人口
    case UnitType::kGiant:
        return 5;      // 巨人：5人口 ✅ 修复
    case UnitType::kWallBreaker:
        return 2;      // 炸弹人：2人口
    default:
        return 1;
    }
}