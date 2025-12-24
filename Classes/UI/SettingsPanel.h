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
 * - 音量控制
 * - 账号切换
 * - 退出登录
 * - 测试功能
 */
class SettingsPanel : public cocos2d::Node
{
public:
    CREATE_FUNC(SettingsPanel);

    virtual bool init() override;

    /** @brief 显示面板 */
    void show();

    /** @brief 隐藏面板 */
    void hide();

    /** @brief 设置账号切换回调 */
    void setOnAccountSwitched(const std::function<void()>& callback) { _onAccountSwitched = callback; }

    /** @brief 设置登出回调 */
    void setOnLogout(const std::function<void()>& callback) { _onLogout = callback; }

    /** @brief 设置地图切换回调 */
    void setOnMapChanged(const std::function<void(const std::string&)>& callback) { _onMapChanged = callback; }

private:
    cocos2d::Size _visibleSize;  ///< 可视区域大小

    cocos2d::ui::Layout* _panel = nullptr;       ///< 面板
    cocos2d::ui::Button* _closeButton = nullptr; ///< 关闭按钮

    cocos2d::ui::Slider* _musicSlider = nullptr;   ///< 音乐滑块
    cocos2d::ui::Slider* _sfxSlider = nullptr;     ///< 音效滑块
    cocos2d::Label* _musicValueLabel = nullptr;    ///< 音乐值标签
    cocos2d::Label* _sfxValueLabel = nullptr;      ///< 音效值标签

    cocos2d::ui::Button* _mapSwitchButton = nullptr;     ///< 地图切换按钮
    cocos2d::ui::Button* _accountSwitchButton = nullptr; ///< 账号切换按钮
    cocos2d::ui::Button* _logoutButton = nullptr;        ///< 登出按钮
    cocos2d::ui::Button* _fullResourceButton = nullptr;  ///< 资源全满按钮

    std::function<void()> _onAccountSwitched;  ///< 账号切换回调
    std::function<void()> _onLogout;           ///< 登出回调
    std::function<void(const std::string&)> _onMapChanged;  ///< 地图切换回调

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
