/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyBuilding.cpp
 * File Function: 军事建筑类实现
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#include "ArmyBuilding.h"
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
float ArmyBuilding::getUpgradeTime() const
{
    // 升级时间（秒）
    static const int times[] = {0, 60, 300, 900, 3600, 7200, 14400, 28800, 57600, 86400};
    int idx = std::min(_level, getMaxLevel());
    return times[idx];
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
    
    // 否则使用默认的兵营图片
    if (level <= 3)
        return "buildings/Barracks/Barracks1.png";
    else if (level <= 6)
        return "buildings/Barracks/Barracks2.png";
    else
        return "buildings/Barracks/Barracks3.png";
}