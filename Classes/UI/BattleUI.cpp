/************************************************************************/
/* BattleUI.cpp                                                          */
/*                                                                      */
/* Implementation of the BattleUI class, responsible for managing the   */
/* battle-related UI in the game, including troop selection, battle     */
/* status, and results display.                                           */
/*                                                                      */
/* Author: Zhao Chongzhi                                                */
/* Date: 2025/12/14                                                      */
/*                                                                      */
/* License: MIT License                                                 */
/************************************************************************/

#include "BattleUI.h"

USING_NS_CC;
using namespace ui;

BattleUI* BattleUI::create()
{
    BattleUI* ret = new (std::nothrow) BattleUI();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BattleUI::init()
{
    if (!Layer::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();

    setupTopBar();
    setupBottomButtons();
    setupTroopButtons();

    return true;
}

void BattleUI::setEndBattleCallback(const std::function<void()>& callback)
{
    _onEndBattle = callback;
}

void BattleUI::setReturnCallback(const std::function<void()>& callback)
{
    _onReturn = callback;
}

void BattleUI::setTroopSelectionCallback(const std::function<void(UnitType)>& callback)
{
    _onTroopSelected = callback;
}

void BattleUI::setTroopDeselectionCallback(const std::function<void()>& callback)
{
    _onTroopDeselected = callback;
}

void BattleUI::setupTopBar()
{
    // 状态标签
    _statusLabel = Label::createWithSystemFont("", "Arial", 24);
    _statusLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    _statusLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height - 10));
    _statusLabel->setTextColor(Color4B::YELLOW);
    this->addChild(_statusLabel, 100);

    // 计时器
    _timerLabel = Label::createWithSystemFont("3:00", "Arial", 48);
    _timerLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height - 60));
    _timerLabel->setTextColor(Color4B::WHITE);
    _timerLabel->setVisible(false);
    this->addChild(_timerLabel, 100);

    // 星数显示
    _starsLabel = Label::createWithSystemFont("☆☆☆", "Arial", 52);
    _starsLabel->setPosition(Vec2(100, _visibleSize.height - 60));
    _starsLabel->setTextColor(Color4B::GRAY);
    _starsLabel->setVisible(false);
    this->addChild(_starsLabel, 100);

    // 摧毁百分比
    _destructionLabel = Label::createWithSystemFont("0%", "Arial", 28);
    _destructionLabel->setPosition(Vec2(_visibleSize.width - 100, _visibleSize.height - 60));
    _destructionLabel->setTextColor(Color4B::WHITE);
    _destructionLabel->setVisible(false);
    this->addChild(_destructionLabel, 100);
}

void BattleUI::setupBottomButtons()
{
    // 结束战斗按钮
    _endBattleButton = Button::create("icon/end_battle_button.png");
    if (_endBattleButton->getContentSize().equals(Size::ZERO))
    {
        _endBattleButton = Button::create();
        _endBattleButton->ignoreContentAdaptWithSize(false);
        _endBattleButton->setContentSize(Size(120, 50));
        _endBattleButton->setTitleText("结束战斗");
        _endBattleButton->setTitleFontSize(24);

        auto bg = LayerColor::create(Color4B(200, 50, 50, 255), 120, 50);
        bg->setPosition(Vec2::ZERO);
        _endBattleButton->addChild(bg, -1);

        if (_endBattleButton->getTitleRenderer())
        {
            _endBattleButton->getTitleRenderer()->setPosition(Vec2(60, 25));
        }
    }
    else
    {
        _endBattleButton->setScale(120.0f / _endBattleButton->getContentSize().width);
    }
    _endBattleButton->setPosition(Vec2(_visibleSize.width - 100, 60));
    _endBattleButton->setVisible(false);
    _endBattleButton->addClickEventListener([this](Ref*) {
        if (_onEndBattle)
            _onEndBattle();
    });
    this->addChild(_endBattleButton, 100);

    // 返回按钮（战斗结束后显示）
    _returnButton = Button::create("icon/return_button.png");
    if (_returnButton->getContentSize().equals(Size::ZERO))
    {
        _returnButton = Button::create();
        _returnButton->ignoreContentAdaptWithSize(false);
        _returnButton->setContentSize(Size(160, 50));
        _returnButton->setTitleText("返回主场景");
        _returnButton->setTitleFontSize(24);

        auto bg = LayerColor::create(Color4B(50, 150, 50, 255), 160, 50);
        bg->setPosition(Vec2::ZERO);
        _returnButton->addChild(bg, -1);

        if (_returnButton->getTitleRenderer())
        {
            _returnButton->getTitleRenderer()->setPosition(Vec2(80, 25));
        }
    }
    else
    {
        _returnButton->setScale(120.0f / _returnButton->getContentSize().width);
    }
    _returnButton->setPosition(Vec2(_visibleSize.width / 2, 60));
    _returnButton->setVisible(false);
    _returnButton->addClickEventListener([this](Ref*) {
        if (_onReturn)
            _onReturn();
    });
    this->addChild(_returnButton, 100);
}

Node* BattleUI::createTroopCard(UnitType type, const std::string& iconPath, const std::string& name)
{
    // 🆕 创建卡片容器
    auto card = Node::create();
    card->setContentSize(Size(90, 120));

    // 卡片背景 - 深色渐变效果 (使用DrawNode模拟圆角矩形背景)
    auto cardBg = DrawNode::create();
    cardBg->drawSolidRect(Vec2(0, 0), Vec2(90, 120), Color4F(0.15f, 0.18f, 0.25f, 0.9f));
    card->addChild(cardBg, 0);

    // 顶部装饰条
    auto topBar = LayerColor::create(Color4B(60, 80, 120, 255), 90, 4);
    topBar->setPosition(Vec2(0, 116));
    card->addChild(topBar, 1);

    // 兵种图标
    auto icon = Sprite::create(iconPath);
    if (icon)
    {
        float iconScale = 55.0f / std::max(icon->getContentSize().width, icon->getContentSize().height);
        icon->setScale(iconScale);
        icon->setPosition(Vec2(45, 75));
        icon->setName("icon");
        card->addChild(icon, 2);
    }

    // 兵种名称
    auto nameLabel = Label::createWithSystemFont(name, "Arial", 12);
    nameLabel->setPosition(Vec2(45, 38));
    nameLabel->setTextColor(Color4B(200, 200, 200, 255));
    nameLabel->setName("nameLabel");
    card->addChild(nameLabel, 2);

    // 数量背景框
    auto countBg = LayerColor::create(Color4B(20, 20, 30, 200), 50, 22);
    countBg->setPosition(Vec2(20, 6));
    card->addChild(countBg, 1);

    // 数量显示
    auto countLabel = Label::createWithSystemFont("0", "Arial", 16);
    countLabel->setPosition(Vec2(45, 17));
    countLabel->setTextColor(Color4B::WHITE);
    countLabel->setName("countLabel");
    card->addChild(countLabel, 2);

    // 边框效果
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(90, 120), Color4F(0.4f, 0.5f, 0.7f, 0.5f));
    border->setName("border");
    card->addChild(border, 3);

    // 添加触摸监听器
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);

    touchListener->onTouchBegan = [this, card, type](Touch* touch, Event* event) {
        Vec2 locationInNode = card->convertToNodeSpace(touch->getLocation());
        Size size           = card->getContentSize();
        Rect rect           = Rect(0, 0, size.width, size.height);

        if (rect.containsPoint(locationInNode))
        {
            // 点击时的缩放反馈
            card->runAction(ScaleTo::create(0.05f, 0.95f));
            return true;
        }
        return false;
    };

    touchListener->onTouchEnded = [this, card, type](Touch* touch, Event* event) {
        card->runAction(ScaleTo::create(0.1f, 1.0f));

        Vec2 locationInNode = card->convertToNodeSpace(touch->getLocation());
        Size size           = card->getContentSize();
        Rect rect           = Rect(0, 0, size.width, size.height);

        if (rect.containsPoint(locationInNode))
        {
            onTroopCardClicked(type);
        }
    };

    touchListener->onTouchCancelled = [card](Touch* touch, Event* event) {
        card->runAction(ScaleTo::create(0.1f, 1.0f));
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, card);

    return card;
}

void BattleUI::onTroopCardClicked(UnitType type)
{
    // 🆕 如果点击的是已选中的兵种，则取消选中
    if (_hasSelectedUnit && _selectedUnitType == type)
    {
        clearTroopHighlight();
        _hasSelectedUnit = false;
        if (_onTroopDeselected)
            _onTroopDeselected();
        CCLOG("🔹 取消选中兵种");
    }
    else
    {
        // 选中新兵种
        _selectedUnitType = type;
        _hasSelectedUnit  = true;
        highlightTroopButton(type);
        if (_onTroopSelected)
            _onTroopSelected(type);
        CCLOG("🔹 选中兵种: %d", static_cast<int>(type));
    }
}

void BattleUI::setupTroopButtons()
{
    // 🆕 创建底部兵种面板
    _troopPanel = Node::create();
    _troopPanel->setPosition(Vec2(_visibleSize.width / 2, 80));
    _troopPanel->setVisible(false);
    this->addChild(_troopPanel, 100);

    // 面板背景
    float panelWidth  = 550; // 增加宽度以容纳所有卡片
    float panelHeight = 150;
    auto  panelBg     = LayerColor::create(Color4B(15, 20, 30, 240), panelWidth, panelHeight); // 更深色的背景
    panelBg->setPosition(Vec2(-panelWidth / 2, -25));
    _troopPanel->addChild(panelBg, -1);

    // 面板顶部装饰
    auto panelTop = LayerColor::create(Color4B(100, 120, 180, 255), panelWidth, 4);
    panelTop->setPosition(Vec2(-panelWidth / 2, panelHeight - 29));
    _troopPanel->addChild(panelTop, 0);

    // 创建选中框（初始隐藏）
    _selectionFrame = Sprite::create();
    if (_selectionFrame)
    {
        // 使用DrawNode绘制选中框
        auto frameNode = DrawNode::create();
        // 外发光效果
        frameNode->drawRect(Vec2(-4, -4), Vec2(94, 124), Color4F(1.0f, 0.9f, 0.3f, 0.4f));
        // 主边框
        frameNode->drawRect(Vec2(-2, -2), Vec2(92, 122), Color4F(1.0f, 0.85f, 0.2f, 1.0f));
        
        _selectionFrame->addChild(frameNode);
        _selectionFrame->setVisible(false);
        _selectionFrame->setAnchorPoint(Vec2(0, 0));
        _troopPanel->addChild(_selectionFrame, 10);
    }

    // 卡片布局参数
    float cardWidth   = 90;
    float cardSpacing = 105; // 稍微增加间距
    int   numCards    = 5;
    
    // 计算起始X坐标，使卡片组整体居中
    // 总宽度 = (卡片数量-1) * 间距 + 卡片宽度
    float totalWidth = (numCards - 1) * cardSpacing + cardWidth;
    float startX     = -totalWidth / 2;
    float cardY      = 0;

    // 创建五个兵种卡片
    _barbarianCard = createTroopCard(UnitType::kBarbarian, "units/barbarian_select_button_active.png", "野蛮人");
    _barbarianCard->setPosition(Vec2(startX, cardY));
    _troopPanel->addChild(_barbarianCard, 1);

    _archerCard = createTroopCard(UnitType::kArcher, "units/archer_select_button_active.png", "弓箭手");
    _archerCard->setPosition(Vec2(startX + cardSpacing, cardY));
    _troopPanel->addChild(_archerCard, 1);

    _giantCard = createTroopCard(UnitType::kGiant, "units/giant_select_button_active.png", "巨人");
    _giantCard->setPosition(Vec2(startX + cardSpacing * 2, cardY));
    _troopPanel->addChild(_giantCard, 1);

    _goblinCard = createTroopCard(UnitType::kGoblin, "units/goblin_select_button_active.png", "哥布林");
    _goblinCard->setPosition(Vec2(startX + cardSpacing * 3, cardY));
    _troopPanel->addChild(_goblinCard, 1);

    _wallBreakerCard = createTroopCard(UnitType::kWallBreaker, "units/wallbreaker_select_button_active.png", "炸弹人");
    _wallBreakerCard->setPosition(Vec2(startX + cardSpacing * 4, cardY));
    _troopPanel->addChild(_wallBreakerCard, 1);

    // 保留旧按钮的引用（设为不可见，保持兼容性）
    _barbarianButton = Button::create();
    _barbarianButton->setVisible(false);
    this->addChild(_barbarianButton);

    _archerButton = Button::create();
    _archerButton->setVisible(false);
    this->addChild(_archerButton);

    _giantButton = Button::create();
    _giantButton->setVisible(false);
    this->addChild(_giantButton);

    _goblinButton = Button::create();
    _goblinButton->setVisible(false);
    this->addChild(_goblinButton);

    _wallBreakerButton = Button::create();
    _wallBreakerButton->setVisible(false);
    this->addChild(_wallBreakerButton);

    // 创建数量标签（隐藏，用于兼容性）
    _barbarianCountLabel = Label::createWithSystemFont("0", "Arial", 1);
    _barbarianCountLabel->setVisible(false);
    this->addChild(_barbarianCountLabel);

    _archerCountLabel = Label::createWithSystemFont("0", "Arial", 1);
    _archerCountLabel->setVisible(false);
    this->addChild(_archerCountLabel);

    _giantCountLabel = Label::createWithSystemFont("0", "Arial", 1);
    _giantCountLabel->setVisible(false);
    this->addChild(_giantCountLabel);

    _goblinCountLabel = Label::createWithSystemFont("0", "Arial", 1);
    _goblinCountLabel->setVisible(false);
    this->addChild(_goblinCountLabel);

    _wallBreakerCountLabel = Label::createWithSystemFont("0", "Arial", 1);
    _wallBreakerCountLabel->setVisible(false);
    this->addChild(_wallBreakerCountLabel);
}

void BattleUI::updateTroopCardCount(UnitType type, int count)
{
    Node* card = nullptr;
    switch (type)
    {
    case UnitType::kBarbarian:
        card = _barbarianCard;
        break;
    case UnitType::kArcher:
        card = _archerCard;
        break;
    case UnitType::kGiant:
        card = _giantCard;
        break;
    case UnitType::kGoblin:
        card = _goblinCard;
        break;
    case UnitType::kWallBreaker:
        card = _wallBreakerCard;
        break;
    default:
        return;
    }

    if (!card)
        return;

    auto countLabel = dynamic_cast<Label*>(card->getChildByName("countLabel"));
    if (countLabel)
    {
        countLabel->setString(StringUtils::format("%d", count));

        // 根据数量改变颜色
        if (count <= 0)
        {
            countLabel->setTextColor(Color4B(150, 150, 150, 255)); // 灰色
            // 图标变灰
            auto icon = card->getChildByName("icon");
            if (icon)
                icon->setColor(Color3B(100, 100, 100));
        }
        else
        {
            countLabel->setTextColor(Color4B::WHITE);
            // 图标恢复
            auto icon = card->getChildByName("icon");
            if (icon)
                icon->setColor(Color3B::WHITE);
        }
    }
}

void BattleUI::updateStatus(const std::string& text, const cocos2d::Color4B& color)
{
    if (_statusLabel)
    {
        _statusLabel->setString(text);
        _statusLabel->setTextColor(color);
    }
}

void BattleUI::updateTimer(int remainingTime)
{
    if (!_timerLabel)
        return;

    int minutes = remainingTime / 60;
    int seconds = remainingTime % 60;
    _timerLabel->setString(StringUtils::format("%d:%02d", minutes, seconds));

    if (remainingTime < 30)
    {
        _timerLabel->setTextColor(Color4B::RED);
    }
    else
    {
        _timerLabel->setTextColor(Color4B::WHITE);
    }
}

void BattleUI::updateStars(int stars)
{
    int oldStars = _starsEarned;
    _starsEarned = std::min(stars, 3);

    std::string starsStr = "";
    for (int i = 0; i < 3; i++)
    {
        starsStr += (i < _starsEarned) ? "★" : "☆";
    }
    if (_starsLabel)
    {
        _starsLabel->setString(starsStr);
        _starsLabel->setTextColor(_starsEarned > 0 ? Color4B::YELLOW : Color4B::GRAY);
    }

    // 星数增加时的浮现特效
    if (_starsEarned > oldStars)
    {
        std::string message;
        switch (_starsEarned)
        {
        case 1:
            message = "爽！";
            break;
        case 2:
            message = "太棒了！";
            break;
        case 3:
            message = "堪称完美！";
            break;
        default:
            message = "获得星星!";
            break;
        }

        auto effectNode = Node::create();
        effectNode->setPosition(_visibleSize.width / 2, _visibleSize.height / 2 + 50);
        this->addChild(effectNode, 500);

        auto starIcon = Label::createWithSystemFont("★", "Arial", 120);
        starIcon->setTextColor(Color4B::YELLOW);
        starIcon->enableOutline(Color4B(200, 150, 0, 255), 4);
        starIcon->setPosition(0, 40);
        starIcon->setScale(0.0f);
        starIcon->setOpacity(0);
        effectNode->addChild(starIcon);

        auto textLabel = Label::createWithSystemFont(message, "Arial", 36);
        textLabel->setTextColor(Color4B::WHITE);
        textLabel->enableOutline(Color4B::BLACK, 2);
        textLabel->setPosition(0, -60);
        textLabel->setOpacity(0);
        effectNode->addChild(textLabel);

        auto starPop = Spawn::create(EaseBackOut::create(ScaleTo::create(0.5f, 1.0f)), FadeIn::create(0.25f), nullptr);

        auto starSeq = Sequence::create(starPop, DelayTime::create(1.2f), FadeOut::create(0.8f), nullptr);
        starIcon->runAction(starSeq);

        auto textSeq = Sequence::create(DelayTime::create(0.1f), FadeIn::create(0.35f), DelayTime::create(1.0f),
                                        FadeOut::create(0.8f), nullptr);
        textLabel->runAction(textSeq);

        float totalLifetime = 0.5f + 1.2f + 0.8f;
        effectNode->runAction(Sequence::create(DelayTime::create(0.1f), MoveBy::create(totalLifetime, Vec2(0, 120)),
                                               RemoveSelf::create(), nullptr));
    }
}

void BattleUI::updateDestruction(int percent)
{
    if (_destructionLabel)
    {
        _destructionLabel->setString(StringUtils::format("%d%%", percent));
    }
}

void BattleUI::updateTroopCounts(int barbarianCount, int archerCount, int giantCount, int goblinCount,
                                 int wallBreakerCount)
{
    // 更新新卡片的数量
    updateTroopCardCount(UnitType::kBarbarian, barbarianCount);
    updateTroopCardCount(UnitType::kArcher, archerCount);
    updateTroopCardCount(UnitType::kGiant, giantCount);
    updateTroopCardCount(UnitType::kGoblin, goblinCount);
    updateTroopCardCount(UnitType::kWallBreaker, wallBreakerCount);

    // 保持旧标签兼容性
    if (_barbarianCountLabel)
        _barbarianCountLabel->setString(StringUtils::format("%d", barbarianCount));
    if (_archerCountLabel)
        _archerCountLabel->setString(StringUtils::format("%d", archerCount));
    if (_giantCountLabel)
        _giantCountLabel->setString(StringUtils::format("%d", giantCount));
    if (_goblinCountLabel)
        _goblinCountLabel->setString(StringUtils::format("%d", goblinCount));
    if (_wallBreakerCountLabel)
        _wallBreakerCountLabel->setString(StringUtils::format("%d", wallBreakerCount));
}

void BattleUI::setReplayMode(bool isReplay)
{
    _isReplayMode = isReplay;
}

void BattleUI::showBattleHUD(bool visible)
{
    if (_timerLabel)
        _timerLabel->setVisible(visible);
    if (_starsLabel)
        _starsLabel->setVisible(visible);
    if (_destructionLabel)
        _destructionLabel->setVisible(visible);
    if (_endBattleButton)
        _endBattleButton->setVisible(visible);
}

void BattleUI::showTroopButtons(bool visible)
{
    // 显示新的兵种面板
    if (_troopPanel)
        _troopPanel->setVisible(visible);

    // 旧按钮保持隐藏
    if (_barbarianButton)
        _barbarianButton->setVisible(false);
    if (_archerButton)
        _archerButton->setVisible(false);
    if (_giantButton)
        _giantButton->setVisible(false);
    if (_goblinButton)
        _goblinButton->setVisible(false);
    if (_wallBreakerButton)
        _wallBreakerButton->setVisible(false);
    if (_barbarianCountLabel)
        _barbarianCountLabel->setVisible(false);
    if (_archerCountLabel)
        _archerCountLabel->setVisible(false);
    if (_giantCountLabel)
        _giantCountLabel->setVisible(false);
    if (_goblinCountLabel)
        _goblinCountLabel->setVisible(false);
    if (_wallBreakerCountLabel)
        _wallBreakerCountLabel->setVisible(false);
}

void BattleUI::showReturnButton(bool visible)
{
    if (_returnButton)
        _returnButton->setVisible(visible);
}

void BattleUI::highlightTroopButton(UnitType type)
{
    // 🆕 更新所有卡片的高亮状态
    auto updateCardHighlight = [](Node* card, bool highlight) {
        if (!card)
            return;

        auto border = dynamic_cast<DrawNode*>(card->getChildByName("border"));
        if (border)
        {
            border->clear();
            if (highlight)
            {
                // 高亮边框 - 金黄色
                border->drawRect(Vec2(0, 0), Vec2(90, 120), Color4F(1.0f, 0.85f, 0.2f, 1.0f));
                border->drawRect(Vec2(1, 1), Vec2(89, 119), Color4F(1.0f, 0.85f, 0.2f, 0.5f));
            }
            else
            {
                // 普通边框
                border->drawRect(Vec2(0, 0), Vec2(90, 120), Color4F(0.4f, 0.5f, 0.7f, 0.8f));
            }
        }

        // 卡片缩放效果
        card->setScale(highlight ? 1.08f : 1.0f);
    };

    updateCardHighlight(_barbarianCard, type == UnitType::kBarbarian);
    updateCardHighlight(_archerCard, type == UnitType::kArcher);
    updateCardHighlight(_giantCard, type == UnitType::kGiant);
    updateCardHighlight(_goblinCard, type == UnitType::kGoblin);
    updateCardHighlight(_wallBreakerCard, type == UnitType::kWallBreaker);

    // 移动选中框到对应卡片位置
    if (_selectionFrame)
    {
        Node* selectedCard = nullptr;
        switch (type)
        {
        case UnitType::kBarbarian:
            selectedCard = _barbarianCard;
            break;
        case UnitType::kArcher:
            selectedCard = _archerCard;
            break;
        case UnitType::kGiant:
            selectedCard = _giantCard;
            break;
        case UnitType::kGoblin:
            selectedCard = _goblinCard;
            break;
        case UnitType::kWallBreaker:
            selectedCard = _wallBreakerCard;
            break;
        default:
            break;
        }

        if (selectedCard)
        {
            _selectionFrame->setPosition(selectedCard->getPosition());
            _selectionFrame->setVisible(true);
        }
    }

    // 兼容性：更新旧按钮缩放
    if (_barbarianButton)
        _barbarianButton->setScale(type == UnitType::kBarbarian ? 1.1f : 0.8f);
    if (_archerButton)
        _archerButton->setScale(type == UnitType::kArcher ? 1.1f : 0.8f);
    if (_giantButton)
        _giantButton->setScale(type == UnitType::kGiant ? 1.1f : 0.8f);
    if (_goblinButton)
        _goblinButton->setScale(type == UnitType::kGoblin ? 1.1f : 0.8f);
    if (_wallBreakerButton)
        _wallBreakerButton->setScale(type == UnitType::kWallBreaker ? 1.1f : 0.8f);
}

void BattleUI::clearTroopHighlight()
{
    // 🆕 清除所有卡片的高亮状态
    auto clearCardHighlight = [](Node* card) {
        if (!card)
            return;

        auto border = dynamic_cast<DrawNode*>(card->getChildByName("border"));
        if (border)
        {
            border->clear();
            border->drawRect(Vec2(0, 0), Vec2(90, 120), Color4F(0.4f, 0.5f, 0.7f, 0.8f));
        }

        card->setScale(1.0f);
    };

    clearCardHighlight(_barbarianCard);
    clearCardHighlight(_archerCard);
    clearCardHighlight(_giantCard);
    clearCardHighlight(_goblinCard);
    clearCardHighlight(_wallBreakerCard);

    // 隐藏选中框
    if (_selectionFrame)
    {
        _selectionFrame->setVisible(false);
    }

    // 兼容性：重置旧按钮缩放
    if (_barbarianButton)
        _barbarianButton->setScale(0.8f);
    if (_archerButton)
        _archerButton->setScale(0.8f);
    if (_giantButton)
        _giantButton->setScale(0.8f);
    if (_goblinButton)
        _goblinButton->setScale(0.8f);
    if (_wallBreakerButton)
        _wallBreakerButton->setScale(0.8f);
}

void BattleUI::setEndBattleButtonText(const std::string& text)
{
    if (_endBattleButton)
    {
        _endBattleButton->setTitleText(text);

        // 如果是退出回放，更换按钮背景以避免与 end_battle.png 的文字重叠
        if (text == "退出回放")
        {
            // 使用 return_button.png 作为背景（假设它是一个通用的按钮背景）
            _endBattleButton->loadTextures("icon/return_button.png", "icon/return_button.png", "");

            // 调整按钮大小和缩放
            _endBattleButton->ignoreContentAdaptWithSize(false);
            _endBattleButton->setContentSize(Size(120, 50));
            _endBattleButton->setScale(1.0f);
        }
    }
}

void BattleUI::showResultPanel(int stars, int destructionPercent, int goldLooted, int elixirLooted, int trophyChange,
                               bool isReplayMode)
{
    // 隐藏战斗UI
    showBattleHUD(false);
    showTroopButtons(false);

    // 创建结果面板
    auto panel = LayerColor::create(Color4B(0, 0, 0, 220));
    panel->setContentSize(Size(500, 400));
    panel->setPosition(Vec2((_visibleSize.width - 500) / 2, (_visibleSize.height - 400) / 2));
    panel->setName("result_panel");
    this->addChild(panel, 200);

    // 标题
    std::string titleText = isReplayMode ? "回放结束" : "战斗结束!";
    auto        title     = Label::createWithSystemFont(titleText, "Arial", 42);
    title->setPosition(Vec2(250, 360));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    // 星数
    std::string starsStr = "";
    for (int i = 0; i < 3; i++)
    {
        starsStr += (i < stars) ? "★ " : "☆ ";
    }
    auto starsLabel = Label::createWithSystemFont(starsStr, "Arial", 56);
    starsLabel->setPosition(Vec2(250, 280));
    starsLabel->setTextColor(Color4B::YELLOW);
    panel->addChild(starsLabel);

    // 摧毁百分比
    auto destructionLabel =
        Label::createWithSystemFont(StringUtils::format("摧毁: %d%%", destructionPercent), "Arial", 32);
    destructionLabel->setPosition(Vec2(250, 220));
    panel->addChild(destructionLabel);

    // 掠夺信息
    std::string lootText;
    Color4B     lootColor;

    if (isReplayMode)
    {
        // 回放模式（防守方视角）：显示损失
        lootText  = StringUtils::format("损失金币: -%d\n损失圣水: -%d", goldLooted, elixirLooted);
        lootColor = Color4B::RED;
    }
    else
    {
        // 进攻模式：显示掠夺
        lootText  = StringUtils::format("掠夺金币: +%d\n掠夺圣水: +%d", goldLooted, elixirLooted);
        lootColor = Color4B::GREEN;
    }

    auto lootLabel = Label::createWithSystemFont(lootText, "Arial", 28);
    lootLabel->setPosition(Vec2(250, 150));
    lootLabel->setAlignment(TextHAlignment::CENTER);
    lootLabel->setTextColor(lootColor);
    panel->addChild(lootLabel);

    // 奖杯变化
    std::string trophyText;
    Color4B     trophyColor;

    if (isReplayMode)
    {
        // 回放模式：显示奖杯变化（防守方相反）
        int defenderTrophyChange = -trophyChange;
        trophyText  = StringUtils::format("奖杯: %s%d", defenderTrophyChange >= 0 ? "+" : "", defenderTrophyChange);
        trophyColor = defenderTrophyChange >= 0 ? Color4B::GREEN : Color4B::RED;
    }
    else
    {
        trophyText  = StringUtils::format("奖杯: %s%d", trophyChange >= 0 ? "+" : "", trophyChange);
        trophyColor = trophyChange >= 0 ? Color4B::GREEN : Color4B::RED;
    }

    auto trophyLabel = Label::createWithSystemFont(trophyText, "Arial", 26);
    trophyLabel->setPosition(Vec2(250, 80));
    trophyLabel->setTextColor(trophyColor);
    panel->addChild(trophyLabel);

    // 显示返回按钮
    updateStatus("点击返回按钮回到主场景", Color4B::YELLOW);
    showReturnButton(true);
}