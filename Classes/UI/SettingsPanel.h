/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     SettingsPanel.h
 * File Function: 游戏设置面板 - 负责管理游戏中的设置相关UI
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#ifndef __SETTINGS_PANEL_H__
#define __SETTINGS_PANEL_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <functional>

/**
 * @class SettingsPanel
 * @brief 游戏设置面板
 * 
 * 职责：
 * - 音量控制（音乐和音效）
 * - 账号切换
 * - 退出登录
 * - 测试功能（资源全满）
 */
class SettingsPanel : public cocos2d::Node
{
public:
    CREATE_FUNC(SettingsPanel);
    
    virtual bool init() override;
    
    // 显示/隐藏面板
    void show();
    void hide();
    
    // 回调设置
    void setOnAccountSwitched(const std::function<void()>& callback) { _onAccountSwitched = callback; }
    void setOnLogout(const std::function<void()>& callback) { _onLogout = callback; }
    void setOnMapChanged(const std::function<void(const std::string&)>& callback) { _onMapChanged = callback; }
    
private:
    cocos2d::Size _visibleSize;
    
    // UI元素
    cocos2d::ui::Layout* _panel = nullptr;
    cocos2d::ui::Button* _closeButton = nullptr;
    
    // 音量控制
    cocos2d::ui::Slider* _musicSlider = nullptr;
    cocos2d::ui::Slider* _sfxSlider = nullptr;
    cocos2d::Label* _musicValueLabel = nullptr;
    cocos2d::Label* _sfxValueLabel = nullptr;
    
    // 功能按钮
    cocos2d::ui::Button* _mapSwitchButton = nullptr;
    cocos2d::ui::Button* _accountSwitchButton = nullptr;
    cocos2d::ui::Button* _logoutButton = nullptr;
    cocos2d::ui::Button* _fullResourceButton = nullptr;
    
    // 回调
    std::function<void()> _onAccountSwitched;
    std::function<void()> _onLogout;
    std::function<void(const std::string&)> _onMapChanged;
    
    // 内部方法
    void setupUI();
    void setupVolumeControls(float startY);
    void setupFunctionButtons(float startY);
    
    void onCloseClicked();
    void onMusicVolumeChanged(cocos2d::Ref* sender, cocos2d::ui::Slider::EventType type);
    void onSFXVolumeChanged(cocos2d::Ref* sender, cocos2d::ui::Slider::EventType type);
    void onMapSwitchClicked();
    void onAccountSwitchClicked();
    void onLogoutClicked();
    void onFullResourceClicked();
    
    void showMapSelectionPanel();
    void showAccountList();
    void showPasswordDialog(const std::string& userId, const std::string& username);
    void loadVolumeSettings();
    void saveVolumeSettings();
};

#endif // __SETTINGS_PANEL_H__
