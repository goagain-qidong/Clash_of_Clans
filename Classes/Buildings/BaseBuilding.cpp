/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseBuilding.cpp
 * File Function: 建筑基类实现
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "BaseBuilding.h"

#include "BuildingHealthBarUI.h"
#include "Managers/UpgradeManager.h"
#include "Services/BuildingUpgradeService.h"
#include "Unit/BaseUnit.h"
#include "Unit/CombatStats.h"

USING_NS_CC;

// ==================== 大本营配置数据表 (17级) ====================
namespace TownHallConfigTable
{
// 生命值表
static const int kHitpoints[] = {
    0,      // Level 0 (无效)
    400,    // Level 1
    800,    // Level 2
    1600,   // Level 3
    2000,   // Level 4
    2400,   // Level 5
    2800,   // Level 6
    3300,   // Level 7
    3900,   // Level 8
    4600,   // Level 9
    5500,   // Level 10
    6800,   // Level 11
    7500,   // Level 12
    8200,   // Level 13
    8900,   // Level 14
    9600,   // Level 15
    10000,  // Level 16
    10400   // Level 17
};

// 升级费用表（金币）
static const int kUpgradeCosts[] = {
    0,          // Level 0 (无效)
    0,          // Level 1 -> 2
    1000,       // Level 2 -> 3
    4000,       // Level 3 -> 4
    25000,      // Level 4 -> 5
    150000,     // Level 5 -> 6
    500000,     // Level 6 -> 7
    1000000,    // Level 7 -> 8
    2000000,    // Level 8 -> 9
    2500000,    // Level 9 -> 10
    3500000,    // Level 10 -> 11
    4000000,    // Level 11 -> 12
    6000000,    // Level 12 -> 13
    9000000,    // Level 13 -> 14
    12000000,   // Level 14 -> 15
    13000000,   // Level 15 -> 16
    15000000,   // Level 16 -> 17
    16000000    // Level 17 (满级)
};

// 升级时间表（秒）
static const float kUpgradeTimes[] = {
    0.0f,       // Level 0 (无效)
    0.0f,       // Level 1 (初始)
    60.0f,      // Level 2
    1800.0f,    // Level 3 (30分钟)
    10800.0f,   // Level 4 (3小时)
    21600.0f,   // Level 5 (6小时)
    43200.0f,   // Level 6 (12小时)
    64800.0f,   // Level 7 (18小时)
    86400.0f,   // Level 8 (1天)
    172800.0f,  // Level 9 (2天)
    259200.0f,  // Level 10 (3天)
    432000.0f,  // Level 11 (5天)
    518400.0f,  // Level 12 (6天)
    604800.0f,  // Level 13 (7天)
    648000.0f,  // Level 14 (7.5天)
    691200.0f,  // Level 15 (8天)
    777600.0f,  // Level 16 (9天)
    864000.0f   // Level 17 (10天)
};

// 描述表
static const char* kDescriptions[] = {
    "",                 // Level 0
    "初始大本营",       // Level 1
    "二级大本营",       // Level 2
    "三级大本营",       // Level 3
    "四级大本营",       // Level 4
    "五级大本营",       // Level 5
    "六级大本营",       // Level 6
    "七级大本营",       // Level 7
    "八级大本营",       // Level 8
    "九级大本营",       // Level 9
    "十级大本营",       // Level 10
    "十一级大本营",     // Level 11
    "十二级大本营",     // Level 12
    "十三级大本营",     // Level 13
    "十四级大本营",     // Level 14
    "十五级大本营",     // Level 15
    "十六级大本营",     // Level 16
    "满级大本营"        // Level 17
};

static const int kMaxLevel = 17;
}  // namespace TownHallConfigTable

// ==================== 城墙配置数据表 (16级) ====================
namespace WallConfigTable
{
// 生命值表
static const int kHitpoints[] = {
    0,      // Level 0 (无效)
    300,    // Level 1
    500,    // Level 2
    700,    // Level 3
    900,    // Level 4
    1400,   // Level 5
    2000,   // Level 6
    2500,   // Level 7
    3000,   // Level 8
    4000,   // Level 9
    5500,   // Level 10
    7000,   // Level 11
    8500,   // Level 12
    10000,  // Level 13
    12000,  // Level 14
    14000,  // Level 15
    17000   // Level 16
};

// 升级费用表（金币）
static const int kUpgradeCosts[] = {
    0,        // Level 0 (无效)
    50,       // Level 1
    1000,     // Level 2
    5000,     // Level 3
    10000,    // Level 4
    30000,    // Level 5
    75000,    // Level 6
    200000,   // Level 7
    500000,   // Level 8
    1000000,  // Level 9
    1500000,  // Level 10
    2000000,  // Level 11
    2500000,  // Level 12
    3000000,  // Level 13
    4000000,  // Level 14
    5000000,  // Level 15
    6000000   // Level 16
};

// 升级时间表（秒）
static const float kUpgradeTimes[] = {
    0.0f,       // Level 0 (无效)
    30.0f,      // Level 1
    60.0f,      // Level 2
    300.0f,     // Level 3
    900.0f,     // Level 4
    1800.0f,    // Level 5
    3600.0f,    // Level 6
    7200.0f,    // Level 7
    14400.0f,   // Level 8
    28800.0f,   // Level 9
    43200.0f,   // Level 10
    86400.0f,   // Level 11
    172800.0f,  // Level 12
    259200.0f,  // Level 13
    345600.0f,  // Level 14
    432000.0f,  // Level 15
    518400.0f   // Level 16
};

static const int kMaxLevel = 16;
}  // namespace WallConfigTable

// ==================== 兵营配置数据表 (14级) ====================
namespace ArmyConfigTable
{
// 生命值表
static const int kHitpoints[] = {
    0,    // Level 0 (无效)
    250,  // Level 1
    270,  // Level 2
    300,  // Level 3
    330,  // Level 4
    360,  // Level 5
    400,  // Level 6
    450,  // Level 7
    500,  // Level 8
    560,  // Level 9
    620,  // Level 10
    700,  // Level 11
    780,  // Level 12
    860,  // Level 13
    950   // Level 14
};

// 升级费用表（圣水）
static const int kUpgradeCosts[] = {
    0,       // Level 0 (无效)
    1000,    // Level 1
    2000,    // Level 2
    4000,    // Level 3
    8000,    // Level 4
    15000,   // Level 5
    30000,   // Level 6
    60000,   // Level 7
    120000,  // Level 8
    200000,  // Level 9
    280000,  // Level 10
    360000,  // Level 11
    440000,  // Level 12
    520000,  // Level 13
    600000   // Level 14
};

// 升级时间表（秒）
static const float kUpgradeTimes[] = {
    0.0f,       // Level 0 (无效)
    30.0f,      // Level 1
    60.0f,      // Level 2
    300.0f,     // Level 3
    900.0f,     // Level 4
    1800.0f,    // Level 5
    3600.0f,    // Level 6
    7200.0f,    // Level 7
    14400.0f,   // Level 8
    28800.0f,   // Level 9
    43200.0f,   // Level 10
    86400.0f,   // Level 11
    172800.0f,  // Level 12
    259200.0f,  // Level 13
    345600.0f   // Level 14
};

static const int kMaxLevel = 14;
}  // namespace ArmyConfigTable

// ==================== 军营配置数据表 (13级) ====================
namespace ArmyCampConfigTable
{
// 生命值表
static const int kHitpoints[] = {
    0,    // Level 0 (无效)
    250,  // Level 1
    280,  // Level 2
    320,  // Level 3
    360,  // Level 4
    400,  // Level 5
    450,  // Level 6
    500,  // Level 7
    550,  // Level 8
    620,  // Level 9
    700,  // Level 10
    800,  // Level 11
    1000, // Level 12
    1200  // Level 13
};

// 容纳人口表
static const int kHousingSpace[] = {
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

// 升级费用表（圣水）
static const int kUpgradeCosts[] = {
    0,         // Level 0 (无效)
    250,       // Level 1
    2500,      // Level 2
    10000,     // Level 3
    100000,    // Level 4
    250000,    // Level 5
    750000,    // Level 6
    2250000,   // Level 7
    6000000,   // Level 8
    7500000,   // Level 9
    9000000,   // Level 10
    10500000,  // Level 11
    12000000,  // Level 12
    14000000   // Level 13
};

// 升级时间表（秒）
static const float kUpgradeTimes[] = {
    0.0f,       // Level 0 (无效)
    0.0f,       // Level 1 (即时)
    900.0f,     // Level 2 (15分钟)
    3600.0f,    // Level 3 (1小时)
    28800.0f,   // Level 4 (8小时)
    86400.0f,   // Level 5 (1天)
    172800.0f,  // Level 6 (2天)
    259200.0f,  // Level 7 (3天)
    345600.0f,  // Level 8 (4天)
    432000.0f,  // Level 9 (5天)
    518400.0f,  // Level 10 (6天)
    604800.0f,  // Level 11 (7天)
    691200.0f,  // Level 12 (8天)
    777600.0f   // Level 13 (9天)
};

static const int kMaxLevel = 13;
}  // namespace ArmyCampConfigTable

// ==================== 初始化与配置 ====================

bool BaseBuilding::initWithType(BuildingType type, int level)
{
    _type  = type;
    _level = level;

    // 1. 加载配置数据
    _config = getStaticConfig(type, level);

    // 2. 初始化 Sprite (使用配置中的图片)
    if (!Sprite::initWithFile(_config.imageFile))
    {
        // 如果图片不存在，尝试使用默认图片或纯色块，避免崩溃
        if (!Sprite::init())
            return false;
        setTextureRect(Rect(0, 0, 100, 100));
        setColor(Color3B::GRAY);
        CCLOG("⚠️ Warning: Image not found for building type %d level %d: %s", (int)type, level,
              _config.imageFile.c_str());
    }

    // 3. 应用属性
    updateProperties();

    return true;
}

bool BaseBuilding::init(int level)
{
    // 兼容旧接口，默认类型为 Unknown，子类应调用 initWithType
    return initWithType(BuildingType::kUnknown, level);
}

bool BaseBuilding::init(int level, const std::string& imageFile)
{
    // 兼容旧接口
    if (!Sprite::initWithFile(imageFile))
    {
        return false;
    }
    _level            = level;
    _config.imageFile = imageFile; // 临时存入配置
    updateProperties();
    return true;
}

void BaseBuilding::updateProperties()
{
    // 重新加载配置（防止升级后数据未更新）
    if (_type != BuildingType::kUnknown)
    {
        _config = getStaticConfig(_type, _level);
    }

    // 更新基础属性
    _maxHitpoints = _config.maxHitpoints;

    // 如果当前血量是满的（或者刚初始化），则更新为新的最大血量
    // 如果是受伤状态升级，通常保持当前血量或按比例提升，这里简化为补满
    if (_currentHitpoints >= _maxHitpoints || _currentHitpoints <= 0)
    {
        _currentHitpoints = _maxHitpoints;
    }

    // 更新网格大小
    if (_gridSize.width == 0 && _gridSize.height == 0)
    {
        _gridSize = _config.gridSize;
    }

    // 更新战斗属性
    _combatStats.damage       = _config.damage;
    _combatStats.attackRange  = _config.attackRange;
    _combatStats.attackSpeed  = _config.attackSpeed;
    _combatStats.maxHitpoints = _config.maxHitpoints;

    // 更新外观
    if (getTexture() == nullptr || _config.imageFile != getImageFile())
    {
        if (!_config.imageFile.empty())
        {
            setTexture(_config.imageFile);
        }
    }
}

// ==================== 静态配置数据 ====================

BuildingConfigData BaseBuilding::getStaticConfig(BuildingType type, int level)
{
    BuildingConfigData config;

    // 默认值
    config.name        = "Unknown Building";
    config.description = "";
    config.imageFile   = "buildings/default.png";
    config.gridSize    = Size(3, 3);
    config.maxLevel    = 10;

    switch (type)
    {
    case BuildingType::kTownHall:
    {
        config.name            = "大本营";
        config.maxLevel        = TownHallConfigTable::kMaxLevel;
        config.gridSize        = Size(4, 4);
        config.upgradeCostType = ResourceType::kGold;

        // 限制等级范围
        int idx = std::max(1, std::min(level, TownHallConfigTable::kMaxLevel));
        config.maxHitpoints = TownHallConfigTable::kHitpoints[idx];
        config.upgradeCost  = TownHallConfigTable::kUpgradeCosts[idx];
        config.upgradeTime  = TownHallConfigTable::kUpgradeTimes[idx];
        config.imageFile    = StringUtils::format("buildings/BaseCamp/town-hall-%d.png", idx);
        config.description  = TownHallConfigTable::kDescriptions[idx];
        break;
    }

    case BuildingType::kWall:
    {
        config.name            = "城墙";
        config.maxLevel        = WallConfigTable::kMaxLevel;
        config.gridSize        = Size(1, 1);
        config.upgradeCostType = ResourceType::kGold;

        // 限制等级范围
        int idx = std::max(1, std::min(level, WallConfigTable::kMaxLevel));
        config.maxHitpoints = WallConfigTable::kHitpoints[idx];
        config.upgradeCost  = WallConfigTable::kUpgradeCosts[idx];
        config.upgradeTime  = WallConfigTable::kUpgradeTimes[idx];
        config.imageFile    = StringUtils::format("buildings/Wall/Wall%d.png", idx);
        config.description  = StringUtils::format("生命值: %d", config.maxHitpoints);
        break;
    }

    case BuildingType::kArmy:
    {
        config.name            = "兵营";
        config.maxLevel        = ArmyConfigTable::kMaxLevel;
        config.gridSize        = Size(3, 3);
        config.upgradeCostType = ResourceType::kElixir;

        // 限制等级范围
        int idx = std::max(1, std::min(level, ArmyConfigTable::kMaxLevel));
        config.maxHitpoints = ArmyConfigTable::kHitpoints[idx];
        config.upgradeCost  = ArmyConfigTable::kUpgradeCosts[idx];
        config.upgradeTime  = ArmyConfigTable::kUpgradeTimes[idx];
        config.imageFile    = StringUtils::format("buildings/Barracks/Barracks%d.png", idx);
        
        // 计算训练容量和速度加成用于描述
        int   trainingCapacity = 20 + (idx - 1) * 5;
        float trainingSpeedPct = (idx - 1) * 5.0f;
        config.description = StringUtils::format("训练容量: %d\n训练速度: +%.0f%%", 
                                                  trainingCapacity, trainingSpeedPct);
        break;
    }

    case BuildingType::kArmyCamp:
    {
        config.name            = "军营";
        config.maxLevel        = ArmyCampConfigTable::kMaxLevel;
        config.gridSize        = Size(4, 4);
        config.upgradeCostType = ResourceType::kElixir;

        // 限制等级范围
        int idx = std::max(1, std::min(level, ArmyCampConfigTable::kMaxLevel));
        config.maxHitpoints     = ArmyCampConfigTable::kHitpoints[idx];
        config.upgradeCost      = ArmyCampConfigTable::kUpgradeCosts[idx];
        config.upgradeTime      = ArmyCampConfigTable::kUpgradeTimes[idx];
        config.resourceCapacity = ArmyCampConfigTable::kHousingSpace[idx];  // 用于存储人口容量
        config.imageFile        = StringUtils::format("buildings/ArmyCamp/Army_Camp%d.png", idx);
        config.description      = StringUtils::format("容纳人口: %d", 
                                                       ArmyCampConfigTable::kHousingSpace[idx]);
        break;
    }

    case BuildingType::kResource:
    {
        // 资源建筑有多个子类型（金矿、圣水收集器、仓库等）
        // 这里提供默认值，子类 ResourceBuilding 会根据子类型覆盖
        config.name            = "资源建筑";
        config.maxLevel        = 15;
        config.gridSize        = Size(3, 3);
        config.upgradeCostType = ResourceType::kGold;
        config.maxHitpoints    = 400 + (level * 40);
        config.upgradeCost     = 150 * level * level;
        config.upgradeTime     = 60.0f * level;
        config.productionRate  = 200 + (level - 1) * 100;
        config.imageFile       = StringUtils::format("buildings/GoldMine/Gold_Mine%d.png", level);
        break;
    }

    case BuildingType::kDefense:
    {
        // 防御建筑有多个子类型（加农炮、箭塔、法师塔等）
        // 这里提供默认值，子类 DefenseBuilding 会根据子类型覆盖
        config.name            = "防御建筑";
        config.maxLevel        = 14;
        config.gridSize        = Size(3, 3);
        config.upgradeCostType = ResourceType::kGold;
        config.maxHitpoints    = 300 + (level * 60);
        config.damage          = 6 + (level * 4);
        config.attackRange     = 250.0f;
        config.attackSpeed     = 0.8f;
        config.upgradeCost     = 250 * level;
        config.upgradeTime     = 20.0f + (level * 10.0f);
        config.imageFile       = StringUtils::format("buildings/Cannon_Static/Cannon%d.png", level);
        break;
    }

    case BuildingType::kDecoration:
    {
        config.name            = "装饰";
        config.maxLevel        = 1;
        config.gridSize        = Size(1, 1);
        config.upgradeCostType = ResourceType::kGold;
        config.maxHitpoints    = 1;  // 装饰物不参与战斗
        config.upgradeCost     = 0;
        config.upgradeTime     = 0.0f;
        break;
    }

    default:
        break;
    }

    return config;
}

// ==================== 升级逻辑 ====================

bool BaseBuilding::canUpgrade() const
{
    return BuildingUpgradeService::getInstance().canUpgrade(this);
}

bool BaseBuilding::upgrade()
{
    auto result = BuildingUpgradeService::getInstance().tryUpgrade(this);
    if (_upgradeCallback)
    {
        _upgradeCallback(result.success, _level);
    }
    return result.success;
}

void BaseBuilding::onUpgradeComplete()
{
    _level++;
    onLevelUp();

    if (_upgradeCallback)
    {
        _upgradeCallback(true, _level);
    }
}

void BaseBuilding::onLevelUp()
{
    // 升级后重新加载属性和外观
    updateProperties();
    CCLOG("✨ %s 升级到了 Lv.%d", getDisplayName().c_str(), _level);
}

float BaseBuilding::getUpgradeProgress() const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    auto* task       = upgradeMgr->getUpgradeTask(const_cast<BaseBuilding*>(this));
    return task ? task->getProgress() : 0.0f;
}

float BaseBuilding::getUpgradeRemainingTime() const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    auto* task       = upgradeMgr->getUpgradeTask(const_cast<BaseBuilding*>(this));
    return task ? task->getRemainingTime() : 0.0f;
}

std::string BaseBuilding::getUpgradeInfo() const
{
    // 自动生成升级信息
    if (isMaxLevel())
        return "Max Level Reached";

    BuildingConfigData nextConfig = getStaticConfig(_type, _level + 1);
    std::stringstream ss;
    ss << "Upgrade to Lv." << (_level + 1) << "\n";
    ss << "HP: " << _maxHitpoints << " -> " << nextConfig.maxHitpoints << "\n";
    if (_combatStats.damage > 0)
    {
        ss << "Damage: " << _combatStats.damage << " -> " << nextConfig.damage << "\n";
    }
    ss << "Cost: " << nextConfig.upgradeCost;
    return ss.str();
}

// ==================== 生命值与战斗 ====================

void BaseBuilding::takeDamage(int damage)
{
    if (damage <= 0)
        return;

    _currentHitpoints -= damage;
    if (_currentHitpoints < 0)
        _currentHitpoints = 0;

    CCLOG("🔨 %s 受到 %d 点伤害！剩余生命值：%d/%d", getDisplayName().c_str(), damage, _currentHitpoints,
          _maxHitpoints);

    if (isDestroyed())
    {
        CCLOG("💥 %s 已被摧毁！", getDisplayName().c_str());
        this->setVisible(false);
    }
}

void BaseBuilding::repair(int amount)
{
    if (amount <= 0)
        return;
    _currentHitpoints += amount;
    if (_currentHitpoints > _maxHitpoints)
        _currentHitpoints = _maxHitpoints;
}

void BaseBuilding::setTarget(BaseUnit* target)
{
    _currentTarget = target;
    if (target)
    {
        CCLOG("🎯 %s 锁定目标", getDisplayName().c_str());
    }
}

void BaseBuilding::attackTarget(BaseUnit* target)
{
    if (!target || !isDefenseBuilding())
        return;
    // 基础攻击逻辑，子类可重写以实现发射投射物
    CCLOG("⚔️ %s 攻击目标，造成 %d 点伤害", getDisplayName().c_str(), _combatStats.damage);
}

// ==================== UI与显示 ====================

void BaseBuilding::initHealthBarUI()
{
    auto* healthBarUI = BuildingHealthBarUI::create(this);
    if (healthBarUI)
    {
        this->addChild(healthBarUI, 1000);
        _healthBarUI = healthBarUI;
    }
}

void BaseBuilding::enableBattleMode()
{
    _battleModeEnabled = true;
    if (_healthBarUI)
    {
        _healthBarUI->setAlwaysVisible(true);
        _healthBarUI->show();
    }
}

void BaseBuilding::disableBattleMode()
{
    _battleModeEnabled = false;
    if (_healthBarUI)
    {
        _healthBarUI->setAlwaysVisible(false);
    }
    if (!isDestroyed())
    {
        _currentHitpoints = _maxHitpoints;
    }
}

// ==================== 生命值管理 ====================

void BaseBuilding::setMaxHitpoints(int hp)
{
    _maxHitpoints = hp;
    _config.maxHitpoints = hp;
    _combatStats.maxHitpoints = hp;

    // 如果当前血量未设置或超过最大值，则设为最大值
    if (_currentHitpoints <= 0 || _currentHitpoints > _maxHitpoints)
    {
        _currentHitpoints = _maxHitpoints;
    }
}

std::string BaseBuilding::getImageForLevel(int level) const
{
    // 默认实现：返回配置中的图片路径
    // 子类可以重写以提供自定义逻辑
    return _config.imageFile;
}