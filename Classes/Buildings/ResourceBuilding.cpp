/**
 * @file ResourceBuilding.cpp
 * @brief 资源生产/存储建筑实现
 */
#include "ResourceBuilding.h"
#include "../Managers/ResourceManager.h"
#include "../UI/ResourceCollectionUI.h"
#include "../Managers/BuildingCapacityManager.h"
#include "cocos2d.h"
#include "../Managers/ResourceCollectionManager.h"
USING_NS_CC;

// ==================== 生产型建筑数据表 ====================
// 金矿/圣水收集器每10秒产量（15级）
// 产量公式：200 + (等级-1)*100
// 1级: 200，2级: 300，3级: 400，...，15级: 1600
static const int PRODUCTION_PER_CYCLE[] = {0,    200,  300,  400,  500,  600,  700,  800,
                                       900,  1000, 1100, 1200, 1300, 1400, 1500, 1600};

// 生产型建筑内部存储容量（15级）
static const int PRODUCER_CAPACITIES[] = {0,    500,   1000,  1500,  2000,  3000,  4000,  5000,
                                          7500, 10000, 15000, 20000, 30000, 50000, 75000, 100000};

// ==================== 存储型建筑数据表 ====================
// 金币仓库/圣水仓库存储容量（14-17级，根据实际素材调整）
static const int STORAGE_CAPACITIES[] = {0,      1500,   3000,   6000,   12000,  25000,  45000,  100000,
                                         150000, 200000, 250000, 300000, 400000, 500000, 750000, 1000000,
                                         1500000, 2000000};

// ==================== 升级费用表 ====================
static const int UPGRADE_COSTS[] = {0,     150,   300,    700,    1400,   3000,   7000,    14000,
                                    28000, 56000, 100000, 200000, 400000, 800000, 1500000, 3000000,
                                    6000000, 0};

ResourceBuilding* ResourceBuilding::create(ResourceBuildingType buildingType, int level)
{
    ResourceBuilding* ret = new (std::nothrow) ResourceBuilding();
    if (ret)
    {
        ret->_buildingType = buildingType;
        
        // 根据建筑类型设置资源类型
        switch (buildingType)
        {
        case ResourceBuildingType::kGoldMine:
        case ResourceBuildingType::kGoldStorage:
            ret->_resourceType = kGold;
            break;
        case ResourceBuildingType::kElixirCollector:
        case ResourceBuildingType::kElixirStorage:
            ret->_resourceType = kElixir;
            break;
        }
        
        if (ret->init(level))
        {
            ret->autorelease();
            return ret;
        }
    }
    delete ret;
    return nullptr;
}

bool ResourceBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(3, 3);
    _currentStorage = 0;
    _productionAccumulator = 0.0f;
    std::string imageFile = getImageFile();
    if (!Sprite::initWithFile(imageFile))
        return false;
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    
    // 创建存储量标签
    _storageLabel = Label::createWithSystemFont("0", "Arial", 12);
    _storageLabel->setPosition(Vec2(this->getContentSize().width / 2, -10));
    _storageLabel->setTextColor(Color4B::WHITE);
    _storageLabel->setVisible(false);
    this->addChild(_storageLabel, 100);
    
    // ✅ 为生产型建筑创建收集UI
    if (isProducer())
    {
        auto collectionUI = ResourceCollectionUI::create(const_cast<ResourceBuilding*>(this));
        if (collectionUI)
        {
            collectionUI->setName("collectionUI");
            this->addChild(collectionUI, 1000);
            // 🔴 关键修复：必须向管理器注册自己，否则管理器不知道这个建筑的存在！
            ResourceCollectionManager::getInstance()->registerBuilding(this);
            CCLOG("✅ 为 %s 创建了收集UI", getDisplayName().c_str());
        }
    }
    
    return true;
}

std::string ResourceBuilding::getDisplayName() const
{
    std::string typeName;
    switch (_buildingType)
    {
    case ResourceBuildingType::kGoldMine:
        typeName = "金矿";
        break;
    case ResourceBuildingType::kElixirCollector:
        typeName = "圣水收集器";
        break;
    case ResourceBuildingType::kGoldStorage:
        typeName = "金币仓库";
        break;
    case ResourceBuildingType::kElixirStorage:
        typeName = "圣水仓库";
        break;
    default:
        typeName = "资源建筑";
        break;
    }
    return typeName + " Lv." + std::to_string(_level);
}

int ResourceBuilding::getMaxLevel() const
{
    // 根据建筑类型返回对应的最大等级（与实际素材数量一致）
    switch (_buildingType)
    {
    case ResourceBuildingType::kGoldMine:
        return 15;  // 金矿：15级
    case ResourceBuildingType::kElixirCollector:
        return 15;  // 圣水收集器：15级
    case ResourceBuildingType::kGoldStorage:
        return 14;  // 金币仓库：14级
    case ResourceBuildingType::kElixirStorage:
        return 17;  // 圣水仓库：17级
    default:
        return 14;
    }
}

std::string ResourceBuilding::getImageFile() const
{
    return getImageForLevel(_level);
}

std::string ResourceBuilding::getImageForLevel(int level) const
{
    // 根据建筑类型和等级返回对应的图片路径
    switch (_buildingType)
    {
    case ResourceBuildingType::kGoldMine:
        return "buildings/GoldMine/Gold_Mine" + std::to_string(level) + ".png";
    case ResourceBuildingType::kElixirCollector:
        return "buildings/ElixirCollector/Elixir_Collector" + std::to_string(level) + ".png";
    case ResourceBuildingType::kGoldStorage:
        return "buildings/GoldStorage/Gold_Storage" + std::to_string(level) + ".png";
    case ResourceBuildingType::kElixirStorage:
        return "buildings/ElixirStorage/Elixir_Storage" + std::to_string(level) + ".png";
    default:
        return "buildings/GoldMine/Gold_Mine1.png";
    }
}

bool ResourceBuilding::isProducer() const
{
    return _buildingType == ResourceBuildingType::kGoldMine || 
           _buildingType == ResourceBuildingType::kElixirCollector;
}

bool ResourceBuilding::isStorage() const
{
    return _buildingType == ResourceBuildingType::kGoldStorage || 
           _buildingType == ResourceBuildingType::kElixirStorage;
}

int ResourceBuilding::getProductionRate() const
{
    if (!isProducer()) return 0;
    // 防止数组越界
    int index = std::min(_level, (int)(sizeof(PRODUCTION_PER_CYCLE) / sizeof(int) - 1));
    return PRODUCTION_PER_CYCLE[index];
}

int ResourceBuilding::getStorageCapacity() const
{
    // 🔴 关键修复1：将生产型建筑的内部存储容量设置为当前等级的单次产量
    if (isProducer())
    {
        // 确保数组不越界
        int maxIndex = sizeof(PRODUCTION_PER_CYCLE) / sizeof(int) - 1;
        int index = std::min(_level, maxIndex);
        return PRODUCTION_PER_CYCLE[index];
    }

    // 存储型建筑：保持原有逻辑，读取 STORAGE_CAPACITIES (如果定义了的话)
    if (isStorage())
    {
        int maxIndex = sizeof(STORAGE_CAPACITIES) / sizeof(int) - 1;
        int index = std::min(_level, maxIndex);

        if (index < 1) return 0;

        return STORAGE_CAPACITIES[index];
    }

    return 0;
}
int ResourceBuilding::getUpgradeCost() const
{
    int maxLevel = getMaxLevel();
    if (_level < 1 || _level >= maxLevel)
        return 0;
    
    // 确保不会越界访问
    int maxIndex = sizeof(UPGRADE_COSTS) / sizeof(UPGRADE_COSTS[0]) - 1;
    if (_level > maxIndex)
        return 0;
    
    return UPGRADE_COSTS[_level];
}

bool ResourceBuilding::upgrade()
{
    if (!canUpgrade())
        return false;
    int cost = getUpgradeCost();
    auto& rm = ResourceManager::getInstance();
    if (!rm.hasEnough(kGold, cost))
        return false;
    if (!rm.consume(kGold, cost))
        return false;
    _level++;
    updateAppearance();
    if (_upgradeCallback)
    {
        _upgradeCallback(true, _level);
    }
    return true;
}

void ResourceBuilding::tick(float dt)
{
    // 只有生产型建筑需要生产资源
    if (!isProducer()) return;

    // 🔴 修复点1：如果存储已满，立即返回，不累加时间，停止生产。
    if (isStorageFull())
    {
        // 确保 UI 显示满仓状态
        auto collectionUI = getCollectionUI();
        if (collectionUI)
        {
            collectionUI->updateReadyStatus(_currentStorage);
        }
        return;
    }

    // 累加时间
    _productionAccumulator += dt;

    // 每 15 秒生成一次资源
    const float PRODUCTION_INTERVAL = 15.0f;

    if (_productionAccumulator >= PRODUCTION_INTERVAL)
    {
        // 🔴 修复点2：扣除周期时间（保留多余时间，防止误差累积）
        _productionAccumulator -= PRODUCTION_INTERVAL;

        // 获取当前等级的单次产量
        int productionAmount = getProductionRate();
        int capacity = getStorageCapacity();

        // 增加资源，不超过容量
        int prevStorage = _currentStorage;
        _currentStorage = std::min(_currentStorage + productionAmount, capacity);

        // 如果资源增加了，更新UI显示
        if (_currentStorage > prevStorage)
        {
            CCLOG("💰 %s 产出资源：%d (当前库存: %d)", getDisplayName().c_str(), productionAmount, _currentStorage);

            // 获取并更新收集UI
            auto collectionUI = getCollectionUI();
            if (collectionUI)
            {
                collectionUI->updateReadyStatus(_currentStorage);
            }
        }

        // 🔴 修复点3：如果这次生产导致满仓，重置累加器并停止计时，等待收集。
        if (isStorageFull())
        {
            _productionAccumulator = 0.0f;
            CCLOG("⚠️ %s 已满仓，停止生产。", getDisplayName().c_str());
        }
    }
}

int ResourceBuilding::collect()
{
    int collected = _currentStorage;
    if (collected <= 0) return 0;

    // 清空库存
    _currentStorage = 0;

    // 🔴 关键修复2：收集后，将生产计时器重置为 0，立即开始新的生产周期。
    // 这确保了收集完成后，下次生产是在 15 秒后，而不是剩余的几秒后。
    _productionAccumulator = 0.0f;

    // 获取并更新收集UI (清空图标)
    auto collectionUI = getCollectionUI();
    if (collectionUI)
    {
        collectionUI->updateReadyStatus(0); // 设置为 0，隐藏图标
    }

    // ... (播放动画、日志等保持不变) ...

    return collected;
}

void ResourceBuilding::updateAppearance()
{
    auto texture = Director::getInstance()->getTextureCache()->addImage(getImageFile());
    if (texture)
    {
        this->setTexture(texture);
        this->setName(getDisplayName());
    }
}

void ResourceBuilding::showCollectHint()
{
    // ✅ 新的实现：使用 ResourceCollectionUI 显示提示
    auto collectionUI = this->getChildByName<ResourceCollectionUI*>("collectionUI");
    if (collectionUI)
    {
        collectionUI->updateReadyStatus(_currentStorage);
    }
    else
    {
        // 降级方案：显示简单的黄色感叹号
        auto hint = this->getChildByName("collectHint");
        if (!hint)
        {
            auto hintLabel = Label::createWithSystemFont("!", "Arial", 20);
            hintLabel->setName("collectHint");
            hintLabel->setPosition(Vec2(this->getContentSize().width / 2, this->getContentSize().height + 10));
            hintLabel->setTextColor(Color4B::YELLOW);
            this->addChild(hintLabel, 100);
            auto blink = RepeatForever::create(Blink::create(1.0f, 2));
            hintLabel->runAction(blink);
        }
    }
}

void ResourceBuilding::hideCollectHint()
{
    auto hint = this->getChildByName("collectHint");
    if (hint)
    {
        hint->removeFromParent();
    }
}

// ==================== 新增方法 ====================

ResourceCollectionUI* ResourceBuilding::getCollectionUI() const
{
    if (!isProducer())
        return nullptr;
    
    return this->getChildByName<ResourceCollectionUI*>("collectionUI");
}
void ResourceBuilding::onLevelUp()
{
    // 1. 调用基类逻辑（基类会处理外观更新等基础工作）
    BaseBuilding::onLevelUp();

    // 2. 如果是存储型建筑，通知 Capacity Manager 重新计算容量
    if (isStorage())
    {
        // 注意：这里调用的是我们刚写的 BuildingCapacityManager
        BuildingCapacityManager::getInstance().registerOrUpdateBuilding(this, true);

        CCLOG("🎉 %s 升级到 Lv.%d 完成，已通知 CapacityManager 重新计算总容量。",
            getDisplayName().c_str(), _level);
    }

    // (可选) 如果是生产型建筑，在这里也可以处理生产效率变化的逻辑
}
