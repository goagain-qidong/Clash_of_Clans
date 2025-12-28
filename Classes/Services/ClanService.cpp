/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanService.cpp
 * File Function: 部落服务层实现
 * Author:        赵崇治
 * Update Date:   2025/12/21
 * License:       MIT License
 ****************************************************************/
#include "ClanService.h"
#include "Managers/AccountManager.h"
#include "Managers/SocketClient.h"
#include "cocos2d.h"
#include "json/document.h"
#include <sstream>

USING_NS_CC;

ClanService& ClanService::getInstance()
{
    static ClanService instance;
    return instance;
}

void ClanService::initialize()
{
    // 总是注册网络回调，确保ClanPanel打开时接管回调
    // 即使服务已初始化，也需要重新接管回调，因为可能被DraggableMapScene覆盖
    registerNetworkCallbacks();

    if (_initialized)
        return;
    _initialized = true;

    syncLocalClanInfo();
}

void ClanService::cleanup()
{
    auto& client = SocketClient::getInstance();
    client.setOnConnected(nullptr);
    client.setOnUserListReceived(nullptr);
    client.setOnClanMembers(nullptr);
    client.setOnClanList(nullptr);
    client.setOnClanCreated(nullptr);
    client.setOnClanJoined(nullptr);
    client.setOnClanLeft(nullptr);
    // 注意：不清除聊天回调，保持实时接收
    // client.setOnChatMessage(nullptr);
    client.setOnBattleStatusList(nullptr);

    _initialized = false;
}

void ClanService::syncLocalClanInfo()
{
    auto& accMgr = AccountManager::getInstance();
    auto  cur    = accMgr.getCurrentAccount();

    if (cur && !cur->gameState.progress.clanId.empty())
    {
        auto& cache = ClanDataCache::getInstance();
        cache.setCurrentClan(cur->gameState.progress.clanId, "");
    }
}

void ClanService::registerNetworkCallbacks()
{
    auto& client = SocketClient::getInstance();
    auto& cache  = ClanDataCache::getInstance();

    // 在线玩家列表
    client.setOnUserListReceived([this](const std::string& data) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(
            [this, data]() { parseUserListData(data); });
    });

    // 部落成员列表
    client.setOnClanMembers([this](const std::string& json) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(
            [this, json]() { parseClanMembersData(json); });
    });

    // 部落列表
    client.setOnClanList([&cache](const std::vector<ClanInfoClient>& clans) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(
            [&cache, clans]() { cache.setClanList(clans); });
    });

    // 战斗状态
    client.setOnBattleStatusList([this](const std::string& json) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(
            [this, json]() { parseBattleStatusData(json); });
    });

    // 创建部落回调
    client.setOnClanCreated([this](bool success, const std::string& clanId) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success, clanId]() {
            if (success)
            {
                auto& cache = ClanDataCache::getInstance();
                cache.setCurrentClan(clanId, _pendingClanName);

                // 同步到本地账户
                auto& accMgr = AccountManager::getInstance();
                if (auto cur = const_cast<AccountInfo*>(accMgr.getCurrentAccount()))
                {
                    GameStateData newState   = cur->gameState;
                    newState.progress.clanId = clanId;
                    accMgr.updateGameState(newState);
                    accMgr.save();
                }

                if (_createClanCallback)
                    _createClanCallback(true, "创建部落成功！");
            }
            else
            {
                if (_createClanCallback)
                    _createClanCallback(false, "创建部落失败");
            }
            _createClanCallback = nullptr;
            _pendingClanName.clear();
        });
    });

    // 加入部落回调
    client.setOnClanJoined([this](bool success) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
            if (success && !_pendingClanId.empty())
            {
                auto&       cache    = ClanDataCache::getInstance();
                std::string clanName = cache.findClanNameById(_pendingClanId);
                cache.setCurrentClan(_pendingClanId, clanName);

                // 同步到本地账户
                auto& accMgr = AccountManager::getInstance();
                if (auto cur = const_cast<AccountInfo*>(accMgr.getCurrentAccount()))
                {
                    GameStateData newState   = cur->gameState;
                    newState.progress.clanId = _pendingClanId;
                    accMgr.updateGameState(newState);
                    accMgr.save();
                }

                if (_joinClanCallback)
                    _joinClanCallback(true, "加入部落成功！");
            }
            else
            {
                if (_joinClanCallback)
                    _joinClanCallback(false, "加入部落失败");
            }
            _joinClanCallback = nullptr;
            _pendingClanId.clear();
        });
    });

    // 🆕 退出部落回调
    client.setOnClanLeft([this](bool success) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
            if (success)
            {
                auto& cache = ClanDataCache::getInstance();
                cache.clearCurrentClan();

                // 同步到本地账户
                auto& accMgr = AccountManager::getInstance();
                if (auto cur = const_cast<AccountInfo*>(accMgr.getCurrentAccount()))
                {
                    GameStateData newState = cur->gameState;
                    newState.progress.clanId.clear();
                    accMgr.updateGameState(newState);
                    accMgr.save();
                }

                if (_leaveClanCallback)
                    _leaveClanCallback(true, "已退出部落");
            }
            else
            {
                if (_leaveClanCallback)
                    _leaveClanCallback(false, "退出部落失败");
            }
            _leaveClanCallback = nullptr;
        });
    });

    // 🆕 聊天消息回调 - 始终保持注册，确保能接收实时消息
    client.setOnChatMessage([this](const std::string& sender, const std::string& message) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, sender, message]() {
            CCLOG("[ClanService] 收到聊天消息: %s: %s", sender.c_str(), message.c_str());
            
            // 存入缓存（addChatMessage会自动去重和保存）
            bool isNew = ClanDataCache::getInstance().addChatMessage(sender, message);
            
            // 只有新消息才通知UI（如果有监听者的话）
            if (isNew && _chatCallback)
            {
                _chatCallback(sender, message);
            }
        });
    });
}

void ClanService::connect(const std::string& ip, int port, OperationCallback callback)
{
    _connectCallback = callback;

    auto& client = SocketClient::getInstance();
    client.setOnConnected([this](bool success) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([this, success]() {
            if (success)
            {
                // 登录并上传地图
                auto& accMgr = AccountManager::getInstance();
                if (auto cur = accMgr.getCurrentAccount())
                {
                    // 发送登录信息，包含部落ID
                    SocketClient::getInstance().login(
                        cur->account.userId, 
                        cur->account.username,
                        cur->gameState.progress.trophies,
                        cur->gameState.progress.clanId  // 发送部落ID
                    );
                    std::string mapData = accMgr.exportGameStateJson();
                    if (!mapData.empty())
                    {
                        SocketClient::getInstance().uploadMap(mapData);
                    }
                }
            }

            if (_connectCallback)
            {
                _connectCallback(success, success ? "连接成功！" : "连接失败");
                _connectCallback = nullptr;
            }
        });
    });

    client.connect(ip, port);
}

bool ClanService::isConnected() const
{
    return SocketClient::getInstance().isConnected();
}

void ClanService::requestOnlinePlayers()
{
    SocketClient::getInstance().requestUserList();
}

void ClanService::requestClanMembers()
{
    auto& cache = ClanDataCache::getInstance();
    if (cache.isInClan() && !cache.getCurrentClanId().empty())
    {
        SocketClient::getInstance().getClanMembers(cache.getCurrentClanId());
    }
}

void ClanService::requestClanList()
{
    SocketClient::getInstance().getClanList();
}

void ClanService::requestBattleStatus()
{
    SocketClient::getInstance().requestBattleStatusList();
}

void ClanService::createClan(const std::string& clanName, OperationCallback callback)
{
    _createClanCallback = callback;
    _pendingClanName    = clanName;
    SocketClient::getInstance().createClan(clanName);
}

void ClanService::joinClan(const std::string& clanId, OperationCallback callback)
{
    _joinClanCallback = callback;
    _pendingClanId    = clanId;
    SocketClient::getInstance().joinClan(clanId);
}

// 🆕 退出部落实现
void ClanService::leaveClan(OperationCallback callback)
{
    auto& cache = ClanDataCache::getInstance();
    if (!cache.isInClan())
    {
        if (callback)
            callback(false, "你还没有加入部落");
        return;
    }

    _leaveClanCallback = callback;
    SocketClient::getInstance().leaveClan();
}

void ClanService::sendChatMessage(const std::string& message)
{
    SocketClient::getInstance().sendChatMessage(message);
}

void ClanService::setOnChatMessage(std::function<void(const std::string&, const std::string&)> callback)
{
    _chatCallback = callback;
}

void ClanService::parseUserListData(const std::string& data)
{
    std::vector<OnlinePlayerInfo> players;

    if (!data.empty())
    {
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

            OnlinePlayerInfo info;
            info.userId   = userId;
            info.username = username;
            info.thLevel  = thLevelStr.empty() ? 1 : std::stoi(thLevelStr);
            info.gold     = goldStr.empty() ? 0 : std::stoi(goldStr);
            info.elixir   = elixirStr.empty() ? 0 : std::stoi(elixirStr);

            players.push_back(info);
        }
    }

    ClanDataCache::getInstance().setOnlinePlayers(players);
}

void ClanService::parseClanMembersData(const std::string& json)
{
    std::vector<ClanMemberInfo> members;

    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("members"))
    {
        const auto& arr = doc["members"];
        if (arr.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < arr.Size(); i++)
            {
                const auto&    member = arr[i];
                ClanMemberInfo info;
                info.id       = member["id"].GetString();
                info.name     = member["name"].GetString();
                info.trophies = member["trophies"].GetInt();
                info.isOnline = member["online"].GetBool();
                members.push_back(info);
            }
        }
    }

    ClanDataCache::getInstance().setClanMembers(members);
}

void ClanService::parseBattleStatusData(const std::string& json)
{
    std::map<std::string, PlayerBattleStatus> statusMap;

    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("statuses"))
    {
        const auto& arr = doc["statuses"];
        if (arr.IsArray())
        {
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

                statusMap[uid] = s;
            }
        }
    }

    ClanDataCache::getInstance().setBattleStatusMap(statusMap);
}