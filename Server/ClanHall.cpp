/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanHall.cpp
 * File Function: 部落系统实现
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#include "ClanHall.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

// ============================================================================
// 构造函数
// ============================================================================

ClanHall::ClanHall(PlayerRegistry* registry) 
    : player_registry_(registry)
    , data_file_path_("clan_data.txt")
    , clan_id_counter_(0) {
    LoadFromFile();
}

// ============================================================================
// 私有辅助方法
// ============================================================================

std::string ClanHall::GenerateClanId() {
    return "CLAN_" + std::to_string(++clan_id_counter_);
}

void ClanHall::LoadFromFile() {
    std::ifstream file(data_file_path_);
    if (!file.is_open()) {
        std::cout << "[Clan] 未找到部落数据文件: " << data_file_path_ << "，将创建新文件" << std::endl;
        return;
    }

    std::cout << "[Clan] 正在加载部落数据文件: " << data_file_path_ << std::endl;

    std::string line;
    // 读取计数器
    if (std::getline(file, line)) {
        try {
            clan_id_counter_ = std::stoi(line);
            std::cout << "[Clan] 读取计数器: " << clan_id_counter_ << std::endl;
        } catch (...) {
            clan_id_counter_ = 0;
        }
    }

    // 读取部落数量
    int clan_count = 0;
    if (std::getline(file, line)) {
        try {
            clan_count = std::stoi(line);
            std::cout << "[Clan] 读取部落数量: " << clan_count << std::endl;
        } catch (...) {
            clan_count = 0;
        }
    }

    // 读取每个部落的数据
    for (int i = 0; i < clan_count; ++i) {
        ClanInfo clan;
        
        if (!std::getline(file, clan.clanId)) break;
        if (!std::getline(file, clan.clanName)) break;
        if (!std::getline(file, clan.leaderId)) break;
        if (!std::getline(file, clan.description)) break;
        
        if (std::getline(file, line)) {
            try { clan.clanTrophies = std::stoi(line); } catch (...) {}
        }
        if (std::getline(file, line)) {
            try { clan.requiredTrophies = std::stoi(line); } catch (...) {}
        }
        if (std::getline(file, line)) {
            clan.isOpen = (line == "1");
        }

        // 读取成员数量和成员列表
        int member_count = 0;
        if (std::getline(file, line)) {
            try { member_count = std::stoi(line); } catch (...) {}
        }
        for (int j = 0; j < member_count; ++j) {
            std::string member_id;
            if (std::getline(file, member_id) && !member_id.empty()) {
                clan.memberIds.push_back(member_id);
            }
        }

        if (!clan.clanId.empty()) {
            clans_[clan.clanId] = clan;
            std::cout << "[Clan] 已加载部落: " << clan.clanName 
                      << " (ID: " << clan.clanId 
                      << ", 族长: " << clan.leaderId
                      << ", 成员: " << clan.memberIds.size() << ")" << std::endl;
            for (const auto& m : clan.memberIds) {
                std::cout << "[Clan]   - 成员: " << m << std::endl;
            }
        }
    }

    file.close();
    std::cout << "[Clan] 共加载 " << clans_.size() << " 个部落" << std::endl;
}

void ClanHall::SaveToFile() {
    std::ofstream file(data_file_path_);
    if (!file.is_open()) {
        std::cerr << "[Clan] 无法保存部落数据文件" << std::endl;
        return;
    }

    // 写入计数器
    file << clan_id_counter_ << "\n";
    // 写入部落数量
    file << clans_.size() << "\n";

    // 写入每个部落的数据
    for (const auto& pair : clans_) {
        const auto& clan = pair.second;
        file << clan.clanId << "\n";
        file << clan.clanName << "\n";
        file << clan.leaderId << "\n";
        file << clan.description << "\n";
        file << clan.clanTrophies << "\n";
        file << clan.requiredTrophies << "\n";
        file << (clan.isOpen ? "1" : "0") << "\n";
        file << clan.memberIds.size() << "\n";
        for (const auto& member_id : clan.memberIds) {
            file << member_id << "\n";
        }
    }

    file.close();
    std::cout << "[Clan] 部落数据已保存" << std::endl;
}

// ============================================================================
// 部落创建与管理
// ============================================================================

bool ClanHall::CreateClan(const std::string& player_id,
                          const std::string& clan_name) {
    // 验证玩家存在性
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        std::cout << "[Clan] 创建失败: 玩家 " << player_id << " 未找到"
                  << std::endl;
        return false;
    }

    // 验证玩家未加入其他部落
    if (!player->clanId.empty()) {
        std::cout << "[Clan] 创建失败: " << player_id << " 已在部落中"
                  << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(clan_mutex_);

    std::string clan_id = GenerateClanId();

    // 创建部落记录
    ClanInfo clan;
    clan.clanId = clan_id;
    clan.clanName = clan_name;
    clan.leaderId = player_id;
    clan.memberIds.push_back(player_id);
    clan.clanTrophies = player->trophies;
    clan.requiredTrophies = 0;
    clan.isOpen = true;

    clans_[clan_id] = clan;

    // 更新玩家的部落归属
    player->clanId = clan_id;

    std::cout << "[Clan] 创建成功: " << clan_name << " (ID: " << clan_id
              << ") 创建者: " << player_id << std::endl;
    
    // 保存到文件
    SaveToFile();
    return true;
}

bool ClanHall::JoinClan(const std::string& player_id,
                        const std::string& clan_id) {
    // 验证玩家存在性
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        std::cout << "[Clan] 加入失败: 玩家 " << player_id << " 未找到"
                  << std::endl;
        return false;
    }

    // 验证玩家未加入其他部落
    if (!player->clanId.empty()) {
        std::cout << "[Clan] 加入失败: " << player_id << " 已在部落 "
                  << player->clanId << " 中" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(clan_mutex_);

    // 验证目标部落存在
    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        std::cout << "[Clan] 加入失败: 部落 " << clan_id << " 未找到"
                  << std::endl;
        return false;
    }

    // 验证部落开放状态
    if (!it->second.isOpen) {
        std::cout << "[Clan] 加入失败: 部落 " << clan_id << " 未开放"
                  << std::endl;
        return false;
    }

    // 验证奖杯数要求
    if (player->trophies < it->second.requiredTrophies) {
        std::cout << "[Clan] 加入失败: " << player_id << " 奖杯数 "
                  << player->trophies << " 不满足要求 "
                  << it->second.requiredTrophies << std::endl;
        return false;
    }

    // 执行加入操作
    it->second.memberIds.push_back(player_id);
    it->second.clanTrophies += player->trophies;
    player->clanId = clan_id;

    std::cout << "[Clan] " << player_id << " 加入 " << it->second.clanName
              << " (ID: " << clan_id << ")" << std::endl;
    
    // 保存到文件
    SaveToFile();
    return true;
}

bool ClanHall::LeaveClan(const std::string& player_id) {
    // 验证玩家存在性
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        return false;
    }

    // 验证玩家已加入部落
    std::string clan_id = player->clanId;
    if (clan_id.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(clan_mutex_);

    // 查找部落
    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return false;
    }

    // 从成员列表中移除
    auto& members = it->second.memberIds;
    members.erase(std::remove(members.begin(), members.end(), player_id),
                  members.end());
    it->second.clanTrophies -= player->trophies;
    player->clanId = "";

    // 如果部落为空，删除部落
    if (members.empty()) {
        clans_.erase(it);
        std::cout << "[Clan] 删除空部落: " << clan_id << std::endl;
    }

    std::cout << "[Clan] " << player_id << " 离开部落 " << clan_id << std::endl;
    
    // 保存到文件
    SaveToFile();
    return true;
}

// ============================================================================
// 部落查询
// ============================================================================

std::string ClanHall::GetClanListJson() {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    std::ostringstream oss;
    oss << "[";
    bool first = true;

    for (const auto& pair : clans_) {
        const auto& clan = pair.second;
        if (!first) {
            oss << ",";
        }
        first = false;

        oss << "{" << "\"id\":\"" << clan.clanId << "\",\""
            << "name\":\"" << clan.clanName << "\",\""
            << "members\":" << clan.memberIds.size() << ","
            << "\"trophies\":" << clan.clanTrophies << ","
            << "\"required\":" << clan.requiredTrophies << ","
            << "\"open\":" << (clan.isOpen ? "true" : "false") << "}";
    }

    oss << "]";
    return oss.str();
}

std::string ClanHall::GetClanMembersJson(const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return "{\"error\":\"CLAN_NOT_FOUND\"}";
    }

    std::ostringstream oss;
    oss << "{\"members\":[";

    bool first = true;
    for (const auto& member_id : it->second.memberIds) {
        if (!first) {
            oss << ",";
        }
        first = false;

        // 获取成员的在线状态和信息
        PlayerContext* player = player_registry_->GetById(member_id);
        bool online = (player != nullptr);
        int trophies = online ? player->trophies : 0;
        std::string name = online ? player->playerName : member_id;

        oss << "{" << "\"id\":\"" << member_id << "\",\""
            << "name\":\"" << name << "\",\""
            << "trophies\":" << trophies << ","
            << "\"online\":" << (online ? "true" : "false") << "}";
    }

    oss << "]}";
    return oss.str();
}

bool ClanHall::IsPlayerInClan(const std::string& player_id,
                              const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return false;
    }

    auto& members = it->second.memberIds;
    return std::find(members.begin(), members.end(), player_id) != members.end();
}

std::vector<std::string> ClanHall::GetClanMemberIds(const std::string& clan_id) {
    std::lock_guard<std::mutex> lock(clan_mutex_);

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        return {};
    }

    return it->second.memberIds;  // 返回副本
}

void ClanHall::EnsurePlayerInClan(const std::string& player_id, const std::string& clan_id) {
    if (player_id.empty() || clan_id.empty()) {
        return;
    }

    // 获取玩家信息用于恢复部落数据
    PlayerContext* player = player_registry_->GetById(player_id);
    if (player == nullptr) {
        std::cout << "[Clan] EnsurePlayerInClan: 玩家 " << player_id << " 未找到" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(clan_mutex_);

    std::cout << "[Clan] EnsurePlayerInClan: 玩家=" << player_id 
              << ", 请求部落=" << clan_id 
              << ", 当前部落数=" << clans_.size() << std::endl;

    auto it = clans_.find(clan_id);
    if (it == clans_.end()) {
        // 部落不存在，为玩家重建部落以确保聊天等功能正常
        std::cout << "[Clan] 部落 " << clan_id << " 不存在，为玩家 " 
                  << player_id << " 重建部落" << std::endl;
        
        // 重建部落
        ClanInfo clan;
        clan.clanId = clan_id;
        clan.clanName = "恢复的部落";
        clan.leaderId = player_id;
        clan.memberIds.push_back(player_id);
        clan.clanTrophies = player->trophies;
        clan.requiredTrophies = 0;
        clan.isOpen = true;
        
        clans_[clan_id] = clan;
        
        // 更新计数器，确保不会生成重复ID
        if (clan_id.length() > 5 && clan_id.substr(0, 5) == "CLAN_") {
            try {
                int id_num = std::stoi(clan_id.substr(5));
                if (id_num >= clan_id_counter_) {
                    clan_id_counter_ = id_num;
                }
            } catch (...) {}
        }
        
        // 确保玩家的 clanId 正确
        player->clanId = clan_id;
        
        SaveToFile();
        std::cout << "[Clan] 部落 " << clan_id << " 已重建，玩家clanId=" << player->clanId << std::endl;
        return;
    }

    // 部落存在，始终确保玩家的 clanId 正确设置
    player->clanId = clan_id;
    std::cout << "[Clan] 部落 " << clan_id << " 存在，设置玩家 " << player_id 
              << " 的clanId=" << player->clanId << std::endl;

    // 检查玩家是否已在成员列表中
    auto& members = it->second.memberIds;
    if (std::find(members.begin(), members.end(), player_id) == members.end()) {
        members.push_back(player_id);
        it->second.clanTrophies += player->trophies;
        SaveToFile();
        std::cout << "[Clan] 玩家 " << player_id << " 重新加入部落 " << clan_id << std::endl;
    }
}