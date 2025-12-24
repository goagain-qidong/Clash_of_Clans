/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MatchMaker.cpp
 * File Function: 匹配系统实现
 * Author:        赵崇治
 * Update Date:   2025/12/19
 * License:       MIT License
 ****************************************************************/
#include "MatchMaker.h"
#include <algorithm>
#include <cmath>

void Matchmaker::Enqueue(const MatchQueueEntry& entry)
{
    std::lock_guard<std::mutex> lock(queueMutex);

    // 检查是否已在队列中
    for (const auto& e : queue)
    {
        if (e.socket == entry.socket)
            return;
    }

    queue.push_back(entry);
}

void Matchmaker::Remove(SOCKET s)
{
    std::lock_guard<std::mutex> lock(queueMutex);

    auto it = std::find_if(queue.begin(), queue.end(),
        [s](const MatchQueueEntry& e) { return e.socket == s; });

    if (it != queue.end())
    {
        queue.erase(it);
    }
}

std::vector<std::pair<MatchQueueEntry, MatchQueueEntry>> Matchmaker::ProcessQueue()
{
    std::lock_guard<std::mutex> lock(queueMutex);

    std::vector<std::pair<MatchQueueEntry, MatchQueueEntry>> matches;

    if (queue.size() < 2)
        return matches;

    auto now = std::chrono::steady_clock::now();

    // 标记已匹配的索引
    std::vector<bool> matched(queue.size(), false);

    for (size_t i = 0; i < queue.size(); i++)
    {
        if (matched[i])
            continue;

        auto& entry1 = queue[i];
        auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(now - entry1.queueTime).count();

        // 根据等待时间扩大匹配范围
        int maxDiff = 200 + static_cast<int>(waitTime * 10);

        for (size_t j = i + 1; j < queue.size(); j++)
        {
            if (matched[j])
                continue;

            auto& entry2 = queue[j];
            int trophyDiff = std::abs(entry1.trophies - entry2.trophies);

            if (trophyDiff <= maxDiff)
            {
                matches.push_back({ entry1, entry2 });
                matched[i] = true;
                matched[j] = true;
                break;
            }
        }
    }

    // 从队列中移除已匹配的玩家（从后向前删除避免索引问题）
    for (int i = static_cast<int>(queue.size()) - 1; i >= 0; i--)
    {
        if (matched[i])
        {
            queue.erase(queue.begin() + i);
        }
    }

    return matches;
}