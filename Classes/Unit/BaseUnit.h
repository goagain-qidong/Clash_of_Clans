/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseUnit.h
 * File Function: 单位基类 - 所有士兵的父类
 * Author:        赵崇治、薛毓哲
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef BASE_UNIT_H_
#define BASE_UNIT_H_

#include "Unit/CombatStats.h"
#include "Unit/UnitTypes.h"
#include "cocos2d.h"

#include <map>
#include <string>
#include <vector>

class BaseBuilding;
class UnitHealthBarUI;

/**
 * @class BaseUnit
 * @brief 单位基类 - 定义所有士兵的通用接口和行为
 */
class BaseUnit : public cocos2d::Node
{
public:
    virtual ~BaseUnit();

    // ==================== 生命周期管理 ====================

    virtual void tick(float dt);

    // ==================== 移动系统 ====================

    void moveTo(const cocos2d::Vec2& target_pos);
    void moveToPath(const std::vector<cocos2d::Vec2>& path);
    void stopMoving();

    virtual float        getMoveSpeed() const { return _moveSpeed; }
    bool                 isMoving() const { return _isMoving; }
    const cocos2d::Vec2& getTargetPosition() const { return _targetPos; }

    // ==================== 战斗系统 ====================

    virtual void attack(bool useSecondAttack = false);
    virtual bool takeDamage(float damage);
    virtual void die();

    bool          isDead() const { return _isDead; }
    void          setTarget(BaseBuilding* target) { _currentTarget = target; }
    BaseBuilding* getTarget() const { return _currentTarget; }
    void          clearTarget() { _currentTarget = nullptr; }

    bool isInAttackRange(const cocos2d::Vec2& targetPos) const;
    void updateAttackCooldown(float dt);
    bool isAttackReady() const { return _attackCooldown <= 0.0f; }
    void resetAttackCooldown();

    // ==================== 属性访问 ====================

    virtual UnitType    getUnitType() const    = 0;
    virtual std::string getDisplayName() const = 0;

    CombatStats&       getCombatStats() { return _combatStats; }
    const CombatStats& getCombatStats() const { return _combatStats; }

    int   getCurrentHP() const { return _combatStats.currentHitpoints; }
    int   getMaxHP() const { return _combatStats.maxHitpoints; }
    float getDamage() const { return _combatStats.damage; }
    float getAttackRange() const { return _combatStats.attackRange; }
    int   getLevel() const { return _unitLevel; }

    // ==================== UI系统 ====================

    void initHealthBarUI();
    void enableBattleMode();
    void disableBattleMode();

    // ==================== 兼容旧API（大写命名，逐步废弃） ====================

    void     MoveTo(const cocos2d::Vec2& target_pos) { moveTo(target_pos); }
    void     MoveToPath(const std::vector<cocos2d::Vec2>& path) { moveToPath(path); }
    void     StopMoving() { stopMoving(); }
    void     Attack(bool useSecondAttack = false) { attack(useSecondAttack); }
    void     Die() { die(); }
    bool     IsDead() const { return isDead(); }
    void     PlayAnimation(UnitAction action, UnitDirection dir) { playAnimation(action, dir); }
    UnitType GetType() const { return getUnitType(); }
    float    GetMoveSpeed() const { return getMoveSpeed(); }

protected:
    BaseUnit();
    virtual bool init(int level);

    // ==================== 动画系统（子类实现） ====================

    virtual void loadAnimations() = 0;
    void         playAnimation(UnitAction action, UnitDirection dir);
    void         addAnimation(const std::string& key, cocos2d::Animation* anim);
    void addAnimFromFrames(const std::string& unitName, const std::string& key, int start, int end, float delay);
    void addAnimFromFiles(const std::string& basePath, const std::string& namePattern, const std::string& key,
                          int start, int end, float delay);

    // ==================== 特殊行为钩子（子类可选实现） ====================

    virtual void onAttackBefore() {}
    virtual void onAttackAfter() {}
    virtual void onDeathBefore() {}
    virtual void onTakeDamage(float damage) {}

    // ==================== 工具函数 ====================

    UnitDirection calculateDirection(const cocos2d::Vec2& direction);

    // ==================== 成员变量 ====================

    cocos2d::Sprite*                           _sprite;
    std::map<std::string, cocos2d::Animation*> _animCache;

    // 移动相关
    bool                       _isMoving;
    cocos2d::Vec2              _targetPos;
    cocos2d::Vec2              _moveVelocity;
    float                      _moveSpeed;
    UnitDirection              _currentDir;
    std::vector<cocos2d::Vec2> _pathPoints;
    int                        _currentPathIndex;

    // 战斗相关
    CombatStats   _combatStats;
    BaseBuilding* _currentTarget;
    float         _attackCooldown;
    int           _unitLevel;
    bool          _isDead;

    // UI相关
    UnitHealthBarUI* _healthBarUI;
    bool             _battleModeEnabled;
};

#endif // BASE_UNIT_H_