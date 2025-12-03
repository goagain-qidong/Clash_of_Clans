#include "AccountSelectScene.h"

#include "DraggableMapScene.h"
#include "Managers/AccountManager.h"

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

    return true;
}

// 构建账号选择界面的UI元素
void AccountSelectScene::buildUI()
{
    auto vs = Director::getInstance()->getVisibleSize();

    // 标题文本
    auto title = Label::createWithSystemFont("选择账号", "Arial", 36);
    title->setPosition(Vec2(vs.width / 2, vs.height - 120));
    title->setTextColor(Color4B::WHITE);
    this->addChild(title, 1);

    // 账号列表容器
    _list = ListView::create();
    _list->setContentSize(Size(600, 500));
    _list->setPosition(Vec2(vs.width / 2 - 300, vs.height / 2 - 250));
    _list->setBackGroundColor(Color3B(60, 60, 60));
    _list->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _list->setBounceEnabled(true);
    _list->setScrollBarEnabled(true);
    this->addChild(_list, 1);

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
        item->setTouchEnabled(true);

        // 列表项背景
        auto bg = LayerColor::create(Color4B(50, 50, 70, 255));
        bg->setContentSize(Size(600, 80));
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
        selectMarker->setPosition(Vec2(560, 40));
        selectMarker->setColor(Color3B::GREEN);
        selectMarker->setName("marker");
        selectMarker->setVisible(a.userId == _selectedUserId);
        item->addChild(selectMarker);

        // 点击列表项选中账号
        item->addClickEventListener([this, a](Ref*) {
            _selectedUserId = a.userId;
            refreshList();
        });

        _list->pushBackCustomItem(item);
    }
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
