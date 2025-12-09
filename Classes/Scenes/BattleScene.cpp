#include "BattleScene.h"
#include "DraggableMapScene.h"
#include "AccountManager.h"
#include "BuildingManager.h"
#include "GridMap.h"
#include "ResourceManager.h"
#include "Buildings/BaseBuilding.h"
#include "Buildings/DefenseBuilding.h"

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
    
    // ==================== ⭐ 设置士兵部署按钮 ====================
    setupTroopButtons();
}

// ==================== ⭐ 新增：设置士兵部署按钮 ====================
void BattleScene::setupTroopButtons()
{
    float buttonY = 150;
    float buttonSize = 80;
    float buttonSpacing = 100;
    float startX = (_visibleSize.width - buttonSpacing * 2) / 2;
    
    // 野蛮人按钮
    _barbarianButton = Button::create();
    _barbarianButton->setTitleText("野蛮人");
    _barbarianButton->setTitleFontSize(18);
    _barbarianButton->setScale9Enabled(true);
    _barbarianButton->setContentSize(Size(buttonSize, buttonSize));
    _barbarianButton->setPosition(Vec2(startX, buttonY));
    _barbarianButton->setVisible(false);
    _barbarianButton->addClickEventListener([this](Ref*) {
        onTroopButtonClicked(UnitType::kBarbarian);
    });
    this->addChild(_barbarianButton, 100);
    
    _barbarianCountLabel = Label::createWithSystemFont("20", "Arial", 24);
    _barbarianCountLabel->setPosition(Vec2(startX, buttonY - 50));
    _barbarianCountLabel->setTextColor(Color4B::WHITE);
    _barbarianCountLabel->setVisible(false);
    this->addChild(_barbarianCountLabel, 100);
    
    // 弓箭手按钮
    _archerButton = Button::create();
    _archerButton->setTitleText("弓箭手");
    _archerButton->setTitleFontSize(18);
    _archerButton->setScale9Enabled(true);
    _archerButton->setContentSize(Size(buttonSize, buttonSize));
    _archerButton->setPosition(Vec2(startX + buttonSpacing, buttonY));
    _archerButton->setVisible(false);
    _archerButton->addClickEventListener([this](Ref*) {
        onTroopButtonClicked(UnitType::kArcher);
    });
    this->addChild(_archerButton, 100);
    
    _archerCountLabel = Label::createWithSystemFont("20", "Arial", 24);
    _archerCountLabel->setPosition(Vec2(startX + buttonSpacing, buttonY - 50));
    _archerCountLabel->setTextColor(Color4B::WHITE);
    _archerCountLabel->setVisible(false);
    this->addChild(_archerCountLabel, 100);
    
    // 巨人按钮
    _giantButton = Button::create();
    _giantButton->setTitleText("巨人");
    _giantButton->setTitleFontSize(18);
    _giantButton->setScale9Enabled(true);
    _giantButton->setContentSize(Size(buttonSize, buttonSize));
    _giantButton->setPosition(Vec2(startX + buttonSpacing * 2, buttonY));
    _giantButton->setVisible(false);
    _giantButton->addClickEventListener([this](Ref*) {
        onTroopButtonClicked(UnitType::kGiant);
    });
    this->addChild(_giantButton, 100);
    
    _giantCountLabel = Label::createWithSystemFont("5", "Arial", 24);
    _giantCountLabel->setPosition(Vec2(startX + buttonSpacing * 2, buttonY - 50));
    _giantCountLabel->setTextColor(Color4B::WHITE);
    _giantCountLabel->setVisible(false);
    this->addChild(_giantCountLabel, 100);
    
    // 添加触摸监听器，用于部署士兵
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->onTouchBegan = [this](Touch* touch, Event* event) {
        if (_state != BattleState::READY && _state != BattleState::FIGHTING)
            return false;
        
        Vec2 touchPos = touch->getLocation();
        Vec2 mapLocalPos = _mapSprite->convertToNodeSpace(touchPos);
        
        // 部署士兵
        deployUnit(_selectedUnitType, mapLocalPos);
        return true;
    };
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
}

// ==================== ⭐ 新增：士兵部署逻辑 ====================
void BattleScene::deployUnit(UnitType type, const cocos2d::Vec2& position)
{
    int* count = nullptr;
    
    switch (type)
    {
    case UnitType::kBarbarian:
        count = &_barbarianCount;
        break;
    case UnitType::kArcher:
        count = &_archerCount;
        break;
    case UnitType::kGiant:
        count = &_giantCount;
        break;
    default:
        return;
    }
    
    if (*count <= 0)
    {
        CCLOG("⚠️ No more units of this type!");
        return;
    }
    
    // 创建士兵
    Unit* unit = Unit::create(type);
    if (!unit)
    {
        CCLOG("❌ Failed to create unit!");
        return;
    }
    
    unit->setPosition(position);
    _mapSprite->addChild(unit, 100);
    _deployedUnits.push_back(unit);
    
    (*count)--;
    updateTroopCounts();
    
    // 开始战斗（第一个士兵部署时）
    if (_state == BattleState::READY)
    {
        _state = BattleState::FIGHTING;
        activateDefenseBuildings();
    }
    
    CCLOG("✅ Deployed unit at (%.1f, %.1f), remaining: %d", position.x, position.y, *count);
}

void BattleScene::onTroopButtonClicked(UnitType type)
{
    _selectedUnitType = type;
    
    // 高亮选中的按钮
    _barbarianButton->setScale(type == UnitType::kBarbarian ? 1.1f : 1.0f);
    _archerButton->setScale(type == UnitType::kArcher ? 1.1f : 1.0f);
    _giantButton->setScale(type == UnitType::kGiant ? 1.1f : 1.0f);
    
    CCLOG("Selected unit type: %d", static_cast<int>(type));
}

void BattleScene::updateTroopCounts()
{
    _barbarianCountLabel->setString(StringUtils::format("%d", _barbarianCount));
    _archerCountLabel->setString(StringUtils::format("%d", _archerCount));
    _giantCountLabel->setString(StringUtils::format("%d", _giantCount));
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
    
    // ⭐ 显示士兵部署按钮
    _barbarianButton->setVisible(true);
    _archerButton->setVisible(true);
    _giantButton->setVisible(true);
    _barbarianCountLabel->setVisible(true);
    _archerCountLabel->setVisible(true);
    _giantCountLabel->setVisible(true);
    
    // 获取敌方建筑列表并计算总血量
    if (_buildingManager)
    {
        const auto& buildings = _buildingManager->getBuildings();
        _enemyBuildings.clear();
        _enemyBuildings.assign(buildings.begin(), buildings.end());
        
        _totalBuildingHP = 0;
        _destroyedBuildingHP = 0;
        
        for (auto* building : _enemyBuildings)
        {
            if (building)
            {
                _totalBuildingHP += building->getMaxHitpoints();
            }
        }
        
        CCLOG("📊 Total buildings: %zu, Total HP: %d", _enemyBuildings.size(), _totalBuildingHP);
    }

    CCLOG("⚔️ Battle started! Click on map to deploy troops!");
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

    // ⭐ 更新所有士兵的 AI
    updateUnitAI(dt);
    
    // ⭐ 更新防御建筑的攻击逻辑
    for (auto* building : _enemyBuildings)
    {
        if (building && building->isDefenseBuilding())
        {
            auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(building);
            if (defenseBuilding)
            {
                defenseBuilding->tick(dt);
                defenseBuilding->detectEnemies(_deployedUnits);
            }
        }
    }
    
    // ⭐ 计算摧毁百分比
    _destroyedBuildingHP = 0;
    int destroyedCount = 0;
    
    for (auto* building : _enemyBuildings)
    {
        if (building)
        {
            int lostHP = building->getMaxHitpoints() - building->getHitpoints();
            _destroyedBuildingHP += lostHP;
            
            if (building->isDestroyed())
            {
                destroyedCount++;
            }
        }
    }
    
    if (_totalBuildingHP > 0)
    {
        int newDestruction = (_destroyedBuildingHP * 100) / _totalBuildingHP;
        if (newDestruction != _destructionPercent)
        {
            updateDestruction(newDestruction);
        }
    }
    
    // 检查是否所有建筑被摧毁
    if (destroyedCount == _enemyBuildings.size() && !_enemyBuildings.empty())
    {
        CCLOG("🎉 All buildings destroyed!");
        endBattle(false);
    }
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
    auto& resMgr = ResourceManager::getInstance();
    resMgr.addResource(ResourceType::kGold, _goldLooted);
    resMgr.addResource(ResourceType::kElixir, _elixirLooted);
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

// ==================== ⭐ 新增：士兵 AI 更新逻辑 ====================
void BattleScene::updateUnitAI(float dt)
{
    for (auto it = _deployedUnits.begin(); it != _deployedUnits.end();)
    {
        Unit* unit = *it;
        
        // 移除已死亡的士兵
        if (!unit || unit->IsDead())
        {
            it = _deployedUnits.erase(it);
            continue;
        }
        
        // 如果士兵没有目标，寻找最近的建筑
        BaseBuilding* target = unit->getTarget();
        
        if (!target || target->isDestroyed())
        {
            // 寻找最近的建筑作为目标
            BaseBuilding* closestBuilding = nullptr;
            float closestDistance = 99999.0f;
            
            Vec2 unitWorldPos = unit->getParent()->convertToWorldSpace(unit->getPosition());
            
            for (auto* building : _enemyBuildings)
            {
                if (!building || building->isDestroyed())
                    continue;
                
                // 根据士兵类型选择目标
                CombatStats& unitStats = unit->getCombatStats();
                
                // 巨人优先攻击防御建筑
                if (unit->GetType() == UnitType::kGiant)
                {
                    if (!building->isDefenseBuilding())
                        continue;
                }
                
                // 哥布林优先攻击资源建筑
                if (unit->GetType() == UnitType::kGoblin)
                {
                    if (building->getBuildingType() != BuildingType::kResource)
                        continue;
                }
                
                Vec2 buildingWorldPos = building->getParent()->convertToWorldSpace(building->getPosition());
                float distance = unitWorldPos.distance(buildingWorldPos);
                
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestBuilding = building;
                }
            }
            
            if (closestBuilding)
            {
                unit->setTarget(closestBuilding);
                target = closestBuilding;
            }
        }
        
        // 如果有目标，移动并攻击
        if (target && !target->isDestroyed())
        {
            Vec2 unitPos = unit->getPosition();
            Vec2 targetPos = target->getPosition();
            
            // 如果在攻击范围内，攻击
            if (unit->isInAttackRange(targetPos))
            {
                // 攻击冷却
                static std::map<Unit*, float> attackCooldowns;
                
                if (attackCooldowns.find(unit) == attackCooldowns.end())
                {
                    attackCooldowns[unit] = 0.0f;
                }
                
                attackCooldowns[unit] -= dt;
                
                if (attackCooldowns[unit] <= 0.0f)
                {
                    // 播放攻击动画
                    unit->Attack(false);
                    
                    // 对建筑造成伤害
                    target->takeDamage(unit->getDamage());
                    
                    // 重置冷却时间
                    attackCooldowns[unit] = unit->getCombatStats().attackSpeed;
                    
                    CCLOG("⚔️ Unit attacks building! Damage: %d, Building HP: %d/%d",
                          unit->getDamage(),
                          target->getHitpoints(),
                          target->getMaxHitpoints());
                    
                    // 如果建筑被摧毁，清除目标
                    if (target->isDestroyed())
                    {
                        unit->clearTarget();
                        CCLOG("💥 Building destroyed!");
                    }
                }
            }
            else
            {
                // 移动到目标
                unit->MoveTo(targetPos);
            }
        }
        
        ++it;
    }
}

// ==================== ⭐ 新增：激活防御建筑 ====================
void BattleScene::activateDefenseBuildings()
{
    CCLOG("🏹 Activating defense buildings...");
    
    for (auto* building : _enemyBuildings)
    {
        if (building && building->isDefenseBuilding())
        {
            auto* defenseBuilding = dynamic_cast<DefenseBuilding*>(building);
            if (defenseBuilding)
            {
                defenseBuilding->setBattleMode(true);
                CCLOG("✅ Activated: %s", defenseBuilding->getDisplayName().c_str());
            }
        }
    }
}