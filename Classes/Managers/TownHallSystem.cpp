/**
 * @file TownHallSystem.cpp
 * @brief 大本营系统UI实现
 */
 /****************************************************************
  * Project Name:  Clash_of_Clans
  * File Name:     WallBuilding.cpp
  * File Function: 大本营系统
  * Author:        刘相成
  * Update Date:   2025/12/06
  * License:       MIT License
  ****************************************************************/
#include "TownHallSystem.h"
#include "Buildings/TownHallBuilding.h"
    USING_NS_CC;
// ==================== TownHallUpgradeUI 实现 ====================
TownHallUpgradeUI* TownHallUpgradeUI::create(TownHallBuilding* building)
{
    TownHallUpgradeUI* ret = new (std::nothrow) TownHallUpgradeUI();
    if (ret && ret->init(building))
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
bool TownHallUpgradeUI::init(TownHallBuilding* building)
{
    if (!Node::init())
        return false;
    _building = building;
    setupUI();
    return true;
}
void TownHallUpgradeUI::setupUI()
{
    // 背景面板
    auto bg = LayerColor::create(Color4B(0, 0, 0, 160), 280, 120);
    bg->setAnchorPoint(Vec2(0.5f, 0.5f));
    bg->setPosition(Vec2(0, 0));
    this->addChild(bg);
    // 信息标签：显示升级信息 / 消耗等
    _infoLabel = Label::createWithSystemFont("", "Arial", 18);
    _infoLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    _infoLabel->setPosition(Vec2(-120, 20));
    this->addChild(_infoLabel);
    // 升级按钮
    _upgradeButton = ui::Button::create();
    _upgradeButton->setTitleText("升级");
    _upgradeButton->setTitleFontSize(20);
    _upgradeButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _upgradeButton->setPosition(Vec2(70, -20));
    _upgradeButton->addClickEventListener(CC_CALLBACK_1(TownHallUpgradeUI::onUpgradeClicked, this));
    this->addChild(_upgradeButton);
    // 初始隐藏（通过 show()/hide() 控制）
    this->setVisible(false);
    // 确保屏幕上有资源显示组件并且能实时更新显示
    // 使用静态指针避免为每个弹窗重复创建 ResourceDisplayUI
    static ResourceDisplayUI* s_resourceUI = nullptr;
    if (!s_resourceUI)
    {
        s_resourceUI = ResourceDisplayUI::create();
        s_resourceUI->setPositionAtTopRight();
        s_resourceUI->updateDisplay();
        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
            scene->addChild(s_resourceUI, 1000);
        // lambda 不能捕获静态变量，直接在 lambda 内部访问静态变量即可
        ResourceManager::GetInstance()->SetOnResourceChangeCallback([](ResourceType, int) {
            if (s_resourceUI)
                s_resourceUI->updateDisplay();
        });
    }
}
void TownHallUpgradeUI::show()
{
    this->setVisible(true);
    updateInfo();
}
void TownHallUpgradeUI::hide()
{
    this->setVisible(false);
}
bool TownHallUpgradeUI::isVisible() const
{
    return Node::isVisible();
}
void TownHallUpgradeUI::setPositionNearBuilding(cocos2d::Node* building)
{
    if (building)
    {
        auto pos = building->getPosition();
        this->setPosition(pos.x, pos.y + 100);
    }
}
void TownHallUpgradeUI::onUpgradeClicked(cocos2d::Ref* sender)
{
    if (_building && _building->upgrade())
    {
        if (_upgradeCallback)
            _upgradeCallback(true, _building->getLevel());
        updateInfo();
    }
    else
    {
        if (_upgradeCallback)
            _upgradeCallback(false, _building ? _building->getLevel() : 0);
    }
}
void TownHallUpgradeUI::updateInfo()
{
    if (_building && _infoLabel)
    {
        _infoLabel->setString(_building->getUpgradeInfo());
    }
}
// ==================== ResourceDisplayUI 实现 ====================
ResourceDisplayUI* ResourceDisplayUI::create()
{
    ResourceDisplayUI* ret = new (std::nothrow) ResourceDisplayUI();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
bool ResourceDisplayUI::init()
{
    if (!Node::init())
        return false;
    setupResource(ResourceType::kGold, "💰", Color4B::YELLOW);
    setupResource(ResourceType::kElixir, "💧", Color4B(238, 130, 238, 255));
    setupResource(ResourceType::kGem, "💎", Color4B(0, 191, 255, 255));
    setupResource(ResourceType::kBuilder, "👷", Color4B::WHITE);
    return true;
}
void ResourceDisplayUI::setupResource(ResourceType type, const std::string& icon, const cocos2d::Color4B& color)
{
    ResourceDisplay display;
    display.container = Node::create();
    display.icon = Label::createWithSystemFont(icon, "Arial", 24);
    display.amount = Label::createWithSystemFont("0", "Arial", 20);
    display.amount->setTextColor(color);
    display.container->addChild(display.icon);
    display.container->addChild(display.amount);
    display.amount->setPosition(60, 0);
    // 横向排列：根据已有数量设置容器位置
    int index = static_cast<int>(_displays.size());
    display.container->setPosition(Vec2(0, -index * 40));
    this->addChild(display.container);
    _displays[type] = display;
}
void ResourceDisplayUI::updateDisplay()
{
    auto& rm = ResourceManager::getInstance();
    for (auto& pair : _displays)
    {
        int amount = rm.GetResourceCount(pair.first);
        pair.second.amount->setString(std::to_string(amount));
    }
}
void ResourceDisplayUI::setPositionAtTopLeft()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(100, visibleSize.height - 50);
}
void ResourceDisplayUI::setPositionAtTopRight()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(visibleSize.width - 200, visibleSize.height - 50);
}
void ResourceDisplayUI::setCustomPosition(const cocos2d::Vec2& position)
{
    this->setPosition(position);
}
void ResourceDisplayUI::showResource(ResourceType type, bool show)
{
    auto it = _displays.find(type);
    if (it != _displays.end())
    {
        it->second.container->setVisible(show);
    }
}