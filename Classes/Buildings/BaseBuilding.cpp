/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseBuilding.cpp
 * File Function: 建筑基类实现
 * Author:        赵崇治
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/
#include "BaseBuilding.h"
#include "Managers/UpgradeManager.h"
#include "Services/BuildingUpgradeService.h"
USING_NS_CC;
bool BaseBuilding::init(int level)
{
    _level = level;
    return true;
}
bool BaseBuilding::init(int level, const std::string& imageFile)
{
    if (!Sprite::initWithFile(imageFile))
    {
        return false;
    }
    _level = level;
    return true;
}
bool BaseBuilding::canUpgrade() const
{
    // ❗️ 已弃用：委托给 BuildingUpgradeService
    return BuildingUpgradeService::getInstance().canUpgrade(this);
}
bool BaseBuilding::upgrade()
{
    // ❗️ 已弃用：委托给 BuildingUpgradeService
    auto result = BuildingUpgradeService::getInstance().tryUpgrade(this);
    
    // 触发回调
    if (_upgradeCallback)
    {
        _upgradeCallback(result.success, _level);
    }
    
    return result.success;
}

void BaseBuilding::onUpgradeComplete()
{
    // 升级成功
    _level++;
    onLevelUp();
    
    if (_upgradeCallback)
    {
        _upgradeCallback(true, _level);
    }
}

float BaseBuilding::getUpgradeProgress() const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    auto* task = upgradeMgr->getUpgradeTask(const_cast<BaseBuilding*>(this));
    
    return task ? task->getProgress() : 0.0f;
}

float BaseBuilding::getUpgradeRemainingTime() const
{
    auto* upgradeMgr = UpgradeManager::getInstance();
    auto* task = upgradeMgr->getUpgradeTask(const_cast<BaseBuilding*>(this));
    
    return task ? task->getRemainingTime() : 0.0f;
}
void BaseBuilding::onLevelUp()
{
    updateAppearance();
}
void BaseBuilding::updateAppearance()
{
    std::string newImage = getImageForLevel(_level);
    if (!newImage.empty())
    {
        this->setTexture(newImage);
    }
}

// ==================== 生命值系统实现 ====================
void BaseBuilding::takeDamage(int damage)
{
    if (damage <= 0) return;
    
    _currentHitpoints -= damage;
    if (_currentHitpoints < 0)
    {
        _currentHitpoints = 0;
    }
    
    CCLOG("🔨 %s 受到 %d 点伤害！剩余生命值：%d/%d", 
          getDisplayName().c_str(), damage, _currentHitpoints, _maxHitpoints);
    
    // TODO: 播放受伤动画、音效等
    if (isDestroyed())
    {
        CCLOG("💥 %s 已被摧毁！", getDisplayName().c_str());
        // TODO: 播放摧毁动画
    }
}

void BaseBuilding::repair(int amount)
{
    if (amount <= 0) return;
    
    _currentHitpoints += amount;
    if (_currentHitpoints > _maxHitpoints)
    {
        _currentHitpoints = _maxHitpoints;
    }
    
    CCLOG("🔧 %s 修复 %d 点生命值！当前：%d/%d", 
          getDisplayName().c_str(), amount, _currentHitpoints, _maxHitpoints);
}

// ==================== 战斗系统实现 ⭐ 新增 ====================

void BaseBuilding::setTarget(Unit* target)
{
    _currentTarget = target;
    
    if (target)
    {
        CCLOG("🎯 %s 锁定目标", getDisplayName().c_str());
    }
}

void BaseBuilding::attackTarget(Unit* target)
{
    if (!target || !isDefenseBuilding()) return;
    
    CCLOG("⚔️ %s 攻击目标，造成 %d 点伤害", 
          getDisplayName().c_str(), _combatStats.damage);
    
    // 由子类实现具体攻击逻辑（发射炮弹、箭矢等）
}
