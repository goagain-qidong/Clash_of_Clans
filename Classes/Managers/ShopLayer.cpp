#include "ShopLayer.h"
#include "DraggableMapScene.h"
#include "Managers/BuildingLimitManager.h"
#include "Managers/BuildingManager.h"
#include "Managers/GameConfig.h"
#include "Managers/ResourceManager.h"

/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ShopLayer.cpp
 * File Function: 商店界面
 * Author:        刘相成
 * Update Date:   2025/12/08
 * License:       MIT License
 ****************************************************************/

USING_NS_CC;
using namespace ui;

ShopLayer* ShopLayer::create()
{
    ShopLayer* ret = new (std::nothrow) ShopLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ShopLayer::init()
{
    if (!Layer::init())
        return false;

    // 半透明背景屏蔽点击
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(bgMask);

    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    initUI();
    return true;
}

void ShopLayer::initUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 使用 ui::Layout 作为容器
    auto bgLayout = ui::Layout::create();
    bgLayout->setContentSize(Size(visibleSize.width, 350));
    bgLayout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    bgLayout->setBackGroundColor(Color3B(40, 40, 50));
    bgLayout->setBackGroundColorOpacity(255);

    _container = bgLayout;

    // 设置锚点和位置
    _container->setAnchorPoint(Vec2(0.5f, 0.0f));
    _container->setPosition(Vec2(visibleSize.width / 2.0f, 0.0f));

    this->addChild(_container);

    // 标题
    auto title = Label::createWithSystemFont("Shop", "Arial", 28);
    title->setPosition(Vec2(_container->getContentSize().width / 2.0f, _container->getContentSize().height - 30.0f));
    _container->addChild(title);

    // 关闭按钮
    _closeBtn = Button::create("return_button.png");
    _closeBtn->setScale9Enabled(false);
    _closeBtn->setPosition(
        Vec2(_container->getContentSize().width - 40.0f, _container->getContentSize().height - 30.0f));
    _closeBtn->addClickEventListener([this](Ref*) { hide(); });
    _container->addChild(_closeBtn);

    // 商品列表
    _scrollView = ListView::create();
    _scrollView->setDirection(ui::ScrollView::Direction::HORIZONTAL);
    _scrollView->setContentSize(Size(visibleSize.width - 40.0f, 250.0f));
    _scrollView->setPosition(Vec2(20.0f, 20.0f));
    _scrollView->setItemsMargin(20.0f);
    _container->addChild(_scrollView);

    // 加载所有商品
    loadCategory("All");
}

void ShopLayer::loadCategory(const std::string& categoryName)
{
    _scrollView->removeAllItems();

    int   thLevel   = getCurrentTownHallLevel();
    auto& config    = GameConfig::getInstance();
    auto  buildings = config.getAllBuildings();

    CCLOG("📦 ShopLayer 加载建筑列表 (TH Lv.%d)", thLevel);

    for (const auto& cfg : buildings)
    {
        // 构建用于 BuildingManager 的 BuildingData
        BuildingData bData(cfg.name, cfg.iconPath, Size(3, 3), 1.0f, cfg.cost, 0, cfg.costType);

        // 针对特定建筑修正尺寸
        if (cfg.name == "Wall" || cfg.name == "城墙")
            bData.gridSize = Size(1, 1);
        else if (cfg.name == "Town Hall" || cfg.name == "大本营")
            bData.gridSize = Size(5, 5);
        else if (cfg.name == "Barracks" || cfg.name == "Army Camp" || cfg.name == "兵营" || cfg.name == "军营")
            bData.gridSize = Size(4, 4);
        else if (cfg.name == "Builder Hut" || cfg.name == "建筑工人小屋")
            bData.gridSize = Size(2, 2);

        auto item = createShopItem(bData);
        _scrollView->pushBackCustomItem(item);
    }
}

cocos2d::ui::Widget* ShopLayer::createShopItem(const BuildingData& data)
{
    // 外框
    auto itemLayout = Layout::create();
    itemLayout->setContentSize(Size(180, 220));

    // 背景图
    auto bg = LayerColor::create(Color4B(80, 80, 80, 255), 180, 220);
    itemLayout->addChild(bg);

    // ========== 关键修复：建筑名称映射 ==========
    // 将 GameConfig 的建筑名称映射到 BuildingLimitManager 的键
    std::string limitKey = data.name;

    // 名称映射表（支持中英文和多种变体）
    if (data.name == "Town Hall" || data.name == "大本营" || data.name == "TownHall")
    {
        limitKey = "TownHall";
    }
    else if (data.name == "城墙" || data.name == "Wall")
    {
        limitKey = "Wall";
    }
    else if (data.name == "建筑工人小屋" || data.name == "Builder Hut" || data.name == "BuildersHut" || data.name == "BuilderHut")
    {
        limitKey = "BuildersHut";
    }
    else if (data.name == "炮塔" || data.name == "Cannon" || data.name == "加农炮")
    {
        limitKey = "Cannon";
    }
    else if (data.name == "箭塔" || data.name == "ArcherTower" || data.name == "Archer Tower")
    {
        limitKey = "ArcherTower";
    }
    else if (data.name == "金矿" || data.name == "GoldMine" || data.name == "Gold Mine")
    {
        limitKey = "GoldMine";
    }
    else if (data.name == "圣水收集器" || data.name == "ElixirCollector" || data.name == "Elixir Collector")
    {
        limitKey = "ElixirCollector";
    }
    else if (data.name == "金币仓库" || data.name == "GoldStorage" || data.name == "Gold Storage")
    {
        limitKey = "GoldStorage";
    }
    else if (data.name == "圣水仓库" || data.name == "ElixirStorage" || data.name == "Elixir Storage")
    {
        limitKey = "ElixirStorage";
    }
    else if (data.name == "兵营" || data.name == "Barracks")
    {
        limitKey = "Barracks";
    }
    else if (data.name == "军营" || data.name == "ArmyCamp" || data.name == "Army Camp")
    {
        limitKey = "ArmyCamp";
    }

    // 获取当前上限（已考虑大本营等级的影响）
    auto* limitMgr = BuildingLimitManager::getInstance();
    int   maxCount = limitMgr->getLimit(limitKey);

    auto& config  = GameConfig::getInstance();
    int   thLevel = getCurrentTownHallLevel();

    // 🔴 修复：直接从 BuildingLimitManager 获取当前数量，而不是通过场景的 getBuildingCount
    int         currentCount = limitMgr->getBuildingCount(limitKey);
    const auto* cfgItem      = config.getBuildingConfig(data.name);

    bool isUnlocked = (thLevel >= cfgItem->unlockTownHallLevel);
    bool isMaxed    = (maxCount != -1 && currentCount >= maxCount);
    bool canAfford  = ResourceManager::getInstance().hasEnough(data.costType, data.cost);

    CCLOG("  建筑: %s (键:%s), 当前: %d, 上限: %s, 已满: %s", data.name.c_str(), limitKey.c_str(), currentCount,
          (maxCount == -1 ? "∞" : std::to_string(maxCount).c_str()), isMaxed ? "是" : "否");

    // 1. 建筑图标
    auto icon = Sprite::create(data.imageFile);
    if (icon)
    {
        float scale = 120.0f / icon->getContentSize().width;
        icon->setScale(scale);
        icon->setPosition(Vec2(90, 140));
        if (!isUnlocked || isMaxed)
        {
            icon->setColor(Color3B::GRAY);
        }
        itemLayout->addChild(icon);
    }

    // 2. 数量限制文本
    std::string countText;
    if (maxCount == -1)
    {
        countText = StringUtils::format("%d/∞", currentCount);
    }
    else
    {
        countText = StringUtils::format("%d/%d", currentCount, maxCount);
    }

    auto countLabel = Label::createWithSystemFont(countText, "Arial", 16);
    countLabel->setPosition(Vec2(90, 190));
    itemLayout->addChild(countLabel);

    // 3. 底部信息区
    if (!isUnlocked)
    {
        auto lockLabel = Label::createWithSystemFont(
            StringUtils::format("TH %d Required", cfgItem->unlockTownHallLevel), "Arial", 18);
        lockLabel->setTextColor(Color4B::RED);
        lockLabel->setPosition(Vec2(90, 40));
        itemLayout->addChild(lockLabel);
    }
    else if (isMaxed)
    {
        auto maxLabel = Label::createWithSystemFont("MAXED", "Arial", 24);
        maxLabel->setTextColor(Color4B::GRAY);
        maxLabel->setPosition(Vec2(90, 40));
        itemLayout->addChild(maxLabel);
    }
    else
    {
        // 正常购买状态
        std::string resIconPath = "";
        Color4B     priceColor  = Color4B::WHITE;

        switch (data.costType)
        {
        case kGold:
            resIconPath = "icon/Gold.png";
            break;
        case kElixir:
            resIconPath = "icon/Elixir.png";
            break;
        case kGem:
            resIconPath = "icon/Gem.png";
            break;
        }

        if (!canAfford)
            priceColor = Color4B::RED;

        auto resIcon = Sprite::create(resIconPath);
        if (resIcon)
        {
            resIcon->setScale(0.4f);
            resIcon->setPosition(Vec2(50, 40));
            itemLayout->addChild(resIcon);
        }

        auto priceLabel = Label::createWithSystemFont(std::to_string(data.cost), "Arial", 20);
        priceLabel->setPosition(Vec2(100, 40));
        priceLabel->setTextColor(priceColor);
        itemLayout->addChild(priceLabel);

        itemLayout->setTouchEnabled(true);
        itemLayout->addClickEventListener([this, data](Ref*) {
            auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
            if (scene)
            {
                this->hide();
                scene->startPlacingBuilding(data);
            }
        });
    }

    return itemLayout;
}

int ShopLayer::getCurrentTownHallLevel()
{
    auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
    if (scene)
    {
        return scene->getTownHallLevel();
    }
    return 1;
}

int ShopLayer::getBuildingCount(const std::string& name)
{
    auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
    if (scene)
    {
        return scene->getBuildingCount(name);
    }
    return 0;
}

void ShopLayer::show()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();

    // ========== 关键改动：打开 Shop 时重新加载所有数据 ==========
    CCLOG("🛒 Shop 打开，刷新建筑上限...");

    // 获取当前大本营等级
    int thLevel = getCurrentTownHallLevel();
    CCLOG("当前大本营等级: Lv.%d", thLevel);

    // 更新 BuildingLimitManager 的大本营等级
    BuildingLimitManager::getInstance()->updateLimitsFromTownHall(thLevel);

    // 重新加载所有建筑商品（这样会重新计算上限）
    loadCategory("All");

    // 显示动画
    this->setVisible(true);
    _container->setPosition(Vec2(visibleSize.width / 2, -350));
    _container->runAction(MoveTo::create(0.3f, Vec2(visibleSize.width / 2, 0)));

    CCLOG("✅ Shop 已刷新");
}

void ShopLayer::hide()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto moveDown    = MoveTo::create(0.3f, Vec2(visibleSize.width / 2, -350));
    _container->runAction(moveDown);

    // 延迟后移除整个层（包括遮罩），而不仅仅是容器
    auto delay      = DelayTime::create(0.3f);
    auto removeSelf = RemoveSelf::create();
    this->runAction(Sequence::create(delay, removeSelf, nullptr));
}