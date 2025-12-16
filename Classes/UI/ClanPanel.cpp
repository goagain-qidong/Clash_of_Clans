/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.cpp
 * File Function: 负责游戏部落面板
 * Author:        赵崇治
 * Update Date:   2025/12/16
 * License:       MIT License
 ****************************************************************/
#include "ClanPanel.h"
#include "Managers/AccountManager.h"
#include "Managers/SocketClient.h"
#include "Scenes/BattleScene.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"

USING_NS_CC;
using namespace ui;

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
    {
        return false;
    }

    // 半透明背景
    auto bg = LayerColor::create(Color4B(0, 0, 0, 200));
    this->addChild(bg);

    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* touch, Event* event) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    // 初始化锁，防止重入
    _isRefreshing = false;
    _pendingClanId.clear();

    setupUI();
    updateUIState();
    scheduleRefresh();

    return true;
}

void ClanPanel::setupUI()
{
    Size visibleSize = Director::getInstance()->getVisibleSize();

    _panelNode = Node::create();
    _panelNode->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    this->addChild(_panelNode);

    // 背景板
    auto bg = LayerColor::create(Color4B(50, 50, 70, 255), 600, 500);
    bg->setPosition(-300, -300);
    _panelNode->addChild(bg);

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
        {
            closeBtn->getTitleRenderer()->setPosition(Vec2(20, 20));
        }
    }
    else
    {
        closeBtn->setScale(50.0f / closeBtn->getContentSize().width);
    }
    closeBtn->setPosition(Vec2(270, 170));
    closeBtn->addClickEventListener([this](Ref*) { this->removeFromParent(); });
    _panelNode->addChild(closeBtn);

    setupConnectionUI();
    setupMemberUI();
    setupClanManagementUI();
}

void ClanPanel::setupConnectionUI()
{
    _connectionNode = Node::create();
    _panelNode->addChild(_connectionNode);

    auto title = Label::createWithSystemFont("连接服务器", "Arial", 30);
    title->setPosition(0, 120);
    _connectionNode->addChild(title);

    // IP Input
    auto ipBg = LayerColor::create(Color4B(30, 30, 30, 255), 300, 40);
    ipBg->setPosition(-150, 40);
    _connectionNode->addChild(ipBg);

    _ipInput = TextField::create("输入IP地址 (如 127.0.0.1)", "Arial", 20);
    _ipInput->setPosition(Vec2(0, 60));
    _ipInput->setTextHorizontalAlignment(TextHAlignment::CENTER);
    _ipInput->setTextColor(Color4B::WHITE);
    _ipInput->setCursorEnabled(true);
    _connectionNode->addChild(_ipInput);

    // Port Input
    auto portBg = LayerColor::create(Color4B(30, 30, 30, 255), 300, 40);
    portBg->setPosition(-150, -20);
    _connectionNode->addChild(portBg);

    _portInput = TextField::create("输入端口 (如 8888)", "Arial", 20);
    _portInput->setPosition(Vec2(0, 0));
    _portInput->setTextHorizontalAlignment(TextHAlignment::CENTER);
    _portInput->setTextColor(Color4B::WHITE);
    _portInput->setCursorEnabled(true);
    _connectionNode->addChild(_portInput);

    // Connect Button
    auto connectBtn = Button::create();
    connectBtn->setTitleText("连接");
    connectBtn->setTitleFontSize(24);
    connectBtn->setPosition(Vec2(0, -80));
    connectBtn->addClickEventListener([this](Ref*) { onConnectClicked(); });
    _connectionNode->addChild(connectBtn);

    // Default values
    _ipInput->setString("127.0.0.1");
    _portInput->setString("8888");
}

void ClanPanel::setupMemberUI()
{
    _memberNode = Node::create();
    _panelNode->addChild(_memberNode);

    // 标题
    auto title = Label::createWithSystemFont("在线玩家", "Arial", 26);
    title->setPosition(0, 170);
    _memberNode->addChild(title);

    // 标签页按钮 - 🆕 调整布局
    float tabY  = 135;
    auto  tabBg = LayerColor::create(Color4B(40, 40, 60, 255), 560, 36);
    tabBg->setPosition(-280, tabY - 18);
    _memberNode->addChild(tabBg);

    // 🆕 三个标签页平均分布
    float tabWidth  = 175;
    float tabHeight = 32;

    // 在线玩家标签
    _onlinePlayersTab = Button::create();
    _onlinePlayersTab->setTitleText("在线玩家");
    _onlinePlayersTab->setTitleFontSize(18);
    _onlinePlayersTab->setScale9Enabled(true);
    _onlinePlayersTab->setContentSize(Size(tabWidth, tabHeight));
    _onlinePlayersTab->setPosition(Vec2(-185, tabY));
    _onlinePlayersTab->addClickEventListener([this](Ref*) { switchToTab(TabType::ONLINE_PLAYERS); });
    _memberNode->addChild(_onlinePlayersTab);

    // 部落成员标签
    _clanMembersTab = Button::create();
    _clanMembersTab->setTitleText("部落成员");
    _clanMembersTab->setTitleFontSize(18);
    _clanMembersTab->setScale9Enabled(true);
    _clanMembersTab->setContentSize(Size(tabWidth, tabHeight));
    _clanMembersTab->setPosition(Vec2(0, tabY));
    _clanMembersTab->addClickEventListener([this](Ref*) { switchToTab(TabType::CLAN_MEMBERS); });
    _memberNode->addChild(_clanMembersTab);

    // 部落战标签
    //_clanWarTab = Button::create();
    //_clanWarTab->setTitleText("部落战");
    //_clanWarTab->setTitleFontSize(18);
    //_clanWarTab->setScale9Enabled(true);
    //_clanWarTab->setContentSize(Size(tabWidth, tabHeight));
    //_clanWarTab->setPosition(Vec2(185, tabY));
    //_clanWarTab->addClickEventListener([this](Ref*) {
    //    if (!_isInClan)
    //    {
    //        showJoinClanFirstDialog();
    //        return;
    //    }
    //    switchToTab(TabType::CLAN_WAR);
    //});
    //_memberNode->addChild(_clanWarTab);

    // 🆕 列表 - 调整高度和位置，为底部按钮留空间
    _memberList = ListView::create();
    _memberList->setContentSize(Size(560, 220));
    _memberList->setPosition(Vec2(-280, -110));
    _memberList->setBackGroundColor(Color3B(60, 60, 80));
    _memberList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _memberList->setItemsMargin(5);
    _memberList->setScrollBarEnabled(true);
    _memberNode->addChild(_memberList);

    // 🆕 刷新按钮 - 移到列表下方
    auto refreshBtn = Button::create();
    refreshBtn->setTitleText("刷新列表");
    refreshBtn->setTitleFontSize(18);
    refreshBtn->setScale9Enabled(true);
    refreshBtn->setContentSize(Size(120, 36));
    refreshBtn->setPosition(Vec2(0, -260));
    refreshBtn->addClickEventListener([this](Ref*) {
        if (!_isRefreshing)
        {
            refreshCurrentTab();
        }
    });
    _memberNode->addChild(refreshBtn);

    // 默认显示在线玩家
    switchToTab(TabType::ONLINE_PLAYERS);
}

void ClanPanel::setupClanManagementUI()
{
    _clanManagementNode = Node::create();
    // 🆕 调整位置，避免与标签页重叠
    _clanManagementNode->setPosition(Vec2(0, -165));
    _panelNode->addChild(_clanManagementNode, 100);

    // 当前部落信息显示背景
    auto infoBg = LayerColor::create(Color4B(30, 30, 50, 200), 300, 30);
    infoBg->setPosition(-150, -15);
    _clanManagementNode->addChild(infoBg);

    // 当前部落信息显示
    _clanInfoLabel = Label::createWithSystemFont("未加入部落", "Arial", 16);
    _clanInfoLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _clanInfoLabel->setPosition(Vec2(0, 0));
    _clanInfoLabel->setTextColor(Color4B::YELLOW);
    _clanManagementNode->addChild(_clanInfoLabel);

    // 🆕 按钮水平排列，间距调整
    float btnY = -40;
    float btnSpacing = 130;

    // Create Clan 按钮
    _createClanBtn = Button::create();
    _createClanBtn->setTitleText("创建部落");
    _createClanBtn->setTitleFontSize(14);
    _createClanBtn->setScale9Enabled(true);
    _createClanBtn->setContentSize(Size(110, 32));
    _createClanBtn->setPosition(Vec2(-btnSpacing / 2, btnY));
    _createClanBtn->addClickEventListener([this](Ref*) { showCreateClanDialog(); });
    _clanManagementNode->addChild(_createClanBtn);

    // Join Clan 按钮
    _joinClanBtn = Button::create();
    _joinClanBtn->setTitleText("加入部落");
    _joinClanBtn->setTitleFontSize(14);
    _joinClanBtn->setScale9Enabled(true);
    _joinClanBtn->setContentSize(Size(110, 32));
    _joinClanBtn->setPosition(Vec2(btnSpacing / 2, btnY));
    _joinClanBtn->addClickEventListener([this](Ref*) { showClanListDialog(); });
    _clanManagementNode->addChild(_joinClanBtn);

    // Leave button - 隐藏
    _leaveClanBtn = Button::create();
    _leaveClanBtn->setTitleText("退出部落");
    _leaveClanBtn->setTitleFontSize(14);
    _leaveClanBtn->setScale9Enabled(true);
    _leaveClanBtn->setContentSize(Size(110, 32));
    _leaveClanBtn->setPosition(Vec2(0, btnY));
    _leaveClanBtn->setVisible(false);
    _leaveClanBtn->addClickEventListener([this](Ref*) {
        showToast("加入部落后不能退出", Color4B::YELLOW);
    });
    _clanManagementNode->addChild(_leaveClanBtn);

    updateClanInfoDisplay();
}

void ClanPanel::updateClanInfoDisplay()
{
    auto& accMgr = AccountManager::getInstance();
    auto  cur    = accMgr.getCurrentAccount();
    if (cur && !cur->gameData.clanId.empty())
    {
        _currentClanId = cur->gameData.clanId;
    }

    if (!_currentClanId.empty())
    {
        _isInClan               = true;
        std::string displayName = _currentClanName.empty() ? _currentClanId : _currentClanName;
        _clanInfoLabel->setString(StringUtils::format("当前部落: %s", displayName.c_str()));
        _leaveClanBtn->setVisible(false); // 🆕 永不显示退出按钮
        _createClanBtn->setVisible(false);
        _joinClanBtn->setVisible(false);
    }
    else
    {
        _isInClan = false;
        _clanInfoLabel->setString("未加入部落");
        _leaveClanBtn->setVisible(false);
        _createClanBtn->setVisible(true);
        _joinClanBtn->setVisible(true);
    }

    //if (_clanWarTab)
    //{
    //    _clanWarTab->setEnabled(_isInClan);
    //    _clanWarTab->setBright(_isInClan);
    //}
}

// 🆕 强制加入部落对话框
void ClanPanel::showJoinClanFirstDialog()
{
    auto layer = LayerColor::create(Color4B(0, 0, 0, 200));
    layer->setName("JoinClanFirstDialog");
    this->addChild(layer, 10001); // 🆕 更高层级

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
        layer->removeFromParent();
        showClanListDialog();
    });
    panel->addChild(joinBtn);
}

void ClanPanel::registerPvpCallbacks()
{
    auto& client = SocketClient::getInstance();

    // PVP开始回调 - 当发起攻击后收到服务器响应
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
                std::string reason = opponentId; // 失败原因在opponentId字段
                if (reason == "TARGET_OFFLINE")
                {
                    showToast("目标玩家不在线", Color4B::RED);
                }
                else if (reason == "NO_MAP")
                {
                    showToast("目标玩家没有地图数据", Color4B::RED);
                }
                else if (reason == "ALREADY_IN_BATTLE")
                {
                    showToast("你或目标正在战斗中", Color4B::RED);
                }
                else
                {
                    showToast("发起战斗失败: " + reason, Color4B::RED);
                }
            }
        });
    });

    // 观战回调
    client.setOnSpectateJoin(
        [this](bool success, const std::string& attackerId, const std::string& defenderId, const std::string& mapData) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                [this, success, attackerId, defenderId, mapData]() {
                    if (success)
                    {
                        CCLOG("[ClanPanel] 观战加入成功: %s vs %s", attackerId.c_str(), defenderId.c_str());
                        enterSpectateScene(attackerId, defenderId, mapData);
                    }
                    else
                    {
                        CCLOG("[ClanPanel] 观战失败 - 目标可能不在战斗中");
                        showToast("该玩家当前没有进行中的战斗", Color4B::RED);
                    }
                });
        });

    // PVP操作回调（用于观战同步）
    client.setOnPvpAction([](int unitType, float x, float y) {
        CCLOG("[ClanPanel] 收到PVP操作: 单位类型=%d, 位置=(%.1f, %.1f)", unitType, x, y);
        // 这个回调会在BattleScene中处理，用于同步观战者看到的战斗动作
    });

    // PVP结束回调
    client.setOnPvpEnd([this](const std::string& result) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, result]() {
            CCLOG("[ClanPanel] PVP结束: %s", result.c_str());
            // 通知用户战斗结束
            showToast("战斗已结束");
            // 🆕 延迟刷新，避免与其他回调冲突
            this->scheduleOnce(
                [this](float) {
                    if (!_isRefreshing)
                    {
                        refreshCurrentTab();
                    }
                },
                0.5f, "delayed_refresh_after_pvp");
        });
    });
}

void ClanPanel::updateUIState()
{
    bool isConnected = SocketClient::getInstance().isConnected();

    if (_connectionNode)
        _connectionNode->setVisible(!isConnected);
    if (_memberNode)
        _memberNode->setVisible(isConnected);
    if (_clanManagementNode)
        _clanManagementNode->setVisible(isConnected);

    if (isConnected)
    {
        auto& client = SocketClient::getInstance();

        // 注册PVP和观战回调
        registerPvpCallbacks();

        // 🆕 注册战斗状态列表回调（使用弱引用防止悬空指针）
        client.setOnBattleStatusList([this](const std::string& json) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                [this, json]() { onBattleStatusReceived(json); });
        });

        // 部落创建回调
        client.setOnClanCreated([this](bool success, const std::string& clanId) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success, clanId]() {
                if (success)
                {
                    _currentClanId   = clanId;
                    // 🆕 立即从缓存的部落列表中查找名称
                    for (const auto& clan : _cachedClanList)
                    {
                        if (clan.clanId == clanId)
                        {
                            _currentClanName = clan.clanName;
                            break;
                        }
                    }
                    _isInClan        = true;

                    // 同步到本地账户
                    auto& accMgr = AccountManager::getInstance();
                    if (auto cur = const_cast<AccountInfo*>(accMgr.getCurrentAccount()))
                    {
                        AccountGameData newData = cur->gameData;
                        newData.clanId          = clanId;
                        accMgr.updateGameData(newData);
                        accMgr.save();
                    }

                    showToast("创建部落成功！", Color4B::GREEN);
                    // 🆕 刷新部落列表以确保获取最新名称
                    SocketClient::getInstance().getClanList();
                    safeRefreshCurrentTab();
                }
                else
                {
                    showToast("创建部落失败", Color4B::RED);
                }
                updateClanInfoDisplay();
            });
        });

        // 加入部落回调 - 🆕 修复：处理失败情况
        client.setOnClanJoined([this](bool success) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
                if (success)
                {
                    // 🆕 使用预存的clanId
                    if (!_pendingClanId.empty())
                    {
                        _currentClanId = _pendingClanId;
                        // 🆕 从缓存的部落列表中查找名称
                        for (const auto& clan : _cachedClanList)
                        {
                            if (clan.clanId == _currentClanId)
                            {
                                _currentClanName = clan.clanName;
                                break;
                            }
                        }
                    }
                    _isInClan = true;

                    // 更新本地账户的clanId
                    auto& accMgr = AccountManager::getInstance();
                    if (auto cur = const_cast<AccountInfo*>(accMgr.getCurrentAccount()))
                    {
                        AccountGameData newData = cur->gameData;
                        newData.clanId          = _currentClanId;
                        accMgr.updateGameData(newData);
                        accMgr.save();
                    }

                    showToast("加入部落成功！", Color4B::GREEN);
                    safeRefreshCurrentTab();
                }
                else
                {
                    // 🆕 失败时清除预存的clanId
                    _pendingClanId.clear();
                    showToast("加入部落失败，请重试", Color4B::RED);
                }
                _pendingClanId.clear();
                updateClanInfoDisplay();
            });
        });

        // 离开部落回调 - 🆕 禁用退出功能
        client.setOnClanLeft([this](bool success) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
                // 🆕 即使服务器返回成功，也不允许退出
                showToast("加入部落后不能退出", Color4B::YELLOW);
                updateClanInfoDisplay();
            });
        });

        // 🆕 部落列表回调 - 统一注册，避免被覆盖
        registerClanListCallback();

        // 部落成员回调
        client.setOnClanMembers([this](const std::string& json) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, json]() {
                _isRefreshing = false; // 🆕 解锁
                onClanMembersReceived(json);
            });
        });

        // 请求部落列表以同步状态
        SocketClient::getInstance().getClanList();
    }
}

// 🆕 统一注册部落列表回调
void ClanPanel::registerClanListCallback()
{
    SocketClient::getInstance().setOnClanList([this](const std::vector<ClanInfoClient>& clans) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, clans]() {
            _cachedClanList = clans;
            onClanListReceived(clans);
        });
    });
}

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

    auto& client = SocketClient::getInstance();

    // 设置连接回调
    client.setOnConnected([this](bool success) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
            if (success)
            {
                // 连接成功后登录
                auto& accMgr = AccountManager::getInstance();
                if (auto cur = accMgr.getCurrentAccount())
                {
                    SocketClient::getInstance().login(cur->userId, cur->username, cur->gameData.trophies);

                    // 上传地图数据
                    std::string mapData = accMgr.exportGameStateJson();
                    if (!mapData.empty())
                    {
                        SocketClient::getInstance().uploadMap(mapData);
                    }
                }
                showToast("连接服务器成功！", Color4B::GREEN);
                updateUIState();
            }
            else
            {
                showToast("连接服务器失败", Color4B::RED);
            }
        });
    });

    showToast("正在连接...");
    client.connect(ip, port);
}

void ClanPanel::show()
{
    this->setVisible(true);
    updateUIState();
}

void ClanPanel::hide()
{
    this->setVisible(false);
}

void ClanPanel::onExit()
{
    Layer::onExit();
    // 清除回调以防止面板关闭后崩溃
    SocketClient::getInstance().setOnClanList(nullptr);
    SocketClient::getInstance().setOnClanMembers(nullptr);
    SocketClient::getInstance().setOnConnected(nullptr);
    SocketClient::getInstance().setOnClanCreated(nullptr);
    SocketClient::getInstance().setOnClanJoined(nullptr);
    SocketClient::getInstance().setOnClanLeft(nullptr);
    SocketClient::getInstance().setOnUserListReceived(nullptr);
    SocketClient::getInstance().setOnPvpStart(nullptr);
    SocketClient::getInstance().setOnSpectateJoin(nullptr);
    SocketClient::getInstance().setOnPvpEnd(nullptr);
    SocketClient::getInstance().setOnBattleStatusList(nullptr);
    unscheduleRefresh();
}

void ClanPanel::requestClanMembers()
{
    if (!_isInClan || _currentClanId.empty())
    {
        _memberList->removeAllItems();
        auto item = Layout::create();
        item->setContentSize(Size(560, 60));
        auto lbl = Label::createWithSystemFont("请先加入一个部落", "Arial", 20);
        lbl->setTextColor(Color4B::GRAY);
        lbl->setPosition(Vec2(280, 30));
        item->addChild(lbl);
        _memberList->pushBackCustomItem(item);
        _isRefreshing = false; // 🆕 解锁
        return;
    }

    SocketClient::getInstance().getClanMembers(_currentClanId);
}

void ClanPanel::onClanMembersReceived(const std::string& json)
{
    _memberList->removeAllItems();

    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("members"))
    {
        auto item = Layout::create();
        item->setContentSize(Size(560, 60));
        auto lbl = Label::createWithSystemFont("获取成员列表失败", "Arial", 20);
        lbl->setTextColor(Color4B::RED);
        lbl->setPosition(Vec2(280, 30));
        item->addChild(lbl);
        _memberList->pushBackCustomItem(item);
        return;
    }

    const auto& members = doc["members"];
    if (members.IsArray())
    {
        if (members.Size() == 0)
        {
            auto item = Layout::create();
            item->setContentSize(Size(560, 60));
            auto lbl = Label::createWithSystemFont("部落中没有其他成员", "Arial", 20);
            lbl->setTextColor(Color4B::GRAY);
            lbl->setPosition(Vec2(280, 30));
            item->addChild(lbl);
            _memberList->pushBackCustomItem(item);
            return;
        }

        for (rapidjson::SizeType i = 0; i < members.Size(); i++)
        {
            const auto& member   = members[i];
            std::string id       = member["id"].GetString();
            std::string name     = member["name"].GetString();
            int         trophies = member["trophies"].GetInt();
            bool        online   = member["online"].GetBool();

            PlayerBattleStatus status = _battleStatusCache[id];
            // 检查该玩家是否在战斗中
            if (_playersInBattle.find(id) != _playersInBattle.end())
            {
                status.isInBattle = true;
            }
            createMemberItem(id, name, trophies, online, status);
        }
    }
}

void ClanPanel::onClanListReceived(const std::vector<ClanInfoClient>& clans)
{
    _cachedClanList = clans;

    // 如果当前用户在某个部落中，尝试获取部落名称
    if (!_currentClanId.empty())
    {
        for (const auto& clan : clans)
        {
            if (clan.clanId == _currentClanId)
            {
                _currentClanName = clan.clanName;
                break;
            }
        }
    }

    updateClanInfoDisplay();

    // ✅ 如果部落列表对话框打开，也更新它
    auto dialog = this->getChildByName("ClanListDialog");
    if (dialog)
    {
        auto listView = dialog->getChildByName<ListView*>("ClanListView");
        if (listView)
        {
            listView->removeAllItems();

            if (clans.empty())
            {
                auto item = Layout::create();
                item->setContentSize(Size(480, 60));
                auto lbl = Label::createWithSystemFont("暂无部落", "Arial", 20);
                lbl->setPosition(Vec2(240, 30));
                lbl->setTextColor(Color4B::GRAY);
                item->addChild(lbl);
                listView->pushBackCustomItem(item);
            }
            else
            {
                for (const auto& c : clans)
                {
                    auto item = Layout::create();
                    item->setContentSize(Size(480, 60));
                    item->setBackGroundColor(Color3B(60, 60, 80));
                    item->setBackGroundColorType(Layout::BackGroundColorType::SOLID);

                    auto name = Label::createWithSystemFont(c.clanName, "Arial", 18);
                    name->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                    name->setPosition(Vec2(12, 36));
                    name->setTextColor(Color4B::WHITE);
                    item->addChild(name);

                    auto info = Label::createWithSystemFont(
                        StringUtils::format("%d 成员 • %d 奖杯", c.memberCount, c.clanTrophies), "Arial", 14);
                    info->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                    info->setPosition(Vec2(12, 16));
                    info->setTextColor(Color4B::GRAY);
                    item->addChild(info);

                    auto joinBtn = Button::create();
                    joinBtn->setTitleText("加入");
                    joinBtn->setScale9Enabled(true);
                    joinBtn->setContentSize(Size(100, 36));
                    joinBtn->setPosition(Vec2(420, 30));

                    std::string clanId = c.clanId;
                    joinBtn->addClickEventListener([this, clanId](Ref*) {
                        onJoinClanClicked(clanId);
                        auto dlg = this->getChildByName("ClanListDialog");
                        if (dlg)
                            dlg->removeFromParent();
                    });
                    item->addChild(joinBtn);

                    listView->pushBackCustomItem(item);
                }
            }
        }
    }
}

void ClanPanel::createMemberItem(const std::string& id, const std::string& name, int trophies, bool isOnline,
                                 const PlayerBattleStatus& battleStatus)
{
    auto item = Layout::create();
    item->setContentSize(Size(560, 60));
    item->setBackGroundColor(Color3B(70, 70, 90));
    item->setBackGroundColorType(Layout::BackGroundColorType::SOLID);

    // 名字
    auto nameLabel = Label::createWithSystemFont(name, "Arial", 20);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 35));
    item->addChild(nameLabel);

    // 奖杯
    auto trophyLabel = Label::createWithSystemFont(std::to_string(trophies) + " 🏆", "Arial", 18);
    trophyLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    trophyLabel->setPosition(Vec2(20, 15));
    trophyLabel->setTextColor(Color4B::YELLOW);
    item->addChild(trophyLabel);

    // 状态标签（在线/离线/战斗中）
    std::string statusText;
    Color4B     statusColor;
    if (battleStatus.isInBattle)
    {
        statusText  = "⚔️ 战斗中";
        statusColor = Color4B::ORANGE;
    }
    else if (isOnline)
    {
        statusText  = "🟢 在线";
        statusColor = Color4B::GREEN;
    }
    else
    {
        statusText  = "⚫ 离线";
        statusColor = Color4B::GRAY;
    }

    auto statusLabel = Label::createWithSystemFont(statusText, "Arial", 16);
    statusLabel->setPosition(Vec2(240, 30));
    statusLabel->setTextColor(statusColor);
    item->addChild(statusLabel);

    // 观战按钮 - 只有在战斗中的玩家才能观战
    if (battleStatus.isInBattle)
    {
        auto spectateBtn = Button::create();
        spectateBtn->setTitleText("👁 观战");
        spectateBtn->setTitleFontSize(18);
        spectateBtn->setScale9Enabled(true);
        spectateBtn->setContentSize(Size(100, 35));
        spectateBtn->setPosition(Vec2(480, 30));
        spectateBtn->addClickEventListener([this, id](Ref*) { onSpectateClicked(id); });
        item->addChild(spectateBtn);
    }
    // 攻击按钮 - 只有在线且不在战斗中的玩家才能攻击
    else if (isOnline)
    {
        auto attackBtn = Button::create();
        attackBtn->setTitleText("⚔️ 进攻");
        attackBtn->setTitleFontSize(18);
        attackBtn->setScale9Enabled(true);
        attackBtn->setContentSize(Size(100, 35));
        attackBtn->setPosition(Vec2(480, 30));
        attackBtn->addClickEventListener([this, id](Ref*) { onAttackClicked(id); });
        item->addChild(attackBtn);
    }

    _memberList->pushBackCustomItem(item);
}

void ClanPanel::onAttackClicked(const std::string& memberId)
{
    // 检查是否是自己
    auto& accMgr = AccountManager::getInstance();
    if (auto cur = accMgr.getCurrentAccount())
    {
        if (cur->userId == memberId)
        {
            showToast("不能攻击自己", Color4B::YELLOW);
            return;
        }
    }

    CCLOG("[ClanPanel] 发起PVP攻击: %s", memberId.c_str());
    showToast("正在发起攻击...");
    SocketClient::getInstance().requestPvp(memberId);
}

void ClanPanel::onSpectateClicked(const std::string& memberId)
{
    CCLOG("[ClanPanel] 请求观战: %s", memberId.c_str());
    showToast("正在加入观战...");
    SocketClient::getInstance().requestSpectate(memberId);
}

void ClanPanel::switchToTab(TabType tabType)
{
    _currentTab = tabType;

    // 更新标签按钮样式
    _onlinePlayersTab->setBright(tabType != TabType::ONLINE_PLAYERS);
    _clanMembersTab->setBright(tabType != TabType::CLAN_MEMBERS);
    //_clanWarTab->setBright(tabType != TabType::CLAN_WAR);

    // 刷新列表
    safeRefreshCurrentTab();
}

// 🆕 安全刷新，防止重入
void ClanPanel::safeRefreshCurrentTab()
{
    if (_isRefreshing)
    {
        CCLOG("[ClanPanel] 刷新正在进行中，跳过本次请求");
        return;
    }
    refreshCurrentTab();
}

// 🆕 合并后的唯一 refreshCurrentTab 实现
void ClanPanel::refreshCurrentTab()
{
    _isRefreshing = true;

    // 请求战斗状态（异步）
    requestBattleStatusList();

    switch (_currentTab)
    {
    case TabType::ONLINE_PLAYERS:
        requestOnlinePlayers();
        break;
    case TabType::CLAN_MEMBERS:
        requestClanMembers();
        break;
    case TabType::CLAN_WAR:
        requestClanWarInfo();
        break;
    }
}

void ClanPanel::requestOnlinePlayers()
{
    SocketClient::getInstance().setOnUserListReceived([this](const std::string& data) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, data]() {
            _isRefreshing = false; // 🆕 解锁
            onUserListReceived(data);
        });
    });
    SocketClient::getInstance().requestUserList();
}

void ClanPanel::onUserListReceived(const std::string& data)
{
    _memberList->removeAllItems();

    if (data.empty())
    {
        auto infoLabel = Label::createWithSystemFont("没有其他在线玩家", "Arial", 20);
        infoLabel->setTextColor(Color4B::GRAY);

        auto item = Layout::create();
        item->setContentSize(Size(560, 60));
        item->addChild(infoLabel);
        infoLabel->setPosition(Vec2(280, 30));

        _memberList->pushBackCustomItem(item);
        return;
    }

    std::istringstream iss(data);
    std::string        playerStr;

    while (std::getline(iss, playerStr, '|'))
    {
        std::istringstream ps(playerStr);
        std::string        userId, username, thLevelStr, goldStr, elixirStr;
        std::getline(ps, userId, ',');
        std::getline(ps, username, ',');
        std::getline(ps, thLevelStr, ',');
        std::getline(ps, goldStr, ',');
        std::getline(ps, elixirStr, ',');

        int thLevel = thLevelStr.empty() ? 1 : std::stoi(thLevelStr);
        int gold    = goldStr.empty() ? 0 : std::stoi(goldStr);
        int elixir  = elixirStr.empty() ? 0 : std::stoi(elixirStr);

        PlayerBattleStatus status = _battleStatusCache[userId];
        if (_playersInBattle.find(userId) != _playersInBattle.end())
        {
            status.isInBattle = true;
        }
        createOnlinePlayerItem(userId, username, thLevel, gold, elixir, status);
    }
}

void ClanPanel::createOnlinePlayerItem(const std::string& userId, const std::string& username, int thLevel, int gold,
                                       int elixir, const PlayerBattleStatus& battleStatus)
{
    auto item = Layout::create();
    item->setContentSize(Size(560, 70));
    item->setBackGroundColor(Color3B(70, 70, 90));
    item->setBackGroundColorType(Layout::BackGroundColorType::SOLID);

    // 玩家名称
    auto nameLabel = Label::createWithSystemFont(username, "Arial", 22);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 45));
    nameLabel->setTextColor(Color4B::WHITE);
    item->addChild(nameLabel);

    // 大本营等级
    auto thLabel = Label::createWithSystemFont(StringUtils::format("TH %d", thLevel), "Arial", 16);
    thLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    thLabel->setPosition(Vec2(20, 20));
    thLabel->setTextColor(Color4B(200, 200, 200, 255));
    item->addChild(thLabel);

    // 金币
    auto goldLabel = Label::createWithSystemFont(StringUtils::format("💰 %d", gold), "Arial", 16);
    goldLabel->setPosition(Vec2(180, 45));
    goldLabel->setTextColor(Color4B(255, 215, 0, 255));
    item->addChild(goldLabel);

    // 圣水
    auto elixirLabel = Label::createWithSystemFont(StringUtils::format("⚗️ %d", elixir), "Arial", 16);
    elixirLabel->setPosition(Vec2(180, 20));
    elixirLabel->setTextColor(Color4B(255, 0, 255, 255));
    item->addChild(elixirLabel);

    // 🆕 根据战斗状态决定按钮布局
    if (battleStatus.isInBattle)
    {
        // 战斗状态标签
        std::string text             = battleStatus.isAttacker ? "🔴 进攻中" : "🔵 防守中";
        auto        battleStateLabel = Label::createWithSystemFont(text, "Arial", 14);
        battleStateLabel->setPosition(Vec2(320, 35));
        battleStateLabel->setTextColor(Color4B::ORANGE);
        item->addChild(battleStateLabel);

        // 🆕 只显示观战按钮
        auto spectateBtn = Button::create();
        spectateBtn->setTitleText("👁 观战");
        spectateBtn->setTitleFontSize(16);
        spectateBtn->setScale9Enabled(true);
        spectateBtn->setContentSize(Size(90, 32));
        spectateBtn->setPosition(Vec2(480, 35));
        spectateBtn->addClickEventListener([this, userId](Ref*) { onSpectateClicked(userId); });
        item->addChild(spectateBtn);
    }
    else
    {
        // 🆕 不在战斗中：只显示PVP按钮
        auto pvpBtn = Button::create();
        pvpBtn->setTitleText("⚔️ PVP");
        pvpBtn->setTitleFontSize(16);
        pvpBtn->setScale9Enabled(true);
        pvpBtn->setContentSize(Size(90, 32));
        pvpBtn->setPosition(Vec2(480, 35));
        pvpBtn->addClickEventListener([this, userId](Ref*) { onAttackClicked(userId); });
        item->addChild(pvpBtn);
    }

    _memberList->pushBackCustomItem(item);
}

void ClanPanel::onBattleStatusReceived(const std::string& json)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError() || doc.IsNull() || !doc.IsObject())
        return;
    if (!doc.HasMember("statuses") || !doc["statuses"].IsArray())
        return;

    _playersInBattle.clear();
    _battleStatusCache.clear();

    const auto& arr = doc["statuses"];
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
    {
        const auto& it = arr[i];
        if (!it.HasMember("userId"))
            continue;
        std::string        uid = it["userId"].GetString();
        PlayerBattleStatus s;
        if (it.HasMember("inBattle") && it["inBattle"].IsBool())
            s.isInBattle = it["inBattle"].GetBool();
        if (it.HasMember("opponentId") && it["opponentId"].IsString())
            s.opponentId = it["opponentId"].GetString();
        if (it.HasMember("opponentName") && it["opponentName"].IsString())
            s.opponentName = it["opponentName"].GetString();
        if (it.HasMember("isAttacker") && it["isAttacker"].IsBool())
            s.isAttacker = it["isAttacker"].GetBool();
        _battleStatusCache[uid] = s;

        if (s.isInBattle)
        {
            _playersInBattle.insert(uid);
        }
    }

    // 🆕 不再在此处调用刷新，避免循环调用导致死锁
    // 战斗状态已更新到缓存中，下次渲染列表时会使用最新数据
}

void ClanPanel::requestClanWarInfo()
{
    _memberList->removeAllItems();

    auto& accMgr         = AccountManager::getInstance();
    auto  currentAccount = accMgr.getCurrentAccount();

    if (!currentAccount)
    {
        auto infoLabel = Label::createWithSystemFont("请先登录账号", "Arial", 24);
        infoLabel->setTextColor(Color4B::YELLOW);
        auto item = Layout::create();
        item->setContentSize(Size(560, 100));
        item->addChild(infoLabel);
        infoLabel->setPosition(Vec2(280, 50));
        _memberList->pushBackCustomItem(item);
        _isRefreshing = false;
        return;
    }

    if (!_isInClan)
    {
        auto infoLabel = Label::createWithSystemFont("请先加入一个部落", "Arial", 24);
        infoLabel->setTextColor(Color4B::YELLOW);
        auto item = Layout::create();
        item->setContentSize(Size(560, 100));
        item->addChild(infoLabel);
        infoLabel->setPosition(Vec2(280, 50));
        _memberList->pushBackCustomItem(item);
        _isRefreshing = false;
        return;
    }

    if (_currentWarId.empty())
    {
        auto infoLabel = Label::createWithSystemFont("当前没有进行中的部落战", "Arial", 22);
        infoLabel->setTextColor(Color4B::GRAY);

        auto item = Layout::create();
        item->setContentSize(Size(560, 80));
        item->addChild(infoLabel);
        infoLabel->setPosition(Vec2(280, 50));

        auto searchBtn = Button::create();
        searchBtn->setTitleText("搜索部落战");
        searchBtn->setTitleFontSize(20);
        searchBtn->setScale9Enabled(true);
        searchBtn->setContentSize(Size(150, 40));
        searchBtn->setPosition(Vec2(280, 20));
        searchBtn->addClickEventListener([this](Ref*) {
            SocketClient::getInstance().searchClanWar();
            showToast("正在搜索部落战...");
            SocketClient::getInstance().setOnClanWarMatch(
                [this](const std::string& warId, const std::string& clan1Id, const std::string& clan2Id) {
                    Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, warId]() {
                        _currentWarId = warId;
                        CCLOG("[ClanWar] 匹配成功! WarID: %s", warId.c_str());
                        showToast("部落战匹配成功！", Color4B::GREEN);
                        safeRefreshCurrentTab();
                    });
                });
        });
        item->addChild(searchBtn);

        _memberList->pushBackCustomItem(item);
        _isRefreshing = false;
        return;
    }

    SocketClient::getInstance().setOnClanWarMemberList([this](const std::string& json) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, json]() {
            _isRefreshing = false;
            onClanWarMemberListReceived(json);
        });
    });
    SocketClient::getInstance().requestClanWarMemberList(_currentWarId);
}

void ClanPanel::onClanWarMemberListReceived(const std::string& json)
{
    _memberList->removeAllItems();

    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (doc.HasParseError() || !doc.IsObject())
    {
        CCLOG("[ClanPanel] Parse error: %s", json.c_str());
        return;
    }

    // 显示战争状态
    if (doc.HasMember("clan1TotalStars") && doc.HasMember("clan2TotalStars"))
    {
        int clan1Stars = doc["clan1TotalStars"].GetInt();
        int clan2Stars = doc["clan2TotalStars"].GetInt();

        auto statusLabel = Label::createWithSystemFont(
            StringUtils::format("战争进度: 我方%d⭐ VS 敌方%d⭐", clan1Stars, clan2Stars), "Arial", 24);
        statusLabel->setTextColor(Color4B::YELLOW);

        auto statusItem = Layout::create();
        statusItem->setContentSize(Size(560, 50));
        statusItem->setBackGroundColor(Color3B(80, 60, 100));
        statusItem->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        statusItem->addChild(statusLabel);
        statusLabel->setPosition(Vec2(280, 25));
        _memberList->pushBackCustomItem(statusItem);
    }

    if (doc.HasMember("enemyMembers") && doc["enemyMembers"].IsArray())
    {
        auto headerItem = Layout::create();
        headerItem->setContentSize(Size(560, 40));
        headerItem->setBackGroundColor(Color3B(100, 80, 120));
        headerItem->setBackGroundColorType(Layout::BackGroundColorType::SOLID);

        auto headerLabel = Label::createWithSystemFont("敌方成员", "Arial", 22);
        headerLabel->setTextColor(Color4B::RED);
        headerLabel->setPosition(Vec2(280, 20));
        headerItem->addChild(headerLabel);
        _memberList->pushBackCustomItem(headerItem);

        const auto& enemies = doc["enemyMembers"];
        for (rapidjson::SizeType i = 0; i < enemies.Size(); i++)
        {
            const auto& member          = enemies[i];
            std::string id              = member["id"].GetString();
            std::string name            = member["name"].GetString();
            int         bestStars       = member["bestStars"].GetInt();
            float       bestDestruction = static_cast<float>(member["bestDestruction"].GetDouble());
            bool        canAttack       = member["canAttack"].GetBool();

            PlayerBattleStatus status = _battleStatusCache[id];
            createClanWarMemberItem(id, name, bestStars, bestDestruction, canAttack, status);
        }
    }
}

void ClanPanel::createClanWarMemberItem(const std::string& userId, const std::string& username, int bestStars,
                                        float bestDestruction, bool canAttack, const PlayerBattleStatus& battleStatus)
{
    auto item = Layout::create();
    item->setContentSize(Size(560, 70));
    item->setBackGroundColor(Color3B(70, 70, 90));
    item->setBackGroundColorType(Layout::BackGroundColorType::SOLID);

    // 玩家名称
    auto nameLabel = Label::createWithSystemFont(username, "Arial", 22);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 45));
    nameLabel->setTextColor(Color4B::WHITE);
    item->addChild(nameLabel);

    // 最佳战绩
    auto starsLabel = Label::createWithSystemFont(
        StringUtils::format("最佳: %d⭐ %.1f%%", bestStars, bestDestruction * 100), "Arial", 18);
    starsLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    starsLabel->setPosition(Vec2(20, 20));
    starsLabel->setTextColor(Color4B::YELLOW);
    item->addChild(starsLabel);

    // 🆕 根据战斗状态显示不同的按钮布局
    if (battleStatus.isInBattle)
    {
        // 战斗中状态标签
        auto battleLabel = Label::createWithSystemFont("⚔️ 战斗中", "Arial", 16);
        battleLabel->setPosition(Vec2(300, 35));
        battleLabel->setTextColor(Color4B::ORANGE);
        item->addChild(battleLabel);

        // 🆕 只显示观战按钮（居中放置）
        auto spectateBtn = Button::create();
        spectateBtn->setTitleText("👁 观战");
        spectateBtn->setTitleFontSize(18);
        spectateBtn->setScale9Enabled(true);
        spectateBtn->setContentSize(Size(100, 35));
        spectateBtn->setPosition(Vec2(480, 35));
        spectateBtn->addClickEventListener([this, userId](Ref*) { onClanWarSpectateClicked(userId); });
        item->addChild(spectateBtn);
    }
    else if (canAttack)
    {
        // 🆕 不在战斗中：显示攻击按钮，观战按钮禁用
        auto attackBtn = Button::create();
        attackBtn->setTitleText("⚔️ 攻击");
        attackBtn->setTitleFontSize(18);
        attackBtn->setScale9Enabled(true);
        attackBtn->setContentSize(Size(100, 35));
        attackBtn->setPosition(Vec2(420, 35));  // 🆕 调整位置避免重叠
        attackBtn->addClickEventListener([this, userId](Ref*) { onClanWarAttackClicked(userId); });
        item->addChild(attackBtn);
    }
    else
    {
        // 无地图数据
        auto noMapLabel = Label::createWithSystemFont("(无地图数据)", "Arial", 16);
        noMapLabel->setTextColor(Color4B::GRAY);
        noMapLabel->setPosition(Vec2(450, 35));
        item->addChild(noMapLabel);
    }

    _memberList->pushBackCustomItem(item);
}

void ClanPanel::onClanWarAttackClicked(const std::string& targetId)
{
    CCLOG("[ClanWar] 攻击目标: %s in war: %s", targetId.c_str(), _currentWarId.c_str());

    SocketClient::getInstance().setOnClanWarAttackStart(
        [this](const std::string& type, const std::string& targetId, const std::string& mapData) {
            if (type == "ATTACK")
            {
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, targetId, mapData]() {
                    CCLOG("[ClanWar] 攻击开始，加载战斗场景...");
                    enterBattleScene(targetId, mapData);
                });
            }
            else
            {
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, type]() {
                    CCLOG("[ClanWar] 攻击失败: %s", type.c_str());
                    showToast("攻击失败: " + type, Color4B::RED);
                });
            }
        });

    showToast("正在发起攻击...");
    SocketClient::getInstance().startClanWarAttack(_currentWarId, targetId);
}

void ClanPanel::onClanWarSpectateClicked(const std::string& targetId)
{
    CCLOG("[ClanWar] 观战目标: %s in war: %s", targetId.c_str(), _currentWarId.c_str());

    // 🆕 检查目标是否在战斗中
    auto it = _battleStatusCache.find(targetId);
    if (it == _battleStatusCache.end() || !it->second.isInBattle)
    {
        showToast("该玩家当前没有进行中的战斗", Color4B::YELLOW);
        return;
    }

    // 🆕 使用通用的观战请求（部落战观战与普通观战使用相同机制）
    showToast("正在加入观战...");

    // 🆕 设置观战回调
    SocketClient::getInstance().setOnSpectateJoin(
        [this](bool success, const std::string& attackerId, const std::string& defenderId, const std::string& mapData) {
            Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                [this, success, attackerId, defenderId, mapData]() {
                    if (success)
                    {
                        CCLOG("[ClanWar] 观战加入成功: %s vs %s", attackerId.c_str(), defenderId.c_str());
                        enterSpectateScene(attackerId, defenderId, mapData);
                    }
                    else
                    {
                        showToast("观战失败 - 战斗可能已结束", Color4B::RED);
                    }
                });
        });

    // 🆕 发送观战请求
    SocketClient::getInstance().requestSpectate(targetId);
}

// ========== 部落管理相关实现 ==========

void ClanPanel::onCreateClanClicked()
{
    showCreateClanDialog();
}

void ClanPanel::onJoinClanClicked(const std::string& clanId)
{
    // 🆕 保存待加入的clanId，等待服务器确认
    _pendingClanId = clanId;
    SocketClient::getInstance().joinClan(clanId);
    showToast("正在加入部落...");
}

void ClanPanel::onLeaveClanClicked()
{
    // 🆕 禁用退出功能
    showToast("加入部落后不能退出", Color4B::YELLOW);
}

void ClanPanel::showCreateClanDialog()
{
    // 弹出简单对话框，输入部落名
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
        std::string name = input->getString();
        if (name.length() <= 6)
        {
            showToast("部落名长度必须大于6个字符", Color4B::RED);
            return;
        }
        // 🆕 在发送创建请求前，预存部落名称
        _currentClanName = name;
        SocketClient::getInstance().createClan(name);
        layer->removeFromParent();
        showToast("正在创建部落...");
    });

    cancelBtn->addClickEventListener([layer](Ref*) { layer->removeFromParent(); });
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
    closeBtn->addClickEventListener([layer](Ref*) { layer->removeFromParent(); });
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

            auto name = Label::createWithSystemFont(c.clanName, "Arial", 18);
            name->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            name->setPosition(Vec2(12, 36));
            name->setTextColor(Color4B::WHITE);
            item->addChild(name);

            auto info = Label::createWithSystemFont(
                StringUtils::format("%d 成员 • %d 奖杯", c.memberCount, c.clanTrophies), "Arial", 14);
            info->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            info->setPosition(Vec2(12, 16));
            info->setTextColor(Color4B::GRAY);
            item->addChild(info);

            auto joinBtn = Button::create();
            joinBtn->setTitleText("加入");
            joinBtn->setTitleFontSize(20);
            joinBtn->setScale9Enabled(true);
            joinBtn->setContentSize(Size(100, 36));
            joinBtn->setPosition(Vec2(420, 30));

            std::string clanId = c.clanId;
            joinBtn->addClickEventListener([this, clanId, layer](Ref*) {
                onJoinClanClicked(clanId);
                if (layer && layer->getParent())
                {
                    layer->removeFromParent();
                }
            });
            item->addChild(joinBtn);

            list->pushBackCustomItem(item);
        }
    };

    if (!_cachedClanList.empty())
    {
        fillList(_cachedClanList);
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

        // ✅ 不要设置临时回调，直接使用全局回调填充
        // 全局回调已经在 registerClanListCallback() 中注册
        // 当数据到达时会自动更新 _cachedClanList 和 UI
    }

    // ✅ 请求部落列表（会触发全局回调）
    SocketClient::getInstance().getClanList();

    // ✅ 手动触发一次填充（使用最新的缓存数据）
    this->scheduleOnce([this, fillList](float) { fillList(_cachedClanList); }, 0.5f, "fill_clan_list_delayed");
}

void ClanPanel::showToast(const std::string& message, const cocos2d::Color4B& color)
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

// ========== 观战/进入战斗场景辅助 ==========

void ClanPanel::enterSpectateScene(const std::string& attackerId, const std::string& defenderId,
                                   const std::string& mapData)
{
    // 创建战斗场景（观战）
    AccountGameData enemyData   = AccountGameData::fromJson(mapData);
    auto            scene       = BattleScene::createWithEnemyData(enemyData, defenderId);
    auto            battleScene = dynamic_cast<BattleScene*>(scene);
    if (battleScene)
    {
        // 观战模式：让 BattleScene 以非攻击方形式进入并隐藏本地下兵（由服务器转发动作）
        battleScene->setPvpMode(false);
    }
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, scene));
}

void ClanPanel::enterBattleScene(const std::string& targetId, const std::string& mapData)
{
    // 攻击流程：解析地图并进入战斗（作为攻击方）
    AccountGameData enemyData   = AccountGameData::fromJson(mapData);
    auto            scene       = BattleScene::createWithEnemyData(enemyData, targetId);
    auto            battleScene = dynamic_cast<BattleScene*>(scene);
    if (battleScene)
    {
        battleScene->setPvpMode(true);
    }
    Director::getInstance()->replaceScene(TransitionFade::create(0.5f, scene));
}

// ========== 定时刷新相关 ==========

void ClanPanel::scheduleRefresh()
{
    // 每 5 秒刷新当前页面
    this->schedule(
        [this](float dt) {
            if (!_isRefreshing)
            {
                refreshCurrentTab();
            }
        },
        5.0f, "clanpanel_refresh");
}

void ClanPanel::unscheduleRefresh()
{
    this->unschedule("clanpanel_refresh");
}

// ========== 战斗状态请求 ==========

void ClanPanel::requestBattleStatusList()
{
    SocketClient::getInstance().requestBattleStatusList();
}

// End of file