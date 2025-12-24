/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SocketClient.cpp
 * File Function: 负责客户端与服务器的网络通信
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "SocketClient.h"
#include <algorithm>
#include <sstream>
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

// ==================== AttackResult 序列化 ====================
std::string AttackResult::serialize() const
{
    std::ostringstream oss;
    oss << attackerId << "|" << defenderId << "|" << starsEarned << "|" << goldLooted << "|" << elixirLooted << "|"
        << trophyChange << "|" << replayData;
    return oss.str();
}
AttackResult AttackResult::deserialize(const std::string& data)
{
    AttackResult result;
    std::istringstream iss(data);
    std::string token;
    std::getline(iss, result.attackerId, '|');
    std::getline(iss, result.defenderId, '|');
    std::getline(iss, token, '|');
    if (!token.empty())
        result.starsEarned = std::stoi(token);
    std::getline(iss, token, '|');
    if (!token.empty())
        result.goldLooted = std::stoi(token);
    std::getline(iss, token, '|');
    if (!token.empty())
        result.elixirLooted = std::stoi(token);
    std::getline(iss, token, '|');
    if (!token.empty())
        result.trophyChange = std::stoi(token);
    std::getline(iss, result.replayData);
    return result;
}
// ==================== SocketClient 单例 ====================
SocketClient& SocketClient::getInstance()
{
    static SocketClient instance;
    return instance;
}
SocketClient::SocketClient()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
    {
        _wsaInitialized = true;
    }
#endif
}
SocketClient::~SocketClient()
{
    disconnect();
#ifdef _WIN32
    if (_wsaInitialized)
    {
        WSACleanup();
    }
#endif
}
// ==================== 连接管理 ====================
bool SocketClient::connect(const std::string& host, int port)
{
    if (_connected)
    {
        return true;
    }
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket == INVALID_SOCKET)
    {
        if (_onConnected)
        {
            _onConnected(false);
        }
        return false;
    }
    sockaddr_in serverAddr; // 服务器地址结构
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_port = htons(port); // 大小端转换端口号
#ifdef _WIN32
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str()); // 手动指定IP地址，将字符串IP地址转换为整数IP地址
#else
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
#endif
    if (::connect(_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        if (_onConnected)
        {
            _onConnected(false);
        }
        return false;
    }
    _connected = true;
    _running = true;
    // 启动接收线程
    _recvThread = std::thread(&SocketClient::recvThreadFunc, this);
    cocos2d::log("[SocketClient] Connected to %s:%d", host.c_str(), port);
    if (_onConnected)
    {
        _onConnected(true);
    }
    return true;
}
void SocketClient::disconnect()
{
    // 无论当前连接状态如何，都要确保清理资源和线程
    _running = false;
    _connected = false;

    if (_socket != INVALID_SOCKET)
    {
        // 关闭套接字会强制 recv 返回错误，从而退出接收线程循环
        closesocket(_socket);
        _socket = INVALID_SOCKET;
    }

    // 必须等待接收线程结束，否则析构时会导致 abort()
    if (_recvThread.joinable())
    {
        // 防止在接收线程中调用 disconnect 导致死锁（虽然设计上不应发生）
        if (std::this_thread::get_id() != _recvThread.get_id())
        {
            _recvThread.join();
        }
        else
        {
            _recvThread.detach();
        }
    }

    cocos2d::log("[SocketClient] Disconnected");
    if (_onDisconnected)
    {
        _onDisconnected();
    }
}
bool SocketClient::isConnected() const
{
    return _connected;
}
// ==================== 网络基础函数 ====================
bool SocketClient::recvFixedAmount(char* buffer, int totalBytes)
{
    int received = 0;
    while (received < totalBytes && _running)
    {
        int ret = recv(_socket, buffer + received, totalBytes - received, 0);
        if (ret <= 0)
        {
            return false;
        }
        received += ret;
    }
    return received == totalBytes;
}
bool SocketClient::sendPacket(uint32_t type, const std::string& data)
{
    if (!_connected || _socket == INVALID_SOCKET)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(_sendMutex);
    PacketHeader header;
    header.type = type;
    header.length = static_cast<uint32_t>(data.size());
    int headerSent = send(_socket, reinterpret_cast<char*>(&header), sizeof(PacketHeader), 0);
    if (headerSent != sizeof(PacketHeader))
    {
        return false;
    }
    if (header.length > 0)
    {
        int bodySent = send(_socket, data.c_str(), static_cast<int>(header.length), 0);
        if (bodySent != static_cast<int>(header.length))
        {
            return false;
        }
    }
    return true;
}
bool SocketClient::recvPacket(uint32_t& outType, std::string& outData)
{
    PacketHeader header;
    if (!recvFixedAmount(reinterpret_cast<char*>(&header), sizeof(PacketHeader)))
    {
        return false;
    }
    outType = header.type;
    outData.clear();
    if (header.length > 0)
    {
        std::vector<char> buffer(header.length);
        if (!recvFixedAmount(buffer.data(), static_cast<int>(header.length)))
        {
            return false;
        }
        outData.assign(buffer.begin(), buffer.end());
    }
    return true;
}
// ==================== 接收线程 ====================
void SocketClient::recvThreadFunc()
{
    while (_running)
    {
        uint32_t msgType;
        std::string msgData;
        if (recvPacket(msgType, msgData))
        {
            std::lock_guard<std::mutex> lock(_callbackMutex);
            _pendingPackets.push({msgType, msgData});
        }
        else
        {
            if (_running)
            {
                _connected = false;
                _running = false;
                std::lock_guard<std::mutex> lock(_callbackMutex);
                _pendingPackets.push({0, "DISCONNECTED"});
            }
            break;
        }
    }
}
// ==================== 处理回调（主线程） ====================
void SocketClient::processCallbacks()
{
    std::queue<ReceivedPacket> packets;
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        std::swap(packets, _pendingPackets);
    }
    while (!packets.empty())
    {
        auto& packet = packets.front();
        if (packet.type == 0 && packet.data == "DISCONNECTED")
        {
            if (_onDisconnected)
            {
                _onDisconnected();
            }
        }
        else
        {
            handlePacket(packet.type, packet.data);
        }
        packets.pop();
    }
}
void SocketClient::handlePacket(uint32_t type, const std::string& data)
{
    switch (type)
    {
    case CYCKET_LOGIN:
        if (_onLoginResult)
        {
            bool success = (data == "Login Success");
            _onLoginResult(success, data);
        }
        break;
    case PACKET_QUERY_MAP:
        if (_onMapReceived)
        {
            _onMapReceived(data);
        }
        break;
    // 🆕 处理用户列表响应
    case RESP_USER_LIST:
        if (_onUserListReceived)
        {
            _onUserListReceived(data);
        }
        break;
    case PACKET_MATCH_FOUND:
        if (_onMatchFound)
        {
            std::istringstream iss(data);
            std::string opponentId, trophiesStr;
            std::getline(iss, opponentId, '|');
            std::getline(iss, trophiesStr, '|');
            MatchInfo info;
            info.opponentId = opponentId;
            if (!trophiesStr.empty())
            {
                try {
                    info.opponentTrophies = std::stoi(trophiesStr);
                } catch (...) {
                    info.opponentTrophies = 0;
                }
            }
            _onMatchFound(info);
        }
        break;
    case PACKET_ATTACK_START:
        if (_onAttackStart)
        {
            _onAttackStart(data); // 对手的地图数据
        }
        break;
    case PACKET_ATTACK_RESULT:
        if (_onAttackResult)
        {
            AttackResult result = AttackResult::deserialize(data);
            _onAttackResult(result);
        }
        break;
    case PACKET_CREATE_CLAN:
        if (_onClanCreated)
        {
            bool success = (data.length() >= 2) && (data.substr(0, 2) == "OK");
            std::string clanId = "";
            if (success && data.length() > 3)
            {
                clanId = data.substr(3);
            }
            _onClanCreated(success, clanId);
        }
        break;
    case PACKET_JOIN_CLAN:
        if (_onClanJoined)
        {
            _onClanJoined(data == "OK");
        }
        break;
    case PACKET_LEAVE_CLAN:
        if (_onClanLeft)
        {
            _onClanLeft(data == "OK");
        }
        break;
    case PACKET_CLAN_LIST:
        if (_onClanList)
        {
            std::vector<ClanInfoClient> clans;
            rapidjson::Document doc;
            doc.Parse(data.c_str());
            if (!doc.HasParseError() && doc.IsArray())
            {
                for (rapidjson::SizeType i = 0; i < doc.Size(); i++)
                {
                    const auto& item = doc[i];
                    ClanInfoClient clan;
                    if (item.HasMember("id") && item["id"].IsString())
                        clan.clanId = item["id"].GetString();
                    if (item.HasMember("name") && item["name"].IsString())
                        clan.clanName = item["name"].GetString();
                    if (item.HasMember("members") && item["members"].IsInt())
                        clan.memberCount = item["members"].GetInt();
                    if (item.HasMember("trophies") && item["trophies"].IsInt())
                        clan.clanTrophies = item["trophies"].GetInt();
                    if (item.HasMember("required") && item["required"].IsInt())
                        clan.requiredTrophies = item["required"].GetInt();
                    if (item.HasMember("open") && item["open"].IsBool())
                        clan.isOpen = item["open"].GetBool();
                    clans.push_back(clan);
                }
            }
            _onClanList(clans);
        }
        break;
    case PACKET_CLAN_MEMBERS:
        if (_onClanMembers)
        {
            _onClanMembers(data);
        }
        break;
    case PACKET_CLAN_WAR_MATCH:
        if (_onClanWarMatch)
        {
            std::istringstream iss(data);
            std::string warId, clan1Id, clan2Id;
            std::getline(iss, warId, '|');
            std::getline(iss, clan1Id, '|');
            std::getline(iss, clan2Id, '|');
            _onClanWarMatch(warId, clan1Id, clan2Id);
        }
        break;
    case PACKET_CLAN_WAR_STATUS:
        if (_onClanWarStatus)
        {
            std::istringstream iss(data);
            std::string warId, stars1Str, stars2Str;
            std::getline(iss, warId, '|');
            std::getline(iss, stars1Str, '|');
            std::getline(iss, stars2Str, '|');
            int stars1 = 0, stars2 = 0;
            try {
                if (!stars1Str.empty()) stars1 = std::stoi(stars1Str);
                if (!stars2Str.empty()) stars2 = std::stoi(stars2Str);
            } catch (...) {}
            _onClanWarStatus(warId, stars1, stars2);
        }
        break;
    // 🆕 PVP处理
    case PACKET_PVP_START:
        if (_onPvpStart)
        {
            // 格式: ROLE|OpponentID|MapData
            // ROLE: ATTACK, DEFEND, FAIL
            std::istringstream iss(data);
            std::string role, opponentId, mapData;
            std::getline(iss, role, '|');
            std::getline(iss, opponentId, '|');
            std::getline(iss, mapData); // 剩余部分为地图数据
            _onPvpStart(role, opponentId, mapData);
        }
        break;
    case PACKET_PVP_ACTION:
        if (_onPvpAction)
        {
            // 格式: UnitType|X|Y
            try {
                std::istringstream iss(data);
                std::string token;
                std::getline(iss, token, '|');
                if (token.empty()) break;
                int unitType = std::stoi(token);
                std::getline(iss, token, '|');
                if (token.empty()) break;
                float x = std::stof(token);
                std::getline(iss, token, '|');
                if (token.empty()) break;
                float y = std::stof(token);
                _onPvpAction(unitType, x, y);
            } catch (const std::exception& e) {
                cocos2d::log("[SocketClient] Error parsing PVP_ACTION: %s", e.what());
            }
        }
        break;
    case PACKET_PVP_END:
        if (_onPvpEnd)
        {
            _onPvpEnd(data);
        }
        break;
    case PACKET_SPECTATE_JOIN:
        if (_onSpectateJoin)
        {
            // 服务器发送格式: "1|attackerId|defenderId|mapData" 成功
            // 或 "0|||" 失败
            // 🆕 支持历史记录: "1|attackerId|defenderId|mapData|||HISTORY|||action1|||action2|||..."
            if (data.empty() || data[0] == '0')
            {
                _onSpectateJoin(false, "", "", "", {});
            }
            else
            {
                std::istringstream iss(data);
                std::string successFlag, attackerId, defenderId, mapDataAndHistory;
                std::getline(iss, successFlag, '|');
                std::getline(iss, attackerId, '|');
                std::getline(iss, defenderId, '|');
                std::getline(iss, mapDataAndHistory);

                std::string mapData = mapDataAndHistory;
                std::vector<std::string> history;

                size_t historyPos = mapDataAndHistory.find("|||HISTORY|||");
                if (historyPos != std::string::npos)
                {
                    mapData = mapDataAndHistory.substr(0, historyPos);
                    std::string historyStr = mapDataAndHistory.substr(historyPos + 13); // length of |||HISTORY|||

                    size_t pos = 0;
                    while ((pos = historyStr.find("|||")) != std::string::npos)
                    {
                        history.push_back(historyStr.substr(0, pos));
                        historyStr.erase(0, pos + 3);
                    }
                }

                _onSpectateJoin(true, attackerId, defenderId, mapData, history);
            }
        }
        break;
    
    // 🆕 部落战争增强处理
    case PACKET_CLAN_WAR_MEMBER_LIST:
        if (_onClanWarMemberList)
        {
            _onClanWarMemberList(data);
        }
        break;
    
    case PACKET_CLAN_WAR_ATTACK_START:
        if (_onClanWarAttackStart)
        {
            // 格式: ATTACK|TargetID|MapData 或 FAIL|Reason|
            if (data.length() >= 4 && data.substr(0, 4) == "FAIL")
            {
                cocos2d::log("[SocketClient] 部落战攻击失败: %s", data.c_str());
                _onClanWarAttackStart("FAIL", "", "");
            }
            else
            {
                std::istringstream iss(data);
                std::string type, targetId, mapData;
                std::getline(iss, type, '|');
                std::getline(iss, targetId, '|');
                std::getline(iss, mapData);
                _onClanWarAttackStart(type, targetId, mapData);
            }
        }
        break;
    
    case PACKET_CLAN_WAR_SPECTATE:
        if (_onClanWarSpectate)
        {
            // 服务器发送格式: "1|attackerId|defenderId|mapData" 成功
            // 或 "0|||" 失败
            if (data.empty() || data[0] == '0')
            {
                cocos2d::log("[SocketClient] 部落战观战失败");
                _onClanWarSpectate(false, "", "", "");
            }
            else
            {
                std::istringstream iss(data);
                std::string successFlag, attackerId, defenderId, mapData;
                std::getline(iss, successFlag, '|');
                std::getline(iss, attackerId, '|');
                std::getline(iss, defenderId, '|');
                std::getline(iss, mapData);
                _onClanWarSpectate(true, attackerId, defenderId, mapData);
            }
        }
        break;
    
    case PACKET_CLAN_WAR_STATE_UPDATE:
        if (_onClanWarStateUpdate)
        {
            _onClanWarStateUpdate(data);
        }
        break;

    case PACKET_BATTLE_STATUS_LIST:
        if (_onBattleStatusList)
        {
            _onBattleStatusList(data);
        }
        break;
    
    default:
        cocos2d::log("[SocketClient] Unknown packet type: %d", type);
        break;
    }
}
// ==================== 基础功能 ====================
void SocketClient::login(const std::string& playerId, const std::string& playerName, int trophies)
{
    std::ostringstream oss;
    oss << playerId << "|" << playerName << "|" << trophies;
    sendPacket(CYCKET_LOGIN, oss.str());
}
void SocketClient::uploadMap(const std::string& mapData)
{
    sendPacket(PACKET_UPLOAD_MAP, mapData);
}
void SocketClient::queryMap(const std::string& targetId)
{
    sendPacket(PACKET_QUERY_MAP, targetId);
}
void SocketClient::requestUserList()
{
    sendPacket(REQ_USER_LIST, "");
    cocos2d::log("[SocketClient] 请求用户列表");
}
// ==================== 玩家对战 ====================
void SocketClient::findMatch()
{
    sendPacket(PACKET_FIND_MATCH, "");
}
void SocketClient::cancelMatch()
{
    sendPacket(PACKET_MATCH_CANCEL, "");
}
void SocketClient::startAttack(const std::string& targetId)
{
    sendPacket(PACKET_ATTACK_START, targetId);
}
void SocketClient::submitAttackResult(const AttackResult& result)
{
    sendPacket(PACKET_ATTACK_RESULT, result.serialize());
}
// ==================== 部落系统 ====================
void SocketClient::createClan(const std::string& clanName)
{
    sendPacket(PACKET_CREATE_CLAN, clanName);
}
void SocketClient::joinClan(const std::string& clanId)
{
    sendPacket(PACKET_JOIN_CLAN, clanId);
}
void SocketClient::leaveClan()
{
    sendPacket(PACKET_LEAVE_CLAN, "");
}
void SocketClient::getClanList()
{
    sendPacket(PACKET_CLAN_LIST, "");
}
void SocketClient::getClanMembers(const std::string& clanId)
{
    sendPacket(PACKET_CLAN_MEMBERS, clanId);
}
// ==================== 部落战争 ====================
void SocketClient::searchClanWar()
{
    sendPacket(PACKET_CLAN_WAR_SEARCH, "");
}
void SocketClient::attackInClanWar(const std::string& warId, const std::string& targetMemberId)
{
    sendPacket(PACKET_CLAN_WAR_ATTACK, warId + "|" + targetMemberId);
}
void SocketClient::submitClanWarResult(const std::string& warId, const AttackResult& result)
{
    sendPacket(PACKET_CLAN_WAR_RESULT, warId + "|" + result.serialize());
}

// 🆕 部落战争增强实现
void SocketClient::requestClanWarMemberList(const std::string& warId)
{
    sendPacket(PACKET_CLAN_WAR_MEMBER_LIST, warId);
    cocos2d::log("[SocketClient] 请求部落战成员列表: %s", warId.c_str());
}

void SocketClient::startClanWarAttack(const std::string& warId, const std::string& targetId)
{
    std::string data = warId + "|" + targetId;
    sendPacket(PACKET_CLAN_WAR_ATTACK_START, data);
    cocos2d::log("[SocketClient] 发起部落战攻击: warId=%s, target=%s", warId.c_str(), targetId.c_str());
}

void SocketClient::endClanWarAttack(const std::string& warId, int stars, float destructionRate)
{
    // 获取当前玩家信息
    std::string attackerId = "unknown";
    std::string attackerName = "unknown";
    
    // TODO: 从AccountManager获取实际玩家信息
    // auto& accMgr = AccountManager::getInstance();
    // if (auto cur = accMgr.getCurrentAccount()) {
    //     attackerId = cur->userId;
    //     attackerName = cur->username;
    // }
    
    std::ostringstream oss;
    oss << warId << "|" 
        << attackerId << "|"
        << attackerName << "|"
        << stars << "|"
        << destructionRate;
    
    sendPacket(PACKET_CLAN_WAR_ATTACK_END, oss.str());
    cocos2d::log("[SocketClient] 结束部落战攻击: warId=%s, stars=%d, destruction=%.2f", 
                 warId.c_str(), stars, destructionRate);
}

void SocketClient::spectateClanWar(const std::string& warId, const std::string& targetId)
{
    std::string data = warId + "|" + targetId;
    sendPacket(PACKET_CLAN_WAR_SPECTATE, data);
    cocos2d::log("[SocketClient] 请求观战部落战: warId=%s, target=%s", warId.c_str(), targetId.c_str());
}

// 🆕 PVP系统实现
void SocketClient::requestPvp(const std::string& targetId)
{
    sendPacket(PACKET_PVP_REQUEST, targetId);
}

void SocketClient::sendPvpAction(int unitType, float x, float y)
{
    std::ostringstream oss;
    oss << unitType << "|" << x << "|" << y;
    sendPacket(PACKET_PVP_ACTION, oss.str());
}

void SocketClient::endPvp()
{
    sendPacket(PACKET_PVP_END, "");
}

void SocketClient::requestSpectate(const std::string& targetId)
{
    sendPacket(PACKET_SPECTATE_REQUEST, targetId);
}

void SocketClient::requestBattleStatusList()
{
    sendPacket(PACKET_BATTLE_STATUS_LIST, "");
    cocos2d::log("[SocketClient] 请求战斗状态列表");
}

// ==================== 回调设置 ====================
void SocketClient::setOnConnected(std::function<void(bool)> callback)
{
    _onConnected = callback;
}
void SocketClient::setOnLoginResult(std::function<void(bool, const std::string&)> callback)
{
    _onLoginResult = callback;
}
void SocketClient::setOnMatchFound(std::function<void(const MatchInfo&)> callback)
{
    _onMatchFound = callback;
}
void SocketClient::setOnMatchCancelled(std::function<void()> callback)
{
    _onMatchCancelled = callback;
}
void SocketClient::setOnAttackStart(std::function<void(const std::string&)> callback)
{
    _onAttackStart = callback;
}
void SocketClient::setOnAttackResult(std::function<void(const AttackResult&)> callback)
{
    _onAttackResult = callback;
}
void SocketClient::setOnClanCreated(std::function<void(bool, const std::string&)> callback)
{
    _onClanCreated = callback;
}
void SocketClient::setOnClanJoined(std::function<void(bool)> callback)
{
    _onClanJoined = callback;
}
void SocketClient::setOnClanLeft(std::function<void(bool)> callback)
{
    _onClanLeft = callback;
}
void SocketClient::setOnClanList(std::function<void(const std::vector<ClanInfoClient>&)> callback)
{
    _onClanList = callback;
}
void SocketClient::setOnClanMembers(std::function<void(const std::string&)> callback)
{
    _onClanMembers = callback;
}
void SocketClient::setOnClanWarMatch(
    std::function<void(const std::string&, const std::string&, const std::string&)> callback)
{
    _onClanWarMatch = callback;
}
void SocketClient::setOnClanWarStatus(std::function<void(const std::string&, int, int)> callback)
{
    _onClanWarStatus = callback;
}

// 🆕 PVP回调设置
void SocketClient::setOnPvpStart(std::function<void(const std::string&, const std::string&, const std::string&)> callback)
{
    _onPvpStart = callback;
}

void SocketClient::setOnPvpAction(std::function<void(int, float, float)> callback)
{
    _onPvpAction = callback;
}

void SocketClient::setOnPvpEnd(std::function<void(const std::string&)> callback)
{
    _onPvpEnd = callback;
}

void SocketClient::setOnSpectateJoin(std::function<void(bool, const std::string&, const std::string&, const std::string&, const std::vector<std::string>&)> callback)
{
    _onSpectateJoin = callback;
}

void SocketClient::setOnBattleStatusList(std::function<void(const std::string&)> callback)
{
    _onBattleStatusList = callback;
}

// 🆕 部落战争增强回调设置
void SocketClient::setOnClanWarMemberList(std::function<void(const std::string&)> callback)
{
    _onClanWarMemberList = callback;
}

void SocketClient::setOnClanWarAttackStart(std::function<void(const std::string&, const std::string&, const std::string&)> callback)
{
    _onClanWarAttackStart = callback;
}

void SocketClient::setOnClanWarSpectate(std::function<void(bool, const std::string&, const std::string&, const std::string&)> callback)
{
    _onClanWarSpectate = callback;
}

void SocketClient::setOnClanWarStateUpdate(std::function<void(const std::string&)> callback)
{
    _onClanWarStateUpdate = callback;
}

void SocketClient::setOnMapReceived(std::function<void(const std::string&)> callback)
{
    _onMapReceived = callback;
}
// 🆕 设置用户列表回调
void SocketClient::setOnUserListReceived(std::function<void(const std::string&)> callback)
{
    _onUserListReceived = callback;
}
void SocketClient::setOnDisconnected(std::function<void()> callback)
{
    _onDisconnected = callback;
}