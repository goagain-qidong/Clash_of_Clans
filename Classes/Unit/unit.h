#ifndef UNIT_H
#define UNIT_H

#include "cocos2d.h"
#include "Unit/CombatStats.h"
#include <map>
#include <string>
#include <vector>

class BaseBuilding;
class UnitHealthBarUI;

enum class UnitType
{
    kBarbarian,
    kArcher,
    kGiant,
    kGoblin,
    kWallBreaker
};

enum class UnitAction
{
    kRun,
    kIdle,
    kAttack,
    kAttack2,
    kDeath
};

enum class UnitDirection
{
    kUp, kUpRight, kRight, kDownRight, kDown, kDownLeft, kLeft, kUpLeft
};

class Unit : public cocos2d::Node
{
public:
    static Unit* create(UnitType type);
    virtual bool init(UnitType type);
    virtual ~Unit();

    void tick(float dt);

    // --- 移动接口 ---
    void MoveTo(const cocos2d::Vec2& target_pos);
    void MoveToPath(const std::vector<cocos2d::Vec2>& path);
    void StopMoving();

    void Attack(bool useSecondAttack = false);
    void Die();
    bool IsDead() const { return is_dead_; }
    void PlayAnimation(UnitAction action, UnitDirection dir);

    UnitType GetType() const { return type_; }
    float GetMoveSpeed() const { return move_speed_; }

    // --- 战斗接口 ---
    CombatStats& getCombatStats() { return combat_stats_; }
    const CombatStats& getCombatStats() const { return combat_stats_; }

    // ✅ 补回缺失的生命值获取函数
    int getCurrentHP() const { return combat_stats_.currentHitpoints; }
    int getMaxHP() const { return combat_stats_.maxHitpoints; }

    bool takeDamage(float damage);
    void setTarget(BaseBuilding* target);
    BaseBuilding* getTarget() const { return current_target_; }
    void clearTarget() { current_target_ = nullptr; }

    bool isInAttackRange(const cocos2d::Vec2& targetPos) const;
    float getDamage() const { return combat_stats_.damage; }
    float getAttackRange() const { return combat_stats_.attackRange; }

    void updateAttackCooldown(float dt);
    bool isAttackReady() const;
    void resetAttackCooldown();

    const cocos2d::Vec2& getTargetPosition() const { return target_pos_; }
    bool isMoving() const { return is_moving_; }

    // --- UI ---
    void initHealthBarUI();
    void enableBattleMode();
    void disableBattleMode();

private:
    cocos2d::Sprite* sprite_ = nullptr;
    UnitType type_;
    std::map<std::string, cocos2d::Animation*> anim_cache_;

    // --- 移动属性 ---
    bool          is_moving_ = false;
    bool          is_dead_ = false;
    cocos2d::Vec2 target_pos_;
    cocos2d::Vec2 move_velocity_;
    float         move_speed_ = 100.0f;
    UnitDirection current_dir_ = UnitDirection::kRight;

    std::vector<cocos2d::Vec2> _pathPoints;
    int _currentPathIndex = 0;

    // --- 战斗属性 ---
    CombatStats combat_stats_;
    BaseBuilding* current_target_ = nullptr;
    float attack_cooldown_ = 0.0f;
    int unit_level_ = 1;

    class UnitHealthBarUI* _healthBarUI = nullptr;
    bool _battleModeEnabled = false;

    // --- 辅助函数 ---
    void LoadConfig(UnitType type);
    void AddAnim(const std::string& unitName, const std::string& key, int start, int end, float delay);
    void AddAnimWithSkip(const std::string& unitName, const std::string& key, const std::vector<int>& frameIndices, float delay);
    void AddAnimFromFiles(const std::string& basePath, const std::string& namePattern, const std::string& key, int start, int end, float delay);
    UnitDirection CalculateDirection(const cocos2d::Vec2& diff);
};

#endif