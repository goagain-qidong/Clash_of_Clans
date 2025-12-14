/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DefenseBuilding.cpp
 * File Function: 防御建筑实现
 * Author:        薛毓哲
 * Update Date:   2025/12/07
 * License:       MIT License
 ****************************************************************/
#include "DefenseBuilding.h"
#include "UI/BuildingHealthBarUI.h"
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
    initHealthBarUI();  // ✅ 添加血条初始化

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
    initHealthBarUI();

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

    // ==================== 🎯 旋转朝向目标 ====================
    rotateToTarget(target->getPosition());
    
    // ==================== 💥 创建炮弹/箭矢视觉效果 ====================
    Sprite* projectile = nullptr;
    float projectileSpeed = 0.0f;  // 飞行速度（像素/秒）
    
    switch (_defenseType)
    {
        case DefenseType::kCannon:
            projectile = createCannonballSprite();
            projectileSpeed = 600.0f;  // 炮弹较快
            break;
            
        case DefenseType::kArcherTower:
            projectile = createArrowSprite();
            projectileSpeed = 800.0f;  // 箭矢最快
            break;
            
        case DefenseType::kWizardTower:
            // 法师塔可以用粒子效果或魔法球
            projectile = createCannonballSprite();  // 临时用炮弹代替
            projectileSpeed = 500.0f;
            break;
            
        default:
            projectile = createCannonballSprite();
            projectileSpeed = 600.0f;
            break;
    }
    
    if (!projectile || !this->getParent())
    {
        // 如果创建失败，直接造成伤害（无视觉效果）
        target->takeDamage(_combatStats.damage);
        CCLOG("💥 %s 击中目标，造成 %d 点伤害", getDisplayName().c_str(), _combatStats.damage);
        return;
    }
    
    // ==================== 🚀 炮弹飞行动画 ====================
    Vec2 startPos = this->getPosition();
    Vec2 endPos = target->getPosition();
    
    projectile->setPosition(startPos);
    this->getParent()->addChild(projectile, 5000);  // 高Z-order，显示在最前面
    
    // 计算飞行时间
    float distance = startPos.distance(endPos);
    float duration = distance / projectileSpeed;
    
    // 箭矢需要旋转朝向目标
    if (_defenseType == DefenseType::kArcherTower)
    {
        Vec2 direction = endPos - startPos;
        float angle = CC_RADIANS_TO_DEGREES(direction.getAngle());
        projectile->setRotation(-angle);
    }
    
    // 移动到目标位置
    auto moveTo = MoveTo::create(duration, endPos);
    
    // 命中回调：造成伤害并移除炮弹
    auto hitCallback = CallFunc::create([this, target, projectile]() {
        if (target && !target->IsDead())
        {
            target->takeDamage(_combatStats.damage);
            CCLOG("💥 %s 击中目标，造成 %d 点伤害", getDisplayName().c_str(), _combatStats.damage);
            
            // 可选：添加爆炸粒子效果
            if (this->getParent())
            {
                auto explosion = ParticleExplosion::create();
                explosion->setPosition(target->getPosition());
                explosion->setDuration(0.3f);
                explosion->setScale(0.3f);
                this->getParent()->addChild(explosion, 6000);
            }
        }
        
        // 移除炮弹
        projectile->removeFromParent();
    });
    
    // 执行动画序列
    auto sequence = Sequence::create(moveTo, hitCallback, nullptr);
    projectile->runAction(sequence);
}


void DefenseBuilding::playAttackAnimation()
{
    auto scaleUp   = ScaleTo::create(0.1f, 1.1f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f);
    auto seq       = Sequence::create(scaleUp, scaleDown, nullptr);
    this->runAction(seq);
}

// ==================== 攻击范围显示 ====================

void DefenseBuilding::showAttackRange()
{
    if (_rangeCircle)
    {
        _rangeCircle->setVisible(true);
        return;
    }
    
    // 创建半透明圆圈显示攻击范围
    _rangeCircle = DrawNode::create();
    
    // 根据建筑类型选择不同的颜色
    Color4F circleColor;
    switch (_defenseType)
    {
        case DefenseType::kCannon:
            circleColor = Color4F(1.0f, 0.0f, 0.0f, 0.3f);  // 红色 - 加农炮
            break;
        case DefenseType::kArcherTower:
            circleColor = Color4F(0.0f, 1.0f, 0.0f, 0.3f);  // 绿色 - 箭塔
            break;
        case DefenseType::kWizardTower:
            circleColor = Color4F(0.5f, 0.0f, 1.0f, 0.3f);  // 紫色 - 法师塔
            break;
        default:
            circleColor = Color4F(1.0f, 1.0f, 0.0f, 0.3f);  // 黄色
            break;
    }
    
    // 绘制圆圈（中心在建筑位置，半径为攻击范围）
    _rangeCircle->drawCircle(Vec2::ZERO, _combatStats.attackRange, 0, 100, false, 2.0f, 2.0f, circleColor);
    
    // 添加到建筑节点
    this->addChild(_rangeCircle, -1);  // Z-order为-1，显示在建筑下方
    
    CCLOG("🎯 %s 显示攻击范围：%.1f 像素", getDisplayName().c_str(), _combatStats.attackRange);
}

void DefenseBuilding::hideAttackRange()
{
    if (_rangeCircle)
    {
        _rangeCircle->setVisible(false);
    }
}

void DefenseBuilding::rotateToTarget(const cocos2d::Vec2& targetPos)
{
    Vec2 myPos = this->getPosition();
    Vec2 direction = targetPos - myPos;
    
    // 计算角度（弧度转角度）
    float angle = CC_RADIANS_TO_DEGREES(direction.getAngle());
    
    // 平滑旋转到目标角度
    auto rotateTo = RotateTo::create(0.2f, -angle);  // 负号是因为cocos2d-x的旋转方向
    this->runAction(rotateTo);
}

// ==================== 炮弹/箭矢创建 ====================

Sprite* DefenseBuilding::createCannonballSprite()
{
    // 创建一个简单的圆形炮弹（黑色）
    auto cannonball = Sprite::create();
    if (!cannonball)
    {
        // 如果没有图片资源，用DrawNode画一个黑色圆球
        auto drawNode = DrawNode::create();
        drawNode->drawSolidCircle(Vec2::ZERO, 8.0f, 0, 20, Color4F::BLACK);
        return (Sprite*)drawNode;  // 临时方案
    }
    
    cannonball->setScale(0.5f);
    return cannonball;
}

Sprite* DefenseBuilding::createArrowSprite()
{
    // 创建一个简单的箭矢（棕色长条）
    auto arrow = Sprite::create();
    if (!arrow)
    {
        // 如果没有图片资源，用DrawNode画一个箭头
        auto drawNode = DrawNode::create();
        Vec2 arrowPoints[] = {
            Vec2(-15, 0),  // 尾部
            Vec2(15, 0),   // 尖端
        };
        drawNode->drawSegment(arrowPoints[0], arrowPoints[1], 2.0f, Color4F(0.6f, 0.3f, 0.0f, 1.0f));
        return (Sprite*)drawNode;  // 临时方案
    }
    
    return arrow;
}



