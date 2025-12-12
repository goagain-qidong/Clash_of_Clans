#include "PlayerListLayer.h"

USING_NS_CC;
using namespace ui;

PlayerListLayer* PlayerListLayer::create(const std::vector<PlayerInfo>& players)
{
    PlayerListLayer* ret = new (std::nothrow) PlayerListLayer();
    if (ret && ret->init(players))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool PlayerListLayer::init(const std::vector<PlayerInfo>& players)
{
    if (!Layer::init())
    {
        return false;
    }
    
    _players = players;
    _visibleSize = Director::getInstance()->getVisibleSize();
    
    // 半透明背景遮罩
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(bgMask);
    
    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    createUI();
    populatePlayerList();
    
    return true;
}

void PlayerListLayer::setOnPlayerSelected(std::function<void(const std::string&)> callback)
{
    _onPlayerSelected = callback;
}

void PlayerListLayer::createUI()
{
    // 创建容器
    auto layout = ui::Layout::create();
    layout->setContentSize(Size(600, 500));
    layout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    layout->setBackGroundColor(Color3B(40, 40, 50));
    layout->setBackGroundColorOpacity(255);
    
    _container = layout;
    _container->setAnchorPoint(Vec2(0.5f, 0.5f));
    _container->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height / 2));
    this->addChild(_container);
    
    // 标题
    auto title = Label::createWithSystemFont("选择攻击目标", "Arial", 32);
    title->setPosition(Vec2(300, 460));
    title->setTextColor(Color4B::YELLOW);
    _container->addChild(title);
    
    // 关闭按钮
    _closeBtn = Button::create();
    _closeBtn->setTitleText("X");
    _closeBtn->setTitleFontSize(32);
    _closeBtn->setPosition(Vec2(570, 460));
    _closeBtn->addClickEventListener([this](Ref*) {
        hide();
    });
    _container->addChild(_closeBtn);
    
    // 玩家列表
    _listView = ListView::create();
    _listView->setDirection(ui::ScrollView::Direction::VERTICAL);
    _listView->setContentSize(Size(560, 380));
    _listView->setPosition(Vec2(20, 20));
    _listView->setBounceEnabled(true);
    _listView->setScrollBarEnabled(true);
    _listView->setItemsMargin(10.0f);
    _container->addChild(_listView);
}

void PlayerListLayer::populatePlayerList()
{
    _listView->removeAllItems();
    
    if (_players.empty())
    {
        auto tip = Label::createWithSystemFont("暂无可攻击的玩家", "Arial", 24);
        tip->setPosition(Vec2(280, 200));
        tip->setTextColor(Color4B::GRAY);
        
        auto item = Layout::create();
        item->setContentSize(Size(560, 380));
        item->addChild(tip);
        _listView->pushBackCustomItem(item);
        return;
    }
    
    for (const auto& player : _players)
    {
        auto item = createPlayerItem(player);
        _listView->pushBackCustomItem(item);
    }
}

cocos2d::ui::Widget* PlayerListLayer::createPlayerItem(const PlayerInfo& player)
{
    auto item = Layout::create();
    item->setContentSize(Size(560, 100));
    item->setTouchEnabled(true);
    
    // 背景
    auto bg = LayerColor::create(Color4B(60, 60, 80, 255), 560, 100);
    item->addChild(bg);
    
    // 玩家名称
    auto nameLabel = Label::createWithSystemFont(player.username, "Arial", 24);
    nameLabel->setAnchorPoint(Vec2(0, 0.5f));
    nameLabel->setPosition(Vec2(20, 70));
    nameLabel->setTextColor(Color4B::WHITE);
    item->addChild(nameLabel);
    
    // 大本营等级
    auto thLabel = Label::createWithSystemFont(
        StringUtils::format("大本营 Lv.%d", player.townHallLevel), "Arial", 20);
    thLabel->setAnchorPoint(Vec2(0, 0.5f));
    thLabel->setPosition(Vec2(20, 40));
    thLabel->setTextColor(Color4B(200, 200, 200, 255));
    item->addChild(thLabel);
    
    // 奖杯数
    auto trophyLabel = Label::createWithSystemFont(
        StringUtils::format("🏆 %d", player.trophies), "Arial", 18);
    trophyLabel->setAnchorPoint(Vec2(0, 0.5f));
    trophyLabel->setPosition(Vec2(20, 15));
    trophyLabel->setTextColor(Color4B(255, 215, 0, 255));
    item->addChild(trophyLabel);
    
    // 可掠夺资源
    auto goldLabel = Label::createWithSystemFont(
        StringUtils::format("💰 %d", player.gold), "Arial", 18);
    goldLabel->setAnchorPoint(Vec2(0, 0.5f));
    goldLabel->setPosition(Vec2(280, 70));
    goldLabel->setTextColor(Color4B(255, 215, 0, 255));
    item->addChild(goldLabel);
    
    auto elixirLabel = Label::createWithSystemFont(
        StringUtils::format("⚗️ %d", player.elixir), "Arial", 18);
    elixirLabel->setAnchorPoint(Vec2(0, 0.5f));
    elixirLabel->setPosition(Vec2(280, 40));
    elixirLabel->setTextColor(Color4B(255, 0, 255, 255));
    item->addChild(elixirLabel);
    
    // 攻击按钮
    auto attackBtn = Button::create();
    attackBtn->setTitleText("攻击！");
    attackBtn->setTitleFontSize(24);
    attackBtn->setContentSize(Size(120, 60));
    attackBtn->setScale9Enabled(true);
    attackBtn->setPosition(Vec2(480, 50));
    
    // 绿色背景
    auto btnBg = LayerColor::create(Color4B(0, 150, 0, 255), 120, 60);
    btnBg->setPosition(Vec2(-60, -30));
    attackBtn->addChild(btnBg, -1);
    
    attackBtn->addClickEventListener([this, player](Ref*) {
        if (_onPlayerSelected)
        {
            _onPlayerSelected(player.userId);
        }
        hide();
    });
    item->addChild(attackBtn);
    
    return item;
}

void PlayerListLayer::show()
{
    this->setVisible(true);
    _container->setScale(0.0f);
    _container->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}

void PlayerListLayer::hide()
{
    auto scaleAction = ScaleTo::create(0.2f, 0.0f);
    _container->runAction(scaleAction);

    // 延迟后移除整个层（包括遮罩），而不仅仅是容器
    auto delay = DelayTime::create(0.2f);
    auto removeSelf = RemoveSelf::create();
    this->runAction(Sequence::create(delay, removeSelf, nullptr));
}
