/**
 * @file ResourceBuilding.cpp
 * @brief 资源生产/存储建筑实现
 */
#include "ResourceBuilding.h"
USING_NS_CC;

// ==================== 生产型建筑数据表 ====================
// 金矿/圣水收集器每小时产量（15级）
static const int PRODUCTION_RATES[] = {0,    200,  400,  600,  800,  1000, 1200, 1400,
                                       1600, 1800, 2000, 2200, 2400, 2600, 2800, 3000};

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
    _storageLabel = Label::createWithSystemFont("0", "Arial", 12);
    _storageLabel->setPosition(Vec2(this->getContentSize().width / 2, -10));
    _storageLabel->setTextColor(Color4B::WHITE);
    _storageLabel->setVisible(false);
    this->addChild(_storageLabel, 100);
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
    // 只有生产型建筑有产量
    if (!isProducer())
        return 0;
    
    if (_level < 1 || _level > 15)
        return 0;
    return PRODUCTION_RATES[_level];
}
int ResourceBuilding::getStorageCapacity() const
{
    if (_level < 1)
        return 0;
    
    // 生产型建筑：使用内部存储容量表（15级）
    if (isProducer())
    {
        if (_level > 15)
            return 0;
        return PRODUCER_CAPACITIES[_level];
    }
    // 存储型建筑：使用仓库容量表（14-17级）
    else if (isStorage())
    {
        // 根据实际素材数量调整最大等级
        int maxLevel = (_buildingType == ResourceBuildingType::kElixirStorage) ? 17 : 14;
        if (_level > maxLevel)
            return 0;
        return STORAGE_CAPACITIES[_level];
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
    if (!isProducer())
        return;
    
    if (isStorageFull())
        return;
    
    float productionPerSecond = static_cast<float>(getProductionRate()) / 3600.0f;
    _productionAccumulator += productionPerSecond * dt;
    if (_productionAccumulator >= 1.0f)
    {
        int produced = static_cast<int>(_productionAccumulator);
        _productionAccumulator -= static_cast<float>(produced);
        int capacity = getStorageCapacity();
        _currentStorage = std::min(_currentStorage + produced, capacity);
        if (_storageLabel)
        {
            _storageLabel->setString(std::to_string(_currentStorage));
            _storageLabel->setVisible(_currentStorage > 0);
        }
        if (isStorageFull())
        {
            showCollectHint();
        }
    }
}
int ResourceBuilding::collect()
{
    int collected = _currentStorage;
    if (collected <= 0)
        return 0;
    auto& rm = ResourceManager::getInstance();
    int actualAdded = rm.addResource(_resourceType, collected);
    _currentStorage = 0;
    _productionAccumulator = 0.0f;
    if (_storageLabel)
    {
        _storageLabel->setString("0");
        _storageLabel->setVisible(false);
    }
    hideCollectHint();
    auto scaleUp = ScaleTo::create(0.1f, this->getScale() * 1.1f);
    auto scaleDown = ScaleTo::create(0.1f, this->getScale());
    this->runAction(Sequence::create(scaleUp, scaleDown, nullptr));
    return actualAdded;
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
void ResourceBuilding::hideCollectHint()
{
    auto hint = this->getChildByName("collectHint");
    if (hint)
    {
        hint->removeFromParent();
    }
}