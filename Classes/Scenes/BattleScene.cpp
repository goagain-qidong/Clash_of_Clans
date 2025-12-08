#include "BattleScene.h"
#include "DraggableMapScene.h"
#include "AccountManager.h"
#include "BuildingManager.h"
#include "GridMap.h"
#include "ResourceManager.h"

USING_NS_CC;
using namespace ui;

// ==================== 创建场景 ====================

Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

BattleScene* BattleScene::createWithEnemyData(const AccountGameData& enemyData)
{
    BattleScene* scene = new (std::nothrow) BattleScene();
    if (scene && scene->initWithEnemyData(enemyData))
    {
        scene->autorelease();
        return scene;
    }
    CC_SAFE_DELETE(scene);
    return nullptr;
}

// ==================== 初始化 ====================

bool BattleScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();
    
    setupMap();
    setupUI();

    // 显示提示：需要传入敌方数据
    _statusLabel->setString("错误：未加载敌方基地数据！");
    _statusLabel->setTextColor(Color4B::RED);

    return true;
}

bool BattleScene::initWithEnemyData(const AccountGameData& enemyData)
{
    if (!Scene::init())
    {
        return false;
    }

    _visibleSize = Director::getInstance()->getVisibleSize();
    _enemyGameData = enemyData;
    _enemyUserId = "Enemy";  // 从 enemyData 中提取（需要扩展结构）
    _enemyTownHallLevel = enemyData.townHallLevel;

    setupMap();
    setupUI();
    loadEnemyBase();

    scheduleUpdate();

    return true;
}

// ==================== 场景设置 ====================

void BattleScene::setupMap()
{
    // 创建地图背景
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);

    // 创建地图精灵
    _mapSprite = Sprite::create("map/Map1.png");  // 使用默认地图
    if (_mapSprite)
    {
        _mapSprite->setPosition(_visibleSize.width / 2, _visibleSize.height / 2);
        _mapSprite->setScale(1.3f);
        this->addChild(_mapSprite, 0);

        // 创建网格
        auto mapSize = _mapSprite->getContentSize();
        _gridMap = GridMap::create(mapSize, 55.6f);
        _gridMap->setStartPixel(Vec2(1406.0f, 2107.2f));
        _mapSprite->addChild(_gridMap, 999);

        // 创建建筑管理器
        _buildingManager = BuildingManager::create();
        this->addChild(_buildingManager);
        _buildingManager->setup(_mapSprite, _gridMap);
    }
}

void BattleScene::setupUI()
{
    // ==================== 顶部状态栏 ====================
    
    // 状态标签
    _statusLabel = Label::createWithSystemFont("正在加载敌方基地...", "Arial", 24);
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
    _starsLabel = Label::createWithSystemFont("☆☆☆", "Arial", 36);
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

    // ==================== 底部按钮 ====================

    // 结束战斗按钮
    _endBattleButton = Button::create();
    _endBattleButton->setTitleText("结束战斗");
    _endBattleButton->setTitleFontSize(24);
    _endBattleButton->setPosition(Vec2(_visibleSize.width - 100, 60));
    _endBattleButton->setVisible(false);
    _endBattleButton->addClickEventListener([this](Ref*) {
        endBattle(true);  // 投降
    });
    this->addChild(_endBattleButton, 100);

    // 返回按钮（战斗结束后显示）
    _returnButton = Button::create();
    _returnButton->setTitleText("返回主场景");
    _returnButton->setTitleFontSize(24);
    _returnButton->setPosition(Vec2(_visibleSize.width / 2, 60));
    _returnButton->setVisible(false);
    _returnButton->addClickEventListener([this](Ref*) {
        returnToMainScene();
    });
    this->addChild(_returnButton, 100);
}

// ==================== 加载敌方基地 ====================

void BattleScene::loadEnemyBase()
{
    if (!_buildingManager || _enemyGameData.buildings.empty())
    {
        _statusLabel->setString("错误：无法加载敌方基地！");
        _statusLabel->setTextColor(Color4B::RED);
        CCLOG("❌ Failed to load enemy base: no buildings data");
        return;
    }

    CCLOG("🏰 Loading enemy base with %zu buildings...", _enemyGameData.buildings.size());

    // 以只读模式加载敌方建筑（不允许升级）
    _buildingManager->loadBuildingsFromData(_enemyGameData.buildings, true);

    _statusLabel->setString(StringUtils::format("攻击 %s 的村庄 (大本营 Lv.%d)", 
                                                  _enemyUserId.c_str(), 
                                                  _enemyTownHallLevel));
    _statusLabel->setTextColor(Color4B::GREEN);

    // 延迟1秒后开始战斗
    this->scheduleOnce([this](float dt) {
        startBattle();
    }, 1.0f, "start_battle_delay");
}

// ==================== 战斗逻辑 ====================

void BattleScene::startBattle()
{
    _state = BattleState::READY;
    _elapsedTime = 0.0f;

    _statusLabel->setString("部署你的士兵进行攻击！");
    _timerLabel->setVisible(true);
    _starsLabel->setVisible(true);
    _destructionLabel->setVisible(true);
    _endBattleButton->setVisible(true);

    CCLOG("⚔️ Battle started!");

    // TODO: 玩家可以开始部署士兵
    // 这里需要实现士兵部署UI和逻辑
}

void BattleScene::update(float dt)
{
    if (_state == BattleState::READY || _state == BattleState::FIGHTING)
    {
        updateBattleState(dt);
    }
}

void BattleScene::updateBattleState(float dt)
{
    _elapsedTime += dt;
    float remainingTime = _battleTime - _elapsedTime;

    if (remainingTime <= 0)
    {
        remainingTime = 0;
        endBattle(false);  // 时间耗尽，自动结束
    }

    updateTimer();

    // TODO: 更新战斗状态
    // - 士兵移动、攻击
    // - 建筑被摧毁
    // - 计算摧毁百分比和星数
}

void BattleScene::updateTimer()
{
    float remainingTime = _battleTime - _elapsedTime;
    int minutes = static_cast<int>(remainingTime) / 60;
    int seconds = static_cast<int>(remainingTime) % 60;
    _timerLabel->setString(StringUtils::format("%d:%02d", minutes, seconds));

    // 时间快用完时变红
    if (remainingTime < 30)
    {
        _timerLabel->setTextColor(Color4B::RED);
    }
}

void BattleScene::updateStars(int stars)
{
    _starsEarned = std::min(stars, 3);

    std::string starsStr = "";
    for (int i = 0; i < 3; i++)
    {
        starsStr += (i < _starsEarned) ? "★" : "☆";
    }
    _starsLabel->setString(starsStr);
    _starsLabel->setTextColor(_starsEarned > 0 ? Color4B::YELLOW : Color4B::GRAY);
}

void BattleScene::updateDestruction(int percent)
{
    _destructionPercent = std::min(percent, 100);
    _destructionLabel->setString(StringUtils::format("%d%%", _destructionPercent));

    // 根据摧毁百分比更新星数
    int stars = 0;
    if (_destructionPercent >= 50) stars = 1;
    if (_destructionPercent >= 70) stars = 2;
    if (_destructionPercent == 100) stars = 3;

    if (stars > _starsEarned)
    {
        updateStars(stars);
    }
}

void BattleScene::endBattle(bool surrender)
{
    if (_state == BattleState::FINISHED)
        return;

    _state = BattleState::FINISHED;

    calculateBattleResult();
    showBattleResult();

    // 上传战斗结果（可选）
    uploadBattleResult();

    CCLOG("⚔️ Battle ended! Stars: %d, Destruction: %d%%, Gold: %d, Elixir: %d",
          _starsEarned, _destructionPercent, _goldLooted, _elixirLooted);
}

void BattleScene::calculateBattleResult()
{
    // 简化计算：基于摧毁百分比和敌方资源
    int maxGold = _enemyGameData.gold;
    int maxElixir = _enemyGameData.elixir;

    // 掠夺量 = 敌方资源 * (摧毁百分比 / 100) * 掠夺率
    float lootRate = 0.3f;  // 最多掠夺30%
    _goldLooted = static_cast<int>(maxGold * (_destructionPercent / 100.0f) * lootRate);
    _elixirLooted = static_cast<int>(maxElixir * (_destructionPercent / 100.0f) * lootRate);

    // 更新本地资源
    auto* resMgr = ResourceManager::GetInstance();
    resMgr->AddResource(ResourceType::kGold, _goldLooted);
    resMgr->AddResource(ResourceType::kElixir, _elixirLooted);
}

void BattleScene::showBattleResult()
{
    // 隐藏战斗UI
    _timerLabel->setVisible(false);
    _starsLabel->setVisible(false);
    _destructionLabel->setVisible(false);
    _endBattleButton->setVisible(false);

    // 创建结果面板
    auto panel = LayerColor::create(Color4B(0, 0, 0, 220));
    panel->setContentSize(Size(500, 400));
    panel->setPosition(Vec2((_visibleSize.width - 500) / 2, (_visibleSize.height - 400) / 2));
    panel->setName("result_panel");
    this->addChild(panel, 200);

    // 标题
    auto title = Label::createWithSystemFont("战斗结束!", "Arial", 42);
    title->setPosition(Vec2(250, 360));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    // 星数
    std::string starsStr = "";
    for (int i = 0; i < 3; i++)
    {
        starsStr += (i < _starsEarned) ? "★ " : "☆ ";
    }
    auto starsLabel = Label::createWithSystemFont(starsStr, "Arial", 56);
    starsLabel->setPosition(Vec2(250, 280));
    starsLabel->setTextColor(Color4B::YELLOW);
    panel->addChild(starsLabel);

    // 摧毁百分比
    auto destructionLabel = Label::createWithSystemFont(
        StringUtils::format("摧毁: %d%%", _destructionPercent), "Arial", 32);
    destructionLabel->setPosition(Vec2(250, 220));
    panel->addChild(destructionLabel);

    // 掠夺信息
    auto lootLabel = Label::createWithSystemFont(
        StringUtils::format("掠夺金币: +%d\n掠夺圣水: +%d", _goldLooted, _elixirLooted),
        "Arial", 28);
    lootLabel->setPosition(Vec2(250, 150));
    lootLabel->setAlignment(TextHAlignment::CENTER);
    lootLabel->setTextColor(Color4B::GREEN);
    panel->addChild(lootLabel);

    // 奖杯变化（简化计算）
    int trophyChange = _starsEarned * 10 - (3 - _starsEarned) * 3;
    auto trophyLabel = Label::createWithSystemFont(
        StringUtils::format("奖杯: %s%d", trophyChange >= 0 ? "+" : "", trophyChange),
        "Arial", 26);
    trophyLabel->setPosition(Vec2(250, 80));
    trophyLabel->setTextColor(trophyChange >= 0 ? Color4B::GREEN : Color4B::RED);
    panel->addChild(trophyLabel);

    // 显示返回按钮
    _statusLabel->setString("点击返回按钮回到主场景");
    _returnButton->setVisible(true);
}

void BattleScene::returnToMainScene()
{
    // ❌ 错误：replaceScene 会销毁旧场景，导致数据丢失
    // auto scene = DraggableMapScene::createScene();
    // Director::getInstance()->replaceScene(TransitionFade::create(0.5f, scene, Color3B::BLACK));
    
    // ✅ 正确：使用 popScene 返回到之前的 DraggableMapScene
    // 这样可以保留原场景的数据和状态
    Director::getInstance()->popScene();
    
    CCLOG("✅ Returned to main scene (data preserved)");
}

void BattleScene::uploadBattleResult()
{
    // TODO: 上传战斗结果到服务器（可选）
    // 包括：
    // - 攻击者和防守者ID
    // - 星数、掠夺资源、奖杯变化
    // - 战斗回放数据（可选）

    CCLOG("📤 Uploading battle result to server (not implemented)");
    
    /* 示例代码（需要服务器支持）:
    auto& client = SocketClient::getInstance();
    
    json result;
    result["attackerId"] = AccountManager::getInstance().getCurrentAccount()->userId;
    result["defenderId"] = _enemyUserId;
    result["starsEarned"] = _starsEarned;
    result["goldLooted"] = _goldLooted;
    result["elixirLooted"] = _elixirLooted;
    result["trophyChange"] = _starsEarned * 10 - (3 - _starsEarned) * 3;
    
    client.uploadBattleResult(result.dump());
    */
}