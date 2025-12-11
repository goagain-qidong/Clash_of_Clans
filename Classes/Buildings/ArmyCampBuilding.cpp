/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyCampBuilding.cpp
 * File Function: 军营建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/
#include "ArmyCampBuilding.h"
USING_NS_CC;

ArmyCampBuilding* ArmyCampBuilding::create(int level)
{
    ArmyCampBuilding* building = new (std::nothrow) ArmyCampBuilding();
    if (building && building->init(level))
    {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool ArmyCampBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(4, 4); // 军营占用4x4网格
    
    std::string imageFile = getImageFile();
    if (!Sprite::initWithFile(imageFile))
        return false;
    
    // 设置锚点和缩放
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    
    // 初始化时增加人口容量
    int housingSpace = getHousingSpace();
    ResourceManager::getInstance().addCapacity(kTroopPopulation, housingSpace);
    CCLOG("ArmyCampBuilding created at level %d, added %d housing space", _level, housingSpace);
    
    return true;
}

std::string ArmyCampBuilding::getDisplayName() const
{
    return "军营 Lv." + std::to_string(_level);
}

int ArmyCampBuilding::getUpgradeCost() const
{
    // 军营升级费用表（13级）
    static const int costs[] = {
        0,       // Level 0 (无效)
        250,     // Level 1
        2500,    // Level 2
        10000,   // Level 3
        100000,  // Level 4
        250000,  // Level 5
        750000,  // Level 6
        2250000, // Level 7
        6000000, // Level 8
        7500000, // Level 9
        9000000, // Level 10
        10500000,// Level 11
        12000000,// Level 12
        14000000 // Level 13
    };
    
    if (_level < 1 || _level > 13)
        return 0;
    
    return costs[_level];
}

float ArmyCampBuilding::getUpgradeTime() const
{
    // 军营升级时间（秒）
    static const float times[] = {
        0,      // Level 0 (无效)
        0,      // Level 1 (即时)
        900,    // Level 2 (15分钟)
        3600,   // Level 3 (1小时)
        28800,  // Level 4 (8小时)
        86400,  // Level 5 (1天)
        172800, // Level 6 (2天)
        259200, // Level 7 (3天)
        345600, // Level 8 (4天)
        432000, // Level 9 (5天)
        518400, // Level 10 (6天)
        604800, // Level 11 (7天)
        691200, // Level 12 (8天)
        777600  // Level 13 (9天)
    };
    
    if (_level < 1 || _level > 13)
        return 0;
    
    return times[_level];
}

std::string ArmyCampBuilding::getBuildingDescription() const
{
    return StringUtils::format("容纳人口: %d", getHousingSpace());
}

std::string ArmyCampBuilding::getImageFile() const
{
    return getImageForLevel(_level);
}

std::string ArmyCampBuilding::getImageForLevel(int level) const
{
    if (level < 1 || level > 13)
        level = 1;
    
    return "buildings/ArmyCamp/Army_Camp" + std::to_string(level) + ".png";
}

int ArmyCampBuilding::getHousingSpace() const
{
    // 军营容纳人口表（13级）
    static const int housingSpace[] = {
        0,   // Level 0 (无效)
        20,  // Level 1
        30,  // Level 2
        35,  // Level 3
        40,  // Level 4
        45,  // Level 5
        50,  // Level 6
        55,  // Level 7
        60,  // Level 8
        65,  // Level 9
        70,  // Level 10
        75,  // Level 11
        80,  // Level 12
        85   // Level 13
    };
    
    if (_level < 1 || _level > 13)
        return 0;
    
    return housingSpace[_level];
}

void ArmyCampBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    
    // 增加人口容量
    int housingSpace = getHousingSpace();
    int prevHousingSpace = 0;
    if (_level > 1)
    {
        // 获取上一级的容纳人口
        static const int housingSpaceTable[] = {
            0, 20, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85
        };
        prevHousingSpace = housingSpaceTable[_level - 1];
    }
    
    int addedCapacity = housingSpace - prevHousingSpace;
    if (addedCapacity > 0)
    {
        ResourceManager::getInstance().addCapacity(kTroopPopulation, addedCapacity);
        CCLOG("ArmyCampBuilding upgraded to level %d, Housing Space: %d (+%d)", 
              _level, housingSpace, addedCapacity);
    }
}

// ==================== 🆕 小兵显示功能实现 ====================

void ArmyCampBuilding::addTroopDisplay(UnitType type)
{
    // 🎮 创建真实的站立小兵（使用 Unit 类）
    Unit* troopUnit = Unit::create(type);
    if (!troopUnit)
    {
        CCLOG("❌ Failed to create troop unit for display");
        return;
    }
    
    // 设置缩放（军营里的小兵应该比较小）
    troopUnit->setScale(0.5f);
    
    // 计算小兵应该站立的位置
    int index = static_cast<int>(_troopSprites.size());
    Vec2 pos = getTroopDisplayPosition(index);
    troopUnit->setPosition(pos);
    
    // 播放待机动画（朝向右边）
    troopUnit->PlayAnimation(UnitAction::kIdle, UnitDirection::kRight);
    
    // 添加到军营建筑
    this->addChild(troopUnit, 50);  // Z-Order 50，在建筑上方
    
    // 保存到列表（注意：这里存的是 Sprite* 指针，但实际是 Unit*）
    _troopSprites.push_back(troopUnit);
    
    CCLOG("✅ Added troop unit to Army Camp (total: %zu)", _troopSprites.size());
}

void ArmyCampBuilding::removeTroopDisplay(UnitType type)
{
    // 从后往前移除第一个匹配的小兵
    // 简化处理：直接移除最后一个
    if (_troopSprites.empty())
        return;
    
    auto lastSprite = _troopSprites.back();
    lastSprite->removeFromParent();
    _troopSprites.pop_back();
    
    // 更新剩余小兵的位置
    updateTroopPositions();
    
    CCLOG("✅ Removed troop display from Army Camp (remaining: %zu)", _troopSprites.size());
}

void ArmyCampBuilding::clearTroopDisplays()
{
    for (auto* sprite : _troopSprites)
    {
        if (sprite)
        {
            sprite->removeFromParent();
        }
    }
    _troopSprites.clear();
    
    CCLOG("🗑️ Cleared all troop displays from Army Camp");
}

void ArmyCampBuilding::updateTroopPositions()
{
    // 重新排列所有小兵的位置
    for (size_t i = 0; i < _troopSprites.size(); ++i)
    {
        if (_troopSprites[i])
        {
            Vec2 pos = getTroopDisplayPosition(static_cast<int>(i));
            
            // 使用移动动作让小兵走到新位置
            auto moveTo = MoveTo::create(0.3f, pos);
            _troopSprites[i]->runAction(moveTo);
        }
    }
}

Vec2 ArmyCampBuilding::getTroopDisplayPosition(int index) const
{
    // 在军营周围排列小兵
    // 使用2x2或3x3网格排列
    float buildingWidth = this->getContentSize().width;
    float buildingHeight = this->getContentSize().height;
    
    // 每行3个小兵
    int row = index / 3;
    int col = index % 3;
    
    // 小兵站在军营前方（下方）
    float startX = -buildingWidth * 0.3f;
    float startY = -buildingHeight * 0.2f;  // 负值表示在建筑下方
    float spacingX = buildingWidth * 0.3f;
    float spacingY = buildingHeight * 0.25f;
    
    return Vec2(startX + col * spacingX, startY - row * spacingY);
}
