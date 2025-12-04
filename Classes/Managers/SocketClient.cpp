#include "SocketClient.h"

#include <algorithm>
#include <sstream>

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

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

#ifdef _WIN32
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
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
    if (!_connected)
    {
        return;
    }

    _running = false;
    _connected = false;

    if (_socket != INVALID_SOCKET)
    {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
    }

    if (_recvThread.joinable())
    {
        _recvThread.join();
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
                    info.opponentTrophies = std::stoi(trophiesStr);
                }
                _onMatchFound(info);
            }
            break;

        case PACKET_ATTACK_START:
            if (_onAttackStart)
            {
                _onAttackStart(data);  // 对手的地图数据
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
                bool success = data.substr(0, 2) == "OK";
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
                // 简单解析 JSON 数组
                std::vector<ClanInfoClient> clans;
                // TODO: 使用 rapidjson 解析
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
                int stars1 = stars1Str.empty() ? 0 : std::stoi(stars1Str);
                int stars2 = stars2Str.empty() ? 0 : std::stoi(stars2Str);
                _onClanWarStatus(warId, stars1, stars2);
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

void SocketClient::setOnMapReceived(std::function<void(const std::string&)> callback)
{
    _onMapReceived = callback;
}

void SocketClient::setOnDisconnected(std::function<void()> callback)
{
    _onDisconnected = callback;
}