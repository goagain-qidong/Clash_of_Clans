/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanPanel.cpp
 * File Function: 负责游戏部落面板
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "ClanPanel.h"
#include "Managers/SocketClient.h"
#include "Scenes/BattleScene.h"
#include "Managers/AccountManager.h" // 🆕 Include AccountManager
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

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
    
    setupUI();
    updateUIState(); // 🆕 Check initial state
    
    return true;
}

void ClanPanel::setupUI()
{
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    _panelNode = Node::create();
    _panelNode->setPosition(visibleSize.width / 2, visibleSize.height / 2);
    this->addChild(_panelNode);
    
    // 背景板
    auto bg = LayerColor::create(Color4B(50, 50, 70, 255), 600, 400);
    bg->setPosition(-300, -200);
    _panelNode->addChild(bg);
    
    // 关闭按钮
    auto closeBtn = Button::create("icon/return_button.png");
    if (closeBtn->getContentSize().equals(Size::ZERO)) {
        closeBtn = Button::create();
        closeBtn->ignoreContentAdaptWithSize(false);
        closeBtn->setContentSize(Size(40, 40));
        closeBtn->setTitleText("X");
        closeBtn->setTitleFontSize(24);
        
        // auto bg = LayerColor::create(Color4B(200, 50, 50, 255), 40, 40);
        // bg->setPosition(Vec2::ZERO);
        // closeBtn->addChild(bg, -1);
        
        if (closeBtn->getTitleRenderer()) {
            closeBtn->getTitleRenderer()->setPosition(Vec2(20, 20));
        }
    } else {
        closeBtn->setScale(50.0f / closeBtn->getContentSize().width);
    }
    closeBtn->setPosition(Vec2(270, 170));
    closeBtn->addClickEventListener([this](Ref*) {
        this->removeFromParent();
    });
    _panelNode->addChild(closeBtn);
    
    setupConnectionUI();
    setupMemberUI();
}

void ClanPanel::setupConnectionUI()
{
    _connectionNode = Node::create();
    _panelNode->addChild(_connectionNode);
    
    auto title = Label::createWithSystemFont("Connect to Server", "Arial", 30);
    title->setPosition(0, 120);
    _connectionNode->addChild(title);
    
    // IP Input
    auto ipBg = LayerColor::create(Color4B(30, 30, 30, 255), 300, 40);
    ipBg->setPosition(-150, 40);
    _connectionNode->addChild(ipBg);
    
    _ipInput = TextField::create("Enter IP Address (e.g. 127.0.0.1)", "Arial", 20);
    _ipInput->setPosition(Vec2(0, 60));
    _ipInput->setTextHorizontalAlignment(TextHAlignment::CENTER);
    _ipInput->setTextColor(Color4B::WHITE);
    _ipInput->setCursorEnabled(true);
    _connectionNode->addChild(_ipInput);
    
    // Port Input
    auto portBg = LayerColor::create(Color4B(30, 30, 30, 255), 300, 40);
    portBg->setPosition(-150, -20);
    _connectionNode->addChild(portBg);
    
    _portInput = TextField::create("Enter Port (e.g. 8888)", "Arial", 20);
    _portInput->setPosition(Vec2(0, 0));
    _portInput->setTextHorizontalAlignment(TextHAlignment::CENTER);
    _portInput->setTextColor(Color4B::WHITE);
    _portInput->setCursorEnabled(true);
    _connectionNode->addChild(_portInput);
    
    // Connect Button
    auto connectBtn = Button::create();
    connectBtn->setTitleText("Connect");
    connectBtn->setTitleFontSize(24);
    connectBtn->setPosition(Vec2(0, -80));
    connectBtn->addClickEventListener([this](Ref*) {
        onConnectClicked();
    });
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
    auto title = Label::createWithSystemFont("Clan Members", "Arial", 30);
    title->setPosition(0, 170);
    _memberNode->addChild(title);
    
    // 列表
    _memberList = ListView::create();
    _memberList->setContentSize(Size(560, 320));
    _memberList->setPosition(Vec2(-280, -160));
    _memberList->setBackGroundColor(Color3B(60, 60, 80));
    _memberList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _memberList->setItemsMargin(5);
    _memberNode->addChild(_memberList);
    
    // 刷新按钮
    auto refreshBtn = Button::create();
    refreshBtn->setTitleText("Refresh");
    refreshBtn->setPosition(Vec2(0, -180));
    refreshBtn->addClickEventListener([this](Ref*) {
        requestClanMembers();
    });
    _memberNode->addChild(refreshBtn);
}

void ClanPanel::updateUIState()
{
    bool isConnected = SocketClient::getInstance().isConnected();
    
    if (_connectionNode) _connectionNode->setVisible(!isConnected);
    if (_memberNode) _memberNode->setVisible(isConnected);
    
    if (isConnected)
    {
        requestClanMembers();
    }
}

void ClanPanel::onConnectClicked()
{
    std::string ip = _ipInput->getString();
    std::string portStr = _portInput->getString();
    
    if (ip.empty() || portStr.empty())
    {
        return;
    }
    
    int port = std::stoi(portStr);
    
    auto& client = SocketClient::getInstance();
    
    // Set callback to update UI on connection result
    client.setOnConnected([this](bool success) {
        // Ensure UI update runs on main thread
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
            if (success)
            {
                // Login after connection
                auto& accMgr = AccountManager::getInstance();
                if (auto cur = accMgr.getCurrentAccount())
                {
                    SocketClient::getInstance().login(cur->userId, cur->username, cur->gameData.trophies);
                }
                updateUIState();
            }
            else
            {
                // Show error (simple shake or log for now)
                CCLOG("Connection failed");
            }
        });
    });
    
    client.connect(ip, port);
}

void ClanPanel::show()
{
    this->setVisible(true);
    updateUIState(); // 🆕 Refresh state on show
}

void ClanPanel::hide()
{
    this->setVisible(false);
}

void ClanPanel::requestClanMembers()
{
    // 获取当前用户的部落ID (假设存储在 AccountManager)
    // 这里简化，直接请求 "my_clan" 或者让服务器根据 socket 判断
    // SocketClient::getClanMembers 需要 clanId
    // 我们假设 SocketClient 知道怎么获取，或者我们先获取 ClanInfo
    // 暂时传空字符串，让服务器处理（如果服务器支持）或者需要先获取 ClanList
    // 根据 Server.cpp，getClanMembersJson 需要 clanId
    // 客户端可能不知道 clanId，除非登录时返回了。
    // 假设 AccountManager 有 clanId
    
    // 临时：请求部落列表，然后请求第一个部落的成员（测试用）
    // 或者直接请求 "CLAN_123456" 如果知道的话
    // 正确做法：AccountManager::getInstance().getCurrentAccount()->clanId
    
    // 由于无法获取 clanId，我们先请求 ClanList，然后取第一个
    SocketClient::getInstance().setOnClanList([this](const std::vector<ClanInfoClient>& clans) {
        if (!clans.empty()) {
            SocketClient::getInstance().getClanMembers(clans[0].clanId);
        }
    });
    SocketClient::getInstance().getClanList();
    
    // 设置成员列表回调
    SocketClient::getInstance().setOnClanMembers([this](const std::string& json) {
        onClanMembersReceived(json);
    });
}

void ClanPanel::onClanMembersReceived(const std::string& json)
{
    _memberList->removeAllItems();
    
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("members"))
    {
        return;
    }
    
    const auto& members = doc["members"];
    if (members.IsArray())
    {
        for (rapidjson::SizeType i = 0; i < members.Size(); i++)
        {
            const auto& member = members[i];
            std::string id = member["id"].GetString();
            std::string name = member["name"].GetString();
            int trophies = member["trophies"].GetInt();
            bool online = member["online"].GetBool();
            
            createMemberItem(id, name, trophies, online);
        }
    }
}

void ClanPanel::createMemberItem(const std::string& id, const std::string& name, int trophies, bool isOnline)
{
    auto item = Layout::create();
    item->setContentSize(Size(560, 50));
    item->setBackGroundColor(Color3B(70, 70, 90));
    item->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    
    // 名字
    auto nameLabel = Label::createWithSystemFont(name, "Arial", 20);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 25));
    item->addChild(nameLabel);
    
    // 奖杯
    auto trophyLabel = Label::createWithSystemFont(std::to_string(trophies) + " 🏆", "Arial", 18);
    trophyLabel->setPosition(Vec2(200, 25));
    trophyLabel->setTextColor(Color4B::YELLOW);
    item->addChild(trophyLabel);
    
    // 在线状态
    auto statusLabel = Label::createWithSystemFont(isOnline ? "Online" : "Offline", "Arial", 16);
    statusLabel->setPosition(Vec2(300, 25));
    statusLabel->setTextColor(isOnline ? Color4B::GREEN : Color4B::GRAY);
    item->addChild(statusLabel);
    
    // 攻击/挑战按钮 (仅当在线时可用，或者离线也可以打但不是实时的)
    // 题目要求实时，所以限制在线
    if (isOnline)
    {
        auto attackBtn = Button::create();
        attackBtn->setTitleText("Attack");
        attackBtn->setTitleFontSize(18);
        attackBtn->setScale9Enabled(true);
        attackBtn->setContentSize(Size(80, 30));
        attackBtn->setPosition(Vec2(400, 25));
        attackBtn->addClickEventListener([this, id](Ref*) {
            onAttackClicked(id);
        });
        item->addChild(attackBtn);
        
        // 观战按钮 (如果正在战斗中 - 需要服务器状态，这里先加上按钮，点击请求观战)
        auto spectateBtn = Button::create();
        spectateBtn->setTitleText("Watch");
        spectateBtn->setTitleFontSize(18);
        spectateBtn->setScale9Enabled(true);
        spectateBtn->setContentSize(Size(80, 30));
        spectateBtn->setPosition(Vec2(500, 25));
        spectateBtn->addClickEventListener([this, id](Ref*) {
            onSpectateClicked(id);
        });
        item->addChild(spectateBtn);
    }
    
    _memberList->pushBackCustomItem(item);
}

void ClanPanel::onAttackClicked(const std::string& memberId)
{
    // 发送PVP请求
    SocketClient::getInstance().requestPvp(memberId);
}

void ClanPanel::onSpectateClicked(const std::string& memberId)
{
    // 发送观战请求
    SocketClient::getInstance().requestSpectate(memberId);
}