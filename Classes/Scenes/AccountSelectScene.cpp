/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     AccountSelectScene.cpp
 * File Function: 负责账号选择界面
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "AccountSelectScene.h"

#include "DraggableMapScene.h"
#include "Managers/AccountManager.h"
#include "Managers/MusicManager.h"

USING_NS_CC;

using namespace ui;

Scene* AccountSelectScene::createScene()
{
    return AccountSelectScene::create();
}

bool AccountSelectScene::init()
{
    if (!Scene::init())
        return false;

    buildUI();

    refreshList();

    // 播放准备界面背景音乐
    MusicManager::getInstance().playMusic(MusicType::BATTLE_PREPARING, true);

    return true;
}

// 构建账号选择界面的UI元素
void AccountSelectScene::buildUI()
{
    auto vs = Director::getInstance()->getVisibleSize();

    // 标题文本
    auto title = Label::createWithSystemFont("选择账号", "Arial", 32);
    title->setPosition(Vec2(vs.width / 2, vs.height - 120));
    title->setTextColor(Color4B::WHITE);
    this->addChild(title, 1);

    // 账号列表容器
    _list = ListView::create();
    _list->setContentSize(Size(800, 250));
    _list->setPosition(Vec2(vs.width / 2 - 400, vs.height / 2 - 125));
    _list->setBackGroundColor(Color3B(60, 60, 60));
    _list->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _list->setBounceEnabled(true);
    _list->setScrollBarEnabled(true);
    this->addChild(_list, 1);

    // 保持 ListView 的事件监听（仅注册一次）
    _list->addEventListener([this](Ref* sender, ui::ListView::EventType type) {
        if (type == ui::ListView::EventType::ON_SELECTED_ITEM_END)
        {
            auto listView = static_cast<ui::ListView*>(sender);
            auto item = listView->getItem(listView->getCurSelectedIndex());
            if (item)
            {
                _selectedUserId = item->getName();
                // 不重建列表，只更新选中样式，避免滚动位置重置和闪烁
                // 更新所有项的选中状态
                for (int i = 0; i < listView->getItems().size(); ++i)
                {
                    auto it = listView->getItem(i);
                    bool sel = (it->getName() == _selectedUserId);
                    if (auto marker = it->getChildByName<Label*>("marker"))
                        marker->setVisible(sel);
                    // 背景颜色更新
                    for (auto child : it->getChildren())
                    {
                        if (auto layer = dynamic_cast<LayerColor*>(child))
                        {
                            layer->setColor(sel ? Color3B(70, 70, 90) : Color3B(50, 50, 70));
                        }
                    }
                }
            }
        }
    });

    // 新增账号按钮
    auto addBtn = Button::create();
    addBtn->setTitleText("新增账号");
    addBtn->setTitleFontSize(24);
    addBtn->setContentSize(Size(160, 60));
    addBtn->setScale9Enabled(true);
    addBtn->setPosition(Vec2(vs.width / 2 - 120, 120));
    addBtn->addClickEventListener([this](Ref*) { onAddAccount(); });
    this->addChild(addBtn, 1);

    // 开始游戏按钮
    _startBtn = Button::create();
    _startBtn->setTitleText("开始游戏");
    _startBtn->setTitleFontSize(24);
    _startBtn->setContentSize(Size(160, 60));
    _startBtn->setScale9Enabled(true);
    _startBtn->setPosition(Vec2(vs.width / 2 + 120, 120));
    _startBtn->addClickEventListener([this](Ref*) { onStartGame(); });
    this->addChild(_startBtn, 1);

    // 背景层
    auto bg = LayerColor::create(Color4B(30, 30, 40, 255));
    this->addChild(bg, -1);
}

// 刷新账号列表显示
void AccountSelectScene::refreshList()
{
    // 记录滚动位置，避免重建后跳到顶部
    Vec2 innerPos = _list->getInnerContainerPosition();

    _list->removeAllItems();

    auto& mgr = AccountManager::getInstance();
    const auto& accounts = mgr.listAccounts();

    // 无账号时提示创建
    if (accounts.empty())
    {
        auto tip = Label::createWithSystemFont("暂无账号，请点击新增账号", "Arial", 24);
        tip->setPosition(Vec2(300, 250));

        auto item = Layout::create();
        item->setContentSize(Size(600, 500));
        item->addChild(tip);

        _list->pushBackCustomItem(item);

        _selectedUserId.clear();
        return;
    }

    // 如果当前没有选中账号，默认选中当前账号或第一个
    if (_selectedUserId.empty())
    {
        if (auto cur = mgr.getCurrentAccount())
        {
            _selectedUserId = cur->userId;
        }
        else
        {
            _selectedUserId = accounts.front().userId;
        }
    }

    // 遍历所有账号，创建列表项
    for (const auto& a : accounts)
    {
        auto item = Layout::create();
        item->setContentSize(Size(600, 80));
        item->setTouchEnabled(true); // 允许触摸选择

        // 列表项背景 - 选中时高亮
        Color4B bgColor = (a.userId == _selectedUserId) ? Color4B(70, 70, 90, 255) : Color4B(50, 50, 70, 255);
        auto bg = LayerColor::create(bgColor);
        bg->setContentSize(Size(800, 80));
        bg->setPosition(Vec2::ZERO);
        item->addChild(bg, -1);

        // 显示账号信息：用户名 (ID)
        std::string text = a.username + " (" + a.userId + ")";
        auto name = Label::createWithSystemFont(text, "Arial", 24);
        name->setAnchorPoint(Vec2(0, 0.5));
        name->setPosition(Vec2(20, 40));
        name->setTextColor(Color4B::WHITE);
        item->addChild(name);

        // 选中标记（绿色勾）
        auto selectMarker = Label::createWithSystemFont("✓", "Arial", 28);
        selectMarker->setPosition(Vec2(700, 40));
        selectMarker->setColor(Color3B::GREEN);
        selectMarker->setName("marker");
        selectMarker->setVisible(a.userId == _selectedUserId);
        item->addChild(selectMarker);

        // 删除按钮（红色垃圾桶图标）
        auto deleteBtn = Button::create();
        deleteBtn->setTitleText("🗑️");
        deleteBtn->setTitleFontSize(28);
        deleteBtn->setContentSize(Size(50, 50));
        deleteBtn->setPosition(Vec2(750, 40));
        deleteBtn->setZoomScale(0.1f);
        deleteBtn->addClickEventListener([this, a](Ref*) {
            // 显示删除确认对话框
            showDeleteConfirmDialog(a.userId, a.username);
        });
        item->addChild(deleteBtn);

        // 使用 name 来存储 userId，避免内存管理问题
        item->setName(a.userId);

        // 行点击选择逻辑（不重建列表，只更新样式）
        item->addTouchEventListener([this](Ref* sender, ui::Widget::TouchEventType type) {
            if (type == ui::Widget::TouchEventType::ENDED)
            {
                auto w = static_cast<ui::Widget*>(sender);
                _selectedUserId = w->getName();

                // 更新所有项的选中状态
                for (int i = 0; i < _list->getItems().size(); ++i)
                {
                    auto it = _list->getItem(i);
                    bool sel = (it->getName() == _selectedUserId);
                    if (auto marker = it->getChildByName<Label*>("marker"))
                        marker->setVisible(sel);
                    for (auto child : it->getChildren())
                    {
                        if (auto layer = dynamic_cast<LayerColor*>(child))
                        {
                            layer->setColor(sel ? Color3B(70, 70, 90) : Color3B(50, 50, 70));
                        }
                    }
                }
            }
        });

        _list->pushBackCustomItem(item);
    }

    // 恢复滚动位置，避免自动回到顶部
    _list->setInnerContainerPosition(innerPos);
}

// 处理新增账号：显示创建账号对话框
void AccountSelectScene::onAddAccount()
{
    showCreateAccountDialog();
}

// 处理开始游戏：验证密码后切换账号并进入游戏场景
void AccountSelectScene::onStartGame()
{
    // 如果没有选中账号，先创建一个
    if (_selectedUserId.empty())
    {
        onAddAccount();
        return;
    }

    // 显示密码输入对话框进行验证
    showPasswordDialog(_selectedUserId);
}

// 显示创建账号对话框（分两步：先输入用户名，再输入密码）
void AccountSelectScene::showCreateAccountDialog()
{
    auto vs = Director::getInstance()->getVisibleSize();

    // 创建半透明背景遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setName("DialogMask");
    this->addChild(mask, 100);

    // 对话框背景
    auto dialogBg = LayerColor::create(Color4B(50, 50, 60, 255), 400, 300);
    dialogBg->setPosition(Vec2(vs.width / 2 - 200, vs.height / 2 - 150));
    mask->addChild(dialogBg);

    // 对话框标题
    auto title = Label::createWithSystemFont("创建新账号", "Arial", 28);
    title->setPosition(Vec2(200, 250));
    dialogBg->addChild(title);

    // 用户名输入框
    auto usernameInput = ui::TextField::create("请输入用户名", "Arial", 24);
    usernameInput->setMaxLength(20);
    usernameInput->setMaxLengthEnabled(true);
    usernameInput->setPosition(Vec2(200, 180));
    usernameInput->setContentSize(Size(300, 40));
    usernameInput->setTextColor(Color4B::WHITE);
    usernameInput->setPlaceHolderColor(Color4B::GRAY);
    usernameInput->setName("usernameInput");
    dialogBg->addChild(usernameInput);

    // 密码输入框
    auto passwordInput = ui::TextField::create("请输入密码", "Arial", 24);
    passwordInput->setMaxLength(20);
    passwordInput->setMaxLengthEnabled(true);
    passwordInput->setPasswordEnabled(true);  // 密码模式
    passwordInput->setPasswordStyleText("*");
    passwordInput->setPosition(Vec2(200, 120));
    passwordInput->setContentSize(Size(300, 40));
    passwordInput->setTextColor(Color4B::WHITE);
    passwordInput->setPlaceHolderColor(Color4B::GRAY);
    passwordInput->setName("passwordInput");
    dialogBg->addChild(passwordInput);

    // 确认按钮
    auto confirmBtn = ui::Button::create();
    confirmBtn->setTitleText("确认");
    confirmBtn->setTitleFontSize(24);
    confirmBtn->setContentSize(Size(120, 50));
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setPosition(Vec2(120, 40));
    confirmBtn->addClickEventListener([this, mask, usernameInput, passwordInput](Ref*) {
        std::string username = usernameInput->getString();
        std::string password = passwordInput->getString();

        // 验证输入不为空
        if (username.empty())
        {
            // 提示用户名不能为空
            auto tip = Label::createWithSystemFont("用户名不能为空！", "Arial", 20);
            tip->setPosition(Vec2(200, 60));
            tip->setColor(Color3B::RED);
            tip->setName("errorTip");
            
            auto existingTip = mask->getChildByName("errorTip");
            if (existingTip) existingTip->removeFromParent();
            
            mask->addChild(tip);
            return;
        }

        if (password.empty())
        {
            // 提示密码不能为空
            auto tip = Label::createWithSystemFont("密码不能为空！", "Arial", 20);
            tip->setPosition(Vec2(200, 60));
            tip->setColor(Color3B::RED);
            tip->setName("errorTip");
            
            auto existingTip = mask->getChildByName("errorTip");
            if (existingTip) existingTip->removeFromParent();
            
            mask->addChild(tip);
            return;
        }

        // 创建账号
        auto& mgr = AccountManager::getInstance();
        long long ms = static_cast<long long>(utils::getTimeInMilliseconds());
        
        AccountInfo acc;
        acc.userId = StringUtils::format("local-%lld", ms);
        acc.username = username;
        acc.password = password;
        acc.token = "";

        mgr.upsertAccount(acc);
        _selectedUserId = acc.userId;

        // 关闭对话框并刷新列表
        mask->removeFromParent();
        refreshList();
    });
    dialogBg->addChild(confirmBtn);

    // 取消按钮
    auto cancelBtn = ui::Button::create();
    cancelBtn->setTitleText("取消");
    cancelBtn->setTitleFontSize(24);
    cancelBtn->setContentSize(Size(120, 50));
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setPosition(Vec2(280, 40));
    cancelBtn->addClickEventListener([mask](Ref*) {
        mask->removeFromParent();
    });
    dialogBg->addChild(cancelBtn);
}

// 显示密码输入对话框（用于登录验证）
void AccountSelectScene::showPasswordDialog(const std::string& userId)
{
    auto vs = Director::getInstance()->getVisibleSize();

    // 创建半透明背景遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setName("PasswordDialogMask");
    this->addChild(mask, 100);

    // 对话框背景
    auto dialogBg = LayerColor::create(Color4B(50, 50, 60, 255), 400, 250);
    dialogBg->setPosition(Vec2(vs.width / 2 - 200, vs.height / 2 - 125));
    mask->addChild(dialogBg);

    // 对话框标题
    auto title = Label::createWithSystemFont("请输入密码", "Arial", 28);
    title->setPosition(Vec2(200, 200));
    dialogBg->addChild(title);

    // 密码输入框
    auto passwordInput = ui::TextField::create("密码", "Arial", 24);
    passwordInput->setMaxLength(20);
    passwordInput->setMaxLengthEnabled(true);
    passwordInput->setPasswordEnabled(true);  // 密码模式
    passwordInput->setPasswordStyleText("*");
    passwordInput->setPosition(Vec2(200, 140));
    passwordInput->setContentSize(Size(300, 40));
    passwordInput->setTextColor(Color4B::WHITE);
    passwordInput->setPlaceHolderColor(Color4B::GRAY);
    passwordInput->setName("passwordInput");
    dialogBg->addChild(passwordInput);

    // 确认按钮
    auto confirmBtn = ui::Button::create();
    confirmBtn->setTitleText("确认");
    confirmBtn->setTitleFontSize(24);
    confirmBtn->setContentSize(Size(120, 50));
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setPosition(Vec2(120, 40));
    confirmBtn->addClickEventListener([this, mask, passwordInput, userId](Ref*) {
        std::string password = passwordInput->getString();

        // 验证密码
        auto& mgr = AccountManager::getInstance();
        if (mgr.verifyPassword(userId, password))
        {
            // 密码正确，切换账号并进入游戏
            mgr.switchAccount(userId);
            mask->removeFromParent();

            auto scene = DraggableMapScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
        }
        else
        {
            // 密码错误，显示提示
            auto tip = Label::createWithSystemFont("密码错误！", "Arial", 20);
            tip->setPosition(Vec2(200, 80));
            tip->setColor(Color3B::RED);
            tip->setName("errorTip");
            
            auto existingTip = mask->getChildByName("errorTip");
            if (existingTip) existingTip->removeFromParent();
            
            mask->addChild(tip);
        }
    });
    dialogBg->addChild(confirmBtn);

    // 取消按钮
    auto cancelBtn = ui::Button::create();
    cancelBtn->setTitleText("取消");
    cancelBtn->setTitleFontSize(24);
    cancelBtn->setContentSize(Size(120, 50));
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setPosition(Vec2(280, 40));
    cancelBtn->addClickEventListener([mask](Ref*) {
        mask->removeFromParent();
    });
    dialogBg->addChild(cancelBtn);
}

// 显示删除账号确认对话框
void AccountSelectScene::showDeleteConfirmDialog(const std::string& userId, const std::string& username)
{
    auto vs = Director::getInstance()->getVisibleSize();

    // 创建半透明背景遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setName("DeleteDialogMask");
    this->addChild(mask, 100);

    // 对话框背景
    auto dialogBg = LayerColor::create(Color4B(50, 50, 60, 255), 450, 280);
    dialogBg->setPosition(Vec2(vs.width / 2 - 225, vs.height / 2 - 140));
    mask->addChild(dialogBg);

    // 警告图标
    auto warningIcon = Label::createWithSystemFont("⚠️", "Arial", 48);
    warningIcon->setPosition(Vec2(225, 230));
    warningIcon->setColor(Color3B::ORANGE);
    dialogBg->addChild(warningIcon);

    // 对话框标题
    auto title = Label::createWithSystemFont("确认删除账号", "Arial", 28);
    title->setPosition(Vec2(225, 180));
    title->setColor(Color3B::RED);
    dialogBg->addChild(title);

    // 账号信息
    std::string infoText = StringUtils::format("账号: %s", username.c_str());
    auto accountInfo = Label::createWithSystemFont(infoText, "Arial", 20);
    accountInfo->setPosition(Vec2(225, 145));
    accountInfo->setTextColor(Color4B::WHITE);
    dialogBg->addChild(accountInfo);

    // 警告文本
    auto warningText = Label::createWithSystemFont("此操作不可恢复！", "Arial", 18);
    warningText->setPosition(Vec2(225, 115));
    warningText->setColor(Color3B::RED);
    dialogBg->addChild(warningText);

    auto detailText = Label::createWithSystemFont("账号的所有数据将被永久删除", "Arial", 16);
    detailText->setPosition(Vec2(225, 90));
    detailText->setTextColor(Color4B(200, 200, 200, 255));
    dialogBg->addChild(detailText);

    // 确认删除按钮（红色）
    auto confirmBtn = ui::Button::create();
    confirmBtn->setTitleText("确认删除");
    confirmBtn->setTitleFontSize(24);
    confirmBtn->setContentSize(Size(150, 50));
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setPosition(Vec2(140, 35));
    
    // 设置红色背景
    auto confirmBg = LayerColor::create(Color4B(200, 50, 50, 255), 150, 50);
    confirmBg->setPosition(Vec2(-75, -25));
    confirmBtn->addChild(confirmBg, -1);
    
    confirmBtn->addClickEventListener([this, mask, userId, username](Ref*) {
        // 执行删除操作
        auto& mgr = AccountManager::getInstance();
        
        if (mgr.deleteAccount(userId))
        {
            CCLOG("✅ Account deleted successfully: %s", userId.c_str());
            
            // 如果删除的是当前选中的账号，清除选中状态
            if (_selectedUserId == userId)
            {
                _selectedUserId.clear();
            }
            
            // 关闭对话框
            mask->removeFromParent();
            
            // 刷新列表
            refreshList();
            
            // 显示成功提示
            auto hint = Label::createWithSystemFont(
                StringUtils::format("账号 '%s' 已删除", username.c_str()), 
                "Arial", 
                24
            );
            hint->setPosition(Vec2(Director::getInstance()->getVisibleSize().width / 2, 200));
            hint->setColor(Color3B::GREEN);
            this->addChild(hint, 200);
            
            hint->runAction(Sequence::create(
                FadeIn::create(0.2f),
                DelayTime::create(2.0f),
                FadeOut::create(0.5f),
                RemoveSelf::create(),
                nullptr
            ));
        }
        else
        {
            CCLOG("❌ Failed to delete account: %s", userId.c_str());
            
            // 显示错误提示
            auto errorTip = Label::createWithSystemFont("删除失败！", "Arial", 20);
            errorTip->setPosition(Vec2(225, 50));
            errorTip->setColor(Color3B::RED);
            errorTip->setName("errorTip");
            
            auto existingTip = mask->getChildByName("errorTip");
            if (existingTip) existingTip->removeFromParent();
            
            mask->addChild(errorTip);
        }
    });
    dialogBg->addChild(confirmBtn);

    // 取消按钮（灰色）
    auto cancelBtn = ui::Button::create();
    cancelBtn->setTitleText("取消");
    cancelBtn->setTitleFontSize(24);
    cancelBtn->setContentSize(Size(150, 50));
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setPosition(Vec2(310, 35));
    cancelBtn->addClickEventListener([mask](Ref*) {
        mask->removeFromParent();
    });
    dialogBg->addChild(cancelBtn);
    
    // 添加弹出动画
    dialogBg->setScale(0.0f);
    dialogBg->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}
