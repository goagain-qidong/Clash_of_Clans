#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class AccountSelectScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();

    virtual bool init() override;

    CREATE_FUNC(AccountSelectScene);

private:
    cocos2d::ui::ListView* _list = nullptr;

    cocos2d::ui::Button* _startBtn = nullptr;

    std::string _selectedUserId;

    void buildUI();

    void refreshList();

    void onAddAccount();

    void onStartGame();

    void showInputDialog(const std::string& title, 
                        const std::string& placeholder,
                        std::function<void(const std::string&)> onConfirm);

    void showPasswordDialog(const std::string& userId);

    void showCreateAccountDialog();
};
