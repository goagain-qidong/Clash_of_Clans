#include "ShopLayer.h"
#include "Managers/ResourceManager.h"
#include "Managers/BuildingManager.h"
#include "Managers/GameConfig.h"
#include "DraggableMapScene.h" // 为了获取BuildingManager，或者使用单例
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function:  商店界面
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
USING_NS_CC;
using namespace ui;

ShopLayer* ShopLayer::create() {
    ShopLayer* ret = new (std::nothrow) ShopLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ShopLayer::init() {
    if (!Layer::init()) return false;

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

void ShopLayer::initUI() {
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

    this->addChild(_container); // <--- 检查这里是否有分号

    // 标题 (注意：这里全是英文符号)
    auto title = Label::createWithSystemFont("Shop", "Arial", 28);

    // 设置标题位置
    title->setPosition(Vec2(_container->getContentSize().width / 2.0f, _container->getContentSize().height - 30.0f));
    _container->addChild(title);

    // 关闭按钮
    _closeBtn = Button::create();
    _closeBtn->setTitleText("X");
    _closeBtn->setTitleFontSize(30);
    _closeBtn->setPosition(Vec2(_container->getContentSize().width - 40.0f, _container->getContentSize().height - 30.0f));
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

void ShopLayer::loadCategory(const std::string& categoryName) {
    _scrollView->removeAllItems();

    int thLevel = getCurrentTownHallLevel();
    auto& config = GameConfig::getInstance();
    auto buildings = config.getAllBuildings();

    for (const auto& cfg : buildings) {
        // 构建用于 BuildingManager 的 BuildingData
        BuildingData bData(cfg.name, cfg.iconPath, Size(3, 3), 1.0f, cfg.cost, 0, cfg.costType); // 默认尺寸需要优化

        // 针对特定建筑修正尺寸
        if (cfg.name == "Wall" || cfg.name == "城墙") bData.gridSize = Size(1, 1);
        else if (cfg.name == "Town Hall" || cfg.name == "大本营") bData.gridSize = Size(5, 5);
        else if (cfg.name == "Barracks" || cfg.name == "Army Camp" || cfg.name == "兵营" || cfg.name == "军营") bData.gridSize = Size(4, 4);
        else if (cfg.name == "Builder Hut" || cfg.name == "建筑工人小屋") bData.gridSize = Size(2, 2);

        // 将 ConfigItem 传入 createShopItem，因为它包含了解锁等级等信息
        auto item = createShopItem(bData);
        _scrollView->pushBackCustomItem(item);
    }
}

cocos2d::ui::Widget* ShopLayer::createShopItem(const BuildingData& data) {
    // 外框
    auto itemLayout = Layout::create();
    itemLayout->setContentSize(Size(180, 220));

    // 背景图
    auto bg = LayerColor::create(Color4B(80, 80, 80, 255), 180, 220);
    itemLayout->addChild(bg);

    // 获取限制信息
    auto& config = GameConfig::getInstance();
    int thLevel = getCurrentTownHallLevel();
    int maxCount = config.getMaxBuildingCount(data.name, thLevel);
    int currentCount = getBuildingCount(data.name);
    const auto* cfgItem = config.getBuildingConfig(data.name);

    bool isUnlocked = (thLevel >= cfgItem->unlockTownHallLevel);
    bool isMaxed = (currentCount >= maxCount);
    bool canAfford = ResourceManager::getInstance().HasEnough(data.costType, data.cost);

    // 1. 建筑图标
    auto icon = Sprite::create(data.imageFile);
    if (icon) {
        // 保持图标适应框大小
        float scale = 120.0f / icon->getContentSize().width;
        icon->setScale(scale);
        icon->setPosition(Vec2(90, 140));
        // 如果未解锁或已达上限，置灰
        if (!isUnlocked || isMaxed) {
            icon->setColor(Color3B::GRAY);
        }
        itemLayout->addChild(icon);
    }

    // 2. 数量限制文本 (e.g. 2/5)
    auto countLabel = Label::createWithSystemFont(
        StringUtils::format("%d/%d", currentCount, maxCount), "Arial", 16);
    countLabel->setPosition(Vec2(90, 190));
    itemLayout->addChild(countLabel);

    // 3. 底部信息区 (花费或提示)
    if (!isUnlocked) {
        // 未解锁提示
        auto lockLabel = Label::createWithSystemFont(
            StringUtils::format("TH %d Required", cfgItem->unlockTownHallLevel), "Arial", 18);
        lockLabel->setTextColor(Color4B::RED);
        lockLabel->setPosition(Vec2(90, 40));
        itemLayout->addChild(lockLabel);
    }
    else if (isMaxed) {
        // 已达上限
        auto maxLabel = Label::createWithSystemFont("MAXED", "Arial", 24);
        maxLabel->setTextColor(Color4B::GRAY);
        maxLabel->setPosition(Vec2(90, 40));
        itemLayout->addChild(maxLabel);
    }
    else {
        // 正常购买状态：显示资源图标和价格
        std::string resIconPath = "";
        Color4B priceColor = Color4B::WHITE;

        switch (data.costType) {
        case kGold: resIconPath = "icon/Gold.png"; break;
        case kElixir: resIconPath = "icon/Elixir.png"; break;
        case kGem: resIconPath = "icon/Gem.png"; break;
        }

        if (!canAfford) priceColor = Color4B::RED;

        auto resIcon = Sprite::create(resIconPath);
        if (resIcon) {
            resIcon->setScale(0.4f);
            resIcon->setPosition(Vec2(50, 40));
            itemLayout->addChild(resIcon);
        }

        auto priceLabel = Label::createWithSystemFont(std::to_string(data.cost), "Arial", 20);
        priceLabel->setPosition(Vec2(100, 40));
        priceLabel->setTextColor(priceColor);
        itemLayout->addChild(priceLabel);

        // 添加点击事件
        itemLayout->setTouchEnabled(true);
        itemLayout->addClickEventListener([this, data](Ref*) {
            // 触发建造
            auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
            if (scene) {
                // 关闭商店
                this->hide();
                // 开始建造流程
                scene->startPlacingBuilding(data);
            }
            });
    }

    return itemLayout;
}

int ShopLayer::getCurrentTownHallLevel() {
    // 从场景的 BuildingManager 获取大本营等级
    // 这里需要一种方式访问数据，简单起见，假设场景有接口
    // 或者遍历 BuildingManager 的建筑列表找大本营
    auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
    if (scene) {
        return scene->getTownHallLevel();
    }
    return 1; // 默认1级
}

int ShopLayer::getBuildingCount(const std::string& name) {
    auto scene = dynamic_cast<DraggableMapScene*>(Director::getInstance()->getRunningScene());
    if (scene) {
        return scene->getBuildingCount(name);
    }
    return 0;
}

void ShopLayer::show() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    this->setVisible(true);
    _container->setPosition(Vec2(visibleSize.width / 2, -350));
    _container->runAction(MoveTo::create(0.3f, Vec2(visibleSize.width / 2, 0)));
}

void ShopLayer::hide() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto moveDown = MoveTo::create(0.3f, Vec2(visibleSize.width / 2, -350));
    auto callback = CallFunc::create([this]() {
        this->removeFromParent();
        });
    _container->runAction(Sequence::create(moveDown, callback, nullptr));
}