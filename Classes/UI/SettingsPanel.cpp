/**
 * @file SettingsPanel.cpp
 * @brief 游戏设置面板实现
 */

#include "SettingsPanel.h"
#include "AccountManager.h"
#include "ResourceManager.h"
#include "audio/include/AudioEngine.h"

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
    auto musicLabel = Label::createWithSystemFont("🎵 音乐音量:", "Microsoft YaHei", 24);
    musicLabel->setPosition(Vec2(120, startY));
    musicLabel->setAnchorPoint(Vec2(0, 0.5f));
    _panel->addChild(musicLabel);
    
    _musicSlider = Slider::create();
    _musicSlider->loadBarTexture("ui/slider_back.png");
    _musicSlider->loadSlidBallTextures("ui/slider_thumb.png", "ui/slider_thumb.png", "");
    _musicSlider->loadProgressBarTexture("ui/slider_progress.png");
    _musicSlider->setPosition(Vec2(380, startY));
    _musicSlider->setPercent(100);
    _musicSlider->addEventListener(CC_CALLBACK_2(SettingsPanel::onMusicVolumeChanged, this));
    _panel->addChild(_musicSlider);
    
    _musicValueLabel = Label::createWithSystemFont("100%", "Arial", 20);
    _musicValueLabel->setPosition(Vec2(530, startY));
    _panel->addChild(_musicValueLabel);
    
    auto sfxLabel = Label::createWithSystemFont("🔊 音效音量:", "Microsoft YaHei", 24);
    sfxLabel->setPosition(Vec2(120, startY - 70));
    sfxLabel->setAnchorPoint(Vec2(0, 0.5f));
    _panel->addChild(sfxLabel);
    
    _sfxSlider = Slider::create();
    _sfxSlider->loadBarTexture("ui/slider_back.png");
    _sfxSlider->loadSlidBallTextures("ui/slider_thumb.png", "ui/slider_thumb.png", "");
    _sfxSlider->loadProgressBarTexture("ui/slider_progress.png");
    _sfxSlider->setPosition(Vec2(380, startY - 70));
    _sfxSlider->setPercent(100);
    _sfxSlider->addEventListener(CC_CALLBACK_2(SettingsPanel::onSFXVolumeChanged, this));
    _panel->addChild(_sfxSlider);
    
    _sfxValueLabel = Label::createWithSystemFont("100%", "Arial", 20);
    _sfxValueLabel->setPosition(Vec2(530, startY - 70));
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
    
    _accountSwitchButton = createButton("👤 切换账号", startY);
    _accountSwitchButton->addClickEventListener([this](Ref*) { onAccountSwitchClicked(); });
    
    _logoutButton = createButton("🚪 退出游戏", startY - 70);
    _logoutButton->addClickEventListener([this](Ref*) { onLogoutClicked(); });
    
    _fullResourceButton = createButton("💰 资源全满 (测试)", startY - 140);
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
    auto& resMgr = ResourceManager::getInstance();
    resMgr.AddResource(ResourceType::kGold, 100000);
    resMgr.AddResource(ResourceType::kElixir, 100000);
    resMgr.AddResource(ResourceType::kGem, 1000);
    
    CCLOG("✅ Resources filled to maximum!");
    
    auto hint = Label::createWithSystemFont("资源已全满！", "Microsoft YaHei", 24);
    hint->setPosition(Vec2(300, 50));
    hint->setTextColor(Color4B::GREEN);
    _panel->addChild(hint);
    
    hint->runAction(Sequence::create(
        FadeIn::create(0.2f),
        DelayTime::create(1.5f),
        FadeOut::create(0.3f),
        RemoveSelf::create(),
        nullptr
    ));
}

void SettingsPanel::loadVolumeSettings()
{
    auto userDefault = UserDefault::getInstance();
    int musicVolume = userDefault->getIntegerForKey("MusicVolume", 100);
    int sfxVolume = userDefault->getIntegerForKey("SFXVolume", 100);
    
    if (_musicSlider)
    {
        _musicSlider->setPercent(musicVolume);
        _musicValueLabel->setString(StringUtils::format("%d%%", musicVolume));
        AudioEngine::setVolume(AudioEngine::INVALID_AUDIO_ID, musicVolume / 100.0f);
    }
    
    if (_sfxSlider)
    {
        _sfxSlider->setPercent(sfxVolume);
        _sfxValueLabel->setString(StringUtils::format("%d%%", sfxVolume));
    }
}

void SettingsPanel::saveVolumeSettings()
{
    auto userDefault = UserDefault::getInstance();
    userDefault->setIntegerForKey("MusicVolume", _musicSlider->getPercent());
    userDefault->setIntegerForKey("SFXVolume", _sfxSlider->getPercent());
    userDefault->flush();
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
                
                // 关闭账号面板
                accountPanel->removeFromParent();
                
                // 先触发回调，让场景保存当前状态并切换
                // 传递目标账号的 userId
                if (_onAccountSwitched)
                {
                    // 注意：这里需要修改回调签名来传递目标账号ID
                    // 但为了不改动太多代码，我们在这里直接保存当前账号ID
                    UserDefault::getInstance()->setStringForKey("switching_to_account", account.userId);
                    UserDefault::getInstance()->flush();
                    
                    _onAccountSwitched();
                }
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
