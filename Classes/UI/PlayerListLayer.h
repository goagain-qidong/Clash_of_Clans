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
    std::string userId;
    std::string username;
    int townHallLevel = 1;
    int trophies = 0;
    int gold = 0;
    int elixir = 0;
    
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
    static PlayerListLayer* create(const std::vector<PlayerInfo>& players);
    virtual bool init(const std::vector<PlayerInfo>& players);
    
    /**
     * @brief 设置玩家选择回调
     * @param callback 回调函数，参数为选中玩家的userId
     */
    void setOnPlayerSelected(std::function<void(const std::string&)> callback);
    
    /**
     * @brief 显示弹窗
     */
    void show();
    
    /**
     * @brief 隐藏弹窗
     */
    void hide();
    
private:
    void createUI();
    void populatePlayerList();
    cocos2d::ui::Widget* createPlayerItem(const PlayerInfo& player);
    
private:
    std::vector<PlayerInfo> _players;
    std::function<void(const std::string&)> _onPlayerSelected;
    
    cocos2d::Node* _container = nullptr;
    cocos2d::ui::ListView* _listView = nullptr;
    cocos2d::ui::Button* _closeBtn = nullptr;
    cocos2d::Size _visibleSize;
};

#endif // __PLAYER_LIST_LAYER_H__
