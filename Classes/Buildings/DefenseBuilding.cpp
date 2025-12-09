/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DefenseBuilding.cpp
 * File Function: 防御建筑实现
 * Author:        薛毓哲
 * Update Date:   2025/12/07
 * License:       MIT License
 ****************************************************************/
#include "DefenseBuilding.h"
#include "Unit/unit.h"

USING_NS_CC;

// 辅助函数
static bool isInRange(const Vec2& pos1, const Vec2& pos2, float range)
{
    return pos1.distance(pos2) <= range;
}

DefenseBuilding* DefenseBuilding::create(DefenseType defenseType, int level)
{
    DefenseBuilding* ret = new (std::nothrow) DefenseBuilding();
    if (ret && ret->init(defenseType, level))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

DefenseBuilding* DefenseBuilding::create(DefenseType defenseType, int level, const std::string& imageFile)
{
    DefenseBuilding* ret = new (std::nothrow) DefenseBuilding();
    if (ret && ret->init(defenseType, level, imageFile))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DefenseBuilding::init(DefenseType defenseType, int level)
{
    if (!BaseBuilding::init(level))
        return false;

    _defenseType = defenseType;
    _level       = level;

    initCombatStats();

    return true;
}

bool DefenseBuilding::init(DefenseType defenseType, int level, const std::string& imageFile)
{
    if (!BaseBuilding::init(level, imageFile))
        return false;

    _defenseType     = defenseType;
    _customImagePath = imageFile;
    _level           = level;

    initCombatStats();

    return true;
}

void DefenseBuilding::initCombatStats()
{
    switch (_defenseType)
    {
    case DefenseType::kCannon:
        _combatStats      = DefenseConfig::getCannon(_level);
        _maxHitpoints     = _combatStats.maxHitpoints;
        _currentHitpoints = _maxHitpoints;
        break;

    case DefenseType::kArcherTower:
        _combatStats      = DefenseConfig::getArcherTower(_level);
        _maxHitpoints     = _combatStats.maxHitpoints;
        _currentHitpoints = _maxHitpoints;
        break;

    case DefenseType::kWizardTower:
        _combatStats      = DefenseConfig::getWizardTower(_level);
        _maxHitpoints     = _combatStats.maxHitpoints;
        _currentHitpoints = _maxHitpoints;
        break;

    default:
        _combatStats      = DefenseConfig::getCannon(_level);
        _maxHitpoints     = _combatStats.maxHitpoints;
        _currentHitpoints = _maxHitpoints;
        break;
    }

    CCLOG("🏹 %s 初始化：攻击力=%d, 攻击范围=%.1f, 血量=%d", getDisplayName().c_str(), _combatStats.damage,
          _combatStats.attackRange, _maxHitpoints);
}

std::string DefenseBuilding::getDisplayName() const
{
    if (!_customName.empty())
        return _customName;

    switch (_defenseType)
    {
    case DefenseType::kCannon:
        return StringUtils::format("加农炮 (Lv.%d)", _level);
    case DefenseType::kArcherTower:
        return StringUtils::format("箭塔 (Lv.%d)", _level);
    case DefenseType::kWizardTower:
        return StringUtils::format("法师塔 (Lv.%d)", _level);
    default:
        return StringUtils::format("防御建筑 (Lv.%d)", _level);
    }
}

int DefenseBuilding::getUpgradeCost() const
{
    int baseCost = 0;

    switch (_defenseType)
    {
    case DefenseType::kCannon:
        baseCost = 250;
        break;
    case DefenseType::kArcherTower:
        baseCost = 500;
        break;
    case DefenseType::kWizardTower:
        baseCost = 1500;
        break;
    default:
        baseCost = 300;
        break;
    }

    return baseCost * _level;
}

float DefenseBuilding::getUpgradeTime() const
{
    return 20.0f + (_level * 10.0f);
}

int DefenseBuilding::getMaxLevel() const
{
    switch (_defenseType)
    {
    case DefenseType::kCannon:
    case DefenseType::kArcherTower:
        return 14;
    case DefenseType::kWizardTower:
        return 10;
    default:
        return 10;
    }
}

std::string DefenseBuilding::getBuildingDescription() const
{
    switch (_defenseType)
    {
    case DefenseType::kCannon:
        return "强大的地面防御建筑，对地面单位造成高额伤害。";
    case DefenseType::kArcherTower:
        return "可以攻击空中和地面目标的远程防御建筑。";
    case DefenseType::kWizardTower:
        return "释放强大的范围魔法攻击，对多个目标造成伤害。";
    default:
        return "防御建筑";
    }
}

std::string DefenseBuilding::getImageForLevel(int level) const
{
    if (!_customImagePath.empty())
        return _customImagePath;

    switch (_defenseType)
    {
    case DefenseType::kCannon:
        return StringUtils::format("buildings/Cannon_Static/Cannon%d.png", level);
    case DefenseType::kArcherTower:
        return StringUtils::format("buildings/ArcherTower/Archer_Tower%d.png", level);
    case DefenseType::kWizardTower:
        return StringUtils::format("buildings/WizardTower/Wizard_Tower%d.png", level);
    default:
        return "";
    }
}

void DefenseBuilding::onLevelUp()
{
    BaseBuilding::onLevelUp();
    initCombatStats();
}

// ==================== 战斗逻辑 ====================

void DefenseBuilding::tick(float dt)
{
    if (!_battleModeEnabled || isDestroyed())
        return;

    if (_attackCooldown > 0.0f)
    {
        _attackCooldown -= dt;
    }

    if (_currentTarget)
    {
        if (_currentTarget->IsDead())
        {
            clearTarget();
            return;
        }

        Vec2 targetPos = _currentTarget->getPosition();
        Vec2 myPos     = this->getPosition();

        if (!isInRange(targetPos, myPos, _combatStats.attackRange))
        {
            clearTarget();
            return;
        }

        if (_attackCooldown <= 0.0f)
        {
            attackTarget(_currentTarget);
            _attackCooldown = _combatStats.attackSpeed;
        }
    }
}

void DefenseBuilding::detectEnemies(const std::vector<Unit*>& units)
{
    if (!_battleModeEnabled || isDestroyed())
        return;

    if (_currentTarget && !_currentTarget->IsDead())
        return;

    Vec2  myPos           = this->getPosition();
    Unit* closestUnit     = nullptr;
    float closestDistance = _combatStats.attackRange;

    for (auto* unit : units)
    {
        if (!unit || unit->IsDead())
            continue;

        Vec2  unitPos  = unit->getPosition();
        float distance = myPos.distance(unitPos);

        if (distance <= _combatStats.attackRange && distance < closestDistance)
        {
            closestUnit     = unit;
            closestDistance = distance;
        }
    }

    if (closestUnit)
    {
        setTarget(closestUnit);
    }
}

void DefenseBuilding::attackTarget(Unit* target)
{
    if (!target || target->IsDead())
        return;

    BaseBuilding::attackTarget(target);

    fireProjectile(target);
    playAttackAnimation();
}

void DefenseBuilding::fireProjectile(Unit* target)
{
    if (!target)
        return;

    target->takeDamage(_combatStats.damage);

    CCLOG("💥 %s 击中目标，造成 %d 点伤害", getDisplayName().c_str(), _combatStats.damage);
}

void DefenseBuilding::playAttackAnimation()
{
    auto scaleUp   = ScaleTo::create(0.1f, 1.1f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f);
    auto seq       = Sequence::create(scaleUp, scaleDown, nullptr);
    this->runAction(seq);
}