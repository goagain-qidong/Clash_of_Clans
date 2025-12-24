/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ResourceBuilding.h
 * File Function: 资源生产建筑类（金矿、圣水收集器等）
 * Author:        赵崇治
 * Update Date:   2025/01/10
 * License:       MIT License
 ****************************************************************/
#ifndef RESOURCE_BUILDING_H_
#define RESOURCE_BUILDING_H_

#include "BaseBuilding.h"

class ResourceCollectionUI;

/**
 * @enum ResourceBuildingType
 * @brief 资源建筑类型枚举
 */
enum class ResourceBuildingType
{
    kGoldMine,         ///< 金矿（生产金币）
    kElixirCollector,  ///< 圣水收集器（生产圣水）
    kGoldStorage,      ///< 金币仓库（存储金币）
    kElixirStorage     ///< 圣水仓库（存储圣水）
};

/**
 * @class ResourceBuilding
 * @brief 资源生产/存储建筑类
 */
class ResourceBuilding : public BaseBuilding
{
public:
    /**
     * @brief 创建资源建筑
     * @param buildingType 资源建筑类型
     * @param level 建筑等级
     * @return ResourceBuilding* 资源建筑指针
     */
    static ResourceBuilding* create(ResourceBuildingType buildingType, int level = 1);

    virtual ~ResourceBuilding();

    /** @brief 获取建筑类型 */
    virtual BuildingType getBuildingType() const override { return BuildingType::kResource; }

    /** @brief 获取显示名称 */
    virtual std::string getDisplayName() const override;

    /** @brief 获取最大等级 */
    virtual int getMaxLevel() const override;

    /** @brief 获取升级费用 */
    virtual int getUpgradeCost() const override;

    /** @brief 获取升级资源类型 */
    virtual ResourceType getUpgradeCostType() const override { return kGold; }

    /** @brief 获取当前图片文件 */
    virtual std::string getImageFile() const override;

    /** @brief 尝试升级 */
    virtual bool upgrade() override;

    /**
     * @brief 每帧更新
     * @param dt 帧时间间隔
     */
    virtual void tick(float dt) override;

    /** @brief 获取资源建筑子类型 */
    ResourceBuildingType getBuildingSubType() const { return _buildingType; }

    /** @brief 获取生产的资源类型 */
    ResourceType getResourceType() const { return _resourceType; }

    /** @brief 是否为生产型建筑 */
    bool isProducer() const;

    /** @brief 是否为存储型建筑 */
    bool isStorage() const;

    /** @brief 获取升级时间 */
    virtual float getUpgradeTime() const override;

    /** @brief 获取生产速率（每10秒产量） */
    int getProductionRate() const;

    /** @brief 获取存储容量 */
    int getStorageCapacity() const;

    /** @brief 获取当前存储量 */
    int getCurrentStorage() const { return _currentStorage; }

    /**
     * @brief 收集资源
     * @return int 收集的资源数量
     */
    int collect();

    /** @brief 存储是否已满 */
    bool isStorageFull() const { return _currentStorage >= getStorageCapacity(); }

    /** @brief 获取收集UI */
    ResourceCollectionUI* getCollectionUI() const;

    /** @brief 初始化资源收集UI */
    void initCollectionUI();

protected:
    ResourceBuilding() = default;
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual void updateAppearance() override;
    virtual std::string getImageForLevel(int level) const override;

    void showCollectHint();  ///< 显示收集提示
    void hideCollectHint();  ///< 隐藏收集提示

private:
    ResourceBuildingType _buildingType = ResourceBuildingType::kGoldMine;  ///< 资源建筑类型
    ResourceType _resourceType = kGold;           ///< 生产的资源类型
    int _currentStorage = 0;                      ///< 当前存储量
    float _productionAccumulator = 0.0f;          ///< 生产累积器
    cocos2d::Label* _storageLabel = nullptr;      ///< 存储量标签
};

#endif // RESOURCE_BUILDING_H_