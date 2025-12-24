/**
* @file ResourceBuilding.cpp
* @brief 资源生产/存储建筑实现
* @author 赵崇治、薛毓哲
* @date 2025/12/24
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
// 在文件顶部的常量区域添加 HP 数据表
// ==================== 生命值数据表 ====================
// 生产设施 (金矿/圣水收集器) 生命值 (1-15级)
static const int PRODUCER_HP[] = {0, 400, 450, 500, 550, 600, 640, 680, 720, 780, 840, 900, 960, 1020, 1080, 1180};

// 存储设施 (金库/圣水瓶) 生命值 (1-17级)
static const int STORAGE_HP[] = {0,    600,  700,  800,  900,  1000, 1200, 1300, 1400,
                                 1600, 1800, 2100, 2400, 2700, 3000, 3400, 3800, 4200};
ResourceBuilding::~ResourceBuilding()
{
    // ✅ 析构时自动从 ResourceCollectionManager 注销
    if (isProducer())
    {
        ResourceCollectionManager::getInstance()->unregisterBuilding(this);
    }
}

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
    
    // 🔴 修复：不在 init 中创建收集UI
    // 收集UI 将在 BuildingManager::loadBuildingsFromData 中根据 isReadOnly 参数决定是否创建
    // 这样战斗场景（isReadOnly=true）就不会显示资源采集框
    
    // ✅ 【新增】根据建筑类型和等级设置生命值
    int hp = 400; // 默认值

    if (isProducer())
    {
        int idx = std::min(_level, (int)(sizeof(PRODUCER_HP) / sizeof(int) - 1));
        hp      = PRODUCER_HP[idx];
    }
    else if (isStorage())
    {
        int idx = std::min(_level, (int)(sizeof(STORAGE_HP) / sizeof(int) - 1));
        hp      = STORAGE_HP[idx];
    }

    // 设置最大生命值（这会自动将当前生命值也设为满血）
    setMaxHitpoints(hp);

    CCLOG("🏗️ %s 初始化完成，HP: %d", getDisplayName().c_str(), hp);
    initHealthBarUI();
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

// 新增：根据等级返回升级所需时间（秒）
float ResourceBuilding::getUpgradeTime() const
{
    // 升级时间表（16级）- 单位：秒
    static const int times[] = {
        0,        // Level 0 (无效)
        15,        // Level 1 -> 2: 15秒
        30,       // Level 2 -> 3: 30秒
        60,       // Level 3 -> 4: 1分钟
        120,      // Level 4 -> 5: 2分钟
        300,      // Level 5 -> 6: 5分钟
        900,      // Level 6 -> 7: 15分钟
        1800,     // Level 7 -> 8: 30分钟
        3600,     // Level 8 -> 9: 1小时
        7200,     // Level 9 -> 10: 2小时
        10800,    // Level 10 -> 11: 3小时
        14400,    // Level 11 -> 12: 4小时
        21600,    // Level 12 -> 13: 6小时
        28800,    // Level 13 -> 14: 8小时
        36000,    // Level 14 -> 15: 10小时
        64800,    // Level 15 -> 16: 18小时
        172800    // Level 16 -> 17: 2天
    };
    
    if (_level < 1 || _level > 16)
        return 0;
    
    return times[_level];
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
    // 如果没有资源可收集
    if (_currentStorage <= 0) return 0;

    int buildingCapacity = getStorageCapacity();  // 建筑内部容量
    int collected = _currentStorage;              // 默认收集当前积累的资源

    // ========== 关键改动：点击后增加生成数量，而不是直接填满 =========
    // 无论是否满仓，都只收集建筑当前储存的资源
    // 这样每次点击都能获得增量，符合《部落冲突》的游戏逻辑
    
    CCLOG("💰 %s 收集资源：获得 %d", 
          getDisplayName().c_str(), collected);

    // 清空库存（准备下一个生产周期）
    _currentStorage = 0;
    _productionAccumulator = 0.0f;

    // 更新 UI 状态
    auto collectionUI = getCollectionUI();
    if (collectionUI)
    {
        collectionUI->updateReadyStatus(0);  // 隐藏收集图标
    }

    // 记录日志
    CCLOG("✅ %s 收集完成，返回给玩家：%d 资源", 
          getDisplayName().c_str(), collected);

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
    // 1. 不调用基类 onLevelUp()，避免 getStaticConfig 返回错误的图片路径
    //    ResourceBuilding 有自己的图片路径逻辑
    
    // 2. 强制更新纹理，确保外观改变
    std::string newImageFile = getImageForLevel(_level);
    CCLOG("🔍 %s 尝试更新外观: level=%d, path=%s", 
          getDisplayName().c_str(), _level, newImageFile.c_str());
    
    if (!newImageFile.empty())
    {
        // 先移除旧纹理缓存，确保加载最新的
        auto textureCache = Director::getInstance()->getTextureCache();
        auto texture = textureCache->addImage(newImageFile);
        if (texture)
        {
            this->setTexture(texture);
            // 重新设置纹理后需要更新内容大小
            this->setTextureRect(Rect(0, 0, texture->getContentSize().width, 
                                            texture->getContentSize().height));
            CCLOG("🖼️ %s 外观更新成功: %s (size: %.0fx%.0f)", 
                  getDisplayName().c_str(), newImageFile.c_str(),
                  texture->getContentSize().width, texture->getContentSize().height);
        }
        else
        {
            CCLOG("❌ %s 外观更新失败：无法加载纹理 %s", 
                  getDisplayName().c_str(), newImageFile.c_str());
        }
    }
    
    // 3. 更新生命值（根据新等级）
    int hp = 400;
    if (isProducer())
    {
        static const int PRODUCER_HP_TABLE[] = {0, 400, 450, 500, 550, 600, 640, 680, 720, 780, 840, 900, 960, 1020, 1080, 1180};
        int idx = std::min(_level, (int)(sizeof(PRODUCER_HP_TABLE) / sizeof(int) - 1));
        hp = PRODUCER_HP_TABLE[idx];
    }
    else if (isStorage())
    {
        static const int STORAGE_HP_TABLE[] = {0, 600, 700, 800, 900, 1000, 1200, 1300, 1400, 1600, 1800, 2100, 2400, 2700, 3000, 3400, 3800, 4200};
        int idx = std::min(_level, (int)(sizeof(STORAGE_HP_TABLE) / sizeof(int) - 1));
        hp = STORAGE_HP_TABLE[idx];
    }
    setMaxHitpoints(hp);

    // 4. 如果是存储型建筑，通知 Capacity Manager 重新计算容量
    //    使用 this 指针的弱引用模式确保安全
    if (isStorage())
    {
        // 保存 this 指针用于延迟回调
        ResourceBuilding* self = this;
        ResourceType resType = _resourceType;  // 保存资源类型
        
        this->scheduleOnce([self, resType](float) {
            // 验证建筑仍然有效
            if (self && self->getReferenceCount() > 0 && !self->isDestroyed())
            {
                // 双重验证资源类型
                if (self->getResourceType() == resType)
                {
                    BuildingCapacityManager::getInstance().registerOrUpdateBuilding(self, true);
                    CCLOG("🎉 %s 升级到 Lv.%d 完成，资源类型: %s，已更新容量",
                          self->getDisplayName().c_str(), self->getLevel(),
                          resType == ResourceType::kGold ? "金币" : "圣水");
                }
            }
        }, 0.0f, "capacity_update");
    }

    // 5. 如果是生产型建筑，更新生产效率
    if (isProducer())
    {
        CCLOG("📈 %s 升级到 Lv.%d，产量提升至: %d/周期",
              getDisplayName().c_str(), _level, getProductionRate());
    }
    
    // 6. 更新名称
    this->setName(getDisplayName());
}

// 🆕 新增：初始化资源收集UI（仅在非战斗模式下调用）
void ResourceBuilding::initCollectionUI()
{
    // 只有生产型建筑需要收集UI
    if (!isProducer())
        return;
    
    // 避免重复创建
    if (this->getChildByName("collectionUI"))
        return;
    
    auto collectionUI = ResourceCollectionUI::create(this);
    if (collectionUI)
    {
        collectionUI->setName("collectionUI");
        this->addChild(collectionUI, 1000);
        // 向管理器注册
        ResourceCollectionManager::getInstance()->registerBuilding(this);
        CCLOG("✅ 为 %s 创建了收集UI", getDisplayName().c_str());
    }
}
