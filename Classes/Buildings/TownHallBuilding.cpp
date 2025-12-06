/**
 * @file TownHallBuilding.cpp
 * @brief 大本营建筑实现
 */
#include "TownHallBuilding.h"
#include "ResourceManager.h"
USING_NS_CC;
// ==================== TownHallConfig 实现 ====================
TownHallConfig* TownHallConfig::getInstance()
{
    static TownHallConfig instance;
    return &instance;
}
TownHallConfig::TownHallConfig()
{
    initialize();
}
void TownHallConfig::initialize()
{
    // 大本营等级配置表（统一限制为14级）
    // 格式: {等级, 图片路径, 升级费用, 升级时间(秒), 描述}
    _levels = {{1, "buildings/BaseCamp/town-hall-1.png", 0, 0, "初始大本营"},
               {2, "buildings/BaseCamp/town-hall-2.png", 1000, 3600, "二级大本营"},
               {3, "buildings/BaseCamp/town-hall-3.png", 4000, 14400, "三级大本营"},
               {4, "buildings/BaseCamp/town-hall-4.png", 25000, 43200, "四级大本营"},
               {5, "buildings/BaseCamp/town-hall-5.png", 150000, 86400, "五级大本营"},
               {6, "buildings/BaseCamp/town-hall-6.png", 500000, 172800, "六级大本营"},
               {7, "buildings/BaseCamp/town-hall-7.png", 1200000, 259200, "七级大本营"},
               {8, "buildings/BaseCamp/town-hall-8.png", 3000000, 345600, "八级大本营"},
               {9, "buildings/BaseCamp/town-hall-9.png", 4000000, 432000, "九级大本营"},
               {10, "buildings/BaseCamp/town-hall-10.png", 6000000, 518400, "十级大本营"},
               {11, "buildings/BaseCamp/town-hall-11.png", 8000000, 604800, "十一级大本营"},
               {12, "buildings/BaseCamp/town-hall-12.png", 10000000, 691200, "十二级大本营"},
               {13, "buildings/BaseCamp/town-hall-13.png", 12000000, 777600, "十三级大本营"},
               {14, "buildings/BaseCamp/town-hall-14.png", 0, 0, "满级大本营"}};
}
const TownHallConfig::LevelData* TownHallConfig::getLevel(int level) const
{
    if (level < 1 || level > static_cast<int>(_levels.size()))
        return nullptr;
    return &_levels[level - 1];
}
const TownHallConfig::LevelData* TownHallConfig::getNextLevel(int currentLevel) const
{
    if (currentLevel < 1 || currentLevel >= static_cast<int>(_levels.size()))
        return nullptr;
    return &_levels[currentLevel];
}
bool TownHallConfig::canUpgrade(int currentLevel) const
{
    return currentLevel >= 1 && currentLevel < static_cast<int>(_levels.size());
}
int TownHallConfig::getUpgradeCost(int currentLevel) const
{
    if (!canUpgrade(currentLevel))
        return 0;
    return _levels[currentLevel].upgradeCost;
}
// ==================== TownHallBuilding 实现 ====================
TownHallBuilding* TownHallBuilding::create(int level)
{
    TownHallBuilding* ret = new (std::nothrow) TownHallBuilding();
    if (ret && ret->init(level))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
bool TownHallBuilding::init(int level)
{
    _level = std::max(1, std::min(level, getMaxLevel()));
    _gridSize = cocos2d::Size(5, 5); // 大本营占用5x5网格
    auto* levelConfig = TownHallConfig::getInstance()->getLevel(_level);
    if (!levelConfig)
        return false;
    if (!Sprite::initWithFile(levelConfig->imageFile))
        return false;
    // 设置锚点和缩放
    this->setAnchorPoint(Vec2(0.5f, 0.5f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    return true;
}
int TownHallBuilding::getMaxLevel() const
{
    return TownHallConfig::getInstance()->getMaxLevel();
}
bool TownHallBuilding::canUpgrade() const
{
    return TownHallConfig::getInstance()->canUpgrade(_level) && !_isUpgrading;
}
int TownHallBuilding::getUpgradeCost() const
{
    return TownHallConfig::getInstance()->getUpgradeCost(_level);
}
float TownHallBuilding::getUpgradeTime() const
{
    auto* nextLevel = TownHallConfig::getInstance()->getNextLevel(_level);
    return nextLevel ? nextLevel->upgradeTime : 0.0f;
}
bool TownHallBuilding::upgrade()
{
    if (!canUpgrade())
        return false;
    int cost = getUpgradeCost();
    auto& rm = ResourceManager::getInstance();
    // 检查并扣除资源
    if (!rm.HasEnough(ResourceType::kGold, cost))
        return false;
    if (!rm.ConsumeResource(ResourceType::kGold, cost))
        return false;
    // 升级成功
    _level++;
    updateAppearance();
    return true;
}
std::string TownHallBuilding::getDisplayName() const
{
    return "大本营 Lv." + std::to_string(_level);
}
std::string TownHallBuilding::getUpgradeInfo() const
{
    if (!canUpgrade())
        return "大本营已满级";
    int cost = getUpgradeCost();
    auto* nextLevel = TownHallConfig::getInstance()->getNextLevel(_level);
    if (!nextLevel)
        return "";
    return "升级到 " + nextLevel->description + "\n需要: " + std::to_string(cost) + " 金币";
}
std::string TownHallBuilding::getImageFile() const
{
    auto* levelConfig = TownHallConfig::getInstance()->getLevel(_level);
    return levelConfig ? levelConfig->imageFile : "";
}
std::string TownHallBuilding::getImageForLevel(int level) const
{
    auto* levelConfig = TownHallConfig::getInstance()->getLevel(level);
    return levelConfig ? levelConfig->imageFile : "";
}
void TownHallBuilding::updateAppearance()
{
    auto* levelConfig = TownHallConfig::getInstance()->getLevel(_level);
    if (!levelConfig)
        return;
    auto texture = Director::getInstance()->getTextureCache()->addImage(levelConfig->imageFile);
    if (texture)
    {
        this->setTexture(texture);
        this->setName(getDisplayName());
    }
}