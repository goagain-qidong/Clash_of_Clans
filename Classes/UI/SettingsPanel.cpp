/**
 * @file SettingsPanel.cpp
 * @brief 游戏设置面板实现
 */

#include "SettingsPanel.h"
#include "AccountManager.h"
#include "ResourceManager.h"
#include "Managers/GlobalAudioManager.h"

USING_NS_CC;
using namespace ui;

bool SettingsPanel::init()
{
    if (!Node::init())
        return false;
    
    _visibleSize = Director::getInstance()->getVisibleSize();
    
    setupUI();
    loadVolumeSettings();
    
    return true;
}

void SettingsPanel::setupUI()
{
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setContentSize(_visibleSize);
    this->addChild(mask, -1);
    
    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [this](Touch* touch, Event* event) {
        this->hide();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, mask);
    
    _panel = Layout::create();
    _panel->setContentSize(Size(600, 600));
    _panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _panel->setBackGroundColor(Color3B(40, 40, 50));
    _panel->setBackGroundColorOpacity(255);
    _panel->setAnchorPoint(Vec2(0.5f, 0.5f));
    _panel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height / 2));
    this->addChild(_panel, 10);
    
    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch* touch, Event* event) {
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, _panel);
    
    auto titleLabel = Label::createWithSystemFont("⚙ 游戏设置", "Microsoft YaHei", 32);
    titleLabel->setPosition(Vec2(300, 560));
    titleLabel->setTextColor(Color4B::YELLOW);
    _panel->addChild(titleLabel);
    
    _closeButton = Button::create();
    _closeButton->setTitleText("X");
    _closeButton->setTitleFontSize(30);
    _closeButton->setPosition(Vec2(560, 560));
    _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });
    _panel->addChild(_closeButton);
    
    setupVolumeControls(480);
    setupFunctionButtons(320);
}

void SettingsPanel::setupVolumeControls(float startY)
{
    // ==================== 音乐音量 ====================
    auto musicLabel = Label::createWithSystemFont("🎵 音乐音量:", "Microsoft YaHei", 24);
    musicLabel->setPosition(Vec2(100, startY));
    musicLabel->setAnchorPoint(Vec2(0, 0.5f));
    _panel->addChild(musicLabel);
    
    // 🎨 创建自定义滑动条背景（使用纯色 LayerColor）
    auto musicBarBg = LayerColor::create(Color4B(80, 80, 80, 255), 250, 10);
    musicBarBg->setPosition(Vec2(280, startY - 5));
    _panel->addChild(musicBarBg);
    
    // 🎨 创建进度条
    auto musicBarProgress = LayerColor::create(Color4B(50, 205, 50, 255), 250, 10);
    musicBarProgress->setPosition(Vec2(280, startY - 5));
    musicBarProgress->setName("musicProgress");
    _panel->addChild(musicBarProgress);
    
    // 🎨 创建滑块（圆形按钮）
    auto musicThumb = LayerColor::create(Color4B(255, 255, 255, 255), 20, 20);
    musicThumb->setPosition(Vec2(530, startY - 10));
    musicThumb->setName("musicThumb");
    _panel->addChild(musicThumb);
    
    // 🎮 创建透明的触摸响应层（覆盖整个滑动条区域）
    auto musicTouchLayer = LayerColor::create(Color4B(0, 0, 0, 1), 250, 30);  // 几乎透明
    musicTouchLayer->setPosition(Vec2(280, startY - 15));
    _panel->addChild(musicTouchLayer, 10);
    
    // 添加触摸监听器
    auto musicTouchListener = EventListenerTouchOneByOne::create();
    musicTouchListener->setSwallowTouches(true);
    musicTouchListener->onTouchBegan = [this, musicBarProgress, musicThumb, musicTouchLayer](Touch* touch, Event* event) {
        Vec2 localPos = musicTouchLayer->convertToNodeSpace(touch->getLocation());
        Rect rect(Vec2::ZERO, musicTouchLayer->getContentSize());
        if (rect.containsPoint(localPos))
        {
            // 计算百分比
            float percent = (localPos.x / 250.0f) * 100.0f;
            percent = std::max(0.0f, std::min(100.0f, percent));
            
            // 更新进度条
            musicBarProgress->setContentSize(Size(250 * percent / 100.0f, 10));
            musicThumb->setPositionX(280 + 250 * percent / 100.0f - 10);
            _musicValueLabel->setString(StringUtils::format("%.0f%%", percent));
            
            // 🎵 设置音乐音量（通过全局管理器）
            GlobalAudioManager::getInstance().setMusicVolume(percent / 100.0f);
            
            return true;
        }
        return false;
    };
    musicTouchListener->onTouchMoved = [this, musicBarProgress, musicThumb, musicTouchLayer](Touch* touch, Event* event) {
        Vec2 localPos = musicTouchLayer->convertToNodeSpace(touch->getLocation());
        
        // 计算百分比（允许超出范围但限制在0-100）
        float percent = (localPos.x / 250.0f) * 100.0f;
        percent = std::max(0.0f, std::min(100.0f, percent));
        
        // 更新进度条
        musicBarProgress->setContentSize(Size(250 * percent / 100.0f, 10));
        musicThumb->setPositionX(280 + 250 * percent / 100.0f - 10);
        _musicValueLabel->setString(StringUtils::format("%.0f%%", percent));
        
        // 🎵 设置音乐音量
        GlobalAudioManager::getInstance().setMusicVolume(percent / 100.0f);
    };
    musicTouchListener->onTouchEnded = [this](Touch* touch, Event* event) {
        saveVolumeSettings();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(musicTouchListener, musicTouchLayer);
    
    // 保存slider的百分比（用于加载设置）
    _musicSlider = Slider::create();
    _musicSlider->setPercent(100);
    _musicSlider->setVisible(false);  // 隐藏，只用于存储值
    _panel->addChild(_musicSlider);
    
    _musicValueLabel = Label::createWithSystemFont("100%", "Arial", 20);
    _musicValueLabel->setPosition(Vec2(550, startY));
    _panel->addChild(_musicValueLabel);
    
    // ==================== 音效音量 ====================
    auto sfxLabel = Label::createWithSystemFont("🔊 音效音量:", "Microsoft YaHei", 24);
    sfxLabel->setPosition(Vec2(100, startY - 70));
    sfxLabel->setAnchorPoint(Vec2(0, 0.5f));
    _panel->addChild(sfxLabel);
    
    // 🎨 创建自定义滑动条背景
    auto sfxBarBg = LayerColor::create(Color4B(80, 80, 80, 255), 250, 10);
    sfxBarBg->setPosition(Vec2(280, startY - 75));
    _panel->addChild(sfxBarBg);
    
    // 🎨 创建进度条
    auto sfxBarProgress = LayerColor::create(Color4B(30, 144, 255, 255), 250, 10);
    sfxBarProgress->setPosition(Vec2(280, startY - 75));
    sfxBarProgress->setName("sfxProgress");
    _panel->addChild(sfxBarProgress);
    
    // 🎨 创建滑块
    auto sfxThumb = LayerColor::create(Color4B(255, 255, 255, 255), 20, 20);
    sfxThumb->setPosition(Vec2(530, startY - 80));
    sfxThumb->setName("sfxThumb");
    _panel->addChild(sfxThumb);
    
    // 🎮 创建透明的触摸响应层（音效）
    auto sfxTouchLayer = LayerColor::create(Color4B(0, 0, 0, 1), 250, 30);
    sfxTouchLayer->setPosition(Vec2(280, startY - 85));
    _panel->addChild(sfxTouchLayer, 10);
    
    // 添加触摸监听器
    auto sfxTouchListener = EventListenerTouchOneByOne::create();
    sfxTouchListener->setSwallowTouches(true);
    sfxTouchListener->onTouchBegan = [this, sfxBarProgress, sfxThumb, sfxTouchLayer](Touch* touch, Event* event) {
        Vec2 localPos = sfxTouchLayer->convertToNodeSpace(touch->getLocation());
        Rect rect(Vec2::ZERO, sfxTouchLayer->getContentSize());
        if (rect.containsPoint(localPos))
        {
            // 计算百分比
            float percent = (localPos.x / 250.0f) * 100.0f;
            percent = std::max(0.0f, std::min(100.0f, percent));
            
            // 更新进度条
            sfxBarProgress->setContentSize(Size(250 * percent / 100.0f, 10));
            sfxThumb->setPositionX(280 + 250 * percent / 100.0f - 10);
            _sfxValueLabel->setString(StringUtils::format("%.0f%%", percent));
            
            // 🔊 设置音效音量（通过全局管理器）
            GlobalAudioManager::getInstance().setEffectVolume(percent / 100.0f);
            
            return true;
        }
        return false;
    };
    sfxTouchListener->onTouchMoved = [this, sfxBarProgress, sfxThumb, sfxTouchLayer](Touch* touch, Event* event) {
        Vec2 localPos = sfxTouchLayer->convertToNodeSpace(touch->getLocation());
        
        // 计算百分比
        float percent = (localPos.x / 250.0f) * 100.0f;
        percent = std::max(0.0f, std::min(100.0f, percent));
        
        // 更新进度条
        sfxBarProgress->setContentSize(Size(250 * percent / 100.0f, 10));
        sfxThumb->setPositionX(280 + 250 * percent / 100.0f - 10);
        _sfxValueLabel->setString(StringUtils::format("%.0f%%", percent));
        
        // 🔊 设置音效音量
        GlobalAudioManager::getInstance().setEffectVolume(percent / 100.0f);
    };
    sfxTouchListener->onTouchEnded = [this](Touch* touch, Event* event) {
        saveVolumeSettings();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(sfxTouchListener, sfxTouchLayer);
    
    // 保存slider的百分比
    _sfxSlider = Slider::create();
    _sfxSlider->setPercent(100);
    _sfxSlider->setVisible(false);
    _panel->addChild(_sfxSlider);
    
    _sfxValueLabel = Label::createWithSystemFont("100%", "Arial", 20);
    _sfxValueLabel->setPosition(Vec2(550, startY - 70));
    _panel->addChild(_sfxValueLabel);
}

void SettingsPanel::setupFunctionButtons(float startY)
{
    auto createButton = [this](const std::string& title, float y) -> Button* {
        auto btn = Button::create();
        btn->setTitleText(title);
        btn->setTitleFontSize(22);
        btn->setTitleFontName("Microsoft YaHei");
        btn->setContentSize(Size(450, 55));
        btn->setPosition(Vec2(300, y));
        btn->setZoomScale(0.05f);
        _panel->addChild(btn);
        return btn;
    };
    
    // 地图切换按钮
    _mapSwitchButton = createButton("🗺️ 切换地图", startY);
    _mapSwitchButton->addClickEventListener([this](Ref*) { onMapSwitchClicked(); });
    
    _accountSwitchButton = createButton("👤 切换账号", startY - 70);
    _accountSwitchButton->addClickEventListener([this](Ref*) { onAccountSwitchClicked(); });
    
    _logoutButton = createButton("🚪 退出游戏", startY - 140);
    _logoutButton->addClickEventListener([this](Ref*) { onLogoutClicked(); });
    
    _fullResourceButton = createButton("💰 资源全满 (测试)", startY - 210);
    _fullResourceButton->addClickEventListener([this](Ref*) { onFullResourceClicked(); });
}

void SettingsPanel::onCloseClicked()
{
    hide();
}

void SettingsPanel::onMusicVolumeChanged(Ref* sender, Slider::EventType type)
{
    if (type == Slider::EventType::ON_PERCENTAGE_CHANGED)
    {
        int volume = _musicSlider->getPercent();
        _musicValueLabel->setString(StringUtils::format("%d%%", volume));
        
        float audioVolume = volume / 100.0f;
        AudioEngine::setVolume(AudioEngine::INVALID_AUDIO_ID, audioVolume);
        
        saveVolumeSettings();
    }
}

void SettingsPanel::onSFXVolumeChanged(Ref* sender, Slider::EventType type)
{
    if (type == Slider::EventType::ON_PERCENTAGE_CHANGED)
    {
        int volume = _sfxSlider->getPercent();
        _sfxValueLabel->setString(StringUtils::format("%d%%", volume));
        saveVolumeSettings();
    }
}

void SettingsPanel::onAccountSwitchClicked()
{
    showAccountList();
}

void SettingsPanel::onLogoutClicked()
{
    auto& accMgr = AccountManager::getInstance();
    accMgr.signOut();
    
    if (_onLogout)
    {
        _onLogout();
    }
}

void SettingsPanel::onFullResourceClicked()
{
    CCLOG("📊 点击了资源全满按钮");
    
    // 调用 ResourceManager 的新方法
    auto& resMgr = ResourceManager::getInstance();
    resMgr.fillAllResourcesMax();
    
    // 显示提示信息
    auto hint = Label::createWithSystemFont("✅ 资源已全满！", "Microsoft YaHei", 24);
    hint->setPosition(Vec2(300, 50));
    hint->setTextColor(Color4B::GREEN);
    hint->setOpacity(0);
    _panel->addChild(hint);
    
    // 播放提示动画
    hint->runAction(Sequence::create(
        FadeIn::create(0.2f),
        DelayTime::create(2.0f),
        FadeOut::create(0.3f),
        RemoveSelf::create(),
        nullptr
    ));
    
    CCLOG("✅ 资源全满提示已显示");
}

void SettingsPanel::onMapSwitchClicked()
{
    showMapSelectionPanel();
}

void SettingsPanel::showMapSelectionPanel()
{
    auto& accMgr = AccountManager::getInstance();
    const auto* currentAccount = accMgr.getCurrentAccount();
    
    if (!currentAccount)
    {
        CCLOG("No current account");
        return;
    }
    
    auto mapPanel = Layout::create();
    mapPanel->setContentSize(Size(500, 350));
    mapPanel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    mapPanel->setBackGroundColor(Color3B(30, 30, 40));
    mapPanel->setBackGroundColorOpacity(255);
    mapPanel->setPosition(Vec2(50, 150));
    _panel->addChild(mapPanel, 100);
    
    auto titleLabel = Label::createWithSystemFont("选择地图", "Microsoft YaHei", 24);
    titleLabel->setPosition(Vec2(250, 320));
    mapPanel->addChild(titleLabel);
    
    auto closeBtn = Button::create();
    closeBtn->setTitleText("X");
    closeBtn->setTitleFontSize(24);
    closeBtn->setPosition(Vec2(470, 320));
    closeBtn->addClickEventListener([mapPanel](Ref*) {
        mapPanel->removeFromParent();
    });
    mapPanel->addChild(closeBtn);
    
    // 地图选项
    struct MapOption {
        std::string name;
        std::string path;
        std::string description;
    };
    
    std::vector<MapOption> maps = {
        {"地图 1", "map/Map1.png", "夜世界"},
        {"地图 2", "map/Map2.png", "雪地冰原"},
        {"地图 3", "map/Map3.png", "嫩绿草原"}
    };
    
    std::string currentMap = currentAccount->assignedMapName;
    
    float startY = 250.0f;
    for (size_t i = 0; i < maps.size(); ++i)
    {
        const auto& mapOption = maps[i];
        
        auto itemLayout = Layout::create();
        itemLayout->setContentSize(Size(460, 70));
        itemLayout->setPosition(Vec2(20, startY - 70 - i * 80));
        
        bool isCurrent = (mapOption.path == currentMap);
        Color3B bgColor = isCurrent ? Color3B(60, 100, 60) : Color3B(50, 50, 60);
        auto bg = LayerColor::create(Color4B(bgColor.r, bgColor.g, bgColor.b, 255), 460, 70);
        itemLayout->addChild(bg);
        
        std::string labelText = mapOption.name + " - " + mapOption.description;
        if (isCurrent)
        {
            labelText += " (当前)";
        }
        
        auto nameLabel = Label::createWithSystemFont(labelText, "Microsoft YaHei", 20);
        nameLabel->setPosition(Vec2(230, 45));
        itemLayout->addChild(nameLabel);
        
        auto descLabel = Label::createWithSystemFont(mapOption.path, "Arial", 14);
        descLabel->setPosition(Vec2(230, 20));
        descLabel->setTextColor(Color4B(200, 200, 200, 255));
        itemLayout->addChild(descLabel);
        
        if (!isCurrent)
        {
            itemLayout->setTouchEnabled(true);
            itemLayout->addClickEventListener([this, mapOption, mapPanel](Ref*) {
                CCLOG("✅ Switching to map: %s", mapOption.path.c_str());
                
                auto& accMgr = AccountManager::getInstance();
                const auto* account = accMgr.getCurrentAccount();
                if (account)
                {
                    // 更新账号的地图设置
                    AccountInfo updatedAccount = *account;
                    updatedAccount.assignedMapName = mapOption.path;
                    accMgr.upsertAccount(updatedAccount);
                    
                    // 关闭面板
                    mapPanel->removeFromParent();
                    
                    // 显示提示
                    auto hint = Label::createWithSystemFont(
                        "地图已切换！请重新进入游戏生效", 
                        "Microsoft YaHei", 
                        24
                    );
                    hint->setPosition(Vec2(300, 50));
                    hint->setTextColor(Color4B::GREEN);
                    _panel->addChild(hint);
                    
                    hint->runAction(Sequence::create(
                        FadeIn::create(0.2f),
                        DelayTime::create(2.0f),
                        FadeOut::create(0.3f),
                        RemoveSelf::create(),
                        nullptr
                    ));
                    
                    // 触发场景重新加载（通过账号切换逻辑）
                    if (_onMapChanged)
                    {
                        _onMapChanged(mapOption.path);
                    }
                }
            });
        }
        
        mapPanel->addChild(itemLayout);
    }
}

void SettingsPanel::loadVolumeSettings()
{
    // 从全局音频管理器读取音量
    auto& audioMgr = GlobalAudioManager::getInstance();
    float musicVolume = audioMgr.getMusicVolume() * 100.0f;
    float sfxVolume = audioMgr.getEffectVolume() * 100.0f;
    
    if (_musicSlider)
    {
        _musicSlider->setPercent(static_cast<int>(musicVolume));
        _musicValueLabel->setString(StringUtils::format("%.0f%%", musicVolume));
        
        // 更新视觉效果
        auto musicProgress = _panel->getChildByName("musicProgress");
        auto musicThumb = _panel->getChildByName("musicThumb");
        if (musicProgress)
        {
            dynamic_cast<LayerColor*>(musicProgress)->setContentSize(Size(250 * musicVolume / 100.0f, 10));
        }
        if (musicThumb)
        {
            musicThumb->setPositionX(280 + 250 * musicVolume / 100.0f - 10);
        }
    }
    
    if (_sfxSlider)
    {
        _sfxSlider->setPercent(static_cast<int>(sfxVolume));
        _sfxValueLabel->setString(StringUtils::format("%.0f%%", sfxVolume));
        
        // 更新视觉效果
        auto sfxProgress = _panel->getChildByName("sfxProgress");
        auto sfxThumb = _panel->getChildByName("sfxThumb");
        if (sfxProgress)
        {
            dynamic_cast<LayerColor*>(sfxProgress)->setContentSize(Size(250 * sfxVolume / 100.0f, 10));
        }
        if (sfxThumb)
        {
            sfxThumb->setPositionX(280 + 250 * sfxVolume / 100.0f - 10);
        }
    }
}

void SettingsPanel::saveVolumeSettings()
{
    // 全局音频管理器会自动保存设置
}

void SettingsPanel::showAccountList()
{
    auto& accMgr = AccountManager::getInstance();
    const auto& accounts = accMgr.listAccounts();
    const auto* currentAccount = accMgr.getCurrentAccount();
    
    if (accounts.empty())
    {
        CCLOG("No accounts available");
        return;
    }
    
    auto accountPanel = Layout::create();
    accountPanel->setContentSize(Size(400, 400));
    accountPanel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    accountPanel->setBackGroundColor(Color3B(30, 30, 40));
    accountPanel->setBackGroundColorOpacity(255);
    accountPanel->setPosition(Vec2(100, 100));
    _panel->addChild(accountPanel, 100);
    
    auto titleLabel = Label::createWithSystemFont("选择账号", "Microsoft YaHei", 24);
    titleLabel->setPosition(Vec2(200, 370));
    accountPanel->addChild(titleLabel);
    
    auto closeBtn = Button::create();
    closeBtn->setTitleText("X");
    closeBtn->setTitleFontSize(24);
    closeBtn->setPosition(Vec2(370, 370));
    closeBtn->addClickEventListener([accountPanel](Ref*) {
        accountPanel->removeFromParent();
    });
    accountPanel->addChild(closeBtn);
    
    auto scrollView = ListView::create();
    scrollView->setDirection(ScrollView::Direction::VERTICAL);
    scrollView->setContentSize(Size(360, 300));
    scrollView->setPosition(Vec2(20, 20));
    scrollView->setScrollBarEnabled(true);
    accountPanel->addChild(scrollView);
    
    for (const auto& account : accounts)
    {
        auto itemLayout = Layout::create();
        itemLayout->setContentSize(Size(340, 60));
        
        bool isCurrent = currentAccount && (account.userId == currentAccount->userId);
        Color3B bgColor = isCurrent ? Color3B(60, 100, 60) : Color3B(50, 50, 60);
        auto bg = LayerColor::create(Color4B(bgColor.r, bgColor.g, bgColor.b, 255), 340, 60);
        itemLayout->addChild(bg);
        
        std::string labelText = account.username;
        if (isCurrent)
        {
            labelText += " (当前)";
        }
        
        auto nameLabel = Label::createWithSystemFont(labelText, "Microsoft YaHei", 20);
        nameLabel->setPosition(Vec2(170, 30));
        itemLayout->addChild(nameLabel);
        
        if (!isCurrent)
        {
            itemLayout->setTouchEnabled(true);
            itemLayout->addClickEventListener([this, account, accountPanel](Ref*) {
                CCLOG("✅ Preparing to switch to account: %s", account.username.c_str());
                
                // 关闭账号选择面板
                accountPanel->removeFromParent();
                
                // 显示密码验证对话框
                showPasswordDialog(account.userId, account.username);
            });
        }
        
        scrollView->pushBackCustomItem(itemLayout);
    }
}

void SettingsPanel::show()
{
    this->setVisible(true);
    _panel->setScale(0.0f);
    _panel->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}

void SettingsPanel::hide()
{
    auto scaleOut = ScaleTo::create(0.2f, 0.0f);
    auto callback = CallFunc::create([this]() {
        this->removeFromParent();
    });
    _panel->runAction(Sequence::create(scaleOut, callback, nullptr));
}

void SettingsPanel::showPasswordDialog(const std::string& userId, const std::string& username)
{
    // 创建半透明背景遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setContentSize(_visibleSize);
    mask->setName("PasswordDialogMask");
    this->addChild(mask, 200);
    
    // 对话框背景
    auto dialogBg = LayerColor::create(Color4B(50, 50, 60, 255), 400, 250);
    dialogBg->setPosition(Vec2(_visibleSize.width / 2 - 200, _visibleSize.height / 2 - 125));
    mask->addChild(dialogBg);
    
    // 对话框标题
    std::string titleText = StringUtils::format("切换到账号: %s", username.c_str());
    auto title = Label::createWithSystemFont(titleText, "Microsoft YaHei", 24);
    title->setPosition(Vec2(200, 210));
    dialogBg->addChild(title);
    
    auto subtitle = Label::createWithSystemFont("请输入密码", "Microsoft YaHei", 20);
    subtitle->setPosition(Vec2(200, 180));
    subtitle->setTextColor(Color4B(200, 200, 200, 255));
    dialogBg->addChild(subtitle);
    
    // 密码输入框
    auto passwordInput = TextField::create("密码", "Arial", 24);
    passwordInput->setMaxLength(20);
    passwordInput->setMaxLengthEnabled(true);
    passwordInput->setPasswordEnabled(true);  // 密码模式
    passwordInput->setPasswordStyleText("*");
    passwordInput->setPosition(Vec2(200, 130));
    passwordInput->setContentSize(Size(300, 40));
    passwordInput->setTextColor(Color4B::WHITE);
    passwordInput->setPlaceHolderColor(Color4B::GRAY);
    passwordInput->setName("passwordInput");
    dialogBg->addChild(passwordInput);
    
    // 错误提示标签（初始隐藏）
    auto errorTip = Label::createWithSystemFont("", "Microsoft YaHei", 18);
    errorTip->setPosition(Vec2(200, 90));
    errorTip->setTextColor(Color4B::RED);
    errorTip->setName("errorTip");
    errorTip->setVisible(false);
    dialogBg->addChild(errorTip);
    
    // 确认按钮
    auto confirmBtn = Button::create();
    confirmBtn->setTitleText("确认");
    confirmBtn->setTitleFontSize(24);
    confirmBtn->setTitleFontName("Microsoft YaHei");
    confirmBtn->setContentSize(Size(120, 50));
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setPosition(Vec2(120, 40));
    confirmBtn->addClickEventListener([this, mask, passwordInput, errorTip, userId](Ref*) {
        std::string password = passwordInput->getString();
        
        if (password.empty())
        {
            errorTip->setString("密码不能为空！");
            errorTip->setVisible(true);
            return;
        }
        
        // 验证密码
        auto& accMgr = AccountManager::getInstance();
        if (accMgr.verifyPassword(userId, password))
        {
            // 密码正确，执行切换
            mask->removeFromParent();
            
            // 保存目标账号ID并触发切换
            UserDefault::getInstance()->setStringForKey("switching_to_account", userId);
            UserDefault::getInstance()->flush();
            
            if (_onAccountSwitched)
            {
                _onAccountSwitched();
            }
        }
        else
        {
            // 密码错误
            errorTip->setString("密码错误！请重试");
            errorTip->setVisible(true);
            
            // 清空输入框
            passwordInput->setString("");
            
            // 播放错误动画
            auto shake = Sequence::create(
                MoveBy::create(0.05f, Vec2(-5, 0)),
                MoveBy::create(0.05f, Vec2(10, 0)),
                MoveBy::create(0.05f, Vec2(-10, 0)),
                MoveBy::create(0.05f, Vec2(10, 0)),
                MoveBy::create(0.05f, Vec2(-5, 0)),
                nullptr
            );
            passwordInput->runAction(shake);
        }
    });
    dialogBg->addChild(confirmBtn);
    
    // 取消按钮
    auto cancelBtn = Button::create();
    cancelBtn->setTitleText("取消");
    cancelBtn->setTitleFontSize(24);
    cancelBtn->setTitleFontName("Microsoft YaHei");
    cancelBtn->setContentSize(Size(120, 50));
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setPosition(Vec2(280, 40));
    cancelBtn->addClickEventListener([mask](Ref*) {
        mask->removeFromParent();
    });
    dialogBg->addChild(cancelBtn);
    
    // 添加弹出动画
    dialogBg->setScale(0.0f);
    dialogBg->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}
