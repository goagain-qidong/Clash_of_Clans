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

// ==================== 静态配置数据 (模拟数据库) ====================

BuildingConfigData BaseBuilding::getStaticConfig(BuildingType type, int level)
{
    BuildingConfigData config;
    config.maxLevel = 10;

    // 默认值
    config.name      = "Unknown Building";
    config.imageFile = "buildings/default.png";
    config.gridSize  = Size(3, 3);

    // 简单的硬编码配置，实际项目中应读取 JSON/CSV
    switch (type)
    {
    case BuildingType::kTownHall:
        config.name         = "Town Hall";
        config.description  = "The heart of your village.";
        config.maxHitpoints = 1500 + (level * 500);
        config.upgradeCost  = 1000 * level;
        config.upgradeTime  = 10.0f * level;
        config.imageFile    = StringUtils::format("buildings/TownHall/TownHall%d.png", level);
        config.gridSize     = Size(4, 4);
        break;

    case BuildingType::kResource: // 假设是金矿
        config.name           = "Gold Mine";
        config.maxHitpoints   = 400 + (level * 100);
        config.upgradeCost    = 150 * level;
        config.productionRate = 100 * level;
        config.imageFile      = StringUtils::format("buildings/GoldMine/GoldMine%d.png", level);
        config.gridSize       = Size(3, 3);
        break;

    case BuildingType::kDefense: // 假设是加农炮
        config.name         = "Cannon";
        config.maxHitpoints = 600 + (level * 120);
        config.damage       = 50 + (level * 10);
        config.attackRange  = 300.0f;
        config.attackSpeed  = 1.0f;
        config.upgradeCost  = 200 * level;
        config.imageFile    = StringUtils::format("buildings/Cannon_Static/Cannon%d.png", level);
        config.gridSize     = Size(3, 3);
        break;

    case BuildingType::kWall:
        config.name         = "Wall";
        config.maxHitpoints = 300 + (level * 300);
        config.upgradeCost  = 50 * level;
        config.imageFile    = StringUtils::format("buildings/Wall/Wall%d.png", level);
        config.gridSize     = Size(1, 1);
        break;

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