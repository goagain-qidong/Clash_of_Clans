 /****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     DefenseLogSystem.cpp
 * File Function: 防守日志系统 - 记录和管理玩家的防守日志
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "DefenseLogSystem.h"
#include "AccountManager.h"
#include "ResourceManager.h"
#include "Scenes/BattleScene.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "base/base64.h"
#include <sstream>
#include <algorithm>
#include <exception>
#include <stdexcept>

USING_NS_CC;
using namespace ui;

// ==================== DefenseLog 序列化 ====================

std::string DefenseLog::serialize() const
{
    try {
        // Sanitize attackerName
        std::string safeName = attackerName;
        std::replace(safeName.begin(), safeName.end(), '|', ' ');
        std::replace(safeName.begin(), safeName.end(), '\n', ' ');
        std::replace(safeName.begin(), safeName.end(), '\r', ' ');

        // Encode replayData
        std::string encodedReplay = "";
        if (!replayData.empty())
        {
            char* encoded = nullptr;
            base64Encode((const unsigned char*)replayData.c_str(), (unsigned int)replayData.length(), &encoded);
            if (encoded)
            {
                encodedReplay = "B64:" + std::string(encoded);
                free(encoded);
            }
            else
            {
                encodedReplay = replayData;
            }
        }

        std::ostringstream oss;
        oss << attackerId << "|" << safeName << "|" << starsLost << "|" 
            << goldLost << "|" << elixirLost << "|" << trophyChange << "|" 
            << timestamp << "|" << (isViewed ? "1" : "0") << "|" << encodedReplay;
        return oss.str();
    }
    catch (const std::exception& e) {
        CCLOG("❌ DefenseLog::serialize 异常: %s", e.what());
        return "";
    }
}

DefenseLog DefenseLog::deserialize(const std::string& data)
{
    DefenseLog log;
    try {
        std::istringstream iss(data);
        std::string token;
        
        std::getline(iss, log.attackerId, '|');
        std::getline(iss, log.attackerName, '|');
        std::getline(iss, token, '|');
        if (!token.empty()) log.starsLost = std::stoi(token);
        std::getline(iss, token, '|');
        if (!token.empty()) log.goldLost = std::stoi(token);
        std::getline(iss, token, '|');
        if (!token.empty()) log.elixirLost = std::stoi(token);
        std::getline(iss, token, '|');
        if (!token.empty()) log.trophyChange = std::stoi(token);
        std::getline(iss, log.timestamp, '|');
        std::getline(iss, token, '|');
        log.isViewed = (token == "1");
        
        std::string remaining;
        std::getline(iss, remaining); // 读取直到行尾
        
        if (remaining.find("B64:") == 0)
        {
            std::string b64 = remaining.substr(4);
            unsigned char* decoded = nullptr;
            int len = base64Decode((const unsigned char*)b64.c_str(), (unsigned int)b64.length(), &decoded);
            if (decoded && len > 0)
            {
                log.replayData = std::string((char*)decoded, len);
                free(decoded);
            }
            else
            {
                log.replayData = ""; // Decode failed
            }
        }
        else
        {
            log.replayData = remaining;
        }
    }
    catch (const std::exception& e) {
        CCLOG("❌ DefenseLog::deserialize 异常: %s", e.what());
    }
    return log;
}

// ==================== DefenseLogSystem ====================

DefenseLogSystem& DefenseLogSystem::getInstance()
{
    static DefenseLogSystem instance;
    return instance;
}

void DefenseLogSystem::addDefenseLog(const DefenseLog& log)
{
    DefenseLog newLog = log;
    
    // 🆕 Offload replay data to file to avoid UserDefault size limits
    if (!newLog.replayData.empty() && newLog.replayData.find("FILE:") != 0)
    {
        // Generate unique filename: replay_ATTACKER_TIMESTAMP_RAND.dat
        std::string timestamp = StringUtils::toString((long)time(nullptr));
        std::string random = StringUtils::toString(rand() % 1000);
        std::string filename = "replay_" + newLog.attackerId + "_" + timestamp + "_" + random + ".dat";
        std::string fullPath = FileUtils::getInstance()->getWritablePath() + filename;
        
        if (FileUtils::getInstance()->writeStringToFile(newLog.replayData, fullPath))
        {
            CCLOG("💾 Saved replay data to file: %s (Size: %zu)", filename.c_str(), newLog.replayData.length());
            newLog.replayData = "FILE:" + filename;
        }
        else
        {
            CCLOG("❌ Failed to save replay data to file! Falling back to inline storage.");
        }
    }

    _logs.insert(_logs.begin(), newLog);  // 插入到最前面（最新）
    
    // 限制日志数量
    if (_logs.size() > MAX_LOGS)
    {
        _logs.resize(MAX_LOGS);
    }
    
    save();
    
    CCLOG("🛡️ 新增防守日志: 被 %s 攻击，失去 %d 金币，%d 圣水", 
          log.attackerName.c_str(), log.goldLost, log.elixirLost);
    
    // 🆕 同步更新玩家资源（扣除损失）
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    if (currentAccount)
    {
        auto& resMgr = ResourceManager::getInstance();
        
        // 扣除资源（确保不会变成负数）
        int currentGold = resMgr.getResourceCount(ResourceType::kGold);
        int currentElixir = resMgr.getResourceCount(ResourceType::kElixir);
        
        resMgr.setResourceCount(ResourceType::kGold, std::max(0, currentGold - log.goldLost));
        resMgr.setResourceCount(ResourceType::kElixir, std::max(0, currentElixir - log.elixirLost));
        
        // 修复：使用 GameStateData 的正确结构访问资源
        GameStateData gameData = accMgr.getCurrentGameData();
        gameData.resources.gold = resMgr.getResourceCount(ResourceType::kGold);
        gameData.resources.elixir = resMgr.getResourceCount(ResourceType::kElixir);
        gameData.progress.trophies = std::max(0, gameData.progress.trophies + log.trophyChange);
        
        accMgr.updateGameData(gameData);
        
        CCLOG("💰 Updated resources after attack: Gold=%d (-%d), Elixir=%d (-%d), Trophy=%d (%+d)", 
              gameData.resources.gold, log.goldLost, 
              gameData.resources.elixir, log.elixirLost,
              gameData.progress.trophies, log.trophyChange);
    }
}

std::vector<DefenseLog> DefenseLogSystem::getUnviewedLogs() const
{
    std::vector<DefenseLog> unviewed;
    for (const auto& log : _logs)
    {
        if (!log.isViewed)
        {
            unviewed.push_back(log);
        }
    }
    return unviewed;
}

void DefenseLogSystem::markAllAsViewed()
{
    for (auto& log : _logs)
    {
        log.isViewed = true;
    }
    save();
}

void DefenseLogSystem::clearAllLogs()
{
    _logs.clear();
    save();
}

bool DefenseLogSystem::hasUnviewedLogs() const
{
    for (const auto& log : _logs)
    {
        if (!log.isViewed)
        {
            return true;
        }
    }
    return false;
}

void DefenseLogSystem::save()
{
    try {
        auto& accMgr = AccountManager::getInstance();
        const auto* currentAccount = accMgr.getCurrentAccount();
        if (!currentAccount)
        {
            return;
        }
        
        std::string key = "defense_log_" + currentAccount->account.userId;
        
        std::ostringstream oss;
        for (size_t i = 0; i < _logs.size(); ++i)
        {
            if (i > 0) oss << "\n";
            oss << _logs[i].serialize();
        }
        
        UserDefault::getInstance()->setStringForKey(key.c_str(), oss.str());
        UserDefault::getInstance()->flush();
    }
    catch (const std::exception& e) {
        CCLOG("❌ DefenseLogSystem::save 异常: %s", e.what());
    }
}

void DefenseLogSystem::load()
{
    try {
        auto& accMgr = AccountManager::getInstance();
        const auto* currentAccount = accMgr.getCurrentAccount();
        if (!currentAccount)
        {
            return;
        }
        
        std::string key = "defense_log_" + currentAccount->account.userId;
        std::string data = UserDefault::getInstance()->getStringForKey(key.c_str(), "");
        
        _logs.clear();
        
        if (data.empty())
        {
            return;
        }
        
        std::istringstream iss(data);
        std::string line;
        
        while (std::getline(iss, line))
        {
            if (!line.empty())
            {
                _logs.push_back(DefenseLog::deserialize(line));
            }
        }
        
        CCLOG("📂 加载了 %zu 条防守日志", _logs.size());
    }
    catch (const std::exception& e) {
        CCLOG("❌ DefenseLogSystem::load 异常: %s", e.what());
    }
}

void DefenseLogSystem::showDefenseLogUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto runningScene = Director::getInstance()->getRunningScene();
    if (!runningScene)
    {
        return;
    }
    
    // 创建弹窗层
    auto layer = Layer::create();
    
    // 半透明背景
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 180));
    layer->addChild(bgMask);
    
    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    layer->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, layer);
    
    // 容器
    auto container = ui::Layout::create();
    container->setContentSize(Size(700, 600));
    container->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    container->setBackGroundColor(Color3B(40, 40, 50));
    container->setBackGroundColorOpacity(255);
    container->setPosition(Vec2((visibleSize.width - 700) / 2, (visibleSize.height - 600) / 2));
    layer->addChild(container);
    
    // 标题
    auto title = Label::createWithSystemFont("防守日志", "Arial", 32);
    title->setPosition(Vec2(350, 560));
    title->setTextColor(Color4B::YELLOW);
    container->addChild(title);
    
    // 关闭按钮
    auto closeBtn = Button::create();
    closeBtn->setTitleText("X");
    closeBtn->setTitleFontSize(32);
    closeBtn->setPosition(Vec2(670, 560));
    closeBtn->addClickEventListener([layer](Ref*) {
        layer->removeFromParent();
    });
    container->addChild(closeBtn);
    
    // 日志列表
    auto listView = ListView::create();
    listView->setDirection(ui::ScrollView::Direction::VERTICAL);
    listView->setContentSize(Size(660, 480));
    listView->setPosition(Vec2(20, 20));
    listView->setBounceEnabled(true);
    listView->setScrollBarEnabled(true);
    listView->setItemsMargin(8.0f);
    container->addChild(listView);
    
    if (_logs.empty())
    {
        auto tip = Label::createWithSystemFont("暂无防守记录", "Arial", 24);
        tip->setPosition(Vec2(330, 240));
        tip->setTextColor(Color4B::GRAY);
        
        auto item = Layout::create();
        item->setContentSize(Size(660, 480));
        item->addChild(tip);
        listView->pushBackCustomItem(item);
    }
    else
    {
        for (size_t idx = 0; idx < _logs.size(); ++idx)
        {
            const auto& log = _logs[idx];
            
            auto item = Layout::create();
            item->setContentSize(Size(660, 140));
            item->setTouchEnabled(true);
            
            // 背景
            Color4B bgColor = log.isViewed ? Color4B(60, 60, 80, 255) : Color4B(100, 70, 70, 255);
            auto bg = LayerColor::create(bgColor, 660, 140);
            item->addChild(bg, 0);
            
            // ==================== 第一行：攻击者和星数 ====================
            auto attackerLabel = Label::createWithSystemFont(
                StringUtils::format("被 %s 攻击", log.attackerName.c_str()), 
                "Arial", 24);
            attackerLabel->setAnchorPoint(Vec2(0, 0.5f));
            attackerLabel->setPosition(Vec2(20, 110));
            attackerLabel->setTextColor(Color4B::WHITE);
            item->addChild(attackerLabel);
            
            // 星数
            std::string starsStr = "";
            for (int i = 0; i < log.starsLost; i++)
            {
                starsStr += "★";
            }
            for (int i = log.starsLost; i < 3; i++)
            {
                starsStr += "☆";
            }
            auto starsLabel = Label::createWithSystemFont(starsStr, "Arial", 28);
            starsLabel->setPosition(Vec2(600, 110));
            starsLabel->setTextColor(Color4B::YELLOW);
            item->addChild(starsLabel);
            
            // ==================== 第二行：时间 ====================
            auto timeLabel = Label::createWithSystemFont(log.timestamp, "Arial", 16);
            timeLabel->setAnchorPoint(Vec2(0, 0.5f));
            timeLabel->setPosition(Vec2(20, 80));
            timeLabel->setTextColor(Color4B::GRAY);
            item->addChild(timeLabel);
            
            // ==================== 第三行：掠夺信息（详细） ====================
            auto lossLabel = Label::createWithSystemFont(
                StringUtils::format("掠夺: 💰 %d  ⚗️ %d  |  奖杯: %+d", 
                    log.goldLost, log.elixirLost, log.trophyChange),
                "Arial", 18);
            lossLabel->setAnchorPoint(Vec2(0, 0.5f));
            lossLabel->setPosition(Vec2(20, 50));
            lossLabel->setTextColor(Color4B::RED);
            item->addChild(lossLabel);
            
            // ==================== 战斗回放按钮 ====================
            auto replayBtn = Button::create();
            replayBtn->setTitleText("回放");
            replayBtn->setTitleFontSize(16);
            replayBtn->setScale9Enabled(true);
            replayBtn->setContentSize(Size(80, 35));
            replayBtn->setPosition(Vec2(580, 30));
            replayBtn->addClickEventListener([idx, log](Ref*) {
                CCLOG("🎬 Battle replay clicked for attack from: %s", log.attackerName.c_str());
                
                std::string replayData = log.replayData;
                
                // 🆕 Handle file-based replay data
                if (replayData.find("FILE:") == 0)
                {
                    std::string filename = replayData.substr(5);
                    std::string fullPath = FileUtils::getInstance()->getWritablePath() + filename;
                    if (FileUtils::getInstance()->isFileExist(fullPath))
                    {
                        replayData = FileUtils::getInstance()->getStringFromFile(fullPath);
                        CCLOG("📂 Loaded replay data from file: %s (Size: %zu)", filename.c_str(), replayData.length());
                    }
                    else
                    {
                        CCLOG("❌ Replay file not found: %s", filename.c_str());
                        return;
                    }
                }
                // Handle Base64 (legacy or fallback)
                else if (replayData.find("B64:") == 0)
                {
                    std::string b64 = replayData.substr(4);
                    unsigned char* decoded = nullptr;
                    int len = base64Decode((const unsigned char*)b64.c_str(), (unsigned int)b64.length(), &decoded);
                    if (decoded && len > 0)
                    {
                        replayData = std::string((char*)decoded, len);
                        free(decoded);
                    }
                }
                
                if (replayData.empty())
                {
                    CCLOG("⚠️ No replay data available!");
                    return;
                }
                
                // 切换场景到战斗回放
                // 注意：我们需要保存当前场景以便返回，或者直接替换
                // 这里我们使用 pushScene/popScene 机制，但 BattleScene 比较重
                // 最好是 BattleScene::createWithReplayData
                
                auto scene = BattleScene::createWithReplayData(replayData);
                if (scene)
                {
                    Director::getInstance()->pushScene(TransitionFade::create(0.5f, scene, Color3B::BLACK));
                }
            });
            item->addChild(replayBtn);
            
            // ==================== 详细信息按钮（可选） ====================
            auto detailBtn = Button::create();
            detailBtn->setTitleText("详情");
            detailBtn->setTitleFontSize(16);
            detailBtn->setScale9Enabled(true);
            detailBtn->setContentSize(Size(80, 35));
            detailBtn->setPosition(Vec2(480, 30));
            detailBtn->addClickEventListener([idx, visibleSize, runningScene, log](Ref*) {
                CCLOG("📋 Showing detailed attack info for: %s", log.attackerName.c_str());
                showAttackDetailPopup(visibleSize, runningScene, log);
            });
            item->addChild(detailBtn);
            
            listView->pushBackCustomItem(item);
        }
    }
    
    // 添加到场景
    runningScene->addChild(layer, 1000);
    
    // 标记所有为已查看
    markAllAsViewed();
    
    // 显示动画
    container->setScale(0.0f);
    container->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}

// ==================== 🆕 新增：显示攻击详情弹窗 ====================
void DefenseLogSystem::showAttackDetailPopup(const cocos2d::Size& visibleSize, cocos2d::Scene* scene, const DefenseLog& log)
{
    // 创建弹窗层
    auto layer = Layer::create();
    
    // 半透明背景
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 200));
    layer->addChild(bgMask);
    
    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    layer->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, layer);
    
    // 详情面板
    auto detailPanel = ui::Layout::create();
    detailPanel->setContentSize(Size(550, 450));
    detailPanel->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    detailPanel->setBackGroundColor(Color3B(30, 30, 40));
    detailPanel->setBackGroundColorOpacity(255);
    detailPanel->setPosition(Vec2((visibleSize.width - 550) / 2, (visibleSize.height - 450) / 2));
    layer->addChild(detailPanel);
    
    // 关闭按钮
    auto closeBtn = Button::create();
    closeBtn->setTitleText("关闭");
    closeBtn->setTitleFontSize(18);
    closeBtn->setPosition(Vec2(500, 410));
    closeBtn->addClickEventListener([layer](Ref*) {
        layer->removeFromParent();
    });
    detailPanel->addChild(closeBtn);
    
    // 标题
    auto title = Label::createWithSystemFont("战斗详情", "Arial", 28);
    title->setPosition(Vec2(275, 410));
    title->setTextColor(Color4B::YELLOW);
    detailPanel->addChild(title);
    
    // ==================== 详细信息展示 ====================
    float labelY = 360;
    float labelSpacing = 50;
    
    // 攻击者信息
    auto attackerInfoLabel = Label::createWithSystemFont(
        StringUtils::format("攻击者: %s", log.attackerName.c_str()),
        "Arial", 22);
    attackerInfoLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    attackerInfoLabel->setPosition(Vec2(275, labelY));
    attackerInfoLabel->setTextColor(Color4B::WHITE);
    detailPanel->addChild(attackerInfoLabel);
    
    labelY -= labelSpacing;
    
    // 获得星数
    std::string starsStr = "";
    for (int i = 0; i < log.starsLost; i++)
    {
        starsStr += "★";
    }
    for (int i = log.starsLost; i < 3; i++)
    {
        starsStr += "☆";
    }
    auto starsInfoLabel = Label::createWithSystemFont(
        StringUtils::format("摧毁星数: %s (%d/3)", starsStr.c_str(), log.starsLost),
        "Arial", 20);
    starsInfoLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    starsInfoLabel->setPosition(Vec2(275, labelY));
    starsInfoLabel->setTextColor(Color4B::YELLOW);
    detailPanel->addChild(starsInfoLabel);
    
    labelY -= labelSpacing;
    
    // 掠夺的金币
    auto goldLabel = Label::createWithSystemFont(
        StringUtils::format("💰 掠夺金币: %d", log.goldLost),
        "Arial", 20);
    goldLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    goldLabel->setPosition(Vec2(275, labelY));
    goldLabel->setTextColor(Color4B(255, 215, 0, 255));  // Gold color
    detailPanel->addChild(goldLabel);
    
    labelY -= labelSpacing;
    
    // 掠夺的圣水
    auto elixirLabel = Label::createWithSystemFont(
        StringUtils::format("⚗️  掠夺圣水: %d", log.elixirLost),
        "Arial", 20);
    elixirLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    elixirLabel->setPosition(Vec2(275, labelY));
    elixirLabel->setTextColor(Color4B(100, 200, 255, 255));  // Elixir color
    detailPanel->addChild(elixirLabel);
    
    labelY -= labelSpacing;
    
    // 奖杯变化
    Color4B trophyColor = log.trophyChange >= 0 ? Color4B(100, 255, 100, 255) : Color4B(255, 100, 100, 255);
    auto trophyLabel = Label::createWithSystemFont(
        StringUtils::format("🏆 奖杯变化: %+d", log.trophyChange),
        "Arial", 20);
    trophyLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    trophyLabel->setPosition(Vec2(275, labelY));
    trophyLabel->setTextColor(trophyColor);
    detailPanel->addChild(trophyLabel);
    
    labelY -= labelSpacing;
    
    // 战斗时间
    auto timeLabel = Label::createWithSystemFont(
        StringUtils::format("⏰ 战斗时间: %s", log.timestamp.c_str()),
        "Arial", 18);
    timeLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    timeLabel->setPosition(Vec2(275, labelY));
    timeLabel->setTextColor(Color4B::GRAY);
    detailPanel->addChild(timeLabel);
    
    labelY -= labelSpacing + 10;
    
    // ==================== 战斗回放按钮 ====================
    auto replayBtn = Button::create();
    replayBtn->setTitleText("观看战斗回放");
    replayBtn->setTitleFontSize(20);
    replayBtn->setScale9Enabled(true);
    replayBtn->setContentSize(Size(200, 45));
    replayBtn->setPosition(Vec2(275, labelY));
    replayBtn->addClickEventListener([log](Ref*) {
        CCLOG("🎬 Playing battle replay for attack from: %s", log.attackerName.c_str());
        
        std::string replayData = log.replayData;
        
        // 🆕 Handle file-based replay data
        if (replayData.find("FILE:") == 0)
        {
            std::string filename = replayData.substr(5);
            std::string fullPath = FileUtils::getInstance()->getWritablePath() + filename;
            if (FileUtils::getInstance()->isFileExist(fullPath))
            {
                replayData = FileUtils::getInstance()->getStringFromFile(fullPath);
                CCLOG("📂 Loaded replay data from file: %s (Size: %zu)", filename.c_str(), replayData.length());
            }
            else
            {
                CCLOG("❌ Replay file not found: %s", filename.c_str());
                return;
            }
        }
        
        if (replayData.empty())
        {
            CCLOG("⚠️ No replay data available!");
            return;
        }
        
        auto scene = BattleScene::createWithReplayData(replayData);
        if (scene)
        {
            Director::getInstance()->pushScene(TransitionFade::create(0.5f, scene, Color3B::BLACK));
        }
    });
    detailPanel->addChild(replayBtn);
    
    // 添加到场景
    scene->addChild(layer, 1001);
    
    // 显示动画
    detailPanel->setScale(0.0f);
    detailPanel->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}
