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
    // 大本营等级配置表（1 - 17级）
    // 格式: {等级, 生命值, 升级费用, 升级时间(秒), 经验值, 最大建筑数, 最大陷阱数, 图片路径, 描述}
    _levels = {
        {1,  400,   0,          0,      0,   13, 0,  "buildings/BaseCamp/town-hall-1.png",  "初始大本营"},
        {2,  800,   1000,       10,     3,   17, 0,  "buildings/BaseCamp/town-hall-2.png",  "二级大本营"},
        {3,  1600,  4000,       10,     42,  25, 2,  "buildings/BaseCamp/town-hall-3.png",  "三级大本营"},
        {4,  2000,  8000,      10,     103, 29, 4,  "buildings/BaseCamp/town-hall-4.png",  "四级大本营"},
        {5,  2400,  16000,     10,     146, 36, 8,  "buildings/BaseCamp/town-hall-5.png",  "五级大本营"},
        {6,  2800,  32000,     10,     207, 42, 11, "buildings/BaseCamp/town-hall-6.png",  "六级大本营"},
        {7,  3300,  64000,    10,     254, 54, 15, "buildings/BaseCamp/town-hall-7.png",  "七级大本营"},
        {8,  3900,  128000,    10,     293, 64, 23, "buildings/BaseCamp/town-hall-8.png",  "八级大本营"},
        {9,  4600,  256000,    10,     415, 77, 26, "buildings/BaseCamp/town-hall-9.png",  "九级大本营"},
        {10, 5500,  512000,    10,     509, 84, 30, "buildings/BaseCamp/town-hall-10.png", "十级大本营"},
        {11, 6800,  1024000,    10,     657, 89, 31, "buildings/BaseCamp/town-hall-11.png", "十一级大本营"},
        {12, 7500,  1100000,    10,     720, 92, 36, "buildings/BaseCamp/town-hall-12.png", "十二级大本营"},
        {13, 8200,  1200000,    10,     777, 94, 39, "buildings/BaseCamp/town-hall-13.png", "十三级大本营"},
        {14, 8900,  1300000,   10,     804, 95, 44, "buildings/BaseCamp/town-hall-14.png", "十四级大本营"},
        {15, 9600,  1400000,   10,     831, 98, 44, "buildings/BaseCamp/town-hall-15.png", "十五级大本营"},
        {16, 10000, 1500000,   10,     881, 94, 44, "buildings/BaseCamp/town-hall-16.png", "十六级大本营"},
        {17, 10400, 1600000,   10,     929, 94, 47, "buildings/BaseCamp/town-hall-17.png", "满级大本营"}
    };
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
    // 1. 设置等级
    _level = std::max(1, std::min(level, getMaxLevel()));

    // 2. 设置网格大小 (大本营通常是 4x4 或 5x5)
    _gridSize = cocos2d::Size(5, 5);

    // 3. 初始化 Sprite (加载图片)
    std::string imageFile = getImageForLevel(_level);
    if (!Sprite::initWithFile(imageFile))
    {
        // 如果找不到对应等级的图，尝试加载 1 级的图作为保底
        CCLOG("TownHallBuilding: Failed to load %s, trying fallback...", imageFile.c_str());
        if (!Sprite::initWithFile("buildings/BaseCamp/town-hall-1.png")) {
            return false;
        }
    }

    // 4. 设置锚点 (适配 Isometric 视角)
    this->setAnchorPoint(Vec2(0.5f, 0.35f));
    this->setScale(0.8f);
    this->setName(getDisplayName());
    
    // 5. 🆕 设置生命值（从配置表读取）
    auto* levelConfig = TownHallConfig::getInstance()->getLevel(_level);
    if (levelConfig)
    {
        setMaxHitpoints(levelConfig->hitpoints);
        CCLOG("✅ 大本营 Lv.%d 生命值：%d", _level, levelConfig->hitpoints);
    }

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

void TownHallBuilding::onLevelUp()
{
    // 🔧 修复：不再重写 upgrade()，而是在升级完成后处理逻辑
    BaseBuilding::onLevelUp();  // 调用基类的升级逻辑（更新外观）
    
    // 更新生命值
    auto* levelConfig = TownHallConfig::getInstance()->getLevel(_level);
    if (levelConfig)
    {
        setMaxHitpoints(levelConfig->hitpoints);
        CCLOG("🎉 大本营升级到 Lv.%d，生命值提升至：%d", _level, levelConfig->hitpoints);
    }
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

// ==================== 建筑限制系统（空实现，预留未来）====================
int TownHallConfig::getMaxBuildingLevel(int townHallLevel, const std::string& buildingName) const
{
    // TODO: 实现建筑等级限制逻辑
    // 示例：大本营7级时，箭塔最高只能升到7级
    // 当前返回 townHallLevel，表示建筑等级不能超过大本营等级
    CCLOG("[TODO] TownHallConfig::getMaxBuildingLevel - 建筑等级限制系统尚未实现");
    return townHallLevel;
}

// ==================== 建筑解锁系统（空实现，预留未来）====================
bool TownHallConfig::isBuildingUnlocked(int townHallLevel, const std::string& buildingName) const
{
    // TODO: 实现建筑解锁逻辑
    // 示例：大本营3级解锁迫击炮，大本营7级解锁野蛮人之王
    // 当前默认返回 true，表示所有建筑都已解锁
    CCLOG("[TODO] TownHallConfig::isBuildingUnlocked - 建筑解锁系统尚未实现");
    return true;
}