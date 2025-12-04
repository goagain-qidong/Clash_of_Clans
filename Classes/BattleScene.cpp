#include "BattleScene.h"

#include "DraggableMapScene.h"
#include "Managers/AccountManager.h"

USING_NS_CC;
using namespace ui;

Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

BattleScene* BattleScene::create()
{
    BattleScene* scene = new (std::nothrow) BattleScene();
    if (scene && scene->init())
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

bool BattleScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    setupUI();
    setupCallbacks();
    startMatching();

    scheduleUpdate();

    return true;
}

void BattleScene::setupUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 背景
    auto bg = LayerColor::create(Color4B(30, 30, 50, 255));
    this->addChild(bg);

    // 状态标签
    _statusLabel = Label::createWithSystemFont("正在搜索对手...", "Arial", 32);
    _statusLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height - 100));
    this->addChild(_statusLabel, 10);

    // 计时器标签
    _timerLabel = Label::createWithSystemFont("3:00", "Arial", 48);
    _timerLabel->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height - 50));
    _timerLabel->setVisible(false);
    this->addChild(_timerLabel, 10);

    // 星星标签
    _starsLabel = Label::createWithSystemFont("★★★", "Arial", 36);
    _starsLabel->setPosition(Vec2(origin.x + 100, origin.y + visibleSize.height - 50));
    _starsLabel->setTextColor(Color4B::GRAY);
    _starsLabel->setVisible(false);
    this->addChild(_starsLabel, 10);

    // 掠夺标签
    _lootLabel = Label::createWithSystemFont("Gold: 0  Elixir: 0", "Arial", 24);
    _lootLabel->setPosition(Vec2(origin.x + visibleSize.width - 150, origin.y + visibleSize.height - 50));
    _lootLabel->setVisible(false);
    this->addChild(_lootLabel, 10);

    // 加载动画节点
    _loadingNode = Node::create();
    auto loadingSprite = Sprite::create();
    loadingSprite->setTextureRect(Rect(0, 0, 50, 50));
    loadingSprite->setColor(Color3B::WHITE);
    _loadingNode->addChild(loadingSprite);
    _loadingNode->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2));
    _loadingNode->runAction(RepeatForever::create(RotateBy::create(1.0f, 360)));
    this->addChild(_loadingNode, 10);

    // 取消按钮
    _cancelButton = Button::create();
    _cancelButton->setTitleText("取消匹配");
    _cancelButton->setTitleFontSize(24);
    _cancelButton->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 100));
    _cancelButton->addClickEventListener([this](Ref*) { cancelMatching(); });
    this->addChild(_cancelButton, 10);

    // 攻击按钮（初始隐藏）
    _attackButton = Button::create();
    _attackButton->setTitleText("开始攻击!");
    _attackButton->setTitleFontSize(28);
    _attackButton->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + 100));
    _attackButton->setVisible(false);
    _attackButton->addClickEventListener([this](Ref*) { startBattle(); });
    this->addChild(_attackButton, 10);

    // 结束按钮（初始隐藏）
    _endButton = Button::create();
    _endButton->setTitleText("结束战斗");
    _endButton->setTitleFontSize(24);
    _endButton->setPosition(Vec2(origin.x + visibleSize.width - 100, origin.y + 100));
    _endButton->setVisible(false);
    _endButton->addClickEventListener([this](Ref*) { endBattle(); });
    this->addChild(_endButton, 10);
}

void BattleScene::setupCallbacks()
{
    auto& client = SocketClient::getInstance();

    client.setOnMatchFound([this](const MatchInfo& info) { onMatchFound(info); });

    client.setOnAttackStart([this](const std::string& mapData) { onAttackStart(mapData); });

    client.setOnDisconnected([this]() { onDisconnected(); });
}

void BattleScene::update(float dt)
{
    // 处理网络回调
    SocketClient::getInstance().processCallbacks();

    // 更新计时器
    if (_state == BattleState::FIGHTING)
    {
        updateTimer(dt);
    }
}

void BattleScene::startMatching()
{
    _state = BattleState::MATCHING;
    _statusLabel->setString("正在搜索对手...");
    _loadingNode->setVisible(true);
    _cancelButton->setVisible(true);
    _attackButton->setVisible(false);

    SocketClient::getInstance().findMatch();
}

void BattleScene::cancelMatching()
{
    SocketClient::getInstance().cancelMatch();
    returnToMain();
}

void BattleScene::onMatchFound(const MatchInfo& matchInfo)
{
    _opponentId = matchInfo.opponentId;
    _opponentTrophies = matchInfo.opponentTrophies;

    _state = BattleState::PREPARING;
    _statusLabel->setString("匹配成功! 对手: " + _opponentId + " (奖杯: " + std::to_string(_opponentTrophies) + ")");
    _loadingNode->setVisible(false);
    _cancelButton->setVisible(false);

    // 请求对手地图
    SocketClient::getInstance().startAttack(_opponentId);
}

void BattleScene::onAttackStart(const std::string& mapData)
{
    _opponentMapData = mapData;

    _statusLabel->setString("准备攻击 " + _opponentId + " 的村庄!");
    _attackButton->setVisible(true);

    // TODO: 加载对手地图显示
    // loadOpponentMap(mapData);
}

void BattleScene::onDisconnected()
{
    _statusLabel->setString("连接已断开!");

    auto delay = DelayTime::create(2.0f);
    auto callback = CallFunc::create([this]() { returnToMain(); });
    this->runAction(Sequence::create(delay, callback, nullptr));
}

void BattleScene::setOpponentInfo(const std::string& opponentId, int opponentTrophies)
{
    _opponentId = opponentId;
    _opponentTrophies = opponentTrophies;
}

void BattleScene::setOpponentMap(const std::string& mapData)
{
    _opponentMapData = mapData;
}

void BattleScene::startBattle()
{
    _state = BattleState::FIGHTING;
    _battleTime = 180.0f;  // 3分钟

    _statusLabel->setString("战斗中!");
    _attackButton->setVisible(false);
    _timerLabel->setVisible(true);
    _starsLabel->setVisible(true);
    _lootLabel->setVisible(true);
    _endButton->setVisible(true);

    // 模拟战斗（实际应该是玩家操作）
    simulateBattle();
}

void BattleScene::updateTimer(float dt)
{
    _battleTime -= dt;

    if (_battleTime <= 0)
    {
        _battleTime = 0;
        endBattle();
    }

    int minutes = static_cast<int>(_battleTime) / 60;
    int seconds = static_cast<int>(_battleTime) % 60;
    _timerLabel->setString(StringUtils::format("%d:%02d", minutes, seconds));
}

void BattleScene::simulateBattle()
{
    // 模拟战斗效果（实际应该根据玩家操作计算）
    auto simulate = [this]() {
        // 随机增加掠夺
        _goldLooted += rand() % 100;
        _elixirLooted += rand() % 100;
        _destructionPercent += rand() % 5;

        if (_destructionPercent > 100)
        {
            _destructionPercent = 100;
        }

        // 计算星星
        if (_destructionPercent >= 50 && _starsEarned < 1)
        {
            _starsEarned = 1;
        }
        if (_destructionPercent >= 75 && _starsEarned < 2)
        {
            _starsEarned = 2;
        }
        if (_destructionPercent >= 100 && _starsEarned < 3)
        {
            _starsEarned = 3;
        }

        // 更新UI
        std::string starsStr = "";
        for (int i = 0; i < 3; i++)
        {
            starsStr += (i < _starsEarned) ? "★" : "☆";
        }
        _starsLabel->setString(starsStr);
        _starsLabel->setTextColor(_starsEarned > 0 ? Color4B::YELLOW : Color4B::GRAY);

        _lootLabel->setString(
            StringUtils::format("Gold: %d  Elixir: %d  %d%%", _goldLooted, _elixirLooted, _destructionPercent));
    };

    // 每秒模拟一次
    schedule([simulate](float) { simulate(); }, 1.0f, "simulate_battle");
}

void BattleScene::endBattle()
{
    _state = BattleState::FINISHED;

    unschedule("simulate_battle");

    // 发送战斗结果
    auto account = AccountManager::getInstance().getCurrentAccount();

    AttackResult result;
    result.attackerId = account ? account->userId : "unknown";
    result.defenderId = _opponentId;
    result.starsEarned = _starsEarned;
    result.goldLooted = _goldLooted;
    result.elixirLooted = _elixirLooted;
    result.trophyChange = _starsEarned * 10 - (3 - _starsEarned) * 5;  // 简单计算

    SocketClient::getInstance().submitAttackResult(result);

    showResult();
}

void BattleScene::showResult()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 结果面板
    auto panel = LayerColor::create(Color4B(0, 0, 0, 200));
    panel->setContentSize(Size(400, 300));
    panel->setPosition(Vec2(origin.x + (visibleSize.width - 400) / 2, origin.y + (visibleSize.height - 300) / 2));
    this->addChild(panel, 100);

    // 标题
    auto title = Label::createWithSystemFont("战斗结束!", "Arial", 36);
    title->setPosition(Vec2(200, 260));
    panel->addChild(title);

    // 星星
    std::string starsStr = "";
    for (int i = 0; i < 3; i++)
    {
        starsStr += (i < _starsEarned) ? "★" : "☆";
    }
    auto starsLabel = Label::createWithSystemFont(starsStr, "Arial", 48);
    starsLabel->setPosition(Vec2(200, 200));
    starsLabel->setTextColor(Color4B::YELLOW);
    panel->addChild(starsLabel);

    // 掠夺信息
    auto lootLabel = Label::createWithSystemFont(
        StringUtils::format("掠夺金币: %d\n掠夺圣水: %d\n摧毁: %d%%", _goldLooted, _elixirLooted, _destructionPercent),
        "Arial", 24);
    lootLabel->setPosition(Vec2(200, 130));
    panel->addChild(lootLabel);

    // 奖杯变化
    int trophyChange = _starsEarned * 10 - (3 - _starsEarned) * 5;
    auto trophyLabel = Label::createWithSystemFont(
        StringUtils::format("奖杯: %s%d", trophyChange >= 0 ? "+" : "", trophyChange), "Arial", 28);
    trophyLabel->setPosition(Vec2(200, 70));
    trophyLabel->setTextColor(trophyChange >= 0 ? Color4B::GREEN : Color4B::RED);
    panel->addChild(trophyLabel);

    // 返回按钮
    auto returnBtn = Button::create();
    returnBtn->setTitleText("返回");
    returnBtn->setTitleFontSize(24);
    returnBtn->setPosition(Vec2(200, 25));
    returnBtn->addClickEventListener([this](Ref*) { returnToMain(); });
    panel->addChild(returnBtn);

    // 隐藏其他UI
    _timerLabel->setVisible(false);
    _endButton->setVisible(false);
    _statusLabel->setVisible(false);
}

void BattleScene::returnToMain()
{
    auto scene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, scene, Color3B::BLACK));
}