#include "ReplaySystem.h"
#include <sstream>
#include <iostream>

USING_NS_CC;

// ==================== ReplayEvent 序列化 ====================

std::string ReplayEvent::serialize() const
{
    std::ostringstream oss;
    oss << timestamp << "," << static_cast<int>(type) << "," << unitType << "," << x << "," << y;
    return oss.str();
}

ReplayEvent ReplayEvent::deserialize(const std::string& data)
{
    ReplayEvent event;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, token, ',');
    event.timestamp = std::stof(token);
    
    std::getline(iss, token, ',');
    event.type = static_cast<ReplayEventType>(std::stoi(token));
    
    std::getline(iss, token, ',');
    event.unitType = std::stoi(token);
    
    std::getline(iss, token, ',');
    event.x = std::stof(token);
    
    std::getline(iss, token, ',');
    event.y = std::stof(token);
    
    return event;
}

// ==================== ReplayData 序列化 ====================

std::string ReplayData::serialize() const
{
    std::ostringstream oss;
    // 使用长度前缀来存储JSON数据，防止分隔符冲突
    oss << enemyUserId << "|" << randomSeed << "|" << enemyGameDataJson.length() << "|" << enemyGameDataJson << "|";
    
    for (size_t i = 0; i < events.size(); ++i)
    {
        if (i > 0) oss << ";";
        oss << events[i].serialize();
    }
    
    return oss.str();
}

ReplayData ReplayData::deserialize(const std::string& data)
{
    ReplayData replayData;
    std::istringstream iss(data);
    std::string token;
    
    // 1. Enemy User ID
    std::getline(iss, replayData.enemyUserId, '|');
    
    // 2. Random Seed
    std::getline(iss, token, '|');
    if (!token.empty())
    {
        replayData.randomSeed = std::stoul(token);
    }
    
    // 3. JSON Data Length
    size_t jsonLength = 0;
    std::getline(iss, token, '|');
    if (!token.empty())
    {
        jsonLength = std::stoul(token);
    }
    
    // 4. JSON Data
    if (jsonLength > 0)
    {
        std::vector<char> buffer(jsonLength);
        iss.read(buffer.data(), jsonLength);
        replayData.enemyGameDataJson.assign(buffer.data(), jsonLength);
        
        // 读取紧随其后的分隔符
        char delimiter;
        iss.get(delimiter); 
    }
    else
    {
        // 如果长度为0，可能是一个空字段，读取分隔符
        // 注意：上面的逻辑假设如果有长度，后面紧跟内容然后是分隔符
        // 如果长度为0，那么就是 ||，上面的getline读取了长度，现在应该读取下一个分隔符
        // 但实际上如果没有内容，格式是 ...|0||...
        // getline读取了0，iss指针在第二个|之后。
        // 让我们重新检查逻辑。
        // 格式: ID|Seed|Len|Json|Events
        // getline(ID, |) -> 指针在Seed前
        // getline(Seed, |) -> 指针在Len前
        // getline(Len, |) -> 指针在Json前
        // read(Json) -> 指针在Json后，即Events前的|前
        // get(delimiter) -> 吃掉Events前的|
    }
    
    // 5. Events
    std::string eventsStr;
    std::getline(iss, eventsStr);
    
    if (!eventsStr.empty())
    {
        std::istringstream eventsIss(eventsStr);
        std::string eventStr;
        
        while (std::getline(eventsIss, eventStr, ';'))
        {
            if (!eventStr.empty())
            {
                replayData.events.push_back(ReplayEvent::deserialize(eventStr));
            }
        }
    }
    
    return replayData;
}

// ==================== ReplaySystem 实现 ====================

ReplaySystem& ReplaySystem::getInstance()
{
    static ReplaySystem instance;
    return instance;
}

void ReplaySystem::reset()
{
    _isRecording = false;
    _isReplaying = false;
    _currentReplayData = ReplayData();
    _replayTime = 0.0f;
    _nextEventIndex = 0;
    _deployUnitCallback = nullptr;
    _endBattleCallback = nullptr;
}

void ReplaySystem::startRecording(const std::string& enemyUserId, const std::string& enemyGameDataJson, unsigned int seed)
{
    reset();
    _isRecording = true;
    _currentReplayData.enemyUserId = enemyUserId;
    _currentReplayData.enemyGameDataJson = enemyGameDataJson;
    _currentReplayData.randomSeed = seed;
    CCLOG("🎥 ReplaySystem: Started recording (Enemy: %s, Seed: %u)", enemyUserId.c_str(), seed);
}

void ReplaySystem::recordDeployUnit(float timestamp, UnitType unitType, const cocos2d::Vec2& position)
{
    if (!_isRecording) return;
    
    ReplayEvent event;
    event.timestamp = timestamp;
    event.type = ReplayEventType::DEPLOY_UNIT;
    event.unitType = static_cast<int>(unitType);
    event.x = position.x;
    event.y = position.y;
    
    _currentReplayData.events.push_back(event);
    // CCLOG("🎥 ReplaySystem: Recorded deploy unit at %.2f", timestamp);
}

void ReplaySystem::recordEndBattle(float timestamp)
{
    if (!_isRecording) return;
    
    ReplayEvent event;
    event.timestamp = timestamp;
    event.type = ReplayEventType::END_BATTLE;
    event.unitType = 0;
    event.x = 0;
    event.y = 0;
    
    _currentReplayData.events.push_back(event);
    CCLOG("🎥 ReplaySystem: Recorded end battle at %.2f", timestamp);
}

std::string ReplaySystem::stopRecording()
{
    if (!_isRecording) return "";
    
    _isRecording = false;
    std::string data = _currentReplayData.serialize();
    CCLOG("🎥 ReplaySystem: Stopped recording. Data size: %zu bytes", data.size());
    return data;
}

void ReplaySystem::loadReplay(const std::string& replayDataStr)
{
    reset();
    if (replayDataStr.empty()) return;
    
    _currentReplayData = ReplayData::deserialize(replayDataStr);
    _isReplaying = true;
    _replayTime = 0.0f;
    _nextEventIndex = 0;
    
    CCLOG("🎬 ReplaySystem: Loaded replay with %zu events", _currentReplayData.events.size());
}

void ReplaySystem::update(float dt)
{
    if (!_isReplaying) return;
    
    _replayTime += dt;
    
    while (_nextEventIndex < _currentReplayData.events.size())
    {
        const auto& event = _currentReplayData.events[_nextEventIndex];
        
        // 如果当前时间已经超过事件时间，执行事件
        if (_replayTime >= event.timestamp)
        {
            switch (event.type)
            {
            case ReplayEventType::DEPLOY_UNIT:
                if (_deployUnitCallback)
                {
                    _deployUnitCallback(static_cast<UnitType>(event.unitType), Vec2(event.x, event.y));
                }
                break;
                
            case ReplayEventType::END_BATTLE:
                if (_endBattleCallback)
                {
                    _endBattleCallback();
                }
                break;
            }
            
            _nextEventIndex++;
        }
        else
        {
            // 事件按时间排序，如果当前事件未到时间，后面的也未到
            break;
        }
    }
    
    // 如果所有事件都执行完毕，且超过最后事件一定时间，可以自动结束（可选）
}

void ReplaySystem::setDeployUnitCallback(std::function<void(UnitType, const cocos2d::Vec2&)> callback)
{
    _deployUnitCallback = callback;
}

void ReplaySystem::setEndBattleCallback(std::function<void()> callback)
{
    _endBattleCallback = callback;
}
