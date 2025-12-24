/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     
 * File Function: 
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#pragma once
#include <vector>
#include <mutex>
#include "ClanInfo.h"

class Matchmaker {
public:
    void Enqueue(const MatchQueueEntry& entry);
    void Remove(SOCKET s);
    
    // 执行匹配逻辑，返回成功匹配的对子
    std::vector<std::pair<MatchQueueEntry, MatchQueueEntry>> ProcessQueue();

private:
    std::vector<MatchQueueEntry> queue;
    std::mutex queueMutex;
};