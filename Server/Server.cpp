/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     Server.cpp
 * File Function: 服务器主逻辑
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/

#include "ArmyBuilding.h"
#include "BuildingUpgradeUI.h"
#include "Managers/UpgradeManager.h"
#include "ResourceManager.h"
#include "SceneUIController.h"
#include "Services/BuildingUpgradeService.h"
#include "Unit/TrainingUI.h"

USING_NS_CC;
using namespace ui;

BuildingUpgradeUI* BuildingUpgradeUI::create(BaseBuilding* building)
{
    BuildingUpgradeUI* ui = new (std::nothrow) BuildingUpgradeUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool BuildingUpgradeUI::init(BaseBuilding* building)
{
    if (!Node::init())
    {
        return false;
    }

    _building = building;
    setupUI();
    updateUI();
    return true;
}

void BuildingUpgradeUI::setupUI()
{
    // ============================================================
    // 1. 面板基础设置
    // ============================================================
    bool  isBarracks  = (_building && _building->getBuildingType() == BuildingType::kArmy);
    float panelHeight = isBarracks ? 340.0f : 260.0f;

    _panel = Layout::create();
    _panel->setContentSize(Size(280, panelHeight));
    _panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _panel->setBackGroundColor(Color3B(40, 40, 60));
    _panel->setBackGroundColorOpacity(230);
    _panel->setAnchorPoint(Vec2(0.5f, 0.0f));
    _panel->setTouchEnabled(true); // 面板阻挡穿透
    this->addChild(_panel);

    float panelWidth = _panel->getContentSize().width;
    float centerX    = panelWidth / 2.0f;

    // ============================================================
    // 2. 文字信息
    // ============================================================

    _titleLabel = Label::createWithSystemFont("", "Microsoft YaHei", 20);
    _titleLabel->setPosition(Vec2(centerX, panelHeight - 25));
    _titleLabel->setTextColor(Color4B::YELLOW);
    _panel->addChild(_titleLabel);

    _levelLabel = Label::createWithSystemFont("", "Microsoft YaHei", 16);
    _levelLabel->setPosition(Vec2(centerX, panelHeight - 50));
    _levelLabel->setTextColor(Color4B::WHITE);
    _panel->addChild(_levelLabel);

    _descLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _descLabel->setPosition(Vec2(centerX, panelHeight - 85));
    _descLabel->setTextColor(Color4B(200, 200, 200, 255));
    _descLabel->setAlignment(TextHAlignment::CENTER);
    _panel->addChild(_descLabel);

    _costLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _costLabel->setPosition(Vec2(centerX, panelHeight - 120));
    _costLabel->setTextColor(Color4B::GREEN);
    _panel->addChild(_costLabel);

    _timeLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _timeLabel->setPosition(Vec2(centerX, panelHeight - 145));
    _timeLabel->setTextColor(Color4B(150, 200, 255, 255));
    _panel->addChild(_timeLabel);

    auto gemTip = Label::createWithSystemFont("使用50宝石可将升级时间设置为10秒", "Microsoft YaHei", 12);
    gemTip->setPosition(Vec2(centerX, panelHeight - 170));
    gemTip->setTextColor(Color4B(240, 220, 100, 255));
    _panel->addChild(gemTip);

    // ============================================================
    // 3. 底部三个按钮 (紧贴对齐，无缝隙)
    // ============================================================

    // 按钮参数
    float btnWidth  = 90.0f; // 按钮宽度
    float btnHeight = 40.0f;
    float bottomY   = 40.0f; // 按钮中心距底部高度

    // 计算起始X (左边按钮的中心点)
    // 3个按钮总宽 = btnWidth * 3
    // StartX = (Panel宽 - 总宽)/2 + 半个按钮宽
    float startX = (panelWidth - (btnWidth * 3)) / 2 + btnWidth / 2;

    // --- A. 升级按钮 (绿色) ---
    _upgradeButton = Button::create();
    _upgradeButton->setContentSize(Size(btnWidth, btnHeight));
    _upgradeButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _upgradeButton->setPosition(Vec2(startX, bottomY)); // 位置1
    _upgradeButton->setTitleText("升级");
    _upgradeButton->setTitleFontSize(16);
    _upgradeButton->setTitleColor(Color3B::WHITE);
    _upgradeButton->setTouchEnabled(true);
    _upgradeButton->setPressedActionEnabled(true);
    _upgradeButton->addClickEventListener([this](Ref*) { onUpgradeClicked(); });

    auto upgradeBg = LayerColor::create(Color4B(0, 150, 0, 200), btnWidth, btnHeight);
    upgradeBg->setAnchorPoint(Vec2::ZERO);
    upgradeBg->setPosition(Vec2::ZERO);
    // upgradeBg->setTouchEnabled(false); // 关键：背景不吃点击
    _upgradeButton->addChild(upgradeBg, -1);
    _panel->addChild(_upgradeButton);

    // --- B. 宝石按钮 (紫色) ---
    auto gemButton = Button::create();
    gemButton->setContentSize(Size(btnWidth, btnHeight));
    gemButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    gemButton->setPosition(Vec2(startX + btnWidth, bottomY)); // 位置2：紧挨着
    // 不用图片，直接设置文字
    gemButton->setTitleText("宝石50"); // 尝试用emoji，如果不显示会自动变成方块，也可以改成 "宝石 50"
    gemButton->setTitleFontSize(16);
    gemButton->setTitleColor(Color3B::YELLOW);
    gemButton->setTouchEnabled(true);         // 关键：开启点击
    gemButton->setPressedActionEnabled(true); // 开启缩放反馈
    gemButton->setName("gemButton");

    auto gemBg = LayerColor::create(Color4B(80, 0, 120, 200), btnWidth, btnHeight); // 紫色背景
    gemBg->setAnchorPoint(Vec2::ZERO);
    gemBg->setPosition(Vec2::ZERO);
    // gemBg->setTouchEnabled(false); // 关键：背景不吃点击
    gemButton->addChild(gemBg, -1);

    // 重新写一遍点击逻辑，确保无误
    gemButton->addClickEventListener([this](Ref* sender) {
        CCLOG("Gem Button Clicked!");
        if (!_building)
            return;

        auto& resMgr     = ResourceManager::getInstance();
        auto* upgradeMgr = UpgradeManager::getInstance();

        if (_building->isMaxLevel())
            return;
        // 检查工人
        if (!upgradeMgr->canStartUpgrade(_building, true))
        {
            CCLOG("No builder available");
            return;
        }

        int          cost     = _building->getUpgradeCost();
        ResourceType costType = _building->getUpgradeCostType();
        const int    GEM_COST = 50;

        // 检查资源
        bool hasRes = resMgr.hasEnough(costType, cost);
        bool hasGem = resMgr.hasEnough(ResourceType::kGem, GEM_COST);

        if (hasRes && hasGem)
        {
            // 扣除
            resMgr.consume(ResourceType::kGem, GEM_COST);
            if (resMgr.consume(costType, cost))
            {
                // 开始升级 (10秒)
                if (upgradeMgr->startUpgrade(_building, cost, 10.0f, true))
                {
                    if (_resultCallback)
                        _resultCallback(true, _building->getLevel());
                    onCloseClicked();
                }
                else
                {
                    // 失败回退
                    resMgr.addResource(costType, cost);
                    resMgr.addResource(ResourceType::kGem, GEM_COST);
                }
            }
            else
            {
                // 资源扣除失败回退宝石
                resMgr.addResource(ResourceType::kGem, GEM_COST);
            }
        }
        else
        {
            CCLOG("Not enough resources or gems");
        }
    });
    _panel->addChild(gemButton, 1); // ZOrder设为1，确保在上层

    // --- C. 关闭按钮 (红色) ---
    _closeButton = Button::create("icon/return_button.png");
    if (_closeButton->getContentSize().equals(Size::ZERO))
    {
        _closeButton = Button::create();
        _closeButton->ignoreContentAdaptWithSize(false);
        _closeButton->setContentSize(Size(btnWidth, btnHeight));
        _closeButton->setTitleText("关闭");
        _closeButton->setTitleFontSize(16);
        _closeButton->setTitleColor(Color3B::WHITE);

        auto closeBg = LayerColor::create(Color4B(150, 0, 0, 200), btnWidth, btnHeight);
        closeBg->setAnchorPoint(Vec2::ZERO);
        closeBg->setPosition(Vec2::ZERO);
        _closeButton->addChild(closeBg, -1);

        if (_closeButton->getTitleRenderer())
        {
            _closeButton->getTitleRenderer()->setPosition(Vec2(btnWidth / 2, btnHeight / 2));
        }
    }
    _closeButton->setAnchorPoint(Vec2(0.5f, 0.5f));
    _closeButton->setPosition(Vec2(startX + btnWidth * 2, bottomY)); // 位置3：紧挨着
    _closeButton->setTouchEnabled(true);
    _closeButton->setPressedActionEnabled(true);
    _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });

    _panel->addChild(_closeButton);

    // ============================================================
    // 4. 训练士兵按钮 (仅兵营)
    // ============================================================
    if (isBarracks)
    {
        // 放置在 Y=100，位于文字和底部按钮中间
        _trainButton = Button::create();
        _trainButton->setTitleText("?? 训练士兵");
        _trainButton->setTitleFontSize(18);

        Size trainSize(200, 40);
        _trainButton->setContentSize(trainSize);
        _trainButton->setAnchorPoint(Vec2(0.5f, 0.5f));
        _trainButton->setPosition(Vec2(centerX, 100));
        _trainButton->setTouchEnabled(true);
        _trainButton->setPressedActionEnabled(true);
        _trainButton->addClickEventListener([this](Ref*) { onTrainClicked(); });

        auto trainBg = LayerColor::create(Color4B(50, 100, 200, 200), trainSize.width, trainSize.height);
        trainBg->setAnchorPoint(Vec2::ZERO);
        trainBg->setPosition(Vec2::ZERO);
        // trainBg->setTouchEnabled(false);
        _trainButton->addChild(trainBg, -1);
        _panel->addChild(_trainButton);
    }
}

void BuildingUpgradeUI::updateUI()
{
    if (!_building)
        return;

    _titleLabel->setString(_building->getDisplayName());

    if (_building->isMaxLevel())
    {
        _levelLabel->setString(StringUtils::format("等级: %d (MAX)", _building->getLevel()));
        _costLabel->setString("已达最高等级");
        _timeLabel->setString("");
        if (_upgradeButton)
        {
            _upgradeButton->setEnabled(false);
            _upgradeButton->setTitleColor(Color3B::GRAY);
        }
    }
    else
    {
        _levelLabel->setString(StringUtils::format("等级: %d → %d", _building->getLevel(), _building->getLevel() + 1));

        std::string resName = getResourceTypeName(_building->getUpgradeCostType());
        _costLabel->setString(StringUtils::format("费用: %d %s", _building->getUpgradeCost(), resName.c_str()));

        _timeLabel->setString(StringUtils::format("时间: %s", formatTime(_building->getUpgradeTime()).c_str()));

        if (_upgradeButton)
        {
            _upgradeButton->setEnabled(true);
            _upgradeButton->setTitleColor(Color3B::WHITE);
        }

        bool hasResource =
            ResourceManager::getInstance().hasEnough(_building->getUpgradeCostType(), _building->getUpgradeCost());
        _costLabel->setTextColor(hasResource ? Color4B::GREEN : Color4B::RED);
    }

    _descLabel->setString(_building->getBuildingDescription());
}

void BuildingUpgradeUI::setPositionNearBuilding(BaseBuilding* building)
{
    if (!building)
        return;
    Vec2 worldPos = building->getParent()->convertToWorldSpace(building->getPosition());
    Vec2 offset(0, building->getContentSize().height * building->getScale() * 0.5f + 20);
    this->setPosition(worldPos + offset);
}

void BuildingUpgradeUI::show()
{
    this->setVisible(true);
    this->setScale(0.0f);
    auto scaleIn = EaseBackOut::create(ScaleTo::create(0.2f, 1.0f));
    this->runAction(scaleIn);
}

void BuildingUpgradeUI::hide()
{
    auto scaleOut = EaseBackIn::create(ScaleTo::create(0.15f, 0.0f));
    // ?? 修复内存泄漏：使用 RemoveSelf 替代 lambda 捕获 this
    auto removeSelf = RemoveSelf::create();
    this->runAction(Sequence::create(scaleOut, removeSelf, nullptr));
}

void BuildingUpgradeUI::onUpgradeClicked()
{
    if (!_building)
        return;
    auto result = BuildingUpgradeService::getInstance().tryUpgrade(_building);

    if (result.success)
    {
        if (_resultCallback)
        {
            _resultCallback(true, _building->getLevel());
        }
        updateUI();
        onCloseClicked();
    }
    else
    {
        std::string hintMsg = result.message;
        switch (result.error)
        {
        case UpgradeError::kNoAvailableBuilder:
            hintMsg = "没有空闲的建筑工人！";
            break;
        case UpgradeError::kNotEnoughGold:
            hintMsg = "金币不足，无法升级！";
            break;
        case UpgradeError::kNotEnoughElixir:
            hintMsg = "圣水不足，无法升级！";
            break;
        default:
            break;
        }

        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
        {
            auto label = Label::createWithSystemFont(hintMsg, "Arial", 24);
            label->setPosition(Director::getInstance()->getVisibleSize() / 2);
            label->setTextColor(Color4B::RED);
            scene->addChild(label, 10000);

            auto move = MoveBy::create(1.0f, Vec2(0, 50));
            auto fade = FadeOut::create(1.0f);
            auto seq  = Sequence::create(Spawn::create(move, fade, nullptr), RemoveSelf::create(), nullptr);
            label->runAction(seq);
        }

        CCLOG("升级失败: %s", hintMsg.c_str());
    }
}

void BuildingUpgradeUI::onCloseClicked()
{
    if (_closeCallback)
        _closeCallback();
    hide();
}

void BuildingUpgradeUI::onTrainClicked()
{
    if (!_building || _building->getBuildingType() != BuildingType::kArmy)
        return;

    auto barracks = dynamic_cast<ArmyBuilding*>(_building);
    if (!barracks)
        return;

    CCLOG("打开训练UI：%s", barracks->getDisplayName().c_str());

    if (_closeCallback)
        _closeCallback();

    auto trainingUI = TrainingUI::create(barracks);
    if (trainingUI)
    {
        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
        {
            scene->addChild(trainingUI, 2000);
        }
    }

    hide();
}

std::string BuildingUpgradeUI::getResourceTypeName(ResourceType type) const
{
    switch (type)
    {
    case ResourceType::kGold:
        return "金币";
    case ResourceType::kElixir:
        return "圣水";
    case ResourceType::kGem:
        return "宝石";
    default:
        return "未知";
    }
}

std::string BuildingUpgradeUI::formatTime(int seconds) const
{
    if (seconds < 60)
    {
        return StringUtils::format("%d秒", seconds);
    }
    else if (seconds < 3600)
    {
        return StringUtils::format("%d分钟", seconds / 60);
    }
    else if (seconds < 86400)
    {
        return StringUtils::format("%d小时%d分", seconds / 3600, (seconds % 3600) / 60);
    }
    else
    {
        return StringUtils::format("%d天%d小时", seconds / 86400, (seconds % 86400) / 3600);
    }
}
#define _CRT_SECURE_NO_WARNINGS

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")

#include "Server.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

// ==================== 网络基础函数 ====================

bool Server::recvFixedAmount(SOCKET socket, char* buffer, int totalBytes)

{
    int received = 0;

    while (received < totalBytes)

    {
        int ret = recv(socket, buffer + received, totalBytes - received, 0);

        if (ret <= 0)

        {
            return false;
        }

        received += ret;
    }

    return true;
}

bool Server::sendPacket(SOCKET socket, uint32_t type, const std::string& data)

{
    PacketHeader header;

    header.type = type;

    header.length = static_cast<uint32_t>(data.size());

    int headerSent = send(socket, reinterpret_cast<char*>(&header), sizeof(PacketHeader), 0);

    if (headerSent != sizeof(PacketHeader))

    {
        return false;
    }

    if (header.length > 0)

    {
        int bodySent = send(socket, data.c_str(), static_cast<int>(header.length), 0);

        if (bodySent != static_cast<int>(header.length))

        {
            return false;
        }
    }

    return true;
}

bool Server::recvPacket(SOCKET socket, uint32_t& outType, std::string& outData)

{
    PacketHeader header;

    if (!recvFixedAmount(socket, reinterpret_cast<char*>(&header), sizeof(PacketHeader)))

    {
        return false;
    }

    outType = header.type;

    outData.clear();

    if (header.length > 0)

    {
        std::vector<char> buffer(header.length);

        if (!recvFixedAmount(socket, buffer.data(), static_cast<int>(header.length)))

        {
            return false;
        }

        outData.assign(buffer.begin(), buffer.end());
    }

    return true;
}

// ==================== 辅助函数 ====================

std::string Server::serializeAttackResult(const AttackResult& result)

{
    std::ostringstream oss;

    oss << result.attackerId << "|" << result.defenderId << "|" << result.starsEarned << "|" << result.goldLooted << "|"

        << result.elixirLooted << "|" << result.trophyChange << "|" << result.replayData;

    return oss.str();
}

AttackResult Server::deserializeAttackResult(const std::string& data)

{
    AttackResult result;

    std::istringstream iss(data);

    std::string token;

    std::getline(iss, result.attackerId, '|');

    std::getline(iss, result.defenderId, '|');

    std::getline(iss, token, '|');

    result.starsEarned = std::stoi(token);

    std::getline(iss, token, '|');

    result.goldLooted = std::stoi(token);

    std::getline(iss, token, '|');

    result.elixirLooted = std::stoi(token);

    std::getline(iss, token, '|');

    result.trophyChange = std::stoi(token);

    std::getline(iss, result.replayData);

    return result;
}

SOCKET Server::findSocketByPlayerId(const std::string& playerId)

{
    for (const auto& pair : onlinePlayers)

    {
        if (pair.second.playerId == playerId)

        {
            return pair.first;
        }
    }

    return INVALID_SOCKET;
}

// 🆕 PVP系统实现

void Server::handlePvpRequest(SOCKET clientSocket, const std::string& targetId)
{
    std::lock_guard<std::mutex> lock(pvpMutex);
    
    std::string attackerId = onlinePlayers[clientSocket].playerId;
    
    // 检查目标是否在线
    SOCKET targetSocket = findSocketByPlayerId(targetId);
    if (targetSocket == INVALID_SOCKET)
    {
        // 目标不在线，无法进行实时PVP
        sendPacket(clientSocket, PACKET_PVP_START, "FAIL|OFFLINE");
        return;
    }
    
    // 检查是否已经在战斗中
    if (pvpSessions.find(attackerId) != pvpSessions.end() || pvpSessions.find(targetId) != pvpSessions.end())
    {
        sendPacket(clientSocket, PACKET_PVP_START, "FAIL|BUSY");
        return;
    }
    
    // 获取目标地图数据
    std::string mapData = "";
    if (savedMaps.find(targetId) != savedMaps.end())
    {
        mapData = savedMaps[targetId];
    }
    else if (!onlinePlayers[targetSocket].mapData.empty())
    {
        mapData = onlinePlayers[targetSocket].mapData;
    }
    
    if (mapData.empty())
    {
        sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NO_MAP");
        return;
    }
    
    // 创建会话
    PvpSession session;
    session.attackerId = attackerId;
    session.defenderId = targetId;
    session.mapData = mapData;
    pvpSessions[attackerId] = session;
    
    // 通知攻击者：开始攻击
    // 格式: ATTACK|TargetID|MapData
    sendPacket(clientSocket, PACKET_PVP_START, "ATTACK|" + targetId + "|" + mapData);
    
    // 通知防御者：被攻击
    // 格式: DEFEND|AttackerID
    sendPacket(targetSocket, PACKET_PVP_START, "DEFEND|" + attackerId);
    
    std::cout << "[PVP] Started: " << attackerId << " vs " << targetId << std::endl;
}

void Server::handlePvpAction(SOCKET clientSocket, const std::string& actionData)
{
    std::lock_guard<std::mutex> lock(pvpMutex);
    
    std::string senderId = onlinePlayers[clientSocket].playerId;
    
    // 查找相关会话（发送者必须是攻击者）
    if (pvpSessions.find(senderId) == pvpSessions.end())
    {
        return;
    }
    
    const auto& session = pvpSessions[senderId];
    
    // 转发给防御者
    SOCKET defenderSocket = findSocketByPlayerId(session.defenderId);
    if (defenderSocket != INVALID_SOCKET)
    {
        sendPacket(defenderSocket, PACKET_PVP_ACTION, actionData);
    }
    
    // 转发给观战者
    for (const auto& spectatorId : session.spectatorIds)
    {
        SOCKET specSocket = findSocketByPlayerId(spectatorId);
        if (specSocket != INVALID_SOCKET)
        {
            sendPacket(specSocket, PACKET_PVP_ACTION, actionData);
        }
    }
}

void Server::handleSpectateRequest(SOCKET clientSocket, const std::string& targetId)
{
    std::lock_guard<std::mutex> lock(pvpMutex);
    
    std::string spectatorId = onlinePlayers[clientSocket].playerId;
    
    // 查找目标所在的会话
    std::string sessionKey = "";
    for (const auto& pair : pvpSessions)
    {
        if (pair.second.attackerId == targetId || pair.second.defenderId == targetId)
        {
            sessionKey = pair.first;
            break;
        }
    }
    
    if (sessionKey.empty())
    {
        sendPacket(clientSocket, PACKET_SPECTATE_JOIN, "FAIL|NO_BATTLE");
        return;
    }
    
    auto& session = pvpSessions[sessionKey];
    session.spectatorIds.push_back(spectatorId);
    
    // 通知观战者加入成功，发送地图数据
    // 格式: SPECTATE|AttackerID|DefenderID|MapData
    sendPacket(clientSocket, PACKET_SPECTATE_JOIN, "SPECTATE|" + session.attackerId + "|" + session.defenderId + "|" + session.mapData);
    
    std::cout << "[PVP] " << spectatorId << " is spectating " << session.attackerId << " vs " << session.defenderId << std::endl;
}

void Server::endPvpSession(const std::string& attackerId)
{
    std::lock_guard<std::mutex> lock(pvpMutex);
    
    if (pvpSessions.find(attackerId) != pvpSessions.end())
    {
        const auto& session = pvpSessions[attackerId];
        
        // 通知防御者结束
        SOCKET defenderSocket = findSocketByPlayerId(session.defenderId);
        if (defenderSocket != INVALID_SOCKET)
        {
            sendPacket(defenderSocket, PACKET_PVP_END, "FINISHED");
        }
        
        // 通知观战者结束
        for (const auto& spectatorId : session.spectatorIds)
        {
            SOCKET specSocket = findSocketByPlayerId(spectatorId);
            if (specSocket != INVALID_SOCKET)
            {
                sendPacket(specSocket, PACKET_PVP_END, "FINISHED");
            }
        }
        
        std::cout << "[PVP] Ended: " << attackerId << " vs " << session.defenderId << std::endl;
        
        pvpSessions.erase(attackerId);
    }
}

// ==================== 辅助函数 ====================

// 🆕 获取用户列表JSON
std::string Server::getUserListJson(const std::string& requesterId)
{
    std::lock_guard<std::mutex> lock(dataMutex);
    
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& pair : onlinePlayers)
    {
        const auto& player = pair.second;
        
        // 排除请求者自己
        if (player.playerId == requesterId || player.playerId.empty())
        {
            continue;
        }
        
        if (!first)
        {
            oss << "|";
        }
        first = false;
        
        // 格式: userId,username,thLevel,gold,elixir
        oss << player.playerId << ","
            << player.playerName << ","
            << "1" << ","  // 大本营等级（TODO: 从地图数据解析）
            << player.gold << ","
            << player.elixir;
    }
    
    return oss.str();
}

std::string Server::generateClanId()

{
    static std::random_device rd;

    static std::mt19937 gen(rd());

    static std::uniform_int_distribution<> dis(100000, 999999);

    return "CLAN_" + std::to_string(dis(gen));
}

std::string Server::generateWarId()

{
    static std::random_device rd;

    static std::mt19937 gen(rd());

    static std::uniform_int_distribution<> dis(100000, 999999);

    return "WAR_" + std::to_string(dis(gen));
}

// ==================== 匹配系统 ====================

void Server::addToMatchQueue(SOCKET socket)

{
    std::lock_guard<std::mutex> lock(matchQueueMutex);

    auto& player = onlinePlayers[socket];

    player.isSearchingMatch = true;

    player.matchStartTime = std::chrono::steady_clock::now();

    MatchQueueEntry entry;

    entry.socket = socket;

    entry.playerId = player.playerId;

    entry.trophies = player.trophies;

    entry.queueTime = std::chrono::steady_clock::now();

    matchQueue.push_back(entry);

    std::cout << "[Match] Player " << player.playerId << " joined queue (Trophies: " << player.trophies << ")"

              << std::endl;

    // 尝试立即匹配

    processMatchQueue();
}

void Server::removeFromMatchQueue(SOCKET socket)

{
    std::lock_guard<std::mutex> lock(matchQueueMutex);

    matchQueue.erase(std::remove_if(matchQueue.begin(), matchQueue.end(),

                                    [socket](const MatchQueueEntry& e) { return e.socket == socket; }),

                     matchQueue.end());

    if (onlinePlayers.find(socket) != onlinePlayers.end())

    {
        onlinePlayers[socket].isSearchingMatch = false;
    }
}

void Server::processMatchQueue()

{
    if (matchQueue.size() < 2)

        return;

    // 按奖杯数排序

    std::sort(matchQueue.begin(), matchQueue.end(),

              [](const MatchQueueEntry& a, const MatchQueueEntry& b) { return a.trophies < b.trophies; });

    std::vector<std::pair<size_t, size_t>> matchedPairs;

    // 寻找匹配对（奖杯差距在200以内）

    for (size_t i = 0; i < matchQueue.size(); ++i)

    {
        for (size_t j = i + 1; j < matchQueue.size(); ++j)

        {
            int diff = std::abs(matchQueue[i].trophies - matchQueue[j].trophies);

            // 等待时间越长，匹配范围越宽

            auto now = std::chrono::steady_clock::now();

            auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(now - matchQueue[i].queueTime).count();

            int maxDiff = 200 + static_cast<int>(waitTime * 10);  // 每秒增加10奖杯范围

            if (diff <= maxDiff)

            {
                matchedPairs.push_back({i, j});

                break;
            }
        }
    }

    // 处理匹配成功的玩家

    for (const auto& pair : matchedPairs)

    {
        auto& player1 = matchQueue[pair.first];

        auto& player2 = matchQueue[pair.second];

        // 发送匹配成功消息

        std::string matchData1 = player2.playerId + "|" + std::to_string(player2.trophies);

        std::string matchData2 = player1.playerId + "|" + std::to_string(player1.trophies);

        sendPacket(player1.socket, PACKET_MATCH_FOUND, matchData1);

        sendPacket(player2.socket, PACKET_MATCH_FOUND, matchData2);

        std::cout << "[Match] Matched: " << player1.playerId << " vs " << player2.playerId << std::endl;

        // 更新玩家状态

        if (onlinePlayers.find(player1.socket) != onlinePlayers.end())

        {
            onlinePlayers[player1.socket].isSearchingMatch = false;
        }

        if (onlinePlayers.find(player2.socket) != onlinePlayers.end())

        {
            onlinePlayers[player2.socket].isSearchingMatch = false;
        }
    }

    // 从队列中移除已匹配的玩家

    std::vector<MatchQueueEntry> newQueue;

    for (size_t i = 0; i < matchQueue.size(); ++i)

    {
        bool matched = false;

        for (const auto& pair : matchedPairs)

        {
            if (i == pair.first || i == pair.second)

            {
                matched = true;

                break;
            }
        }

        if (!matched)

        {
            newQueue.push_back(matchQueue[i]);
        }
    }

    matchQueue = std::move(newQueue);
}

// ==================== 部落系统 ====================

bool Server::createClan(const std::string& playerId, const std::string& clanName)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    // 检查玩家是否已在部落中

    SOCKET socket = findSocketByPlayerId(playerId);

    if (socket != INVALID_SOCKET && !onlinePlayers[socket].clanId.empty())

    {
        return false;
    }

    ClanInfo clan;

    clan.clanId = generateClanId();

    clan.clanName = clanName;

    clan.leaderId = playerId;

    clan.memberIds.push_back(playerId);

    clan.clanTrophies = 0;

    clans[clan.clanId] = clan;

    // 更新玩家的部落ID

    if (socket != INVALID_SOCKET)

    {
        onlinePlayers[socket].clanId = clan.clanId;
    }

    std::cout << "[Clan] Created: " << clanName << " (ID: " << clan.clanId << ") by " << playerId << std::endl;

    return true;
}

bool Server::joinClan(const std::string& playerId, const std::string& clanId)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    if (clans.find(clanId) == clans.end())

    {
        return false;
    }

    SOCKET socket = findSocketByPlayerId(playerId);

    if (socket == INVALID_SOCKET)

    {
        return false;
    }

    // 检查是否已在部落中

    if (!onlinePlayers[socket].clanId.empty())

    {
        return false;
    }

    // 检查奖杯要求

    auto& clan = clans[clanId];

    if (onlinePlayers[socket].trophies < clan.requiredTrophies)

    {
        return false;
    }

    clan.memberIds.push_back(playerId);

    onlinePlayers[socket].clanId = clanId;

    // 更新部落总奖杯

    clan.clanTrophies += onlinePlayers[socket].trophies;

    std::cout << "[Clan] " << playerId << " joined " << clan.clanName << std::endl;

    return true;
}

bool Server::leaveClan(const std::string& playerId)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    SOCKET socket = findSocketByPlayerId(playerId);

    if (socket == INVALID_SOCKET)

    {
        return false;
    }

    std::string clanId = onlinePlayers[socket].clanId;

    if (clanId.empty())

    {
        return false;
    }

    auto& clan = clans[clanId];

    // 从成员列表中移除

    clan.memberIds.erase(std::remove(clan.memberIds.begin(), clan.memberIds.end(), playerId), clan.memberIds.end());

    // 更新部落总奖杯

    clan.clanTrophies -= onlinePlayers[socket].trophies;

    // 如果是首领且还有其他成员，转让首领

    if (clan.leaderId == playerId && !clan.memberIds.empty())

    {
        clan.leaderId = clan.memberIds[0];
    }

    // 如果部落没有成员了，删除部落

    if (clan.memberIds.empty())

    {
        clans.erase(clanId);

        std::cout << "[Clan] Dissolved: " << clanId << std::endl;
    }

    onlinePlayers[socket].clanId = "";

    std::cout << "[Clan] " << playerId << " left clan " << clanId << std::endl;

    return true;
}

std::string Server::getClanListJson()

{
    std::lock_guard<std::mutex> lock(clanMutex);

    std::ostringstream oss;

    oss << "[";

    bool first = true;

    for (const auto& pair : clans)

    {
        if (!first)

            oss << ",";

        first = false;

        const auto& clan = pair.second;

        oss << "{\"id\":\"" << clan.clanId << "\""

            << ",\"name\":\"" << clan.clanName << "\""

            << ",\"members\":" << clan.memberIds.size() << ",\"trophies\":" << clan.clanTrophies

            << ",\"required\":" << clan.requiredTrophies << ",\"open\":" << (clan.isOpen ? "true" : "false") << "}";
    }

    oss << "]";

    return oss.str();
}

std::string Server::getClanMembersJson(const std::string& clanId)

{
    std::lock_guard<std::mutex> lock(clanMutex);

    if (clans.find(clanId) == clans.end())

    {
        return "[]";
    }

    const auto& clan = clans[clanId];

    std::ostringstream oss;

    oss << "{\"clanId\":\"" << clan.clanId << "\""

        << ",\"name\":\"" << clan.clanName << "\""

        << ",\"leader\":\"" << clan.leaderId << "\""

        << ",\"members\":[";

    bool first = true;

    for (const auto& memberId : clan.memberIds)

    {
        if (!first)

            oss << ",";

        first = false;

        SOCKET socket = findSocketByPlayerId(memberId);

        int trophies = 0;

        std::string name = memberId;

        if (socket != INVALID_SOCKET)

        {
            trophies = onlinePlayers[socket].trophies;

            name = onlinePlayers[socket].playerName;
        }

        oss << "{\"id\":\"" << memberId << "\""

            << ",\"name\":\"" << name << "\""

            << ",\"trophies\":" << trophies << ",\"online\":" << (socket != INVALID_SOCKET ? "true" : "false") << "}";
    }

    oss << "]}";

    return oss.str();
}

// ==================== 部落战争 ====================

void Server::addToClanWarQueue(const std::string& clanId)

{
    std::lock_guard<std::mutex> lock(warMutex);

    // 检查部落是否存在且未在战争中

    if (clans.find(clanId) == clans.end())

        return;

    for (const auto& war : activeWars)

    {
        if (war.second.clan1Id == clanId || war.second.clan2Id == clanId)

        {
            return;  // 已在战争中
        }
    }

    // 检查是否已在队列中

    if (std::find(clanWarQueue.begin(), clanWarQueue.end(), clanId) != clanWarQueue.end())

    {
        return;
    }

    clanWarQueue.push_back(clanId);

    std::cout << "[ClanWar] Clan " << clanId << " searching for war..." << std::endl;

    processClanWarQueue();
}

void Server::processClanWarQueue()

{
    if (clanWarQueue.size() < 2)

        return;

    // 按部落总奖杯排序

    std::sort(clanWarQueue.begin(), clanWarQueue.end(), [this](const std::string& a, const std::string& b) {
        return clans[a].clanTrophies < clans[b].clanTrophies;
    });

    // 匹配奖杯相近的部落

    for (size_t i = 0; i < clanWarQueue.size() - 1; ++i)

    {
        int diff = std::abs(clans[clanWarQueue[i]].clanTrophies - clans[clanWarQueue[i + 1]].clanTrophies);

        if (diff <= 500)

        {  // 部落战奖杯差距500以内

            startClanWar(clanWarQueue[i], clanWarQueue[i + 1]);

            // 从队列中移除

            clanWarQueue.erase(clanWarQueue.begin() + i + 1);

            clanWarQueue.erase(clanWarQueue.begin() + i);

            return;
        }
    }
}

void Server::startClanWar(const std::string& clan1Id, const std::string& clan2Id)

{
    ClanWarInfo war;

    war.warId = generateWarId();

    war.clan1Id = clan1Id;

    war.clan2Id = clan2Id;

    war.startTime = std::chrono::steady_clock::now();

    war.endTime = war.startTime + std::chrono::hours(24);  // 24小时战争

    war.isActive = true;

    activeWars[war.warId] = war;

    // 通知两个部落的所有在线成员

    std::string warData = war.warId + "|" + clan1Id + "|" + clan2Id;

    auto notifyClan = [this, &warData](const std::string& clanId) {
        if (clans.find(clanId) != clans.end())

        {
            for (const auto& memberId : clans[clanId].memberIds)

            {
                SOCKET socket = findSocketByPlayerId(memberId);

                if (socket != INVALID_SOCKET)

                {
                    sendPacket(socket, PACKET_CLAN_WAR_MATCH, warData);
                }
            }
        }
    };

    notifyClan(clan1Id);

    notifyClan(clan2Id);

    std::cout << "[ClanWar] Started: " << clans[clan1Id].clanName << " vs " << clans[clan2Id].clanName << std::endl;
}

void Server::processClanWarAttack(const std::string& warId, const AttackResult& result)

{
    std::lock_guard<std::mutex> lock(warMutex);

    if (activeWars.find(warId) == activeWars.end())

        return;

    auto& war = activeWars[warId];

    war.attacks.push_back(result);

    // 更新星星数

    SOCKET attackerSocket = findSocketByPlayerId(result.attackerId);

    if (attackerSocket != INVALID_SOCKET)

    {
        std::string attackerClanId = onlinePlayers[attackerSocket].clanId;

        if (attackerClanId == war.clan1Id)

        {
            war.clan1Stars += result.starsEarned;
        }

        else if (attackerClanId == war.clan2Id)

        {
            war.clan2Stars += result.starsEarned;
        }
    }

    // 广播战争状态更新

    std::ostringstream oss;

    oss << war.warId << "|" << war.clan1Stars << "|" << war.clan2Stars;

    std::string statusData = oss.str();

    auto broadcastToClan = [this, &statusData](const std::string& clanId) {
        if (clans.find(clanId) != clans.end())

        {
            for (const auto& memberId : clans[clanId].memberIds)

            {
                SOCKET socket = findSocketByPlayerId(memberId);

                if (socket != INVALID_SOCKET)

                {
                    sendPacket(socket, PACKET_CLAN_WAR_STATUS, statusData);
                }
            }
        }
    };

    broadcastToClan(war.clan1Id);

    broadcastToClan(war.clan2Id);
}

// ==================== 客户端处理 ====================

void clientHandler(SOCKET clientSocket, Server& server)

{
    uint32_t msgType;

    std::string msgData;

    while (Server::recvPacket(clientSocket, msgType, msgData))

    {
        std::lock_guard<std::mutex> lock(server.dataMutex);

        switch (msgType)

        {
                // ========== 基础功能 ==========

            case PACKET_LOGIN: {
                // 格式: playerId|playerName|trophies

                std::istringstream iss(msgData);

                std::string playerId, playerName, trophiesStr;

                std::getline(iss, playerId, '|');

                std::getline(iss, playerName, '|');

                std::getline(iss, trophiesStr, '|');

                server.onlinePlayers[clientSocket].playerId = playerId;

                server.onlinePlayers[clientSocket].playerName = playerName.empty() ? playerId : playerName;

                if (!trophiesStr.empty())

                {
                    server.onlinePlayers[clientSocket].trophies = std::stoi(trophiesStr);
                }

                std::cout << "[Login] User: " << playerId

                          << " (Trophies: " << server.onlinePlayers[clientSocket].trophies << ")" << std::endl;

                Server::sendPacket(clientSocket, PACKET_LOGIN, "Login Success");
            }

            break;

            case PACKET_UPLOAD_MAP: {
                std::string uid = server.onlinePlayers[clientSocket].playerId;

                if (!uid.empty())

                {
                    server.savedMaps[uid] = msgData;

                    server.onlinePlayers[clientSocket].mapData = msgData;

                    std::cout << "[Map] Saved for: " << uid << " (Size: " << msgData.size() << ")" << std::endl;
                }
            }

            break;

            case PACKET_QUERY_MAP: {
                std::string targetId = msgData;

                if (server.savedMaps.find(targetId) != server.savedMaps.end())

                {
                    Server::sendPacket(clientSocket, PACKET_QUERY_MAP, server.savedMaps[targetId]);

                    std::cout << "[Query] Sent map of " << targetId << " to client." << std::endl;
                }

                else

                {
                    Server::sendPacket(clientSocket, PACKET_QUERY_MAP, "");
                }
            }

            break;

            case PACKET_ATTACK_DATA:

                std::cout << "[Attack] Received replay data." << std::endl;

                break;

                // ========== 🆕 用户列表 ==========
            case REQ_USER_LIST: {
                std::string requesterId = server.onlinePlayers[clientSocket].playerId;
                std::string userList = server.getUserListJson(requesterId);
                Server::sendPacket(clientSocket, RESP_USER_LIST, userList);
                std::cout << "[UserList] Sent user list to: " << requesterId << std::endl;
            }
            break;

                // ========== 玩家匹配对战 ==========

            case PACKET_FIND_MATCH:

                server.addToMatchQueue(clientSocket);

                break;

            case PACKET_MATCH_CANCEL:

                server.removeFromMatchQueue(clientSocket);

                std::cout << "[Match] Player cancelled matchmaking." << std::endl;

                break;

            case PACKET_ATTACK_START: {
                // 请求攻击目标的地图

                std::string targetId = msgData;

                if (server.savedMaps.find(targetId) != server.savedMaps.end())

                {
                    Server::sendPacket(clientSocket, PACKET_ATTACK_START, server.savedMaps[targetId]);

                    std::cout << "[Battle] " << server.onlinePlayers[clientSocket].playerId << " attacking " << targetId

                              << std::endl;
                }
            }

            break;

            case PACKET_ATTACK_RESULT: {
                AttackResult result = server.deserializeAttackResult(msgData);

                // 更新攻击者资源

                server.onlinePlayers[clientSocket].gold += result.goldLooted;

                server.onlinePlayers[clientSocket].elixir += result.elixirLooted;

                server.onlinePlayers[clientSocket].trophies += result.trophyChange;

                // 更新被攻击者资源（如果在线）

                SOCKET defenderSocket = server.findSocketByPlayerId(result.defenderId);

                if (defenderSocket != INVALID_SOCKET)

                {
                    server.onlinePlayers[defenderSocket].gold -= result.goldLooted;

                    server.onlinePlayers[defenderSocket].elixir -= result.elixirLooted;

                    server.onlinePlayers[defenderSocket].trophies -= result.trophyChange;

                    // 通知被攻击者

                    Server::sendPacket(defenderSocket, PACKET_ATTACK_RESULT, msgData);
                }

                std::cout << "[Battle] Result - Stars: " << result.starsEarned << ", Gold: " << result.goldLooted

                          << ", Trophies: " << result.trophyChange << std::endl;
            }

            break;

                // ========== 部落系统 ==========

            case PACKET_CREATE_CLAN: {
                std::string playerId = server.onlinePlayers[clientSocket].playerId;

                if (server.createClan(playerId, msgData))

                {
                    std::string clanId = server.onlinePlayers[clientSocket].clanId;

                    Server::sendPacket(clientSocket, PACKET_CREATE_CLAN, "OK|" + clanId);
                }

                else

                {
                    Server::sendPacket(clientSocket, PACKET_CREATE_CLAN, "FAIL");
                }
            }

            break;

            case PACKET_JOIN_CLAN: {
                std::string playerId = server.onlinePlayers[clientSocket].playerId;

                if (server.joinClan(playerId, msgData))

                {
                    Server::sendPacket(clientSocket, PACKET_JOIN_CLAN, "OK");
                }

                else

                {
                    Server::sendPacket(clientSocket, PACKET_JOIN_CLAN, "FAIL");
                }
            }

            break;

            case PACKET_LEAVE_CLAN: {
                std::string playerId = server.onlinePlayers[clientSocket].playerId;

                if (server.leaveClan(playerId))

                {
                    Server::sendPacket(clientSocket, PACKET_LEAVE_CLAN, "OK");
                }

                else

                {
                    Server::sendPacket(clientSocket, PACKET_LEAVE_CLAN, "FAIL");
                }
            }

            break;

            case PACKET_CLAN_LIST: {
                std::string clanList = server.getClanListJson();

                Server::sendPacket(clientSocket, PACKET_CLAN_LIST, clanList);
            }

            break;

            case PACKET_CLAN_MEMBERS: {
                std::string members = server.getClanMembersJson(msgData);

                Server::sendPacket(clientSocket, PACKET_CLAN_MEMBERS, members);
            }

            break;

                // ========== 部落战争 ==========

            case PACKET_CLAN_WAR_SEARCH: {
                std::string clanId = server.onlinePlayers[clientSocket].clanId;

                if (!clanId.empty())

                {
                    server.addToClanWarQueue(clanId);

                    Server::sendPacket(clientSocket, PACKET_CLAN_WAR_SEARCH, "SEARCHING");
                }

                else

                {
                    Server::sendPacket(clientSocket, PACKET_CLAN_WAR_SEARCH, "NO_CLAN");
                }
            }

            break;

            case PACKET_CLAN_WAR_ATTACK: {
                // 格式: warId|targetMemberId

                std::istringstream iss(msgData);

                std::string warId, targetId;

                std::getline(iss, warId, '|');

                std::getline(iss, targetId, '|');

                if (server.savedMaps.find(targetId) != server.savedMaps.end())

                {
                    Server::sendPacket(clientSocket, PACKET_CLAN_WAR_ATTACK, warId + "|" + server.savedMaps[targetId]);
                }
            }

            break;

            case PACKET_CLAN_WAR_RESULT: {
                // 格式: warId|attackResult

                size_t pos = msgData.find('|');

                if (pos != std::string::npos)

                {
                    std::string warId = msgData.substr(0, pos);

                    std::string resultData = msgData.substr(pos + 1);

                    AttackResult result = server.deserializeAttackResult(resultData);

                    server.processClanWarAttack(warId, result);
                }
            }

            break;

            // 🆕 PVP与观战处理
            case PACKET_PVP_REQUEST:
                server.handlePvpRequest(clientSocket, msgData);
                break;

            case PACKET_PVP_ACTION:
                server.handlePvpAction(clientSocket, msgData);
                break;

            case PACKET_SPECTATE_REQUEST:
                server.handleSpectateRequest(clientSocket, msgData);
                break;

            case PACKET_PVP_END:
                {
                    std::string attackerId = server.onlinePlayers[clientSocket].playerId;
                    server.endPvpSession(attackerId);
                }
                break;

            default:

                break;
        }
    }

    // 玩家断开连接时的清理

    server.removeFromMatchQueue(clientSocket);
    // 🆕 清理PVP会话
    server.endPvpSession(server.onlinePlayers[clientSocket].playerId);

    server.closeClientSocket(clientSocket);
}

// ==================== 服务器生命周期 ====================

Server::Server() : port(8888)

{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)

    {
        exit(EXIT_FAILURE);
    }
}

Server::~Server()

{
    closesocket(serverSocket);

    WSACleanup();
}

void Server::run()

{
    createAndBindSocket();

    handleConnections();
}

void Server::createAndBindSocket()

{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET)

    {
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;

    serverAddr.sin_addr.s_addr = INADDR_ANY;

    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)

    {
        closesocket(serverSocket);

        exit(EXIT_FAILURE);
    }
}

void Server::handleConnections()

{
    listen(serverSocket, SOMAXCONN);

    std::cout << "=== Clash of Clans Server ===" << std::endl;

    std::cout << "Server started on port " << port << std::endl;

    std::cout << "Waiting for players..." << std::endl;

    while (true)

    {
        sockaddr_in clientAddr;

        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket != INVALID_SOCKET)

        {
            std::cout << "[Connect] New client: " << clientSocket << std::endl;

            {
                std::lock_guard<std::mutex> lock(dataMutex);

                PlayerContext ctx;

                ctx.socket = clientSocket;

                onlinePlayers[clientSocket] = ctx;
            }

            std::thread clientThread(clientHandler, clientSocket, std::ref(*this));

            clientThread.detach();
        }
    }
}

void Server::closeClientSocket(SOCKET clientSocket)

{
    std::lock_guard<std::mutex> lock(dataMutex);

    std::string playerId = onlinePlayers[clientSocket].playerId;

    closesocket(clientSocket);

    onlinePlayers.erase(clientSocket);

    std::cout << "[Disconnect] Client: " << clientSocket;

    if (!playerId.empty())

    {
        std::cout << " (Player: " << playerId << ")";
    }

    std::cout << std::endl;
}