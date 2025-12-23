/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseUnit.cpp
 * File Function: 单位基类实现
 * Author:        薛毓哲、赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#include "BaseUnit.h"

#include "UI/UnitHealthBarUI.h"

USING_NS_CC;

// ==================== 生命周期管理 ====================

BaseUnit::BaseUnit()
    : _sprite(nullptr), _isMoving(false), _targetPos(Vec2::ZERO), _moveVelocity(Vec2::ZERO), _moveSpeed(100.0f),
      _currentDir(UnitDirection::kRight), _currentPathIndex(0), _currentTarget(nullptr), _attackCooldown(0.0f),
      _unitLevel(1), _isDead(false), _healthBarUI(nullptr), _battleModeEnabled(false)
{}

BaseUnit::~BaseUnit()
{
    // 释放动画缓存
    for (auto& pair : _animCache)
    {
        pair.second->release();
    }
    _animCache.clear();
}

bool BaseUnit::init(int level)
{
    if (!Node::init())
        return false;

    _unitLevel = level;

    // 子类在loadAnimations()中创建精灵和加载动画
    loadAnimations();

    // 初始化血条UI
    initHealthBarUI();

    return true;
}

void BaseUnit::tick(float dt)
{
    if (_isDead)
        return;

    // 更新移动
    if (_isMoving)
    {
        Vec2  current_pos = this->getPosition();
        float distance    = current_pos.distance(_targetPos);
        float step        = _moveSpeed * dt;

        if (step >= distance)
        {
            this->setPosition(_targetPos);

            // 检查路径
            _currentPathIndex++;
            if (_currentPathIndex < _pathPoints.size())
            {
                moveTo(_pathPoints[_currentPathIndex]);
            }
            else
            {
                stopMoving();
            }
        }
        else
        {
            this->setPosition(current_pos + _moveVelocity * dt);
        }
    }

    // 更新攻击冷却
    updateAttackCooldown(dt);
}

// ==================== 移动系统 ====================

void BaseUnit::moveTo(const cocos2d::Vec2& target_pos)
{
    if (_isDead)
        return;

    _targetPos       = target_pos;
    Vec2 current_pos = this->getPosition();
    Vec2 diff        = _targetPos - current_pos;

    if (diff.getLength() < 1.0f)
        return;

    _currentDir = calculateDirection(diff);
    playAnimation(UnitAction::kRun, _currentDir);

    _moveVelocity = diff.getNormalized() * _moveSpeed;
    _isMoving     = true;
}

void BaseUnit::moveToPath(const std::vector<cocos2d::Vec2>& path)
{
    if (path.empty() || _isDead)
        return;

    _pathPoints       = path;
    _currentPathIndex = 0;

    if (_pathPoints.size() > 0 && this->getPosition().distance(_pathPoints[0]) < 10.0f)
    {
        _currentPathIndex = 1;
    }

    if (_currentPathIndex < _pathPoints.size())
    {
        moveTo(_pathPoints[_currentPathIndex]);
    }
}

void BaseUnit::stopMoving()
{
    _isMoving = false;
    _pathPoints.clear();
    playAnimation(UnitAction::kIdle, _currentDir);
}

// ==================== 战斗系统 ====================

void BaseUnit::attack(bool useSecondAttack)
{
    if (_isDead)
        return;

    _isMoving = false;

    // 调用子类钩子
    onAttackBefore();

    // 播放攻击动画
    UnitAction attackAction = useSecondAttack ? UnitAction::kAttack2 : UnitAction::kAttack;
    playAnimation(attackAction, _currentDir);

    // 调用子类钩子
    onAttackAfter();
}

bool BaseUnit::takeDamage(float damage)
{
    if (_isDead)
        return true;

    float actualDamage = _combatStats.takeDamage(damage);

    CCLOG("%s took %.1f damage, HP: %d/%d", getDisplayName().c_str(), actualDamage, _combatStats.currentHitpoints,
          _combatStats.maxHitpoints);

    // 调用子类钩子
    onTakeDamage(actualDamage);

    // 播放受击效果
    if (_sprite)
    {
        auto tint    = TintTo::create(0.1f, 255, 0, 0);
        auto restore = TintTo::create(0.1f, 255, 255, 255);
        auto seq     = Sequence::create(tint, restore, nullptr);
        _sprite->runAction(seq);
    }

    if (_combatStats.currentHitpoints <= 0)
    {
        die();
        return true;
    }

    return false;
}

void BaseUnit::die()
{
    if (_isDead)
        return;

    _isDead = true;
    stopMoving();

    // 调用子类钩子
    onDeathBefore();

    // 播放死亡动画
    playAnimation(UnitAction::kDeath, _currentDir);

    // 3秒后淡出移除
    auto removeAction = Sequence::create(DelayTime::create(3.0f), FadeOut::create(1.0f), RemoveSelf::create(), nullptr);
    this->runAction(removeAction);

    CCLOG("%s died", getDisplayName().c_str());
}

bool BaseUnit::isInAttackRange(const cocos2d::Vec2& targetPos) const
{
    float distance = this->getPosition().distance(targetPos);
    return distance <= _combatStats.attackRange;
}

void BaseUnit::updateAttackCooldown(float dt)
{
    if (_attackCooldown > 0.0f)
    {
        _attackCooldown -= dt;
    }
}

void BaseUnit::resetAttackCooldown()
{
    _attackCooldown = _combatStats.attackSpeed;
}

// ==================== 动画系统 ====================

void BaseUnit::playAnimation(UnitAction action, UnitDirection dir)
{
    if (!_sprite)
        return;

    std::string anim_key = "";
    bool        flip_x   = false;

    // 计算动画键和翻转
    switch (dir)
    {
    case UnitDirection::kRight:
        anim_key = "right";
        flip_x   = false;
        break;
    case UnitDirection::kUpRight:
        anim_key = "up_right";
        flip_x   = false;
        break;
    case UnitDirection::kDownRight:
        anim_key = "down_right";
        flip_x   = false;
        break;
    case UnitDirection::kLeft:
        anim_key = "right";
        flip_x   = true;
        break;
    case UnitDirection::kUpLeft:
        anim_key = "up_right";
        flip_x   = true;
        break;
    case UnitDirection::kDownLeft:
        anim_key = "down_right";
        flip_x   = true;
        break;
    case UnitDirection::kUp:
        anim_key = "up_right";
        flip_x   = false;
        break;
    case UnitDirection::kDown:
        anim_key = "down_right";
        flip_x   = false;
        break;
    }

    // 拼接动作前缀
    std::string prefix = "";
    if (action == UnitAction::kRun)
        prefix = "run_";
    else if (action == UnitAction::kIdle)
        prefix = "idle_";
    else if (action == UnitAction::kAttack)
        prefix = "attack_";
    else if (action == UnitAction::kAttack2)
        prefix = "attack2_";
    else if (action == UnitAction::kDeath)
    {
        // 死亡动画不区分方向
        std::string final_key = "death";
        if (_animCache.count(final_key))
        {
            _sprite->stopAllActions();
            _sprite->setFlippedX(false);
            _sprite->runAction(Animate::create(_animCache[final_key]));
        }
        return;
    }

    std::string final_key = prefix + anim_key;

    // 播放动画
    if (_animCache.count(final_key))
    {
        _sprite->stopAllActions();
        _sprite->setFlippedX(flip_x);

        if (action == UnitAction::kAttack || action == UnitAction::kAttack2)
        {
            // 攻击动画播放一次
            auto animate  = Animate::create(_animCache[final_key]);
            auto callback = CallFunc::create([this]() {
                if (!_isDead && !_isMoving)
                {
                    playAnimation(UnitAction::kIdle, _currentDir);
                }
            });
            _sprite->runAction(Sequence::create(animate, callback, nullptr));
        }
        else
        {
            // 其他动画循环播放
            _sprite->runAction(RepeatForever::create(Animate::create(_animCache[final_key])));
        }
    }
}

void BaseUnit::addAnimation(const std::string& key, cocos2d::Animation* anim)
{
    if (anim)
    {
        anim->retain();
        _animCache[key] = anim;
    }
}

void BaseUnit::addAnimFromFrames(const std::string& unitName, const std::string& key, int start, int end, float delay)
{
    Vector<SpriteFrame*> frames;
    for (int i = start; i <= end; ++i)
    {
        std::string name  = StringUtils::format("%s%d.0.png", unitName.c_str(), i);
        auto        frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(name);
        if (frame)
            frames.pushBack(frame);
    }

    if (!frames.empty())
    {
        auto anim = Animation::createWithSpriteFrames(frames, delay);
        addAnimation(key, anim);
    }
}

void BaseUnit::addAnimFromFiles(const std::string& basePath, const std::string& namePattern, const std::string& key,
                                int start, int end, float delay)
{
    Vector<SpriteFrame*> frames;

    for (int i = start; i <= end; ++i)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), namePattern.c_str(), i);
        std::string fullPath = basePath + buffer;

        auto texture = Director::getInstance()->getTextureCache()->addImage(fullPath);
        if (texture)
        {
            auto frame = SpriteFrame::createWithTexture(
                texture, Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
            if (frame)
                frames.pushBack(frame);
        }
    }

    if (!frames.empty())
    {
        auto anim = Animation::createWithSpriteFrames(frames, delay);
        addAnimation(key, anim);
    }
}

// ==================== UI系统 ====================

void BaseUnit::initHealthBarUI()
{
    auto* healthBarUI = UnitHealthBarUI::create(this);
    if (healthBarUI)
    {
        this->addChild(healthBarUI, 1000);
        _healthBarUI = healthBarUI;
    }
}

void BaseUnit::enableBattleMode()
{
    _battleModeEnabled = true;
    if (_healthBarUI)
    {
        _healthBarUI->setAlwaysVisible(true);
        _healthBarUI->show();
    }
}

void BaseUnit::disableBattleMode()
{
    _battleModeEnabled = false;
    if (_healthBarUI)
    {
        _healthBarUI->setAlwaysVisible(false);
    }
}

// ==================== 工具函数 ====================

UnitDirection BaseUnit::calculateDirection(const cocos2d::Vec2& direction)
{
    float angle = CC_RADIANS_TO_DEGREES(direction.getAngle());

    if (angle >= -22.5f && angle < 22.5f)
        return UnitDirection::kRight;
    if (angle >= 22.5f && angle < 67.5f)
        return UnitDirection::kUpRight;
    if (angle >= 67.5f && angle < 112.5f)
        return UnitDirection::kUp;
    if (angle >= 112.5f && angle < 157.5f)
        return UnitDirection::kUpLeft;
    if (angle >= 157.5f || angle < -157.5f)
        return UnitDirection::kLeft;
    if (angle >= -157.5f && angle < -112.5f)
        return UnitDirection::kDownLeft;
    if (angle >= -112.5f && angle < -67.5f)
        return UnitDirection::kDown;

    return UnitDirection::kDownRight;
}