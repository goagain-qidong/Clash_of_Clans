/****************************************************************
* Project Name:  Clash_of_Clans
* File Name:     BaseBuilding.h
* File Function: 建筑基类 - 定义所有建筑的统一接口
* Author:        赵崇治、薛毓哲
* Update Date:   2025/12/24
* License:       MIT License
****************************************************************/
#ifndef BASE_BUILDING_H_
#define BASE_BUILDING_H_

#include "ResourceManager.h"
#include "Unit/CombatStats.h"
#include "cocos2d.h"

#include <functional>
#include <string>

class BaseUnit;
class BuildingHealthBarUI;

/**
 * @enum BuildingType
 * @brief 建筑类型枚举
 */
enum class BuildingType
{
    kTownHall,   ///< 大本营
    kResource,   ///< 资源建筑
    kArmy,       ///< 军事建筑（兵营）
    kArmyCamp,   ///< 军营（存放士兵）
    kDefense,    ///< 防御建筑
    kWall,       ///< 城墙
    kDecoration, ///< 装饰
    kUnknown     ///< 未知类型
};

/**
 * @struct BuildingConfigData
 * @brief 建筑配置数据结构，用于数据驱动
 * @note 重命名以避免与 CombatStats.h 中的 BuildingConfig 命名空间冲突
 */
struct BuildingConfigData
{
    std::string name;        ///< 显示名称
    std::string description; ///< 描述
    std::string imageFile;   ///< 图片路径

    int          maxHitpoints    = 100;                 ///< 最大生命值
    int          upgradeCost     = 0;                   ///< 升级费用
    ResourceType upgradeCostType = ResourceType::kGold; ///< 升级消耗资源类型
    float        upgradeTime     = 0.0f;                ///< 升级时间（秒）
    int          maxLevel        = 1;                   ///< 最大等级

    cocos2d::Size gridSize = {1, 1}; ///< 占地大小

    // 战斗相关
    int   damage      = 0;    ///< 攻击力
    float attackRange = 0.0f; ///< 攻击范围
    float attackSpeed = 1.0f; ///< 攻击速度

    // 资源相关 (可选)
    int resourceCapacity = 0; ///< 资源容量
    int productionRate   = 0; ///< 生产速率
};

/**
 * @class BaseBuilding
 * @brief 建筑基类，定义所有建筑的公共接口
 */
class BaseBuilding : public cocos2d::Sprite
{
public:
    virtual ~BaseBuilding() = default;

    /**
     * @brief 通用初始化入口
     * @param type 建筑类型
     * @param level 初始等级
     * @return bool 是否成功
     */
    virtual bool initWithType(BuildingType type, int level);

    // ==================== 属性获取 (改为非虚函数或普通虚函数，直接读配置) ====================

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const { return _type; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const { return _config.name; }

    /** @brief 获取当前等级 */
    int getLevel() const { return _level; }

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const { return _config.maxLevel; }

    /** @brief 是否已达到最高等级 */
    bool isMaxLevel() const { return _level >= getMaxLevel(); }

    /** @brief 获取当前生命值 */
    int getHitpoints() const { return _currentHitpoints; }

    /** @brief 获取最大生命值 */
    virtual int getMaxHitpoints() const { return _config.maxHitpoints; }

    /** @brief 设置最大生命值 */
    void setMaxHitpoints(int hp);

    /** @brief 获取升级所需费用 */
    virtual int getUpgradeCost() const { return _config.upgradeCost; }

    /** @brief 获取升级消耗的资源类型 */
    virtual ResourceType getUpgradeCostType() const { return _config.upgradeCostType; }

    /** @brief 获取升级所需时间（秒） */
    virtual float getUpgradeTime() const { return _config.upgradeTime; }

    /** @brief 获取建筑描述信息 */
    virtual std::string getBuildingDescription() const { return _config.description; }

    /** @brief 获取当前等级的图片文件 */
    virtual std::string getImageFile() const { return _config.imageFile; }

    /** @brief 获取指定等级的图片文件（子类可重写）*/
    virtual std::string getImageForLevel(int level) const;

    /** @brief 获取升级信息 */
    virtual std::string getUpgradeInfo() const;

    // ==================== 战斗与状态 ====================

    /** @brief 启用战斗模式 */
    virtual void enableBattleMode();

    /** @brief 禁用战斗模式 */
    virtual void disableBattleMode();

    /**
     * @brief 受到伤害
     * @param damage 伤害值
     */
    void takeDamage(int damage);

    /**
     * @brief 修复建筑
     * @param amount 恢复量
     */
    void repair(int amount);

    /** @brief 是否已被摧毁 */
    bool isDestroyed() const { return _currentHitpoints <= 0; }

    /** @brief 获取战斗属性 */
    CombatStats&       getCombatStats() { return _combatStats; }
    const CombatStats& getCombatStats() const { return _combatStats; }

    /** @brief 是否是防御建筑 */
    virtual bool isDefenseBuilding() const { return _type == BuildingType::kDefense; }

    /** @brief 获取攻击伤害 */
    int getDamage() const { return _combatStats.damage; }

    /** @brief 获取攻击范围 */
    float getAttackRange() const { return _combatStats.attackRange; }

    /** @brief 设置攻击目标 */
    void setTarget(BaseUnit* target);

    /** @brief 获取当前目标 */
    BaseUnit* getTarget() const { return _currentTarget; }

    /** @brief 清除目标 */
    void clearTarget() { _currentTarget = nullptr; }

    /** @brief 攻击目标 */
    virtual void attackTarget(BaseUnit* target);

    // ==================== 升级系统 ====================

    /** @brief 尝试升级建筑 (委托给 Service) */
    virtual bool upgrade();

    /** @brief 检查是否可以升级 (委托给 Service) */
    virtual bool canUpgrade() const;

    /** @brief 是否正在升级中 */
    bool isUpgrading() const { return _isUpgrading; }

    /** @brief 设置升级状态 */
    void setUpgrading(bool upgrading) { _isUpgrading = upgrading; }

    /** @brief 升级完成时调用 */
    void onUpgradeComplete();

    /** @brief 获取升级进度 */
    float getUpgradeProgress() const;

    /** @brief 获取升级剩余时间 */
    float getUpgradeRemainingTime() const;

    using UpgradeCallback = std::function<void(bool success, int newLevel)>;
    void setUpgradeCallback(const UpgradeCallback& callback) { _upgradeCallback = callback; }

    // ==================== 网格与位置 ====================

    void          setGridPosition(const cocos2d::Vec2& pos) { _gridPosition = pos; }
    cocos2d::Vec2 getGridPosition() const { return _gridPosition; }

    void          setGridSize(const cocos2d::Size& size) { _gridSize = size; }
    cocos2d::Size getGridSize() const { return _gridSize; } // 优先返回实例设置的，如果没有则返回配置的

    /** @brief 每帧更新 */
    virtual void tick(float dt) {}

    /**
     * @brief 静态辅助函数：获取指定类型的配置数据
     * @note 实际项目中应从 JSON/CSV 读取
     */
    static BuildingConfigData getStaticConfig(BuildingType type, int level);

protected:
    /**
     * @brief 内部初始化
     */
    virtual bool init(int level);
    virtual bool init(int level, const std::string& imageFile);

    /** @brief 初始化血条UI */
    void initHealthBarUI();

    /** @brief 升级时调用 */
    virtual void onLevelUp();

    /** @brief 更新建筑外观和属性 */
    virtual void updateProperties();

    /** @brief 更新外观（子类可重写） */
    virtual void updateAppearance() {}

protected:
    BuildingType       _type = BuildingType::kUnknown;
    BuildingConfigData _config;  ///< 当前等级的配置数据

    int             _level       = 1;      ///< 当前等级
    bool            _isUpgrading = false;  ///< 是否正在升级
    cocos2d::Vec2   _gridPosition;         ///< 网格位置
    cocos2d::Size   _gridSize;             ///< 占用网格大小
    UpgradeCallback _upgradeCallback = nullptr;  ///< 升级回调

    int _maxHitpoints     = 100;  ///< 最大生命值 (缓存自 config)
    int _currentHitpoints = 100;  ///< 当前生命值

    CombatStats _combatStats;               ///< 战斗属性
    BaseUnit*   _currentTarget  = nullptr;  ///< 当前攻击目标
    float       _attackCooldown = 0.0f;     ///< 攻击冷却计时器

    BuildingHealthBarUI* _healthBarUI       = nullptr;  ///< 血条UI
    bool                 _battleModeEnabled = false;    ///< 战斗模式是否启用
};

#endif // BASE_BUILDING_H_