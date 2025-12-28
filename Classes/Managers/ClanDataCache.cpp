/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ClanDataCache.cpp
 * File Function: 部落数据缓存实现
 * Author:        赵崇治
 * Update Date:   2025/12/25
 * License:       MIT License
 ****************************************************************/
#include "ClanDataCache.h"
#include "cocos2d.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"

USING_NS_CC;

const PlayerBattleStatus ClanDataCache::_emptyStatus;

ClanDataCache& ClanDataCache::getInstance()
{
    static ClanDataCache instance;
    return instance;
}

const PlayerBattleStatus& ClanDataCache::getBattleStatus(const std::string& playerId) const
{
    auto it = _battleStatusMap.find(playerId);
    return (it != _battleStatusMap.end()) ? it->second : _emptyStatus;
}

bool ClanDataCache::isPlayerInBattle(const std::string& playerId) const
{
    return _playersInBattle.find(playerId) != _playersInBattle.end();
}

void ClanDataCache::setOnlinePlayers(const std::vector<OnlinePlayerInfo>& players)
{
    _onlinePlayers = players;
    notifyObservers(ClanDataChangeType::ONLINE_PLAYERS);
}

void ClanDataCache::setClanMembers(const std::vector<ClanMemberInfo>& members)
{
    _clanMembers = members;
    notifyObservers(ClanDataChangeType::CLAN_MEMBERS);
}

void ClanDataCache::setClanWarMembers(const std::vector<ClanWarMemberInfo>& members)
{
    _clanWarMembers = members;
    notifyObservers(ClanDataChangeType::CLAN_WAR_MEMBERS);
}

void ClanDataCache::setClanList(const std::vector<ClanInfoClient>& clans)
{
    _clanList = clans;
    
    // 更新当前部落名称
    if (!_currentClanId.empty())
    {
        for (const auto& clan : clans)
        {
            if (clan.clan_id == _currentClanId)
            {
                _currentClanName = clan.clan_name;
                break;
            }
        }
    }
    
    notifyObservers(ClanDataChangeType::CLAN_LIST);
}

void ClanDataCache::setBattleStatusMap(const std::map<std::string, PlayerBattleStatus>& statusMap)
{
    _battleStatusMap = statusMap;
    _playersInBattle.clear();
    
    for (const auto& pair : statusMap)
    {
        if (pair.second.isInBattle)
        {
            _playersInBattle.insert(pair.first);
        }
    }
    
    notifyObservers(ClanDataChangeType::BATTLE_STATUS);
}

void ClanDataCache::setCurrentClan(const std::string& clanId, const std::string& clanName)
{
    // 如果切换到不同部落，先保存当前部落的聊天记录
    if (!_currentClanId.empty() && _currentClanId != clanId)
    {
        saveChatHistory();
        _chatHistory.clear();
    }

    _currentClanId   = clanId;
    _currentClanName = clanName;
    _isInClan        = !clanId.empty();

    // 加载新部落的聊天记录
    if (_isInClan)
    {
        loadChatHistory();
    }

    notifyObservers(ClanDataChangeType::CLAN_INFO);
}

void ClanDataCache::clearCurrentClan()
{
    _currentClanId.clear();
    _currentClanName.clear();
    _isInClan = false;
    notifyObservers(ClanDataChangeType::CLAN_INFO);
}

std::string ClanDataCache::findClanNameById(const std::string& clanId) const
{
    for (const auto& clan : _clanList)
    {
        if (clan.clan_id == clanId)
        {
            return clan.clan_name;
        }
    }
    return "";
}

bool ClanDataCache::addChatMessage(const std::string& sender, const std::string& message)
{
    CCLOG("[ClanDataCache] addChatMessage called: sender=%s, message=%s, historySize=%zu", 
          sender.c_str(), message.c_str(), _chatHistory.size());
    
    // 检查是否是重复消息（最近5条内相同的sender和message视为重复）
    size_t checkCount = std::min(_chatHistory.size(), static_cast<size_t>(5));
    for (size_t i = _chatHistory.size(); i > _chatHistory.size() - checkCount; --i)
    {
        const auto& msg = _chatHistory[i - 1];
        if (msg.sender == sender && msg.message == message)
        {
            CCLOG("[ClanDataCache] 重复消息，跳过");
            return false; // 重复消息，不添加
        }
    }

    _chatHistory.push_back({sender, message});
    CCLOG("[ClanDataCache] 消息已添加，新historySize=%zu", _chatHistory.size());
    
    if (_chatHistory.size() > 50) // 增加保存数量
    {
        _chatHistory.erase(_chatHistory.begin());
    }
    
    // 保存到本地
    saveChatHistory();
    
    // 通知观察者有新聊天消息
    CCLOG("[ClanDataCache] 通知观察者 CHAT_MESSAGE");
    notifyObservers(ClanDataChangeType::CHAT_MESSAGE);
    
    return true;
}

void ClanDataCache::addObserver(void* owner, DataChangeCallback callback)
{
    _observers[owner] = callback;
    CCLOG("[ClanDataCache] addObserver: owner=%p, totalObservers=%zu", owner, _observers.size());
}

void ClanDataCache::removeObserver(void* owner)
{
    _observers.erase(owner);
    CCLOG("[ClanDataCache] removeObserver: owner=%p, remainingObservers=%zu", owner, _observers.size());
}

void ClanDataCache::notifyObservers(ClanDataChangeType type)
{
    CCLOG("[ClanDataCache] notifyObservers: type=%d, observerCount=%zu", 
          static_cast<int>(type), _observers.size());
    for (const auto& pair : _observers)
    {
        if (pair.second)
        {
            CCLOG("[ClanDataCache] 调用观察者回调: owner=%p", pair.first);
            pair.second(type);
        }
    }
}

void ClanDataCache::saveChatHistory()
{
    if (_currentClanId.empty())
        return;

    // 使用部落ID作为文件名的一部分，确保不同部落的聊天记录分开存储
    std::string filename = "chat_" + _currentClanId + ".json";
    std::string path = cocos2d::FileUtils::getInstance()->getWritablePath() + filename;

    rapidjson::Document doc;
    doc.SetArray();
    auto& allocator = doc.GetAllocator();

    for (const auto& msg : _chatHistory)
    {
        rapidjson::Value msgObj(rapidjson::kObjectType);
        msgObj.AddMember("sender", rapidjson::Value(msg.sender.c_str(), allocator), allocator);
        msgObj.AddMember("message", rapidjson::Value(msg.message.c_str(), allocator), allocator);
        doc.PushBack(msgObj, allocator);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    cocos2d::FileUtils::getInstance()->writeStringToFile(buffer.GetString(), path);
    CCLOG("[ClanDataCache] 聊天记录已保存: %s (%zu条)", filename.c_str(), _chatHistory.size());
}

void ClanDataCache::loadChatHistory()
{
    if (_currentClanId.empty())
        return;

    std::string filename = "chat_" + _currentClanId + ".json";
    std::string path = cocos2d::FileUtils::getInstance()->getWritablePath() + filename;

    if (!cocos2d::FileUtils::getInstance()->isFileExist(path))
    {
        CCLOG("[ClanDataCache] 聊天记录文件不存在: %s", filename.c_str());
        return;
    }

    std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(path);
    if (content.empty())
        return;

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (doc.HasParseError() || !doc.IsArray())
    {
        CCLOG("[ClanDataCache] 聊天记录解析失败");
        return;
    }

    _chatHistory.clear();
    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
    {
        const auto& item = doc[i];
        if (item.HasMember("sender") && item.HasMember("message"))
        {
            ChatMessage msg;
            msg.sender = item["sender"].GetString();
            msg.message = item["message"].GetString();
            _chatHistory.push_back(msg);
        }
    }

    CCLOG("[ClanDataCache] 聊天记录已加载: %s (%zu条)", filename.c_str(), _chatHistory.size());
}

void ClanDataCache::clearChatHistory()
{
    _chatHistory.clear();
    
    if (!_currentClanId.empty())
    {
        std::string filename = "chat_" + _currentClanId + ".json";
        std::string path = cocos2d::FileUtils::getInstance()->getWritablePath() + filename;
        cocos2d::FileUtils::getInstance()->removeFile(path);
    }
}