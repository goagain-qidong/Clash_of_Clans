/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BattleUI.cpp
 * File Function: 战斗界面 - 负责管理游戏中的战斗相关UI
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
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
    _endBattleButton = Button::create();
    _endBattleButton->setTitleText("结束战斗");
    _endBattleButton->setTitleFontSize(24);
    _endBattleButton->setPosition(Vec2(_visibleSize.width - 100, 60));
    _endBattleButton->setVisible(false);
    _endBattleButton->addClickEventListener([this](Ref*) {
        if (_onEndBattle) _onEndBattle();
    });
    this->addChild(_endBattleButton, 100);

    // 返回按钮（战斗结束后显示）
    _returnButton = Button::create();
    _returnButton->setTitleText("返回主场景");
    _returnButton->setTitleFontSize(24);
    _returnButton->setPosition(Vec2(_visibleSize.width / 2, 60));
    _returnButton->setVisible(false);
    _returnButton->addClickEventListener([this](Ref*) {
        if (_onReturn) _onReturn();
    });
    this->addChild(_returnButton, 100);
}

void BattleUI::setupTroopButtons()
{
    float buttonY = 150;
    float buttonSpacing = 100;
    float startX = (_visibleSize.width - buttonSpacing * 2) / 2;
    
    // 野蛮人按钮
    _barbarianButton = Button::create(
        "units/barbarian_select_button_active.png",
        "units/barbarian_select_button_active.png",
        "units/barbarian_select_button_active.png"
    );
    _barbarianButton->setScale(0.8f);
    _barbarianButton->setPosition(Vec2(startX, buttonY));
    _barbarianButton->setVisible(false);
    _barbarianButton->addClickEventListener([this](Ref*) {
        if (_onTroopSelected) _onTroopSelected(UnitType::kBarbarian);
    });
    this->addChild(_barbarianButton, 100);
    
    _barbarianCountLabel = Label::createWithSystemFont("0", "Arial", 24);
    _barbarianCountLabel->setPosition(Vec2(startX, buttonY - 50));
    _barbarianCountLabel->setTextColor(Color4B::WHITE);
    _barbarianCountLabel->setVisible(false);
    this->addChild(_barbarianCountLabel, 100);
    
    // 弓箭手按钮
    _archerButton = Button::create(
        "units/archer_select_button_active.png",
        "units/archer_select_button_active.png",
        "units/archer_select_button_active.png"
    );
    _archerButton->setScale(0.8f);
    _archerButton->setPosition(Vec2(startX + buttonSpacing, buttonY));
    _archerButton->setVisible(false);
    _archerButton->addClickEventListener([this](Ref*) {
        if (_onTroopSelected) _onTroopSelected(UnitType::kArcher);
    });
    this->addChild(_archerButton, 100);
    
    _archerCountLabel = Label::createWithSystemFont("0", "Arial", 24);
    _archerCountLabel->setPosition(Vec2(startX + buttonSpacing, buttonY - 50));
    _archerCountLabel->setTextColor(Color4B::WHITE);
    _archerCountLabel->setVisible(false);
    this->addChild(_archerCountLabel, 100);
    
    // 巨人按钮
    _giantButton = Button::create(
        "units/giant_select_button_active.png",
        "units/giant_select_button_active.png",
        "units/giant_select_button_active.png"
    );
    _giantButton->setScale(0.8f);
    _giantButton->setPosition(Vec2(startX + buttonSpacing * 2, buttonY));
    _giantButton->setVisible(false);
    _giantButton->addClickEventListener([this](Ref*) {
        if (_onTroopSelected) _onTroopSelected(UnitType::kGiant);
    });
    this->addChild(_giantButton, 100);
    
    _giantCountLabel = Label::createWithSystemFont("0", "Arial", 24);
    _giantCountLabel->setPosition(Vec2(startX + buttonSpacing * 2, buttonY - 50));
    _giantCountLabel->setTextColor(Color4B::WHITE);
    _giantCountLabel->setVisible(false);
    this->addChild(_giantCountLabel, 100);
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
    if (!_timerLabel) return;

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
        case 1: message = "爽！"; break;
        case 2: message = "太棒了！"; break;
        case 3: message = "堪称完美！"; break;
        default: message = "获得星星!"; break;
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

        auto starPop = Spawn::create(
            EaseBackOut::create(ScaleTo::create(0.5f, 1.0f)),
            FadeIn::create(0.25f),
            nullptr);

        auto starSeq = Sequence::create(
            starPop,
            DelayTime::create(1.2f),
            FadeOut::create(0.8f),
            nullptr);
        starIcon->runAction(starSeq);

        auto textSeq = Sequence::create(
            DelayTime::create(0.1f),
            FadeIn::create(0.35f),
            DelayTime::create(1.0f),
            FadeOut::create(0.8f),
            nullptr);
        textLabel->runAction(textSeq);

        float totalLifetime = 0.5f + 1.2f + 0.8f;
        effectNode->runAction(Sequence::create(
            DelayTime::create(0.1f),
            MoveBy::create(totalLifetime, Vec2(0, 120)),
            RemoveSelf::create(),
            nullptr));
    }
}

void BattleUI::updateDestruction(int percent)
{
    if (_destructionLabel)
    {
        _destructionLabel->setString(StringUtils::format("%d%%", percent));
    }
}

void BattleUI::updateTroopCounts(int barbarianCount, int archerCount, int giantCount)
{
    if (_barbarianCountLabel) _barbarianCountLabel->setString(StringUtils::format("%d", barbarianCount));
    if (_archerCountLabel) _archerCountLabel->setString(StringUtils::format("%d", archerCount));
    if (_giantCountLabel) _giantCountLabel->setString(StringUtils::format("%d", giantCount));
}

void BattleUI::setReplayMode(bool isReplay)
{
    _isReplayMode = isReplay;
}

void BattleUI::showBattleHUD(bool visible)
{
    if (_timerLabel) _timerLabel->setVisible(visible);
    if (_starsLabel) _starsLabel->setVisible(visible);
    if (_destructionLabel) _destructionLabel->setVisible(visible);
    if (_endBattleButton) _endBattleButton->setVisible(visible);
}

void BattleUI::showTroopButtons(bool visible)
{
    if (_barbarianButton) _barbarianButton->setVisible(visible);
    if (_archerButton) _archerButton->setVisible(visible);
    if (_giantButton) _giantButton->setVisible(visible);
    if (_barbarianCountLabel) _barbarianCountLabel->setVisible(visible);
    if (_archerCountLabel) _archerCountLabel->setVisible(visible);
    if (_giantCountLabel) _giantCountLabel->setVisible(visible);
}

void BattleUI::showReturnButton(bool visible)
{
    if (_returnButton) _returnButton->setVisible(visible);
}

void BattleUI::highlightTroopButton(UnitType type)
{
    if (_barbarianButton) _barbarianButton->setScale(type == UnitType::kBarbarian ? 1.1f : 0.8f);
    if (_archerButton) _archerButton->setScale(type == UnitType::kArcher ? 1.1f : 0.8f);
    if (_giantButton) _giantButton->setScale(type == UnitType::kGiant ? 1.1f : 0.8f);
}

void BattleUI::setEndBattleButtonText(const std::string& text)
{
    if (_endBattleButton) _endBattleButton->setTitleText(text);
}

void BattleUI::showResultPanel(int stars, int destructionPercent, int goldLooted, int elixirLooted, int trophyChange, bool isReplayMode)
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
    auto title = Label::createWithSystemFont(titleText, "Arial", 42);
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
    auto destructionLabel = Label::createWithSystemFont(
        StringUtils::format("摧毁: %d%%", destructionPercent), "Arial", 32);
    destructionLabel->setPosition(Vec2(250, 220));
    panel->addChild(destructionLabel);

    // 掠夺信息
    std::string lootText;
    Color4B lootColor;
    
    if (isReplayMode)
    {
        // 回放模式（防守方视角）：显示损失
        lootText = StringUtils::format("损失金币: -%d\n损失圣水: -%d", goldLooted, elixirLooted);
        lootColor = Color4B::RED;
    }
    else
    {
        // 进攻模式：显示掠夺
        lootText = StringUtils::format("掠夺金币: +%d\n掠夺圣水: +%d", goldLooted, elixirLooted);
        lootColor = Color4B::GREEN;
    }
    
    auto lootLabel = Label::createWithSystemFont(lootText, "Arial", 28);
    lootLabel->setPosition(Vec2(250, 150));
    lootLabel->setAlignment(TextHAlignment::CENTER);
    lootLabel->setTextColor(lootColor);
    panel->addChild(lootLabel);

    // 奖杯变化
    std::string trophyText;
    Color4B trophyColor;
    
    if (isReplayMode)
    {
        // 回放模式：显示奖杯变化（防守方相反）
        int defenderTrophyChange = -trophyChange;
        trophyText = StringUtils::format("奖杯: %s%d", defenderTrophyChange >= 0 ? "+" : "", defenderTrophyChange);
        trophyColor = defenderTrophyChange >= 0 ? Color4B::GREEN : Color4B::RED;
    }
    else
    {
        trophyText = StringUtils::format("奖杯: %s%d", trophyChange >= 0 ? "+" : "", trophyChange);
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