/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AccountSelectScene.h
 * File Function: 负责账号选择界面
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

/**
 * @class AccountSelectScene
 * @brief 账号选择场景
 */
class AccountSelectScene : public cocos2d::Scene
{
public:
    /**
     * @brief 创建场景
     * @return cocos2d::Scene* 场景指针
     */
    static cocos2d::Scene* createScene();

    virtual bool init() override;

    CREATE_FUNC(AccountSelectScene);

private:
    cocos2d::ui::ListView* _list = nullptr;    ///< 账号列表
    cocos2d::ui::Button* _startBtn = nullptr;  ///< 开始按钮
    std::string _selectedUserId;               ///< 选中的用户ID

    void buildUI();       ///< 构建UI
    void refreshList();   ///< 刷新列表
    void onAddAccount();  ///< 添加账号
    void onStartGame();   ///< 开始游戏

    /**
     * @brief 显示输入对话框
     * @param title 标题
     * @param placeholder 占位符
     * @param onConfirm 确认回调
     */
    void showInputDialog(const std::string& title,
                        const std::string& placeholder,
                        std::function<void(const std::string&)> onConfirm);

    /**
     * @brief 显示密码对话框
     * @param userId 用户ID
     */
    void showPasswordDialog(const std::string& userId);

    /** @brief 显示创建账号对话框 */
    void showCreateAccountDialog();

    /**
     * @brief 显示删除确认对话框
     * @param userId 用户ID
     * @param username 用户名
     */
    void showDeleteConfirmDialog(const std::string& userId, const std::string& username);
};
