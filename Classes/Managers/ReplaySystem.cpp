/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     ReplaySystem.cpp
 * File Function: 回放系统 - 负责游戏的回放录制和回放
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "ReplaySystem.h"
#include <sstream>
#include <iostream>

USING_NS_CC;

// ==================== ReplayEvent 序列化 ====================

std::string ReplayEvent::serialize() const
{
    std::ostringstream oss;
    oss << frameIndex << "," << static_cast<int>(type) << "," << unitType << "," << x << "," << y;
    return oss.str();
}

ReplayEvent ReplayEvent::deserialize(const std::string& data)
{
    ReplayEvent event;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, token, ',');
    event.frameIndex = std::stoul(token);
    
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
    
    std::getline(iss, replayData.enemyUserId, '|');
    
    std::getline(iss, token, '|');
    if (!token.empty())
    {
        replayData.randomSeed = std::stoul(token);
    }
    
    size_t jsonLength = 0;
    std::getline(iss, token, '|');
    if (!token.empty())
    {
        jsonLength = std::stoul(token);
    }
    
    if (jsonLength > 0)
    {
        std::vector<char> buffer(jsonLength);
        iss.read(buffer.data(), jsonLength);
        replayData.enemyGameDataJson.assign(buffer.data(), jsonLength);
    }

    char delimiter;
    iss.get(delimiter); 
    
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

void ReplaySystem::recordDeployUnit(unsigned int frameIndex, UnitType unitType, const cocos2d::Vec2& position)
{
    if (!_isRecording) return;
    
    ReplayEvent event;
    event.frameIndex = frameIndex;
    event.type = ReplayEventType::DEPLOY_UNIT;
    event.unitType = static_cast<int>(unitType);
    event.x = position.x;
    event.y = position.y;
    
    _currentReplayData.events.push_back(event);
    // CCLOG("🎥 ReplaySystem: Recorded deploy unit at frame %u", frameIndex);
}

void ReplaySystem::recordEndBattle(unsigned int frameIndex)
{
    if (!_isRecording) return;
    
    ReplayEvent event;
    event.frameIndex = frameIndex;
    event.type = ReplayEventType::END_BATTLE;
    event.unitType = 0;
    event.x = 0;
    event.y = 0;
    
    _currentReplayData.events.push_back(event);
    CCLOG("🎥 ReplaySystem: Recorded end battle at frame %u", frameIndex);
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
    _nextEventIndex = 0;
    
    CCLOG("🎬 ReplaySystem: Loaded replay with %zu events", _currentReplayData.events.size());
}

void ReplaySystem::updateFrame(unsigned int currentFrame)
{
    if (!_isReplaying) return;
    
    while (_nextEventIndex < _currentReplayData.events.size())
    {
        const auto& event = _currentReplayData.events[_nextEventIndex];
        
        // 如果当前帧已经达到事件帧，执行事件
        if (currentFrame >= event.frameIndex)
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
            // 事件按帧排序，如果当前事件未到，后面的也未到
            break;
        }
    }
}

void ReplaySystem::setDeployUnitCallback(std::function<void(UnitType, const cocos2d::Vec2&)> callback)
{
    _deployUnitCallback = callback;
}

void ReplaySystem::setEndBattleCallback(std::function<void()> callback)
{
    _endBattleCallback = callback;
}
