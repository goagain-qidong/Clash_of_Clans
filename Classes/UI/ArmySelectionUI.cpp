#include "ArmySelectionUI.h"

USING_NS_CC;
using namespace ui;

ArmySelectionUI* ArmySelectionUI::create()
{
    ArmySelectionUI* ret = new (std::nothrow) ArmySelectionUI();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ArmySelectionUI::init()
{
    if (!Layer::init())
    {
        return false;
    }
    
    _visibleSize = Director::getInstance()->getVisibleSize();
    
    // 半透明背景遮罩
    auto bgMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(bgMask);
    
    // 吞噬触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    createUI();
    
    return true;
}

void ArmySelectionUI::setOnConfirmed(std::function<void()> callback)
{
    _onConfirmed = callback;
}

void ArmySelectionUI::setOnCancelled(std::function<void()> callback)
{
    _onCancelled = callback;
}

void ArmySelectionUI::createUI()
{
    // 创建容器
    auto layout = ui::Layout::create();
    layout->setContentSize(Size(500, 400));
    layout->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    layout->setBackGroundColor(Color3B(40, 40, 50));
    layout->setBackGroundColorOpacity(255);
    
    _container = layout;
    _container->setAnchorPoint(Vec2(0.5f, 0.5f));
    _container->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height / 2));
    this->addChild(_container);
    
    // 标题
    auto title = Label::createWithSystemFont("选择你的军队", "Arial", 32);
    title->setPosition(Vec2(250, 360));
    title->setTextColor(Color4B::YELLOW);
    _container->addChild(title);
    
    // 提示文本
    auto hint = Label::createWithSystemFont(
        "准备你的军队进行攻击！\n（当前版本：自动部署所有可用士兵）", 
        "Arial", 20);
    hint->setPosition(Vec2(250, 280));
    hint->setTextColor(Color4B::WHITE);
    hint->setAlignment(TextHAlignment::CENTER);
    _container->addChild(hint);
    
    // 军队信息占位符（未来可扩展）
    auto armyInfo = Label::createWithSystemFont(
        "可用士兵：自动从兵营调取\n可用英雄：暂未实现", 
        "Arial", 18);
    armyInfo->setPosition(Vec2(250, 200));
    armyInfo->setTextColor(Color4B(200, 200, 200, 255));
    armyInfo->setAlignment(TextHAlignment::CENTER);
    _container->addChild(armyInfo);
    
    // 确认按钮
    _confirmBtn = Button::create();
    _confirmBtn->setTitleText("开始攻击！");
    _confirmBtn->setTitleFontSize(28);
    _confirmBtn->setContentSize(Size(180, 70));
    _confirmBtn->setScale9Enabled(true);
    _confirmBtn->setPosition(Vec2(300, 70));
    
    // 绿色背景
    auto confirmBg = LayerColor::create(Color4B(0, 150, 0, 255), 180, 70);
    confirmBg->setPosition(Vec2(-90, -35));
    _confirmBtn->addChild(confirmBg, -1);
    
    _confirmBtn->addClickEventListener([this](Ref*) {
        if (_onConfirmed)
        {
            _onConfirmed();
        }
        hide();
    });
    _container->addChild(_confirmBtn);
    
    // 取消按钮
    _cancelBtn = Button::create();
    _cancelBtn->setTitleText("取消");
    _cancelBtn->setTitleFontSize(24);
    _cancelBtn->setContentSize(Size(140, 70));
    _cancelBtn->setScale9Enabled(true);
    _cancelBtn->setPosition(Vec2(140, 70));
    _cancelBtn->addClickEventListener([this](Ref*) {
        if (_onCancelled)
        {
            _onCancelled();
        }
        hide();
    });
    _container->addChild(_cancelBtn);
}

void ArmySelectionUI::show()
{
    this->setVisible(true);
    _container->setScale(0.0f);
    _container->runAction(EaseBackOut::create(ScaleTo::create(0.3f, 1.0f)));
}

void ArmySelectionUI::hide()
{
    auto scaleAction = ScaleTo::create(0.2f, 0.0f);
    _container->runAction(scaleAction);

    // 延迟后移除整个层（包括遮罩），而不仅仅是容器
    auto delay = DelayTime::create(0.2f);
    auto removeSelf = RemoveSelf::create();
    this->runAction(Sequence::create(delay, removeSelf, nullptr));
}
