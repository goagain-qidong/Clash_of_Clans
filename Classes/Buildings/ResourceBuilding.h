/**
 * @file ResourceBuilding.h
 * @brief 资源生产建筑类（金矿、圣水收集器等）
 */
#ifndef RESOURCE_BUILDING_H_
#define RESOURCE_BUILDING_H_
#include "BaseBuilding.h"

// ✅ 前向声明
class ResourceCollectionUI;

/**
 * @brief 资源建筑类型枚举
 */
enum class ResourceBuildingType
{
    kGoldMine,          // 金矿（生产金币）
    kElixirCollector,   // 圣水收集器（生产圣水）
    kGoldStorage,       // 金币仓库（存储金币）
    kElixirStorage      // 圣水仓库（存储圣水）
};

/**
 * @class ResourceBuilding
 * @brief 资源生产/存储建筑基类
 */
class ResourceBuilding : public BaseBuilding
{
public:
    static ResourceBuilding* create(ResourceBuildingType buildingType, int level = 1);
    // ==================== BaseBuilding 接口实现 ====================
    virtual BuildingType getBuildingType() const override { return BuildingType::kResource; }
    virtual std::string getDisplayName() const override;
    virtual int getMaxLevel() const override;
    virtual int getUpgradeCost() const override;
    virtual ResourceType getUpgradeCostType() const override { return kGold; }
    virtual std::string getImageFile() const override;
    virtual bool upgrade() override;
    virtual void tick(float dt) override;
    // ==================== 资源建筑相关 ====================
    ResourceBuildingType getBuildingSubType() const { return _buildingType; }
    ResourceType getResourceType() const { return _resourceType; }
    bool isProducer() const; // 是否为生产型建筑（金矿/圣水收集器）
    bool isStorage() const;  // 是否为存储型建筑（金币仓库/圣水仓库）
    virtual float getUpgradeTime() const override { return 10.0f; }
    int getProductionRate() const;      // 生产型建筑：每10秒产量
    int getStorageCapacity() const;     // 存储容量（生产型和存储型都有）
    int getCurrentStorage() const { return _currentStorage; }
    int collect();                      // 收集资源
    bool isStorageFull() const { return _currentStorage >= getStorageCapacity(); }
    
    // ✅ 新增：获取收集UI
    ResourceCollectionUI* getCollectionUI() const;

protected:
    ResourceBuilding() = default;
    virtual bool init(int level) override;
    virtual void onLevelUp() override;
    virtual void updateAppearance() override;
    virtual std::string getImageForLevel(int level) const override;
    void showCollectHint();
    void hideCollectHint();

private:
    ResourceBuildingType _buildingType = ResourceBuildingType::kGoldMine;
    ResourceType _resourceType = kGold;
    int _currentStorage = 0;
    float _productionAccumulator = 0.0f;
    cocos2d::Label* _storageLabel = nullptr;
};
#endif // RESOURCE_BUILDING_H_