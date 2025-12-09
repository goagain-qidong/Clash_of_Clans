/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BaseBuilding.h
 * File Function: 建筑基类 - 定义所有建筑的统一接口
 * Author:        赵崇治
 * Update Date:   2025/11/29
 * License:       MIT License
 ****************************************************************/
#pragma once
#include "ResourceManager.h"
#include "cocos2d.h"
#include <functional>
/** @brief 建筑类型枚举 */
enum class BuildingType
{
    kTownHall,   // 大本营
    kResource,   // 资源建筑
    kArmy,       // 军事建筑（兵营 - 训练士兵）
    kArmyCamp,   // 军营（存放士兵）
    kDefense,    // 防御建筑
    kWall,       // 城墙
    kDecoration, // 装饰
    kUnknown
};
// 注意：ResourceType 已在 ResourceManager.h 中定义，此处不再重复定义
/**
 * @class BaseBuilding
 * @brief 建筑基类，定义所有建筑的公共接口
 */
class BaseBuilding : public cocos2d::Sprite
{
public:
    virtual ~BaseBuilding() = default;
    // ==================== 基础属性 ====================
    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const = 0;
    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const = 0;
    /** @brief 获取当前等级 */
    int getLevel() const { return _level; }
    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const = 0;
    /** @brief 是否已达到最高等级 */
    bool isMaxLevel() const { return _level >= getMaxLevel(); }
    
    // ==================== 生命值系统 ====================
    /** @brief 获取当前生命值 */
    int getHitpoints() const { return _currentHitpoints; }
    /** @brief 获取最大生命值 */
    int getMaxHitpoints() const { return _maxHitpoints; }
    /** @brief 设置最大生命值（用于初始化和升级） */
    void setMaxHitpoints(int hp) { _maxHitpoints = hp; _currentHitpoints = hp; }
    /** @brief 受到伤害 */
    void takeDamage(int damage);
    /** @brief 修复建筑（恢复生命值） */
    void repair(int amount);
    /** @brief 是否已被摧毁 */
    bool isDestroyed() const { return _currentHitpoints <= 0; }
    // ==================== 升级相关（统一接口） ====================
    /** @brief 获取升级所需费用 */
    virtual int getUpgradeCost() const = 0;
    /** @brief 获取升级消耗的资源类型 */
    virtual ResourceType getUpgradeCostType() const { return kGold; }
    /** @brief 获取升级所需时间（秒） */
    virtual float getUpgradeTime() const { return 10.0f; }
    /** @brief 获取建筑描述信息 */
    virtual std::string getBuildingDescription() const { return ""; }
    /** @brief 获取升级信息 */
    virtual std::string getUpgradeInfo() const { return ""; }
    /** @brief 获取当前等级的图片文件 */
    virtual std::string getImageFile() const { return ""; }
    
    /**
     * @brief 尝试升级建筑（❗️ 已弃用，请使用 BuildingUpgradeService::tryUpgrade()）
     * @deprecated 请使用 BuildingUpgradeService::tryUpgrade() 代替
     * @return 是否升级成功
     */
    virtual bool upgrade();
    
    /**
     * @brief 检查是否可以升级（❗️ 已弃用，请使用 BuildingUpgradeService::canUpgrade()）
     * @deprecated 请使用 BuildingUpgradeService::canUpgrade() 代替
     * @return 是否可以升级
     */
    virtual bool canUpgrade() const;
    /** @brief 是否正在升级中 */
    bool isUpgrading() const { return _isUpgrading; }
    /** @brief 设置升级状态 */
    void setUpgrading(bool upgrading) { _isUpgrading = upgrading; }
    
    /**
     * @brief 升级完成时调用（由 UpgradeManager 调用）
     */
    void onUpgradeComplete();
    
    /**
     * @brief 获取升级进度（0.0 ~ 1.0）
     */
    float getUpgradeProgress() const;
    
    /**
     * @brief 获取升级剩余时间（秒）
     */
    float getUpgradeRemainingTime() const;
    // ==================== 升级回调 ====================
    using UpgradeCallback = std::function<void(bool success, int newLevel)>;
    /** @brief 设置升级回调 */
    void setUpgradeCallback(const UpgradeCallback& callback) { _upgradeCallback = callback; }
    // ==================== 网格位置 ====================
    void setGridPosition(const cocos2d::Vec2& pos) { _gridPosition = pos; }
    cocos2d::Vec2 getGridPosition() const { return _gridPosition; }
    void setGridSize(const cocos2d::Size& size) { _gridSize = size; }
    cocos2d::Size getGridSize() const { return _gridSize; }
    // ==================== 每帧更新 ====================
    virtual void tick(float dt) {}

protected:
    /**
     * @brief 初始化建筑
     * @param level 初始等级
     */
    virtual bool init(int level);
    /**
     * @brief 初始化建筑（带图片）
     * @param level 初始等级
     * @param imageFile 图片文件
     */
    virtual bool init(int level, const std::string& imageFile);
    /** @brief 升级时调用，子类可重写 */
    virtual void onLevelUp();
    /** @brief 根据等级获取图片路径，子类需重写 */
    virtual std::string getImageForLevel(int level) const { return ""; }
    /** @brief 更新建筑外观 */
    virtual void updateAppearance();

protected:
    int _level = 1;              // 当前等级
    bool _isUpgrading = false;   // 是否正在升级
    cocos2d::Vec2 _gridPosition; // 网格位置
    cocos2d::Size _gridSize;     // 占用网格大小
    UpgradeCallback _upgradeCallback = nullptr;
    
    // 生命值系统
    int _maxHitpoints = 100;     // 最大生命值
    int _currentHitpoints = 100; // 当前生命值
};