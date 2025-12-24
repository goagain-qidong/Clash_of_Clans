/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TownHallBuilding.cpp
 * File Function: 大本营建筑实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "TownHallBuilding.h"

#include "Managers/BuildingLimitManager.h"
#include "ResourceManager.h"

USING_NS_CC;

// ==================== TownHallBuilding 实现 ====================

TownHallBuilding* TownHallBuilding::create(int level)
{
    TownHallBuilding* ret = new (std::nothrow) TownHallBuilding();
    if (ret && ret->init(level))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TownHallBuilding::init(int level)
{
    // 使用 initWithType 统一初始化，配置数据由基类管理
    if (!initWithType(BuildingType::kTownHall, level))
    {
        return false;
    }

    // 大本营特有的外观设置
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);

    // 初始化建筑限制管理器
    BuildingLimitManager::getInstance()->updateLimitsFromTownHall(_level);

    // 初始化血条UI
    initHealthBarUI();

    CCLOG("🏰 %s 初始化 HP: %d", getDisplayName().c_str(), getMaxHitpoints());
    return true;
}

bool TownHallBuilding::canUpgrade() const
{
    return _level < getMaxLevel() && !_isUpgrading;
}

std::string TownHallBuilding::getUpgradeInfo() const
{
    if (!canUpgrade())
    {
        return "大本营已满级";
    }

    BuildingConfigData nextConfig = getStaticConfig(BuildingType::kTownHall, _level + 1);
    return StringUtils::format("升级到 %s\n需要: %d 金币", 
                                nextConfig.description.c_str(), 
                                nextConfig.upgradeCost);
}

void TownHallBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();

    CCLOG("🎉 TownHall upgraded to Lv.%d", _level);

    // 更新所有建筑的数量限制
    BuildingLimitManager::getInstance()->updateLimitsFromTownHall(_level);
}

void TownHallBuilding::updateAppearance()
{
    std::string imageFile = getImageFile();
    auto texture = Director::getInstance()->getTextureCache()->addImage(imageFile);
    if (texture)
    {
        this->setTexture(texture);
        this->setName(getDisplayName());
    }
}

// ==================== 建筑限制系统 ====================

int TownHallBuilding::getMaxBuildingLevel(const std::string& buildingName) const
{
    // TODO: 实现建筑等级限制逻辑
    // 示例：大本营7级时，箭塔最高只能升到7级
    // 当前返回 _level，表示建筑等级不能超过大本营等级
    return _level;
}

bool TownHallBuilding::isBuildingUnlocked(const std::string& buildingName) const
{
    // TODO: 实现建筑解锁逻辑
    // 示例：大本营3级解锁迫击炮，大本营7级解锁野蛮人之王
    // 当前默认返回 true，表示所有建筑都已解锁
    return true;
}