/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     PlayerListLayer.h
 * File Function: 负责玩家列表界面
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#ifndef __PLAYER_LIST_LAYER_H__
#define __PLAYER_LIST_LAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#include <string>
#include <vector>
#include <functional>

/**
 * @struct PlayerInfo
 * @brief 玩家信息结构
 */
struct PlayerInfo
{
    std::string userId;      ///< 用户ID
    std::string username;    ///< 用户名
    int townHallLevel = 1;   ///< 大本营等级
    int trophies = 0;        ///< 奖杯数
    int gold = 0;            ///< 金币
    int elixir = 0;          ///< 圣水

    PlayerInfo() = default;
    PlayerInfo(const std::string& id, const std::string& name, int thLevel, int trophy, int g, int e)
        : userId(id), username(name), townHallLevel(thLevel), trophies(trophy), gold(g), elixir(e) {}
};

/**
 * @class PlayerListLayer
 * @brief 玩家列表弹窗 - 显示可攻击的玩家列表
 */
class PlayerListLayer : public cocos2d::Layer
{
public:
    /**
     * @brief 创建玩家列表层
     * @param players 玩家列表
     * @return PlayerListLayer* 层指针
     */
    static PlayerListLayer* create(const std::vector<PlayerInfo>& players);

    /**
     * @brief 初始化
     * @param players 玩家列表
     * @return bool 是否成功
     */
    virtual bool init(const std::vector<PlayerInfo>& players);

    /**
     * @brief 设置玩家选择回调
     * @param callback 回调函数
     */
    void setOnPlayerSelected(std::function<void(const std::string&)> callback);

    /** @brief 显示弹窗 */
    void show();

    /** @brief 隐藏弹窗 */
    void hide();

private:
    void createUI();            ///< 创建UI
    void populatePlayerList();  ///< 填充玩家列表
    cocos2d::ui::Widget* createPlayerItem(const PlayerInfo& player);

    std::vector<PlayerInfo> _players;  ///< 玩家列表
    std::function<void(const std::string&)> _onPlayerSelected;  ///< 选择回调

    cocos2d::Node* _container = nullptr;      ///< 容器
    cocos2d::ui::ListView* _listView = nullptr;  ///< 列表视图
    cocos2d::ui::Button* _closeBtn = nullptr;    ///< 关闭按钮
    cocos2d::Size _visibleSize;                  ///< 可视区域大小
};

#endif // __PLAYER_LIST_LAYER_H__
