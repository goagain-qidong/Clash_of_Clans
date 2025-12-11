/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     TrainingUI.cpp
 * File Function: 训练小兵UI界面实现
 * Author:        薛毓哲
 * Update Date:   2025/01/09
 * License:       MIT License
 ****************************************************************/

#include "TrainingUI.h"
#include "ArmyBuilding.h"
#include "ResourceManager.h"

USING_NS_CC;
using namespace ui;

TrainingUI* TrainingUI::create(ArmyBuilding* barracks)
{
    TrainingUI* ui = new (std::nothrow) TrainingUI();
    if (ui && ui->init(barracks))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool TrainingUI::init(ArmyBuilding* barracks)
{
    if (!Node::init())
    {
        return false;
    }

    if (!barracks)
    {
        CCLOG("TrainingUI::init - barracks is null!");
        return false;
    }

    _barracks = barracks;
    
    // 🆕 初始化人口缓存值
    auto& resMgr = ResourceManager::getInstance();
    _lastTroopCount = resMgr.getCurrentTroopCount();
    _lastTroopCapacity = resMgr.getMaxTroopCapacity();
    
    // 🆕 启用每帧更新，用于实时同步人口显示
    this->scheduleUpdate();
    
    setupUI();
    return true;
}

void TrainingUI::setupUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 半透明黑色遮罩层（点击关闭）
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setContentSize(visibleSize);
    mask->setPosition(origin);
    this->addChild(mask, -1);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [this](Touch* touch, Event* event) {
        // 点击遮罩层关闭UI
        this->hide();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, mask);

    // 主面板 - 参考ShopLayer的设计
    _panel = Layout::create();
    _panel->setContentSize(Size(visibleSize.width, 350));
    _panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _panel->setBackGroundColor(Color3B(40, 40, 50));
    _panel->setBackGroundColorOpacity(255);
    _panel->setAnchorPoint(Vec2(0.5f, 0.0f));
    _panel->setPosition(Vec2(visibleSize.width / 2.0f, 0.0f));
    this->addChild(_panel, 10);

    // 阻止点击穿透到遮罩层
    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch* touch, Event* event) {
        return true; // 吞掉触摸事件
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, _panel);

    float panelWidth = _panel->getContentSize().width;
    float panelHeight = _panel->getContentSize().height;

    // 标题
    _titleLabel = Label::createWithSystemFont("🎖 训练士兵", "Microsoft YaHei", 28);
    _titleLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 30));
    _titleLabel->setTextColor(Color4B::YELLOW);
    _panel->addChild(_titleLabel);

    // 人口显示
    auto& resMgr = ResourceManager::getInstance();
    auto popLabel = Label::createWithSystemFont(
        StringUtils::format("人口：%d/%d", 
            resMgr.getCurrentTroopCount(), 
            resMgr.getMaxTroopCapacity()),
        "Microsoft YaHei", 18);
    popLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 60));
    popLabel->setTextColor(Color4B(200, 200, 255, 255));
    popLabel->setName("populationLabel");
    _panel->addChild(popLabel);

    // 关闭按钮
    _closeButton = Button::create();
    _closeButton->setTitleText("X");
    _closeButton->setTitleFontSize(30);
    _closeButton->setPosition(Vec2(panelWidth - 40.0f, panelHeight - 30.0f));
    _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });
    _panel->addChild(_closeButton);

    // 横向滚动列表 - 像ShopLayer一样
    auto scrollView = ListView::create();
    scrollView->setDirection(ui::ScrollView::Direction::HORIZONTAL);
    scrollView->setContentSize(Size(panelWidth - 40.0f, 250.0f));
    scrollView->setPosition(Vec2(20.0f, 20.0f));
    scrollView->setItemsMargin(20.0f);
    scrollView->setName("scrollView");
    _panel->addChild(scrollView);

    // 创建兵种卡片
    createUnitCard(scrollView, UnitType::kBarbarian, "野蛮人", 25, 1);
    createUnitCard(scrollView, UnitType::kArcher, "弓箭手", 50, 1);
    createUnitCard(scrollView, UnitType::kGoblin, "哥布林", 40, 1);
    createUnitCard(scrollView, UnitType::kGiant, "巨人", 250, 5);
    createUnitCard(scrollView, UnitType::kWallBreaker, "炸弹人", 600, 1);
}

void TrainingUI::createUnitCard(cocos2d::ui::ListView* scrollView, UnitType unitType, 
                                const std::string& name, int cost, int housingSpace)
{
    // 创建卡片布局 - 参考ShopLayer的设计
    auto cardLayout = Layout::create();
    cardLayout->setContentSize(Size(180, 220));

    // 背景
    auto bg = LayerColor::create(Color4B(80, 80, 80, 255), 180, 220);
    cardLayout->addChild(bg);

    // 获取兵种图标路径（使用units根目录下的active按钮图片）
    std::string iconPath;
    switch (unitType)
    {
    case UnitType::kBarbarian:
        iconPath = "units/barbarian_select_button_active.png";
        break;
    case UnitType::kArcher:
        iconPath = "units/archer_select_button_active.png";
        break;
    case UnitType::kGoblin:
        iconPath = "units/goblin_select_button_active.png";
        break;
    case UnitType::kGiant:
        iconPath = "units/giant_select_button_active.png";
        break;
    case UnitType::kWallBreaker:
        iconPath = "units/wallbreaker_select_button_active.png";
        break;
    }

    // 兵种图标
    auto icon = Sprite::create(iconPath);
    if (icon)
    {
        float scale = 100.0f / icon->getContentSize().width;
        icon->setScale(scale);
        icon->setPosition(Vec2(90, 150));
        cardLayout->addChild(icon);
    }

    // 兵种名称
    auto nameLabel = Label::createWithSystemFont(name, "Microsoft YaHei", 20);
    nameLabel->setPosition(Vec2(90, 90));
    nameLabel->setTextColor(Color4B::WHITE);
    cardLayout->addChild(nameLabel);

    // 人口占用
    auto popLabel = Label::createWithSystemFont(
        StringUtils::format("👤 %d", housingSpace),
        "Microsoft YaHei", 16);
    popLabel->setPosition(Vec2(90, 65));
    popLabel->setTextColor(Color4B(200, 200, 255, 255));
    cardLayout->addChild(popLabel);

    // 检查是否可以训练
    auto& resMgr = ResourceManager::getInstance();
    bool canAfford = resMgr.hasEnough(ResourceType::kElixir, cost);
    bool hasSpace = resMgr.hasTroopSpace(housingSpace);
    bool canTrain = canAfford && hasSpace;

    // 费用显示
    auto elixirIcon = Sprite::create("icon/Elixir.png");
    if (elixirIcon)
    {
        elixirIcon->setScale(0.4f);
        elixirIcon->setPosition(Vec2(50, 35));
        cardLayout->addChild(elixirIcon);
    }

    auto costLabel = Label::createWithSystemFont(std::to_string(cost), "Microsoft YaHei", 18);
    costLabel->setPosition(Vec2(110, 35));
    costLabel->setTextColor(canAfford ? Color4B::GREEN : Color4B::RED);
    cardLayout->addChild(costLabel);

    // 如果不能训练，显示原因
    if (!hasSpace)
    {
        auto lockLabel = Label::createWithSystemFont("人口已满", "Microsoft YaHei", 16);
        lockLabel->setTextColor(Color4B::RED);
        lockLabel->setPosition(Vec2(90, 10));
        cardLayout->addChild(lockLabel);
        
        // 图标变灰
        if (icon) icon->setColor(Color3B::GRAY);
    }
    else if (!canAfford)
    {
        auto lockLabel = Label::createWithSystemFont("圣水不足", "Microsoft YaHei", 16);
        lockLabel->setTextColor(Color4B::RED);
        lockLabel->setPosition(Vec2(90, 10));
        cardLayout->addChild(lockLabel);
    }
    else
    {
        // 可以训练，显示"点击训练"
        auto trainLabel = Label::createWithSystemFont("点击训练", "Microsoft YaHei", 16);
        trainLabel->setTextColor(Color4B::YELLOW);
        trainLabel->setPosition(Vec2(90, 10));
        cardLayout->addChild(trainLabel);
    }

    // 添加点击事件
    if (canTrain)
    {
        cardLayout->setTouchEnabled(true);
        cardLayout->addClickEventListener([this, unitType](Ref*) {
            onTrainButtonClicked(unitType);
            // 训练后更新UI
            updatePopulationDisplay();
        });
    }

    scrollView->pushBackCustomItem(cardLayout);
}

void TrainingUI::onTrainButtonClicked(UnitType unitType)
{
    if (!_barracks)
    {
        CCLOG("TrainingUI::onTrainButtonClicked - barracks is null!");
        return;
    }

    std::string unitName = getUnitName(unitType);
    CCLOG("点击训练按钮：%s", unitName.c_str());

    // 调用兵营的添加训练任务方法
    bool success = _barracks->addTrainingTask(unitType);

    if (success)
    {
        CCLOG("✅ 成功添加训练任务：%s", unitName.c_str());
        
        // 更新人口显示
        updatePopulationDisplay();
        
        // 重新加载卡片以更新状态
        auto scrollView = dynamic_cast<cocos2d::ui::ListView*>(_panel->getChildByName("scrollView"));
        if (scrollView)
        {
            scrollView->removeAllItems();
            // 重新创建所有卡片
            createUnitCard(scrollView, UnitType::kBarbarian, "野蛮人", 25, 1);
            createUnitCard(scrollView, UnitType::kArcher, "弓箭手", 50, 1);
            createUnitCard(scrollView, UnitType::kGoblin, "哥布林", 40, 1);
            createUnitCard(scrollView, UnitType::kGiant, "巨人", 250, 5);
            createUnitCard(scrollView, UnitType::kWallBreaker, "炸弹人", 600, 1);
        }
    }
    else
    {
        CCLOG("❌ 添加训练任务失败：%s (资源不足或队列已满或人口已满)", unitName.c_str());
    }
}

void TrainingUI::updatePopulationDisplay()
{
    auto& resMgr = ResourceManager::getInstance();
    auto popLabel = dynamic_cast<Label*>(_panel->getChildByName("populationLabel"));
    if (popLabel)
    {
        int current = resMgr.getCurrentTroopCount();
        int max = resMgr.getMaxTroopCapacity();
        popLabel->setString(StringUtils::format("人口：%d/%d", current, max));
        
        // 🎨 根据人口比例改变颜色
        if (current >= max)
        {
            popLabel->setTextColor(Color4B::RED);  // 满了，红色警告
        }
        else if (current >= max * 0.8f)
        {
            popLabel->setTextColor(Color4B::YELLOW);  // 快满了，黄色提示
        }
        else
        {
            popLabel->setTextColor(Color4B(200, 200, 255, 255));  // 正常，蓝色
        }
    }
}

void TrainingUI::onCloseClicked()
{
    hide();
}

void TrainingUI::show()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    this->setVisible(true);
    _panel->setPosition(Vec2(visibleSize.width / 2, -350));
    _panel->runAction(MoveTo::create(0.3f, Vec2(visibleSize.width / 2, 0)));
    
    // 🆕 显示时立即更新一次人口显示
    updatePopulationDisplay();
    
    CCLOG("✅ TrainingUI: 已启动实时人口同步");
}

void TrainingUI::hide()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto moveDown = MoveTo::create(0.3f, Vec2(visibleSize.width / 2, -350));
    auto callback = CallFunc::create([this]() {
        this->removeFromParent();
    });
    _panel->runAction(Sequence::create(moveDown, callback, nullptr));
}

void TrainingUI::update(float dt)
{
    // 🆕 每帧检查人口是否变化，如果变化则更新显示
    auto& resMgr = ResourceManager::getInstance();
    int currentCount = resMgr.getCurrentTroopCount();
    int currentCapacity = resMgr.getMaxTroopCapacity();
    
    // 只有当人口或容量变化时才更新UI（减少不必要的UI刷新）
    if (currentCount != _lastTroopCount || currentCapacity != _lastTroopCapacity)
    {
        CCLOG("🔄 TrainingUI: 检测到人口变化 %d/%d -> %d/%d",
              _lastTroopCount, _lastTroopCapacity,
              currentCount, currentCapacity);
        
        // 更新缓存值
        _lastTroopCount = currentCount;
        _lastTroopCapacity = currentCapacity;
        
        // 更新UI显示
        updatePopulationDisplay();
        
        // 如果人口变化，可能影响能否训练，需要刷新卡片
        auto scrollView = dynamic_cast<cocos2d::ui::ListView*>(_panel->getChildByName("scrollView"));
        if (scrollView)
        {
            scrollView->removeAllItems();
            createUnitCard(scrollView, UnitType::kBarbarian, "野蛮人", 25, 1);
            createUnitCard(scrollView, UnitType::kArcher, "弓箭手", 50, 1);
            createUnitCard(scrollView, UnitType::kGoblin, "哥布林", 40, 1);
            createUnitCard(scrollView, UnitType::kGiant, "巨人", 250, 5);
            createUnitCard(scrollView, UnitType::kWallBreaker, "炸弹人", 600, 1);
        }
    }
}

std::string TrainingUI::getUnitName(UnitType type) const
{
    switch (type)
    {
    case UnitType::kBarbarian:
        return "野蛮人";
    case UnitType::kArcher:
        return "弓箭手";
    case UnitType::kGiant:
        return "巨人";
    case UnitType::kGoblin:
        return "哥布林";
    case UnitType::kWallBreaker:
        return "炸弹人";
    default:
        return "未知兵种";
    }
}
