/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.cpp
 * File Function: 部落面板主容器实现（重构版 - 三层架构）
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#include "ClanPanel.h"
#include "Audio/AudioManager.h"
#include "ClanDataCache.h"
#include "Managers/AccountManager.h"
#include "Managers/SocketClient.h"
#include "PlayerListItem.h"
#include "Scenes/BattleScene.h"
#include "Services/ClanService.h"

USING_NS_CC;
using namespace ui;

// ============================================================================
// 创建与初始化
// ============================================================================

ClanPanel* ClanPanel::create()
{
    ClanPanel* ret = new (std::nothrow) ClanPanel();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ClanPanel::init()
{
    if (!Layer::init())
        return false;

    // 半透明背景
    auto bg = LayerColor::create(Color4B(0, 0, 0, 200));
    this->addChild(bg);

    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    _isRefreshing = false;
    _isTransitioningToBattle = false;

    // 初始化服务层
    ClanService::getInstance().initialize();

    // 注册数据变更观察者
    ClanDataCache::getInstance().addObserver(this, [this](ClanDataChangeType type) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, type]() { onDataChanged(type); });
    });

    setupUI();

    // 根据连接状态更新UI
    bool isConnected = ClanService::getInstance().isConnected();
    if (_connectionNode)
        _connectionNode->setVisible(!isConnected);
    if (_memberNode)
        _memberNode->setVisible(isConnected);
    if (_clanManagementNode)
        _clanManagementNode->setVisible(isConnected);

    if (isConnected)
    {
        registerPvpCallbacks();
        ClanService::getInstance().requestClanList();
        scheduleRefresh();
    }

    return true;
}

void ClanPanel::onExit()
{
    Layer::onExit();

    // 移除观察者
    ClanDataCache::getInstance().removeObserver(this);

    // 🔧 修复：完全清除所有网络回调，防止残留
    auto& client = SocketClient::getInstance();
    client.setOnPvpStart(nullptr);
    client.setOnSpectateJoin(nullptr);
    
    // 🆕 Only clear PvpEnd if NOT transitioning to battle (BattleScene needs it)
    if (!_isTransitioningToBattle)
    {
        client.setOnPvpEnd(nullptr);
    }
    
    // 🆕 Never clear PvpAction here as ClanPanel doesn't use it, but BattleScene does
    // client.setOnPvpAction(nullptr); 

    client.setOnClanWarMatch(nullptr);
    client.setOnClanWarMemberList(nullptr);
    client.setOnClanWarAttackStart(nullptr);
    client.setOnBattleStatusList(nullptr);

    CCLOG("🔴 [ClanPanel] Network callbacks cleared on exit (Transitioning: %d)", _isTransitioningToBattle);

    unscheduleRefresh();
}

// ============================================================================
// UI 初始化
// ============================================================================

void ClanPanel::setupUI()
{
    Size visibleSize = Director::getInstance()->getVisibleSize();

    _panelNode = Node::create();
    _panelNode->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    this->addChild(_panelNode);

    // 背景板
    auto panelBg = LayerColor::create(Color4B(50, 50, 70, 255), 600, 500);
    panelBg->setPosition(-300, -300);
    _panelNode->addChild(panelBg);

    // 关闭按钮
    auto closeBtn = Button::create("icon/return_button.png");
    if (closeBtn->getContentSize().equals(Size::ZERO))
    {
        closeBtn = Button::create();
        closeBtn->ignoreContentAdaptWithSize(false);
        closeBtn->setContentSize(Size(40, 40));
        closeBtn->setTitleText("X");
        closeBtn->setTitleFontSize(24);
        if (closeBtn->getTitleRenderer())
            closeBtn->getTitleRenderer()->setPosition(Vec2(20, 20));
    }
    else
    {
        closeBtn->setScale(50.0f / closeBtn->getContentSize().width);
    }
    closeBtn->setPosition(Vec2(270, 170));
    closeBtn->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        this->removeFromParent();
    });
    _panelNode->addChild(closeBtn);

    setupConnectionUI();
    setupTabBar();
    setupListView();
    setupClanManagement();
}

void ClanPanel::setupConnectionUI()
{
    _connectionNode = Node::create();
    _panelNode->addChild(_connectionNode);

    auto title = Label::createWithSystemFont("连接服务器", "Arial", 30);
    title->setPosition(0, 120);
    _connectionNode->addChild(title);

    // IP 输入框背景
    auto ipBg = LayerColor::create(Color4B(30, 30, 30, 255), 300, 40);
    ipBg->setPosition(-150, 40);
    _connectionNode->addChild(ipBg);

    _ipInput = TextField::create("输入IP地址 (如 127.0.0.1)", "Arial", 20);
    _ipInput->setPosition(Vec2(0, 60));
    _ipInput->setTextHorizontalAlignment(TextHAlignment::CENTER);
    _ipInput->setTextColor(Color4B::WHITE);
    _ipInput->setCursorEnabled(true);
    _ipInput->setString("127.0.0.1");
    _connectionNode->addChild(_ipInput);

    // 端口输入框背景
    auto portBg = LayerColor::create(Color4B(30, 30, 30, 255), 300, 40);
    portBg->setPosition(-150, -20);
    _connectionNode->addChild(portBg);

    _portInput = TextField::create("输入端口 (如 8888)", "Arial", 20);
    _portInput->setPosition(Vec2(0, 0));
    _portInput->setTextHorizontalAlignment(TextHAlignment::CENTER);
    _portInput->setTextColor(Color4B::WHITE);
    _portInput->setCursorEnabled(true);
    _portInput->setString("8888");
    _connectionNode->addChild(_portInput);

    // 连接按钮
    auto connectBtn = Button::create();
    connectBtn->setTitleText("连接");
    connectBtn->setTitleFontSize(24);
    connectBtn->setPosition(Vec2(0, -80));
    connectBtn->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        onConnectClicked();
    });
    _connectionNode->addChild(connectBtn);
}

void ClanPanel::setupTabBar()
{
    _memberNode = Node::create();
    _panelNode->addChild(_memberNode);

    // 标题
    auto title = Label::createWithSystemFont("在线玩家", "Arial", 26);
    title->setPosition(0, 170);
    _memberNode->addChild(title);

    // 标签页背景
    float tabY  = 135;
    auto  tabBg = LayerColor::create(Color4B(40, 40, 60, 255), 560, 36);
    tabBg->setPosition(-280, tabY - 18);
    _memberNode->addChild(tabBg);

    float tabWidth  = 175;
    float tabHeight = 32;

    // 在线玩家标签
    _onlinePlayersTab = Button::create();
    _onlinePlayersTab->setTitleText("在线玩家");
    _onlinePlayersTab->setTitleFontSize(18);
    _onlinePlayersTab->setScale9Enabled(true);
    _onlinePlayersTab->setContentSize(Size(tabWidth, tabHeight));
    _onlinePlayersTab->setPosition(Vec2(-185, tabY));
    _onlinePlayersTab->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        switchToTab(TabType::ONLINE_PLAYERS);
    });
    _memberNode->addChild(_onlinePlayersTab);

    // 部落成员标签
    _clanMembersTab = Button::create();
    _clanMembersTab->setTitleText("部落成员");
    _clanMembersTab->setTitleFontSize(18);
    _clanMembersTab->setScale9Enabled(true);
    _clanMembersTab->setContentSize(Size(tabWidth, tabHeight));
    _clanMembersTab->setPosition(Vec2(0, tabY));
    _clanMembersTab->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        switchToTab(TabType::CLAN_MEMBERS);
    });
    _memberNode->addChild(_clanMembersTab);

    // 默认高亮在线玩家
    _onlinePlayersTab->setBright(false);
    _clanMembersTab->setBright(true);
}

void ClanPanel::setupListView()
{
    _memberList = ListView::create();
    _memberList->setContentSize(Size(560, 220));
    _memberList->setPosition(Vec2(-280, -110));
    _memberList->setBackGroundColor(Color3B(60, 60, 80));
    _memberList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _memberList->setItemsMargin(5);
    _memberList->setScrollBarEnabled(true);
    _memberNode->addChild(_memberList);

    // 刷新按钮
    auto refreshBtn = Button::create();
    refreshBtn->setTitleText("刷新列表");
    refreshBtn->setTitleFontSize(18);
    refreshBtn->setScale9Enabled(true);
    refreshBtn->setContentSize(Size(120, 36));
    refreshBtn->setPosition(Vec2(0, -260));
    refreshBtn->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        safeRefreshCurrentTab();
    });
    _memberNode->addChild(refreshBtn);
}

void ClanPanel::setupClanManagement()
{
    _clanManagementNode = Node::create();
    _clanManagementNode->setPosition(Vec2(0, -165));
    _panelNode->addChild(_clanManagementNode, 100);

    // 部落信息背景
    auto infoBg = LayerColor::create(Color4B(30, 30, 50, 200), 300, 30);
    infoBg->setPosition(-150, -15);
    _clanManagementNode->addChild(infoBg);

    // 部落信息标签
    _clanInfoLabel = Label::createWithSystemFont("未加入部落", "Arial", 16);
    _clanInfoLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _clanInfoLabel->setPosition(Vec2(0, 0));
    _clanInfoLabel->setTextColor(Color4B::YELLOW);
    _clanManagementNode->addChild(_clanInfoLabel);

    float btnY       = -40;
    float btnSpacing = 130;

    // 创建部落按钮
    _createClanBtn = Button::create();
    _createClanBtn->setTitleText("创建部落");
    _createClanBtn->setTitleFontSize(14);
    _createClanBtn->setScale9Enabled(true);
    _createClanBtn->setContentSize(Size(110, 32));
    _createClanBtn->setPosition(Vec2(-btnSpacing / 2, btnY));
    _createClanBtn->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        showCreateClanDialog();
    });
    _clanManagementNode->addChild(_createClanBtn);

    // 加入部落按钮
    _joinClanBtn = Button::create();
    _joinClanBtn->setTitleText("加入部落");
    _joinClanBtn->setTitleFontSize(14);
    _joinClanBtn->setScale9Enabled(true);
    _joinClanBtn->setContentSize(Size(110, 32));
    _joinClanBtn->setPosition(Vec2(btnSpacing / 2, btnY));
    _joinClanBtn->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        showClanListDialog();
    });
    _clanManagementNode->addChild(_joinClanBtn);

    // 退出部落按钮
    _leaveClanBtn = Button::create();
    _leaveClanBtn->setTitleText("退出部落");
    _leaveClanBtn->setTitleFontSize(14);
    _leaveClanBtn->setScale9Enabled(true);
    _leaveClanBtn->setContentSize(Size(110, 32));
    _leaveClanBtn->setPosition(Vec2(0, btnY));
    _leaveClanBtn->setVisible(false);
    _leaveClanBtn->addClickEventListener([this](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        onLeaveClanClicked();
    });
    _clanManagementNode->addChild(_leaveClanBtn);

    updateClanInfoDisplay();
}

// ============================================================================
// 数据变更响应
// ============================================================================

void ClanPanel::onDataChanged(ClanDataChangeType type)
{
    switch (type)
    {
    case ClanDataChangeType::ONLINE_PLAYERS:
        if (_currentTab == TabType::ONLINE_PLAYERS)
        {
            _isRefreshing = false;
            renderOnlinePlayers();
        }
        break;

    case ClanDataChangeType::CLAN_MEMBERS:
        if (_currentTab == TabType::CLAN_MEMBERS)
        {
            _isRefreshing = false;
            renderClanMembers();
        }
        break;

    case ClanDataChangeType::CLAN_WAR_MEMBERS:
        if (_currentTab == TabType::CLAN_WAR)
        {
            _isRefreshing = false;
            renderClanWarMembers();
        }
        break;

    case ClanDataChangeType::BATTLE_STATUS:
        // 战斗状态更新后，重新渲染当前列表
        if (!_isRefreshing)
        {
            switch (_currentTab)
            {
            case TabType::ONLINE_PLAYERS:
                renderOnlinePlayers();
                break;
            case TabType::CLAN_MEMBERS:
                renderClanMembers();
                break;
            case TabType::CLAN_WAR:
                renderClanWarMembers();
                break;
            }
        }
        break;

    case ClanDataChangeType::CLAN_LIST:
    case ClanDataChangeType::CLAN_INFO:
        updateClanInfoDisplay();
        break;
    }
}

// ============================================================================
// Tab 切换与刷新
// ============================================================================

void ClanPanel::switchToTab(TabType tab)
{
    _currentTab = tab;

    // 更新标签按钮样式
    _onlinePlayersTab->setBright(tab != TabType::ONLINE_PLAYERS);
    _clanMembersTab->setBright(tab != TabType::CLAN_MEMBERS);

    safeRefreshCurrentTab();
}

void ClanPanel::safeRefreshCurrentTab()
{
    if (_isRefreshing)
    {
        CCLOG("[ClanPanel] 刷新正在进行中，跳过本次请求");
        return;
    }
    refreshCurrentTab();
}

void ClanPanel::refreshCurrentTab()
{
    _isRefreshing = true;

    auto& service = ClanService::getInstance();
    service.requestBattleStatus();

    switch (_currentTab)
    {
    case TabType::ONLINE_PLAYERS:
        service.requestOnlinePlayers();
        break;
    case TabType::CLAN_MEMBERS:
        service.requestClanMembers();
        break;
    case TabType::CLAN_WAR:
        // 部落战需要特殊处理
        renderClanWarMembers();
        break;
    }
}

// ============================================================================
// 列表渲染
// ============================================================================

void ClanPanel::renderOnlinePlayers()
{
    _memberList->removeAllItems();

    auto& cache   = ClanDataCache::getInstance();
    auto& players = cache.getOnlinePlayers();

    if (players.empty())
    {
        renderEmptyState("没有其他在线玩家");
        return;
    }

    for (const auto& player : players)
    {
        auto status = cache.getBattleStatus(player.userId);
        auto item   = PlayerListItemWidget::createOnlinePlayer(
            player, status, [this](const std::string& id) { onAttackPlayer(id); },
            [this](const std::string& id) { onSpectatePlayer(id); });

        if (item)
            _memberList->pushBackCustomItem(item);
    }
}

void ClanPanel::renderClanMembers()
{
    _memberList->removeAllItems();

    auto& cache = ClanDataCache::getInstance();

    if (!cache.isInClan())
    {
        renderEmptyState("请先加入一个部落");
        _isRefreshing = false;
        return;
    }

    auto& members = cache.getClanMembers();

    if (members.empty())
    {
        renderEmptyState("部落中没有其他成员");
        return;
    }

    for (const auto& member : members)
    {
        auto status = cache.getBattleStatus(member.id);
        auto item   = PlayerListItemWidget::createClanMember(
            member, status, [this](const std::string& id) { onAttackPlayer(id); },
            [this](const std::string& id) { onSpectatePlayer(id); });

        if (item)
            _memberList->pushBackCustomItem(item);
    }
}

void ClanPanel::renderClanWarMembers()
{
    _memberList->removeAllItems();
    _isRefreshing = false;

    auto& cache = ClanDataCache::getInstance();

    if (!cache.isInClan())
    {
        renderEmptyState("请先加入一个部落", Color4B::YELLOW);
        return;
    }

    if (_currentWarId.empty())
    {
        // 显示搜索部落战按钮
        auto item = Layout::create();
        item->setContentSize(Size(560, 80));

        auto infoLabel = Label::createWithSystemFont("当前没有进行中的部落战", "Arial", 22);
        infoLabel->setTextColor(Color4B::GRAY);
        infoLabel->setPosition(Vec2(280, 50));
        item->addChild(infoLabel);

        auto searchBtn = Button::create();
        searchBtn->setTitleText("搜索部落战");
        searchBtn->setTitleFontSize(20);
        searchBtn->setScale9Enabled(true);
        searchBtn->setContentSize(Size(150, 40));
        searchBtn->setPosition(Vec2(280, 20));
        searchBtn->addClickEventListener([this](Ref*) {
            registerClanWarCallbacks();
            SocketClient::getInstance().searchClanWar();
            showToast("正在搜索部落战...");
        });
        item->addChild(searchBtn);

        _memberList->pushBackCustomItem(item);
        return;
    }

    auto& warMembers = cache.getClanWarMembers();

    if (warMembers.empty())
    {
        renderEmptyState("正在加载部落战成员...");
        return;
    }

    for (const auto& member : warMembers)
    {
        auto status = cache.getBattleStatus(member.userId);
        auto item   = PlayerListItemWidget::createClanWarMember(
            member, status, [this](const std::string& id) { onClanWarAttack(id); },
            [this](const std::string& id) { onClanWarSpectate(id); });

        if (item)
            _memberList->pushBackCustomItem(item);
    }
}

void ClanPanel::renderEmptyState(const std::string& message, const Color4B& color)
{
    auto item = Layout::create();
    item->setContentSize(Size(560, 60));

    auto label = Label::createWithSystemFont(message, "Arial", 20);
    label->setTextColor(color);
    label->setPosition(Vec2(280, 30));
    item->addChild(label);

    _memberList->pushBackCustomItem(item);
}

// ============================================================================
// 部落管理
// ============================================================================

void ClanPanel::updateClanInfoDisplay()
{
    auto& cache = ClanDataCache::getInstance();

    if (cache.isInClan())
    {
        std::string displayName = cache.getCurrentClanName();
        if (displayName.empty())
            displayName = cache.getCurrentClanId();

        _clanInfoLabel->setString(StringUtils::format("当前部落: %s", displayName.c_str()));

        // 🆕 已加入部落：显示退出按钮，隐藏创建/加入按钮
        _leaveClanBtn->setVisible(true);
        _createClanBtn->setVisible(false);
        _joinClanBtn->setVisible(false);
    }
    else
    {
        _clanInfoLabel->setString("未加入部落");

        // 🆕 未加入部落：隐藏退出按钮，显示创建/加入按钮
        _leaveClanBtn->setVisible(false);
        _createClanBtn->setVisible(true);
        _joinClanBtn->setVisible(true);
    }
}

void ClanPanel::showCreateClanDialog()
{
    auto layer = LayerColor::create(Color4B(0, 0, 0, 180));
    layer->setName("CreateClanDialog");
    this->addChild(layer, 10000);

    Size vs = Director::getInstance()->getVisibleSize();

    auto panel = LayerColor::create(Color4B(40, 40, 60, 255), 420, 180);
    panel->setPosition(Vec2((vs.width - 420) / 2, (vs.height - 180) / 2));
    layer->addChild(panel);

    auto title = Label::createWithSystemFont("创建部落", "Microsoft YaHei", 22);
    title->setPosition(Vec2(210, 145));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    auto input = TextField::create("输入部落名称", "Arial", 20);
    input->setPosition(Vec2(210, 95));
    input->setTextHorizontalAlignment(TextHAlignment::CENTER);
    input->setCursorEnabled(true);
    panel->addChild(input);

    auto okBtn = Button::create();
    okBtn->setTitleText("创建");
    okBtn->setScale9Enabled(true);
    okBtn->setContentSize(Size(140, 40));
    okBtn->setPosition(Vec2(140, 35));
    panel->addChild(okBtn);

    auto cancelBtn = Button::create();
    cancelBtn->setTitleText("取消");
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setContentSize(Size(140, 40));
    cancelBtn->setPosition(Vec2(280, 35));
    panel->addChild(cancelBtn);

    okBtn->addClickEventListener([this, layer, input](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        std::string name = input->getString();
        if (name.length() <= 6)
        {
            showToast("部落名长度必须大于6个字符", Color4B::RED);
            return;
        }

        ClanService::getInstance().createClan(name, [this](bool success, const std::string& msg) {
            showToast(msg, success ? Color4B::GREEN : Color4B::RED);
            if (success)
                safeRefreshCurrentTab();
        });

        layer->removeFromParent();
        showToast("正在创建部落...");
    });

    cancelBtn->addClickEventListener([layer](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        layer->removeFromParent();
    });
}

void ClanPanel::showClanListDialog()
{
    auto layer = LayerColor::create(Color4B(0, 0, 0, 180));
    layer->setName("ClanListDialog");
    this->addChild(layer, 10000);

    Size vs = Director::getInstance()->getVisibleSize();

    auto panel = LayerColor::create(Color4B(40, 40, 60, 255), 520, 360);
    panel->setPosition(Vec2((vs.width - 520) / 2, (vs.height - 360) / 2));
    layer->addChild(panel);

    auto title = Label::createWithSystemFont("选择部落加入", "Microsoft YaHei", 22);
    title->setPosition(Vec2(260, 320));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    auto list = ListView::create();
    list->setName("ClanListView");
    list->setContentSize(Size(480, 260));
    list->setPosition(Vec2(20, 40));
    list->setItemsMargin(8);
    list->setBackGroundColor(Color3B(50, 50, 70));
    list->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    panel->addChild(list);

    auto closeBtn = Button::create();
    closeBtn->setTitleText("关闭");
    closeBtn->setScale9Enabled(true);
    closeBtn->setContentSize(Size(100, 40));
    closeBtn->setPosition(Vec2(260, 20));
    closeBtn->addClickEventListener([this, layer](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        this->unschedule("fill_clan_list_delayed");
        layer->removeFromParent();
    });
    panel->addChild(closeBtn);

    auto fillList = [this, list, layer](const std::vector<ClanInfoClient>& clans) {
        if (!list || !layer)
            return;

        list->removeAllItems();

        if (clans.empty())
        {
            auto item = Layout::create();
            item->setContentSize(Size(480, 60));
            auto lbl = Label::createWithSystemFont("暂无部落，请先创建一个", "Arial", 20);
            lbl->setPosition(Vec2(240, 30));
            lbl->setTextColor(Color4B::GRAY);
            item->addChild(lbl);
            list->pushBackCustomItem(item);
            return;
        }

        for (const auto& c : clans)
        {
            auto item = Layout::create();
            item->setContentSize(Size(480, 60));
            item->setBackGroundColor(Color3B(60, 60, 80));
            item->setBackGroundColorType(Layout::BackGroundColorType::SOLID);

            auto name = Label::createWithSystemFont(c.clan_name, "Arial", 18);
            name->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            name->setPosition(Vec2(12, 36));
            name->setTextColor(Color4B::WHITE);
            item->addChild(name);

            // 显示部落信息
            auto info = Label::createWithSystemFont(
                StringUtils::format("%d 成员 • %d 奖杯", c.member_count, c.clan_trophies), "Arial", 14);
            info->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            info->setPosition(Vec2(12, 16));
            info->setTextColor(Color4B::GRAY);
            item->addChild(info);

            // 加入按钮
            auto joinBtn = Button::create();
            joinBtn->setTitleText("加入");
            joinBtn->setTitleFontSize(20);
            joinBtn->setScale9Enabled(true);
            joinBtn->setContentSize(Size(100, 36));
            joinBtn->setPosition(Vec2(420, 30));

            std::string clanId = c.clan_id;
            joinBtn->addClickEventListener([this, clanId, layer](Ref*) {
                AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
                onJoinClanClicked(clanId);
                if (layer && layer->getParent())
                    layer->removeFromParent();
            });
            item->addChild(joinBtn);

            list->pushBackCustomItem(item);
        }
    };

    // 使用缓存数据填充
    auto& cache = ClanDataCache::getInstance();
    auto& clans = cache.getClanList();

    if (!clans.empty())
    {
        fillList(clans);
    }
    else
    {
        auto item = Layout::create();
        item->setContentSize(Size(480, 60));
        auto lbl = Label::createWithSystemFont("正在加载部落列表...", "Arial", 20);
        lbl->setPosition(Vec2(240, 30));
        lbl->setTextColor(Color4B::GRAY);
        item->addChild(lbl);
        list->pushBackCustomItem(item);
    }

    // 请求最新数据
    ClanService::getInstance().requestClanList();

    // 延迟刷新
    this->scheduleOnce([fillList, &cache](float) { fillList(cache.getClanList()); }, 0.5f, "fill_clan_list_delayed");
}

void ClanPanel::showJoinClanFirstDialog()
{
    auto layer = LayerColor::create(Color4B(0, 0, 0, 200));
    layer->setName("JoinClanFirstDialog");
    this->addChild(layer, 10001);

    Size vs = Director::getInstance()->getVisibleSize();

    auto panel = LayerColor::create(Color4B(60, 40, 80, 255), 400, 200);
    panel->setPosition(Vec2((vs.width - 400) / 2, (vs.height - 200) / 2));
    layer->addChild(panel);

    auto title = Label::createWithSystemFont("需要加入部落", "Microsoft YaHei", 24);
    title->setPosition(Vec2(200, 160));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    auto desc = Label::createWithSystemFont("请先加入一个部落才能参与部落战", "Arial", 18);
    desc->setPosition(Vec2(200, 110));
    desc->setTextColor(Color4B::WHITE);
    panel->addChild(desc);

    auto createBtn = Button::create();
    createBtn->setTitleText("创建部落");
    createBtn->setTitleFontSize(18);
    createBtn->setScale9Enabled(true);
    createBtn->setContentSize(Size(140, 45));
    createBtn->setPosition(Vec2(100, 45));
    createBtn->addClickEventListener([this, layer](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        layer->removeFromParent();
        showCreateClanDialog();
    });
    panel->addChild(createBtn);

    auto joinBtn = Button::create();
    joinBtn->setTitleText("加入部落");
    joinBtn->setTitleFontSize(18);
    joinBtn->setScale9Enabled(true);
    joinBtn->setContentSize(Size(140, 45));
    joinBtn->setPosition(Vec2(300, 45));
    joinBtn->addClickEventListener([this, layer](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        layer->removeFromParent();
        showClanListDialog();
    });
    panel->addChild(joinBtn);
}

void ClanPanel::showLeaveClanConfirmDialog()
{
    auto layer = LayerColor::create(Color4B(0, 0, 0, 200));
    layer->setName("LeaveClanConfirmDialog");
    this->addChild(layer, 10001);

    Size vs = Director::getInstance()->getVisibleSize();

    auto panel = LayerColor::create(Color4B(60, 40, 80, 255), 400, 180);
    panel->setPosition(Vec2((vs.width - 400) / 2, (vs.height - 180) / 2));
    layer->addChild(panel);

    auto title = Label::createWithSystemFont("确认退出部落", "Microsoft YaHei", 24);
    title->setPosition(Vec2(200, 145));
    title->setTextColor(Color4B::YELLOW);
    panel->addChild(title);

    auto desc = Label::createWithSystemFont("退出后需要重新申请加入部落", "Arial", 18);
    desc->setPosition(Vec2(200, 100));
    desc->setTextColor(Color4B::WHITE);
    panel->addChild(desc);

    auto confirmBtn = Button::create();
    confirmBtn->setTitleText("确认退出");
    confirmBtn->setTitleFontSize(18);
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setContentSize(Size(140, 45));
    confirmBtn->setPosition(Vec2(100, 40));
    confirmBtn->addClickEventListener([this, layer](Ref*) {
        AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
        layer->removeFromParent();

        ClanService::getInstance().leaveClan([this](bool success, const std::string& msg) {
            showToast(msg, success ? Color4B::GREEN : Color4B::RED);
            if (success)
            {
                updateClanInfoDisplay();
                safeRefreshCurrentTab();
            }
        });
        showToast("正在退出部落...");
    });
    panel->addChild(confirmBtn);

    auto cancelBtn = Button::create();
    cancelBtn->setTitleText("取消");
    cancelBtn->setTitleFontSize(18);
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setContentSize(Size(140, 45));
    cancelBtn->setPosition(Vec2(300, 40));
    cancelBtn->addClickEventListener([layer](Ref*) { layer->removeFromParent(); });
    panel->addChild(cancelBtn);
}

// ============================================================================
// 网络回调注册
// ============================================================================

void ClanPanel::registerPvpCallbacks()
{
    auto& client = SocketClient::getInstance();

    // 先清除旧回调
    client.setOnPvpStart(nullptr);
    client.setOnSpectateJoin(nullptr);
    // 注意：不在这里清除 OnPvpEnd，因为 BattleScene 可能需要它

    // PVP 开始回调
    client.setOnPvpStart([this](const std::string& role, const std::string& opponentId, const std::string& mapData) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, role, opponentId, mapData]() {
            if (role == "ATTACK")
            {
                CCLOG("[ClanPanel] PVP开始 - 作为攻击方攻击: %s", opponentId.c_str());
                enterBattleScene(opponentId, mapData);
            }
            else if (role == "DEFEND")
            {
                CCLOG("[ClanPanel] 被玩家 %s 攻击", opponentId.c_str());
                showToast(StringUtils::format("你正在被玩家 %s 攻击!", opponentId.c_str()), Color4B::ORANGE);
            }
            else if (role == "FAIL")
            {
                std::string reason = opponentId;
                if (reason == "TARGET_OFFLINE")
                    showToast("目标玩家不在线", Color4B::RED);
                else if (reason == "NO_MAP")
                    showToast("目标玩家没有地图数据", Color4B::RED);
                else if (reason == "ALREADY_IN_BATTLE")
                    showToast("你或目标正在战斗中", Color4B::RED);
                else if (reason == "TARGET_IN_BATTLE")
                    showToast("目标玩家正在战斗中", Color4B::RED);
                else if (reason == "CANNOT_ATTACK_SELF")
                    showToast("不能攻击自己", Color4B::RED);
                else
                    showToast("发起战斗失败: " + reason, Color4B::RED);
            }
        });
    });

    // 观战回调 - 使用 SpectateInfo 结构体
    client.setOnSpectateJoin([this](const SpectateInfo& info) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, info]() {
            if (info.success)
            {
                CCLOG("[ClanPanel] 观战加入成功: %s vs %s (已进行: %lldms, 历史: %zu 操作)", 
                      info.attacker_id.c_str(), info.defender_id.c_str(), 
                      static_cast<long long>(info.elapsed_ms), info.action_history.size());
                enterSpectateScene(info.attacker_id, info.defender_id, info.map_data, 
                                   info.elapsed_ms, info.action_history);
            }
            else
            {
                showToast("该玩家当前没有进行中的战斗", Color4B::RED);
            }
        });
    });

    // PVP 结束回调 - 仅在 ClanPanel 可见时处理
    client.setOnPvpEnd([this](const std::string& result) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, result]() {
            // 只有当 ClanPanel 可见时才处理
            if (this->isVisible())
            {
                CCLOG("[ClanPanel] PVP结束: %s", result.c_str());
                showToast("战斗已结束");
                this->scheduleOnce([this](float) { safeRefreshCurrentTab(); }, 0.5f, "delayed_refresh_after_pvp");
            }
        });
    });

    CCLOG("✅ [ClanPanel] PVP callbacks registered");
}

void ClanPanel::registerClanWarCallbacks()
{
    auto& client = SocketClient::getInstance();

    client.setOnClanWarMatch([this](const std::string& warId, const std::string& clan1Id, const std::string& clan2Id) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, warId]() {
            _currentWarId = warId;
            ClanDataCache::getInstance().setCurrentWarId(warId);
            CCLOG("[ClanWar] 匹配成功! WarID: %s", warId.c_str());
            showToast("部落战匹配成功！", Color4B::GREEN);
            safeRefreshCurrentTab();
        });
    });

    client.setOnClanWarAttackStart(
        [this](const std::string& type, const std::string& targetId, const std::string& mapData) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, type, targetId, mapData]() {
                if (type == "ATTACK")
                {
                    CCLOG("[ClanWar] 攻击开始，加载战斗场景...");
                    enterBattleScene(targetId, mapData);
                }
                else
                {
                    CCLOG("[ClanWar] 攻击失败: %s", type.c_str());
                    showToast("攻击失败: " + type, Color4B::RED);
                }
            });
        });
}

// ============================================================================
// 动作处理
// ============================================================================

void ClanPanel::onConnectClicked()
{
    std::string ip      = _ipInput->getString();
    std::string portStr = _portInput->getString();

    if (ip.empty() || portStr.empty())
    {
        showToast("请输入IP和端口", Color4B::YELLOW);
        return;
    }

    int port = 0;
    try
    {
        port = std::stoi(portStr);
    }
    catch (...)
    {
        showToast("端口格式错误", Color4B::RED);
        return;
    }

    showToast("正在连接...");

    ClanService::getInstance().connect(ip, port, [this](bool success, const std::string& msg) {
        showToast(msg, success ? Color4B::GREEN : Color4B::RED);

        if (success)
        {
            if (_connectionNode)
                _connectionNode->setVisible(false);
            if (_memberNode)
                _memberNode->setVisible(true);
            if (_clanManagementNode)
                _clanManagementNode->setVisible(true);

            registerPvpCallbacks();
            ClanService::getInstance().requestClanList();
            scheduleRefresh();
            switchToTab(TabType::ONLINE_PLAYERS);
        }
    });
}

void ClanPanel::onAttackPlayer(const std::string& playerId)
{
    auto& accMgr = AccountManager::getInstance();
    if (auto cur = accMgr.getCurrentAccount())
    {
        if (cur->account.userId == playerId)
        {
            showToast("不能攻击自己", Color4B::YELLOW);
            return;
        }
    }

    CCLOG("[ClanPanel] 发起PVP攻击: %s", playerId.c_str());
    showToast("正在发起攻击...");
    SocketClient::getInstance().requestPvp(playerId);
}

void ClanPanel::onSpectatePlayer(const std::string& playerId)
{
    CCLOG("[ClanPanel] 请求观战: %s", playerId.c_str());
    showToast("正在加入观战...");
    SocketClient::getInstance().requestSpectate(playerId);
}

void ClanPanel::onClanWarAttack(const std::string& targetId)
{
    CCLOG("[ClanWar] 攻击目标: %s in war: %s", targetId.c_str(), _currentWarId.c_str());
    registerClanWarCallbacks();
    showToast("正在发起攻击...");
    SocketClient::getInstance().startClanWarAttack(_currentWarId, targetId);
}

void ClanPanel::onClanWarSpectate(const std::string& targetId)
{
    CCLOG("[ClanWar] 观战目标: %s in war: %s", targetId.c_str(), _currentWarId.c_str());

    auto& cache  = ClanDataCache::getInstance();
    auto  status = cache.getBattleStatus(targetId);

    if (!status.isInBattle)
    {
        showToast("该玩家当前没有进行中的战斗", Color4B::YELLOW);
        return;
    }

    showToast("正在加入观战...");
    SocketClient::getInstance().requestSpectate(targetId);
}

void ClanPanel::onJoinClanClicked(const std::string& clanId)
{
    ClanService::getInstance().joinClan(clanId, [this](bool success, const std::string& msg) {
        showToast(msg, success ? Color4B::GREEN : Color4B::RED);
        if (success)
            safeRefreshCurrentTab();
    });
    showToast("正在加入部落...");
}

// 🆕 退出部落按钮点击
void ClanPanel::onLeaveClanClicked()
{
    // 显示确认对话框
    showLeaveClanConfirmDialog();
}

// ============================================================================
// 场景切换
// ============================================================================

void ClanPanel::enterBattleScene(const std::string& targetId, const std::string& mapData)
{
    _isTransitioningToBattle = true;
    
    AccountGameData enemyData   = AccountGameData::fromJson(mapData);
    auto            scene       = BattleScene::createWithEnemyData(enemyData, targetId);
    auto            battleScene = dynamic_cast<BattleScene*>(scene);
    if (battleScene)
    {
        battleScene->setPvpMode(true);
    }

    // 使用 pushScene 而不是 replaceScene，以便返回时能恢复 ClanPanel
    Director::getInstance()->pushScene(TransitionFade::create(0.5f, scene));
}

void ClanPanel::enterSpectateScene(const std::string& attackerId, const std::string& defenderId,
                                   const std::string& mapData, int64_t elapsedMs,
                                   const std::vector<std::string>& history)
{
    _isTransitioningToBattle = true;
    
    AccountGameData enemyData   = AccountGameData::fromJson(mapData);
    auto            scene       = BattleScene::createWithEnemyData(enemyData, defenderId);
    auto            battleScene = dynamic_cast<BattleScene*>(scene);
    if (battleScene)
    {
        // 使用新的观战模式设置方法
        battleScene->setSpectateMode(attackerId, defenderId, elapsedMs, history);
    }

    // 使用 pushScene
    Director::getInstance()->pushScene(TransitionFade::create(0.5f, scene));
}

// ============================================================================
// 辅助方法
// ============================================================================

void ClanPanel::show()
{
    this->setVisible(true);

    bool isConnected = ClanService::getInstance().isConnected();
    if (_connectionNode)
        _connectionNode->setVisible(!isConnected);
    if (_memberNode)
        _memberNode->setVisible(isConnected);
    if (_clanManagementNode)
        _clanManagementNode->setVisible(isConnected);

    if (isConnected)
        safeRefreshCurrentTab();
}

void ClanPanel::hide()
{
    this->setVisible(false);
}

void ClanPanel::showToast(const std::string& message, const Color4B& color)
{
    auto scene = Director::getInstance()->getRunningScene();
    if (!scene)
        return;

    auto label = Label::createWithSystemFont(message, "Arial", 20);
    label->setTextColor(color);
    label->setPosition(Director::getInstance()->getVisibleSize() / 2);
    scene->addChild(label, 10000);

    auto move = MoveBy::create(1.0f, Vec2(0, 30));
    auto fade = FadeOut::create(1.0f);
    auto seq  = Sequence::create(Spawn::create(move, fade, nullptr), RemoveSelf::create(), nullptr);
    label->runAction(seq);
}

void ClanPanel::scheduleRefresh()
{
    this->schedule([this](float) { safeRefreshCurrentTab(); }, 5.0f, "clanpanel_refresh");
}

void ClanPanel::unscheduleRefresh()
{
    this->unschedule("clanpanel_refresh");
}