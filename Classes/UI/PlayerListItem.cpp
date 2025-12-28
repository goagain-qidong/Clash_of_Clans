/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PlayerListItemWidget.cpp
 * File Function: 玩家列表项组件实现
 * Author:        赵崇治
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/
#include "PlayerListItem.h"

#include "Audio/AudioManager.h"

USING_NS_CC;
using namespace ui;

PlayerListItemWidget* PlayerListItemWidget::createOnlinePlayer(const OnlinePlayerInfo&   info,
                                                               const PlayerBattleStatus& status,
                                                               ActionCallback onAttack, ActionCallback onSpectate)
{
    auto widget = new (std::nothrow) PlayerListItemWidget();
    if (widget && widget->initOnlinePlayer(info, status, onAttack, onSpectate))
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return nullptr;
}

PlayerListItemWidget* PlayerListItemWidget::createClanMember(const ClanMemberInfo&     info,
                                                             const PlayerBattleStatus& status, ActionCallback onAttack,
                                                             ActionCallback onSpectate)
{
    auto widget = new (std::nothrow) PlayerListItemWidget();
    if (widget && widget->initClanMember(info, status, onAttack, onSpectate))
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return nullptr;
}

PlayerListItemWidget* PlayerListItemWidget::createClanWarMember(const ClanWarMemberInfo&  info,
                                                                const PlayerBattleStatus& status,
                                                                ActionCallback onAttack, ActionCallback onSpectate)
{
    auto widget = new (std::nothrow) PlayerListItemWidget();
    if (widget && widget->initClanWarMember(info, status, onAttack, onSpectate))
    {
        widget->autorelease();
        return widget;
    }
    CC_SAFE_DELETE(widget);
    return nullptr;
}

bool PlayerListItemWidget::initOnlinePlayer(const OnlinePlayerInfo& info, const PlayerBattleStatus& status,
                                            ActionCallback onAttack, ActionCallback onSpectate)
{
    if (!Layout::init())
        return false;

    setContentSize(Size(560, 70));
    setBackGroundColor(Color3B(70, 70, 90));
    setBackGroundColorType(Layout::BackGroundColorType::SOLID);

    // 玩家名称
    auto nameLabel = Label::createWithSystemFont(info.username, "Arial", 22);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 45));
    nameLabel->setTextColor(Color4B::WHITE);
    addChild(nameLabel);

    // 大本营等级
    auto thLabel = Label::createWithSystemFont(StringUtils::format("TH %d", info.thLevel), "Arial", 16);
    thLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    thLabel->setPosition(Vec2(20, 20));
    thLabel->setTextColor(Color4B(200, 200, 200, 255));
    addChild(thLabel);

    // 金币
    auto goldLabel = Label::createWithSystemFont(StringUtils::format("💰 %d", info.gold), "Arial", 16);
    goldLabel->setPosition(Vec2(180, 45));
    goldLabel->setTextColor(Color4B(255, 215, 0, 255));
    addChild(goldLabel);

    // 圣水
    auto elixirLabel = Label::createWithSystemFont(StringUtils::format("⚗️ %d", info.elixir), "Arial", 16);
    elixirLabel->setPosition(Vec2(180, 20));
    elixirLabel->setTextColor(Color4B(255, 0, 255, 255));
    addChild(elixirLabel);

    addBattleStatusLabel(status, 320, 35);
    addActionButtons(info.userId, status, true, onAttack, onSpectate);

    return true;
}

bool PlayerListItemWidget::initClanMember(const ClanMemberInfo& info, const PlayerBattleStatus& status,
                                          ActionCallback onAttack, ActionCallback onSpectate)
{
    if (!Layout::init())
        return false;

    setContentSize(Size(560, 60));
    setBackGroundColor(Color3B(70, 70, 90));
    setBackGroundColorType(Layout::BackGroundColorType::SOLID);

    // 名字
    auto nameLabel = Label::createWithSystemFont(info.name, "Arial", 20);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 35));
    addChild(nameLabel);

    // 奖杯
    auto trophyLabel = Label::createWithSystemFont(std::to_string(info.trophies) + " 🏆", "Arial", 18);
    trophyLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    trophyLabel->setPosition(Vec2(20, 15));
    trophyLabel->setTextColor(Color4B::YELLOW);
    addChild(trophyLabel);

    // 在线状态
    std::string statusText;
    Color4B     statusColor;
    if (status.isInBattle)
    {
        statusText  = "⚔️ 战斗中";
        statusColor = Color4B::ORANGE;
    }
    else if (info.isOnline)
    {
        statusText  = "🟢 在线";
        statusColor = Color4B::GREEN;
    }
    else
    {
        statusText  = "⚫ 离线";
        statusColor = Color4B::GRAY;
    }

    auto statusLabel = Label::createWithSystemFont(statusText, "Arial", 16);
    statusLabel->setPosition(Vec2(240, 30));
    statusLabel->setTextColor(statusColor);
    addChild(statusLabel);

    addActionButtons(info.id, status, info.isOnline, onAttack, onSpectate);

    return true;
}

bool PlayerListItemWidget::initClanWarMember(const ClanWarMemberInfo& info, const PlayerBattleStatus& status,
                                             ActionCallback onAttack, ActionCallback onSpectate)
{
    if (!Layout::init())
        return false;

    setContentSize(Size(560, 70));
    setBackGroundColor(Color3B(70, 70, 90));
    setBackGroundColorType(Layout::BackGroundColorType::SOLID);

    // 玩家名称
    auto nameLabel = Label::createWithSystemFont(info.username, "Arial", 22);
    nameLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    nameLabel->setPosition(Vec2(20, 45));
    nameLabel->setTextColor(Color4B::WHITE);
    addChild(nameLabel);

    // 最佳战绩
    auto starsLabel = Label::createWithSystemFont(
        StringUtils::format("最佳: %d⭐ %.1f%%", info.bestStars, info.bestDestruction * 100), "Arial", 18);
    starsLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    starsLabel->setPosition(Vec2(20, 20));
    starsLabel->setTextColor(Color4B::YELLOW);
    addChild(starsLabel);

    addBattleStatusLabel(status, 300, 35);
    addActionButtons(info.userId, status, info.canAttack, onAttack, onSpectate);

    return true;
}

void PlayerListItemWidget::addBattleStatusLabel(const PlayerBattleStatus& status, float x, float y)
{
    if (status.isInBattle)
    {
        std::string text  = status.isAttacker ? "🔴 进攻中" : "🔵 防守中";
        auto        label = Label::createWithSystemFont(text, "Arial", 14);
        label->setPosition(Vec2(x, y));
        label->setTextColor(Color4B::ORANGE);
        addChild(label);
    }
}

void PlayerListItemWidget::addActionButtons(const std::string& playerId, const PlayerBattleStatus& status,
                                            bool canAttack, ActionCallback onAttack, ActionCallback onSpectate)
{
    if (status.isInBattle)
    {
        // 战斗中：只显示观战按钮
        auto spectateBtn = Button::create();
        spectateBtn->setTitleText("👁 观战");
        spectateBtn->setTitleFontSize(16);
        spectateBtn->setScale9Enabled(true);
        spectateBtn->setContentSize(Size(90, 32));
        spectateBtn->setPosition(Vec2(480, 35));
        spectateBtn->addClickEventListener([playerId, onSpectate](Ref*) {
            AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
            if (onSpectate)
                onSpectate(playerId);
        });
        addChild(spectateBtn);
    }
    else if (canAttack)
    {
        // 可攻击：显示攻击按钮
        auto attackBtn = Button::create();
        attackBtn->setTitleText("⚔️ PVP");
        attackBtn->setTitleFontSize(16);
        attackBtn->setScale9Enabled(true);
        attackBtn->setContentSize(Size(90, 32));
        attackBtn->setPosition(Vec2(480, 35));
        attackBtn->addClickEventListener([playerId, onAttack](Ref*) {
            AudioManager::GetInstance().PlayEffect(SoundEffectId::kUiButtonClick);
            if (onAttack)
                onAttack(playerId);
        });
        addChild(attackBtn);
    }
}