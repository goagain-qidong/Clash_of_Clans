/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SocketClient.h
 * File Function: 负责客户端与服务器的网络通信
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once
#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "cocos2d.h"
// ==================== 数据包类型（与服务器一致） ====================
enum PacketType
{
    // 基础功能
    CYCKET_LOGIN = 1,
    PACKET_UPLOAD_MAP = 2,
    PACKET_QUERY_MAP = 3,
    PACKET_ATTACK_DATA = 4,
    // 🆕 用户列表
    REQ_USER_LIST = 5,      // 请求可攻击的用户列表
    RESP_USER_LIST = 6,     // 返回用户列表
    // 玩家对战
    PACKET_FIND_MATCH = 10,
    PACKET_MATCH_FOUND = 11,
    PACKET_MATCH_CANCEL = 12,
    PACKET_ATTACK_START = 13,
    PACKET_ATTACK_RESULT = 14,
    PACKET_BATTLE_REPLAY = 15,
    // 部落系统
    PACKET_CREATE_CLAN = 20,
    PACKET_JOIN_CLAN = 21,
    PACKET_LEAVE_CLAN = 22,
    PACKET_CLAN_LIST = 23,
    PACKET_CLAN_MEMBERS = 24,
    PACKET_CLAN_INFO = 25,
    // 部落战争
    PACKET_CLAN_WAR_SEARCH = 30,
    PACKET_CLAN_WAR_MATCH = 31,
    PACKET_CLAN_WAR_ATTACK = 32,
    PACKET_CLAN_WAR_RESULT = 33,
    PACKET_CLAN_WAR_STATUS = 34,  // 部落战状态

    // 🆕 实时PVP与观战
    PACKET_PVP_REQUEST = 40,      // 请求挑战/攻击
    PACKET_PVP_START = 41,        // PVP开始通知
    PACKET_PVP_ACTION = 42,       // PVP操作（下兵）
    PACKET_PVP_END = 43,          // PVP结束
    PACKET_SPECTATE_REQUEST = 44, // 请求观战
    PACKET_SPECTATE_JOIN = 45,    // 加入观战通知
    
    // 🆕 部落战争增强
    PACKET_CLAN_WAR_MEMBER_LIST = 50,   // 获取部落战成员列表
    PACKET_CLAN_WAR_ATTACK_START = 51,  // 开始攻击部落战目标
    PACKET_CLAN_WAR_ATTACK_END = 52,    // 部落战攻击结束
    PACKET_CLAN_WAR_SPECTATE = 53,      // 观战部落战
    PACKET_CLAN_WAR_STATE_UPDATE = 54, // 部落战状态更新

    // 🆕 战斗状态广播
    PACKET_BATTLE_STATUS_LIST = 60,     // 请求/返回战斗状态列表
    PACKET_BATTLE_STATUS_UPDATE = 61    // 战斗状态更新通知
};
// ==================== 数据包头 ====================
struct PacketHeader
{
    uint32_t type;
    uint32_t length;
};
// ==================== 攻击结果 ====================
struct AttackResult
{
    std::string attackerId;
    std::string defenderId;
    int starsEarned = 0;
    int goldLooted = 0;
    int elixirLooted = 0;
    int trophyChange = 0;
    std::string replayData;
    std::string serialize() const;
    static AttackResult deserialize(const std::string& data);
};
// ==================== 匹配信息 ====================
struct MatchInfo
{
    std::string opponentId;
    int opponentTrophies = 0;
};
// ==================== 部落信息 ====================
struct ClanInfoClient
{
    std::string clanId;
    std::string clanName;
    int memberCount = 0;
    int clanTrophies = 0;
    int requiredTrophies = 0;
    bool isOpen = true;
};
// ==================== 接收到的消息 ====================
struct ReceivedPacket
{
    uint32_t type;
    std::string data;
};
// ==================== SocketClient 类 ====================
class SocketClient
{
public:
    static SocketClient& getInstance();
    // 连接管理
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;
    // 基础功能
    void login(const std::string& playerId, const std::string& playerName, int trophies);
    void uploadMap(const std::string& mapData);
    void queryMap(const std::string& targetId);
    // 🆕 请求用户列表
    void requestUserList();
    // 玩家对战
    void findMatch();
    void cancelMatch();
    void startAttack(const std::string& targetId);
    void submitAttackResult(const AttackResult& result);
    // 部落系统
    void createClan(const std::string& clanName);
    void joinClan(const std::string& clanId);
    void leaveClan();
    void getClanList();
    void getClanMembers(const std::string& clanId);
    // 部落战争
    void searchClanWar();
    void attackInClanWar(const std::string& warId, const std::string& targetMemberId);
    void submitClanWarResult(const std::string& warId, const AttackResult& result);
    
    // 🆕 部落战争增强
    void requestClanWarMemberList(const std::string& warId);
    void startClanWarAttack(const std::string& warId, const std::string& targetId);
    void endClanWarAttack(const std::string& warId, int stars, float destructionRate);
    void spectateClanWar(const std::string& warId, const std::string& targetId);

    // 🆕 PVP系统
    void requestPvp(const std::string& targetId);
    void sendPvpAction(int unitType, float x, float y);
    void endPvp();
    void requestSpectate(const std::string& targetId);

    // 🆕 请求战斗状态列表
    void requestBattleStatusList();

    // ==================== 回调设置 ====================
    void setOnConnected(std::function<void(bool)> callback);
    void setOnLoginResult(std::function<void(bool, const std::string&)> callback);
    void setOnMatchFound(std::function<void(const MatchInfo&)> callback);
    void setOnMatchCancelled(std::function<void()> callback);
    void setOnAttackStart(std::function<void(const std::string&)> callback); // 收到对手地图
    void setOnAttackResult(std::function<void(const AttackResult&)> callback);
    // 🆕 用户列表回调
    void setOnUserListReceived(std::function<void(const std::string&)> callback);
    void setOnClanCreated(std::function<void(bool, const std::string&)> callback);
    void setOnClanJoined(std::function<void(bool)> callback);
    void setOnClanLeft(std::function<void(bool)> callback);
    void setOnClanList(std::function<void(const std::vector<ClanInfoClient>&)> callback);
    void setOnClanMembers(std::function<void(const std::string&)> callback);
    void setOnClanWarMatch(std::function<void(const std::string&, const std::string&, const std::string&)> callback);
    void setOnClanWarStatus(std::function<void(const std::string&, int, int)> callback);
    
    // 🆕 PVP回调
    // role: "ATTACK", "DEFEND", "FAIL"
    void setOnPvpStart(std::function<void(const std::string& role, const std::string& opponentId, const std::string& mapData)> callback);
    void setOnPvpAction(std::function<void(int unitType, float x, float y)> callback);
    void setOnPvpEnd(std::function<void(const std::string& result)> callback);
    void setOnSpectateJoin(std::function<void(bool success, const std::string& attackerId, const std::string& defenderId, const std::string& mapData)> callback);

    // 🆕 战斗状态回调
    void setOnBattleStatusList(std::function<void(const std::string&)> callback);

    // 🆕 部落战争增强回调
    void setOnClanWarMemberList(std::function<void(const std::string&)> callback);
    void setOnClanWarAttackStart(std::function<void(const std::string&, const std::string&, const std::string&)> callback);
    void setOnClanWarSpectate(std::function<void(bool, const std::string&, const std::string&, const std::string&)> callback);
    void setOnClanWarStateUpdate(std::function<void(const std::string&)> callback);

    void setOnMapReceived(std::function<void(const std::string&)> callback);
    void setOnDisconnected(std::function<void()> callback);
    // 在主线程中处理回调（需要在 update 中调用）
    void processCallbacks();

private:
    SocketClient();
    ~SocketClient();
    SocketClient(const SocketClient&) = delete;
    SocketClient& operator=(const SocketClient&) = delete;
    SOCKET _socket = INVALID_SOCKET;
    std::atomic<bool> _connected{false};
    std::atomic<bool> _running{false};
    std::thread _recvThread;
    std::mutex _sendMutex;
    std::mutex _callbackMutex;
    std::queue<ReceivedPacket> _pendingPackets;
    // 回调函数
    // 在 private 部分的回调函数存储中添加：
    std::function<void(const std::string&)> _onBattleStatusList;
    std::function<void(bool)> _onConnected;
    std::function<void(bool, const std::string&)> _onLoginResult;
    std::function<void(const MatchInfo&)> _onMatchFound;
    std::function<void()> _onMatchCancelled;
    std::function<void(const std::string&)> _onAttackStart;
    std::function<void(const AttackResult&)> _onAttackResult;
    std::function<void(bool, const std::string&)> _onClanCreated;
    std::function<void(bool)> _onClanJoined;
    std::function<void(bool)> _onClanLeft;
    std::function<void(const std::vector<ClanInfoClient>&)> _onClanList;
    std::function<void(const std::string&)> _onClanMembers;
    std::function<void(const std::string&, const std::string&, const std::string&)> _onClanWarMatch;
    std::function<void(const std::string&, int, int)> _onClanWarStatus;
    
    // 🆕 PVP回调存储
    std::function<void(const std::string&, const std::string&, const std::string&)> _onPvpStart;
    std::function<void(int, float, float)> _onPvpAction;
    std::function<void(const std::string&)> _onPvpEnd;
    std::function<void(bool, const std::string&, const std::string&, const std::string&)> _onSpectateJoin;
    
    // 🆕 部落战争增强回调存储
    std::function<void(const std::string&)> _onClanWarMemberList;
    std::function<void(const std::string&, const std::string&, const std::string&)> _onClanWarAttackStart;
    std::function<void(bool, const std::string&, const std::string&, const std::string&)> _onClanWarSpectate;
    std::function<void(const std::string&)> _onClanWarStateUpdate;

    std::function<void(const std::string&)> _onMapReceived;
    std::function<void()> _onDisconnected;
    // 🆕 用户列表回调
    std::function<void(const std::string&)> _onUserListReceived;
    // 网络函数
    bool sendPacket(uint32_t type, const std::string& data);
    bool recvPacket(uint32_t& outType, std::string& outData);
    bool recvFixedAmount(char* buffer, int totalBytes);
    void recvThreadFunc();
    void handlePacket(uint32_t type, const std::string& data);
#ifdef _WIN32
    bool _wsaInitialized = false;
#endif
};
#endif // _SOCKET_CLIENT_H_