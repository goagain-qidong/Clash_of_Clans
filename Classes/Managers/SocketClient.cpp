/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SocketClient.cpp
 * File Function: 客户端网络通信管理器实现
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#include "SocketClient.h"

#include <algorithm>
#include <sstream>

#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"

// ============================================================================
// 协议格式常量
// ============================================================================
namespace {
    constexpr char kFieldSeparator = '|';
    constexpr char kActionSeparator = ',';
    constexpr const char* kHistoryMarker = "[[[HISTORY]]]";
    constexpr const char* kActionDelimiter = "[[[ACTION]]]";
}

// ============================================================================
// AttackResult 序列化
// ============================================================================

std::string AttackResult::Serialize() const {
    std::ostringstream oss;
    oss << attacker_id << kFieldSeparator
        << defender_id << kFieldSeparator
        << stars_earned << kFieldSeparator
        << gold_looted << kFieldSeparator
        << elixir_looted << kFieldSeparator
        << trophy_change << kFieldSeparator
        << replay_data;
    return oss.str();
}

AttackResult AttackResult::Deserialize(const std::string& data) {
    AttackResult result;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, result.attacker_id, kFieldSeparator);
    std::getline(iss, result.defender_id, kFieldSeparator);
    
    if (std::getline(iss, token, kFieldSeparator) && !token.empty()) {
        result.stars_earned = std::stoi(token);
    }
    if (std::getline(iss, token, kFieldSeparator) && !token.empty()) {
        result.gold_looted = std::stoi(token);
    }
    if (std::getline(iss, token, kFieldSeparator) && !token.empty()) {
        result.elixir_looted = std::stoi(token);
    }
    if (std::getline(iss, token, kFieldSeparator) && !token.empty()) {
        result.trophy_change = std::stoi(token);
    }
    std::getline(iss, result.replay_data);
    
    return result;
}

// ============================================================================
// 单例实现
// ============================================================================

SocketClient& SocketClient::getInstance() {
    static SocketClient instance;
    return instance;
}

SocketClient::SocketClient() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0) {
        wsa_initialized_ = true;
    }
#endif
}

SocketClient::~SocketClient() {
    disconnect();
#ifdef _WIN32
    if (wsa_initialized_) {
        WSACleanup();
    }
#endif
}

// ============================================================================
// 连接管理
// ============================================================================

bool SocketClient::connect(const std::string& host, int port) {
    if (connected_) {
        return true;
    }

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET) {
        if (on_connected_) {
            on_connected_(false);
        }
        return false;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
#ifdef _WIN32
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());
#else
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);
#endif

    if (::connect(socket_, 
                  reinterpret_cast<sockaddr*>(&server_addr), 
                  sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        if (on_connected_) {
            on_connected_(false);
        }
        return false;
    }

    connected_ = true;
    running_ = true;
    recv_thread_ = std::thread(&SocketClient::recvThreadFunc, this);

    cocos2d::log("[SocketClient] 已连接到 %s:%d", host.c_str(), port);
    
    if (on_connected_) {
        on_connected_(true);
    }
    return true;
}

void SocketClient::disconnect() {
    running_ = false;
    connected_ = false;

    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }

    if (recv_thread_.joinable()) {
        if (std::this_thread::get_id() != recv_thread_.get_id()) {
            recv_thread_.join();
        } else {
            recv_thread_.detach();
        }
    }

    cocos2d::log("[SocketClient] 已断开连接");
    
    if (on_disconnected_) {
        on_disconnected_();
    }
}

bool SocketClient::isConnected() const {
    return connected_;
}

// ============================================================================
// 底层网络操作
// ============================================================================

bool SocketClient::recvFixedAmount(char* buffer, int total_bytes) {
    int received = 0;
    while (received < total_bytes && running_) {
        int ret = recv(socket_, buffer + received, total_bytes - received, 0);
        if (ret <= 0) {
            return false;
        }
        received += ret;
    }
    return received == total_bytes;
}

bool SocketClient::sendPacket(uint32_t type, const std::string& data) {
    if (!connected_ || socket_ == INVALID_SOCKET) {
        return false;
    }

    std::lock_guard<std::mutex> lock(send_mutex_);
    
    PacketHeader header;
    header.type = type;
    header.length = static_cast<uint32_t>(data.size());

    int header_sent = send(socket_, 
                           reinterpret_cast<char*>(&header), 
                           sizeof(PacketHeader), 0);
    if (header_sent != sizeof(PacketHeader)) {
        return false;
    }

    if (header.length > 0) {
        int body_sent = send(socket_, 
                             data.c_str(), 
                             static_cast<int>(header.length), 0);
        if (body_sent != static_cast<int>(header.length)) {
            return false;
        }
    }

    return true;
}

bool SocketClient::recvPacket(uint32_t& out_type, std::string& out_data) {
    PacketHeader header;
    if (!recvFixedAmount(reinterpret_cast<char*>(&header), sizeof(PacketHeader))) {
        return false;
    }

    out_type = header.type;
    out_data.clear();

    if (header.length > 0) {
        std::vector<char> buffer(header.length);
        if (!recvFixedAmount(buffer.data(), static_cast<int>(header.length))) {
            return false;
        }
        out_data.assign(buffer.begin(), buffer.end());
    }

    return true;
}

// ============================================================================
// 接收线程
// ============================================================================

void SocketClient::recvThreadFunc() {
    while (running_) {
        uint32_t msg_type;
        std::string msg_data;
        
        if (recvPacket(msg_type, msg_data)) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            pending_packets_.push({msg_type, msg_data});
        } else {
            if (running_) {
                connected_ = false;
                running_ = false;
                std::lock_guard<std::mutex> lock(callback_mutex_);
                pending_packets_.push({0, "DISCONNECTED"});
            }
            break;
        }
    }
}

// ============================================================================
// 主线程回调处理
// ============================================================================

void SocketClient::processCallbacks() {
    std::queue<ReceivedPacket> packets;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        std::swap(packets, pending_packets_);
    }

    while (!packets.empty()) {
        auto& packet = packets.front();
        
        if (packet.type == 0 && packet.data == "DISCONNECTED") {
            if (on_disconnected_) {
                on_disconnected_();
            }
        } else {
            handlePacket(packet.type, packet.data);
        }
        
        packets.pop();
    }
}

// ============================================================================
// 消息处理
// ============================================================================

void SocketClient::handlePacket(uint32_t type, const std::string& data) {
    switch (type) {
        case PACKET_LOGIN:
            if (on_login_result_) {
                bool success = (data == "Login Success");
                on_login_result_(success, data);
            }
            break;

        case PACKET_QUERY_MAP:
            if (on_map_received_) {
                on_map_received_(data);
            }
            break;

        case PACKET_USER_LIST_RESP:
            if (on_user_list_received_) {
                on_user_list_received_(data);
            }
            break;

        case PACKET_MATCH_FOUND:
            if (on_match_found_) {
                std::istringstream iss(data);
                std::string opponent_id, trophies_str;
                std::getline(iss, opponent_id, kFieldSeparator);
                std::getline(iss, trophies_str, kFieldSeparator);
                
                MatchInfo info;
                info.opponent_id = opponent_id;
                if (!trophies_str.empty()) {
                    try {
                        info.opponent_trophies = std::stoi(trophies_str);
                    } catch (...) {
                        info.opponent_trophies = 0;
                    }
                }
                on_match_found_(info);
            }
            break;

        case PACKET_ATTACK_START:
            if (on_attack_start_) {
                on_attack_start_(data);
            }
            break;

        case PACKET_ATTACK_RESULT:
            if (on_attack_result_) {
                on_attack_result_(AttackResult::Deserialize(data));
            }
            break;

        case PACKET_CLAN_CREATE:
            if (on_clan_created_) {
                bool success = (data.length() >= 2) && (data.substr(0, 2) == "OK");
                std::string clan_id;
                if (success && data.length() > 3) {
                    clan_id = data.substr(3);
                }
                on_clan_created_(success, clan_id);
            }
            break;

        case PACKET_CLAN_JOIN:
            if (on_clan_joined_) {
                on_clan_joined_(data == "OK");
            }
            break;

        case PACKET_CLAN_LEAVE:
            if (on_clan_left_) {
                on_clan_left_(data == "OK");
            }
            break;

        case PACKET_CLAN_LIST:
            handleClanList(data);
            break;

        case PACKET_CLAN_MEMBERS:
            if (on_clan_members_) {
                on_clan_members_(data);
            }
            break;

        case PACKET_CHAT_MESSAGE:
            if (on_chat_message_) {
                std::istringstream iss(data);
                std::string sender, message;
                std::getline(iss, sender, kFieldSeparator);
                std::getline(iss, message);
                on_chat_message_(sender, message);
            }
            break;

        case PACKET_WAR_MATCH:
            if (on_clan_war_match_) {
                std::istringstream iss(data);
                std::string war_id, clan1_id, clan2_id;
                std::getline(iss, war_id, kFieldSeparator);
                std::getline(iss, clan1_id, kFieldSeparator);
                std::getline(iss, clan2_id, kFieldSeparator);
                on_clan_war_match_(war_id, clan1_id, clan2_id);
            }
            break;

        case PACKET_WAR_STATUS:
            if (on_clan_war_status_) {
                std::istringstream iss(data);
                std::string war_id, stars1_str, stars2_str;
                std::getline(iss, war_id, kFieldSeparator);
                std::getline(iss, stars1_str, kFieldSeparator);
                std::getline(iss, stars2_str, kFieldSeparator);
                int stars1 = 0, stars2 = 0;
                try {
                    if (!stars1_str.empty()) stars1 = std::stoi(stars1_str);
                    if (!stars2_str.empty()) stars2 = std::stoi(stars2_str);
                } catch (...) {}
                on_clan_war_status_(war_id, stars1, stars2);
            }
            break;

        case PACKET_PVP_START:
            handlePvpStart(data);
            break;

        case PACKET_PVP_ACTION:
            handlePvpAction(data);
            break;

        case PACKET_PVP_END:
            cocos2d::log("[SocketClient] PVP_END 收到: %s", data.c_str());
            if (on_pvp_end_) {
                on_pvp_end_(data);
            }
            break;

        case PACKET_SPECTATE_JOIN:
            handleSpectateJoin(data);
            break;

        case PACKET_WAR_MEMBER_LIST:
            if (on_clan_war_member_list_) {
                on_clan_war_member_list_(data);
            }
            break;

        case PACKET_WAR_ATTACK_START:
            if (on_clan_war_attack_start_) {
                if (data.length() >= 4 && data.substr(0, 4) == "FAIL") {
                    cocos2d::log("[SocketClient] 部落战攻击失败: %s", data.c_str());
                    on_clan_war_attack_start_("FAIL", "", "");
                } else {
                    std::istringstream iss(data);
                    std::string type, target_id, map_data;
                    std::getline(iss, type, kFieldSeparator);
                    std::getline(iss, target_id, kFieldSeparator);
                    std::getline(iss, map_data);
                    on_clan_war_attack_start_(type, target_id, map_data);
                }
            }
            break;

        case PACKET_WAR_SPECTATE:
            if (on_clan_war_spectate_) {
                if (data.empty() || data[0] == '0') {
                    cocos2d::log("[SocketClient] 部落战观战失败");
                    on_clan_war_spectate_(false, "", "", "");
                } else {
                    std::istringstream iss(data);
                    std::string success_flag, attacker_id, defender_id, map_data;
                    std::getline(iss, success_flag, kFieldSeparator);
                    std::getline(iss, attacker_id, kFieldSeparator);
                    std::getline(iss, defender_id, kFieldSeparator);
                    std::getline(iss, map_data);
                    on_clan_war_spectate_(true, attacker_id, defender_id, map_data);
                }
            }
            break;

        case PACKET_WAR_STATE_UPDATE:
            if (on_clan_war_state_update_) {
                on_clan_war_state_update_(data);
            }
            break;

        case PACKET_BATTLE_STATUS_LIST:
            if (on_battle_status_list_) {
                on_battle_status_list_(data);
            }
            break;

        default:
            cocos2d::log("[SocketClient] 未知消息类型: %d", type);
            break;
    }
}

// ============================================================================
// 消息解析辅助方法
// ============================================================================

void SocketClient::handlePvpStart(const std::string& data) {
    if (!on_pvp_start_) {
        return;
    }

    // 格式: ROLE|OpponentID|MapData
    std::istringstream iss(data);
    std::string role, opponent_id, map_data;
    std::getline(iss, role, kFieldSeparator);
    std::getline(iss, opponent_id, kFieldSeparator);
    std::getline(iss, map_data);

    cocos2d::log("[SocketClient] PVP_START: role=%s, opponent=%s, mapLen=%zu",
                 role.c_str(), opponent_id.c_str(), map_data.size());

    on_pvp_start_(role, opponent_id, map_data);
}

void SocketClient::handlePvpAction(const std::string& data) {
    if (!on_pvp_action_) {
        return;
    }

    // 格式: unitType,x,y（逗号分隔）
    try {
        std::istringstream iss(data);
        std::string token;
        
        std::getline(iss, token, kActionSeparator);
        if (token.empty()) {
            cocos2d::log("[SocketClient] PVP_ACTION: 空的 unitType");
            return;
        }
        int unit_type = std::stoi(token);

        std::getline(iss, token, kActionSeparator);
        if (token.empty()) {
            cocos2d::log("[SocketClient] PVP_ACTION: 空的 x");
            return;
        }
        float x = std::stof(token);

        std::getline(iss, token, kActionSeparator);
        if (token.empty()) {
            cocos2d::log("[SocketClient] PVP_ACTION: 空的 y");
            return;
        }
        float y = std::stof(token);

        cocos2d::log("[SocketClient] PVP_ACTION: type=%d, pos=(%.1f,%.1f)", 
                     unit_type, x, y);
        on_pvp_action_(unit_type, x, y);
        
    } catch (const std::exception& e) {
        cocos2d::log("[SocketClient] PVP_ACTION 解析错误: %s (data=%s)", 
                     e.what(), data.c_str());
    }
}

void SocketClient::handleSpectateJoin(const std::string& data) {
    if (!on_spectate_join_) {
        return;
    }

    SpectateInfo info;

    // 失败格式: "0|||0|"
    if (data.empty() || data[0] == '0') {
        cocos2d::log("[SocketClient] SPECTATE_JOIN 失败");
        on_spectate_join_(info);
        return;
    }

    // 成功格式: "1|attackerId|defenderId|elapsedMs|mapData[[[HISTORY]]]action1[[[ACTION]]]action2..."
    std::string base_data = data;
    
    // 查找并解析历史记录
    size_t history_pos = data.find(kHistoryMarker);
    if (history_pos != std::string::npos) {
        base_data = data.substr(0, history_pos);
        std::string history_str = data.substr(history_pos + strlen(kHistoryMarker));

        // 解析历史操作
        size_t pos = 0;
        while (!history_str.empty()) {
            pos = history_str.find(kActionDelimiter);
            if (pos != std::string::npos) {
                std::string action = history_str.substr(0, pos);
                if (!action.empty()) {
                    info.action_history.push_back(action);
                }
                history_str.erase(0, pos + strlen(kActionDelimiter));
            } else {
                if (!history_str.empty()) {
                    info.action_history.push_back(history_str);
                }
                break;
            }
        }
    }

    // 解析基本信息
    std::istringstream iss(base_data);
    std::string success_flag, elapsed_str;
    std::getline(iss, success_flag, kFieldSeparator);
    std::getline(iss, info.attacker_id, kFieldSeparator);
    std::getline(iss, info.defender_id, kFieldSeparator);
    std::getline(iss, elapsed_str, kFieldSeparator);
    std::getline(iss, info.map_data);

    info.success = true;
    
    if (!elapsed_str.empty()) {
        try {
            info.elapsed_ms = std::stoll(elapsed_str);
        } catch (...) {
            info.elapsed_ms = 0;
        }
    }

    cocos2d::log("[SocketClient] SPECTATE_JOIN: attacker=%s, defender=%s, "
                 "elapsed=%lldms, history=%zu",
                 info.attacker_id.c_str(), info.defender_id.c_str(),
                 static_cast<long long>(info.elapsed_ms), 
                 info.action_history.size());

    on_spectate_join_(info);
}

void SocketClient::handleClanList(const std::string& data) {
    if (!on_clan_list_) {
        return;
    }

    std::vector<ClanInfoClient> clans;
    rapidjson::Document doc;
    doc.Parse(data.c_str());

    if (!doc.HasParseError() && doc.IsArray()) {
        for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
            const auto& item = doc[i];
            ClanInfoClient clan;
            
            if (item.HasMember("id") && item["id"].IsString()) {
                clan.clan_id = item["id"].GetString();
            }
            if (item.HasMember("name") && item["name"].IsString()) {
                clan.clan_name = item["name"].GetString();
            }
            if (item.HasMember("members") && item["members"].IsInt()) {
                clan.member_count = item["members"].GetInt();
            }
            if (item.HasMember("trophies") && item["trophies"].IsInt()) {
                clan.clan_trophies = item["trophies"].GetInt();
            }
            if (item.HasMember("required") && item["required"].IsInt()) {
                clan.required_trophies = item["required"].GetInt();
            }
            if (item.HasMember("open") && item["open"].IsBool()) {
                clan.is_open = item["open"].GetBool();
            }
            
            clans.push_back(clan);
        }
    }

    on_clan_list_(clans);
}

// ============================================================================
// 基础功能
// ============================================================================

void SocketClient::login(const std::string& player_id, 
                         const std::string& player_name, 
                         int trophies,
                         const std::string& clan_id) {
    std::ostringstream oss;
    oss << player_id << kFieldSeparator 
        << player_name << kFieldSeparator 
        << trophies << kFieldSeparator
        << clan_id;
    sendPacket(PACKET_LOGIN, oss.str());
}

void SocketClient::uploadMap(const std::string& map_data) {
    sendPacket(PACKET_UPLOAD_MAP, map_data);
}

void SocketClient::queryMap(const std::string& target_id) {
    sendPacket(PACKET_QUERY_MAP, target_id);
}

void SocketClient::requestUserList() {
    sendPacket(PACKET_USER_LIST_REQ, "");
    cocos2d::log("[SocketClient] 请求用户列表");
}

// ============================================================================
// 玩家对战
// ============================================================================

void SocketClient::findMatch() {
    sendPacket(PACKET_MATCH_FIND, "");
}

void SocketClient::cancelMatch() {
    sendPacket(PACKET_MATCH_CANCEL, "");
}

void SocketClient::startAttack(const std::string& target_id) {
    sendPacket(PACKET_ATTACK_START, target_id);
}

void SocketClient::submitAttackResult(const AttackResult& result) {
    sendPacket(PACKET_ATTACK_RESULT, result.Serialize());
}

// ============================================================================
// 部落系统
// ============================================================================

void SocketClient::createClan(const std::string& clan_name) {
    sendPacket(PACKET_CLAN_CREATE, clan_name);
}

void SocketClient::joinClan(const std::string& clan_id) {
    sendPacket(PACKET_CLAN_JOIN, clan_id);
}

void SocketClient::leaveClan() {
    sendPacket(PACKET_CLAN_LEAVE, "");
}

void SocketClient::getClanList() {
    sendPacket(PACKET_CLAN_LIST, "");
}

void SocketClient::getClanMembers(const std::string& clan_id) {
    sendPacket(PACKET_CLAN_MEMBERS, clan_id);
}

void SocketClient::sendChatMessage(const std::string& message) {
    sendPacket(PACKET_CLAN_CHAT, message);
}

// ============================================================================
// 部落战争
// ============================================================================

void SocketClient::searchClanWar() {
    sendPacket(PACKET_WAR_SEARCH, "");
}

void SocketClient::attackInClanWar(const std::string& war_id, 
                                   const std::string& target_member_id) {
    sendPacket(PACKET_WAR_ATTACK, war_id + kFieldSeparator + target_member_id);
}

void SocketClient::submitClanWarResult(const std::string& war_id, 
                                       const AttackResult& result) {
    sendPacket(PACKET_WAR_RESULT, war_id + kFieldSeparator + result.Serialize());
}

void SocketClient::requestClanWarMemberList(const std::string& war_id) {
    sendPacket(PACKET_WAR_MEMBER_LIST, war_id);
    cocos2d::log("[SocketClient] 请求部落战成员列表: %s", war_id.c_str());
}

void SocketClient::startClanWarAttack(const std::string& war_id, 
                                      const std::string& target_id) {
    std::string data = war_id + kFieldSeparator + target_id;
    sendPacket(PACKET_WAR_ATTACK_START, data);
    cocos2d::log("[SocketClient] 发起部落战攻击: warId=%s, target=%s", 
                 war_id.c_str(), target_id.c_str());
}

void SocketClient::endClanWarAttack(const std::string& war_id, 
                                    int stars, 
                                    float destruction_rate) {
    std::ostringstream oss;
    oss << war_id << kFieldSeparator
        << "unknown" << kFieldSeparator  // attackerId（服务器会填充）
        << "unknown" << kFieldSeparator  // attackerName
        << stars << kFieldSeparator
        << destruction_rate;
    
    sendPacket(PACKET_WAR_ATTACK_END, oss.str());
    cocos2d::log("[SocketClient] 结束部落战攻击: warId=%s, stars=%d, destruction=%.2f",
                 war_id.c_str(), stars, destruction_rate);
}

void SocketClient::spectateClanWar(const std::string& war_id, 
                                   const std::string& target_id) {
    std::string data = war_id + kFieldSeparator + target_id;
    sendPacket(PACKET_WAR_SPECTATE, data);
    cocos2d::log("[SocketClient] 请求观战部落战: warId=%s, target=%s", 
                 war_id.c_str(), target_id.c_str());
}

// ============================================================================
// PVP 系统
// ============================================================================

void SocketClient::requestPvp(const std::string& target_id) {
    sendPacket(PACKET_PVP_REQUEST, target_id);
    cocos2d::log("[SocketClient] 请求 PVP: target=%s", target_id.c_str());
}

void SocketClient::sendPvpAction(int unit_type, float x, float y) {
    // 格式: unitType,x,y
    std::ostringstream oss;
    oss << unit_type << kActionSeparator << x << kActionSeparator << y;
    sendPacket(PACKET_PVP_ACTION, oss.str());
    cocos2d::log("[SocketClient] 发送 PVP 操作: type=%d, pos=(%.1f,%.1f)", 
                 unit_type, x, y);
}

void SocketClient::endPvp() {
    sendPacket(PACKET_PVP_END, "");
    cocos2d::log("[SocketClient] 发送 PVP 结束");
}

void SocketClient::requestSpectate(const std::string& target_id) {
    sendPacket(PACKET_SPECTATE_REQUEST, target_id);
    cocos2d::log("[SocketClient] 请求观战: target=%s", target_id.c_str());
}

void SocketClient::requestBattleStatusList() {
    sendPacket(PACKET_BATTLE_STATUS_LIST, "");
    cocos2d::log("[SocketClient] 请求战斗状态列表");
}

// ============================================================================
// 回调设置
// ============================================================================

void SocketClient::setOnConnected(SocketCallback::OnConnected callback) {
    on_connected_ = callback;
}

void SocketClient::setOnDisconnected(SocketCallback::OnDisconnected callback) {
    on_disconnected_ = callback;
}

void SocketClient::setOnLoginResult(SocketCallback::OnLoginResult callback) {
    on_login_result_ = callback;
}

void SocketClient::setOnMatchFound(SocketCallback::OnMatchFound callback) {
    on_match_found_ = callback;
}

void SocketClient::setOnMatchCancelled(SocketCallback::OnMatchCancelled callback) {
    on_match_cancelled_ = callback;
}

void SocketClient::setOnAttackStart(SocketCallback::OnAttackStart callback) {
    on_attack_start_ = callback;
}

void SocketClient::setOnAttackResult(SocketCallback::OnAttackResult callback) {
    on_attack_result_ = callback;
}

void SocketClient::setOnUserListReceived(SocketCallback::OnUserListReceived callback) {
    on_user_list_received_ = callback;
}

void SocketClient::setOnMapReceived(SocketCallback::OnMapReceived callback) {
    on_map_received_ = callback;
}

void SocketClient::setOnBattleStatusList(SocketCallback::OnBattleStatusList callback) {
    on_battle_status_list_ = callback;
}

void SocketClient::setOnClanCreated(SocketCallback::OnClanCreated callback) {
    on_clan_created_ = callback;
}

void SocketClient::setOnClanJoined(SocketCallback::OnClanJoined callback) {
    on_clan_joined_ = callback;
}

void SocketClient::setOnClanLeft(SocketCallback::OnClanLeft callback) {
    on_clan_left_ = callback;
}

void SocketClient::setOnClanList(SocketCallback::OnClanList callback) {
    on_clan_list_ = callback;
}

void SocketClient::setOnClanMembers(SocketCallback::OnClanMembers callback) {
    on_clan_members_ = callback;
}

void SocketClient::setOnChatMessage(SocketCallback::OnChatMessage callback) {
    on_chat_message_ = callback;
}

void SocketClient::setOnClanWarMatch(SocketCallback::OnClanWarMatch callback) {
    on_clan_war_match_ = callback;
}

void SocketClient::setOnClanWarStatus(SocketCallback::OnClanWarStatus callback) {
    on_clan_war_status_ = callback;
}

void SocketClient::setOnClanWarMemberList(SocketCallback::OnClanWarMemberList callback) {
    on_clan_war_member_list_ = callback;
}

void SocketClient::setOnClanWarAttackStart(SocketCallback::OnClanWarAttackStart callback) {
    on_clan_war_attack_start_ = callback;
}

void SocketClient::setOnClanWarSpectate(SocketCallback::OnClanWarSpectate callback) {
    on_clan_war_spectate_ = callback;
}

void SocketClient::setOnClanWarStateUpdate(SocketCallback::OnClanWarStateUpdate callback) {
    on_clan_war_state_update_ = callback;
}

void SocketClient::setOnPvpStart(SocketCallback::OnPvpStart callback) {
    on_pvp_start_ = callback;
}

void SocketClient::setOnPvpAction(SocketCallback::OnPvpAction callback) {
    on_pvp_action_ = callback;
}

void SocketClient::setOnPvpEnd(SocketCallback::OnPvpEnd callback) {
    on_pvp_end_ = callback;
}

void SocketClient::setOnSpectateJoin(SocketCallback::OnSpectateJoin callback) {
    on_spectate_join_ = callback;
}