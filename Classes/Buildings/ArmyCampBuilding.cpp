/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ArmyCampBuilding.cpp
 * File Function: 军营建筑类实现
 * Author:        薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ArmyCampBuilding.h"

#include "Managers/TroopInventory.h"
#include "Unit/UnitFactory.h"

USING_NS_CC;

// ==================== 创建与初始化 ====================

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
    // 使用 initWithType 统一初始化，配置数据由基类管理
    if (!initWithType(BuildingType::kArmyCamp, level))
    {
        return false;
    }

    // 军营特有的外观设置
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);

    // 初始化时增加人口容量
    int housingSpace = getHousingSpace();
    ResourceManager::getInstance().addCapacity(kTroopPopulation, housingSpace);

    // 初始化血条UI
    initHealthBarUI();

    CCLOG("⛺ %s 初始化 HP: %d, 容纳人口: %d", 
          getDisplayName().c_str(), getMaxHitpoints(), housingSpace);
    return true;
}

int ArmyCampBuilding::getHousingSpace() const
{
    // 从配置中获取人口容量（存储在 resourceCapacity 字段）
    return _config.resourceCapacity;
}

void ArmyCampBuilding::onLevelUp()
{
    // 获取升级前的人口容量
    BuildingConfigData prevConfig = getStaticConfig(BuildingType::kArmyCamp, _level - 1);
    int prevHousingSpace = prevConfig.resourceCapacity;

    // 调用基类升级逻辑（会更新 _config）
    BaseBuilding::onLevelUp();

    // 计算新增的人口容量
    int currentHousingSpace = getHousingSpace();
    int addedCapacity = currentHousingSpace - prevHousingSpace;

    if (addedCapacity > 0)
    {
        ResourceManager::getInstance().addCapacity(kTroopPopulation, addedCapacity);
        CCLOG("ArmyCampBuilding upgraded to level %d, Housing Space: %d (+%d)",
              _level, currentHousingSpace, addedCapacity);
    }
}

// ==================== 小兵显示功能实现 ====================

void ArmyCampBuilding::addTroopDisplay(UnitType type)
{
    // 创建真实的站立小兵（使用 UnitFactory）
    BaseUnit* troopUnit = UnitFactory::createUnit(type);
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
    
    // 保存到列表
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
    float buildingWidth  = this->getContentSize().width;
    float buildingHeight = this->getContentSize().height;

    // 每行6个小兵
    int row = index / 6;
    int col = index % 6;

    // 小兵站在军营内（向右上方调整）
    float startX   = buildingWidth * 0.1f;
    float startY   = buildingHeight * 0.25f;
    float spacingX = buildingWidth * 0.15f;
    float spacingY = buildingHeight * 0.2f;

    return Vec2(startX + col * spacingX, startY - row * spacingY);
}

void ArmyCampBuilding::refreshDisplayFromInventory()
{
    // 清空当前显示
    clearTroopDisplays();
    
    // 从士兵库存读取所有士兵数量
    auto& troopInv = TroopInventory::getInstance();
    const auto& allTroops = troopInv.getAllTroops();
    
    // 按顺序添加士兵显示（优先级：野蛮人 > 弓箭手 > 哥布林 > 巨人 > 炸弹人）
    std::vector<UnitType> displayOrder = {
        UnitType::kBarbarian,
        UnitType::kArcher,
        UnitType::kGoblin,
        UnitType::kGiant,
        UnitType::kWallBreaker
    };
    
    for (UnitType type : displayOrder)
    {
        auto it = allTroops.find(type);
        if (it != allTroops.end() && it->second > 0)
        {
            // 添加该兵种的显示（最多显示容纳人口数的小兵）
            int count = std::min(it->second, getHousingSpace());
            for (int i = 0; i < count; ++i)
            {
                addTroopDisplay(type);
            }
        }
    }
    
    CCLOG("✅ 军营显示已刷新（从库存）：显示 %zu 个小兵", _troopSprites.size());
}
