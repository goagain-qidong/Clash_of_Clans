/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     BuildingUpgradeUI.cpp
 * File Function: 通用建筑升级界面实现
 * Author:        赵崇沛
 * Update Date:   2025/12/28
 * License:       MIT License
 ****************************************************************/

#include "BuildingUpgradeUI.h"
#include "Services/BuildingUpgradeService.h" // 引入服务
#include "SceneUIController.h" // 引入场景控制器用于显示提示
#include "ResourceManager.h"
#include "ArmyBuilding.h"
#include "Unit/TrainingUI.h"

USING_NS_CC;
using namespace ui;

BuildingUpgradeUI* BuildingUpgradeUI::create(BaseBuilding* building)
{
    BuildingUpgradeUI* ui = new (std::nothrow) BuildingUpgradeUI();
    if (ui && ui->init(building))
    {
        ui->autorelease();
        return ui;
    }
    CC_SAFE_DELETE(ui);
    return nullptr;
}

bool BuildingUpgradeUI::init(BaseBuilding* building)
{
    if (!Node::init())
    {
        return false;
    }

    _building = building;
    setupUI();
    updateUI();
    return true;
}

void BuildingUpgradeUI::setupUI()
{
    // 检查是否为兵营，如果是则面板高度增加
    bool isBarracks = (_building && _building->getBuildingType() == BuildingType::kArmy);
    float panelHeight = isBarracks ? 270.0f : 220.0f;  // 兵营面板更高
    
    // 背景面板
    _panel = Layout::create();
    _panel->setContentSize(Size(280, panelHeight));
    _panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
    _panel->setBackGroundColor(Color3B(40, 40, 60));
    _panel->setBackGroundColorOpacity(230);
    _panel->setAnchorPoint(Vec2(0.5f, 0.0f));
    this->addChild(_panel);

    float panelWidth = _panel->getContentSize().width;

    // 标题
    _titleLabel = Label::createWithSystemFont("", "Microsoft YaHei", 20);
    _titleLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 25));
    _titleLabel->setTextColor(Color4B::YELLOW);
    _panel->addChild(_titleLabel);

    // 等级
    _levelLabel = Label::createWithSystemFont("", "Microsoft YaHei", 16);
    _levelLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 50));
    _levelLabel->setTextColor(Color4B::WHITE);
    _panel->addChild(_levelLabel);

    // 描述
    _descLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _descLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 85));
    _descLabel->setTextColor(Color4B(200, 200, 200, 255));
    _descLabel->setAlignment(TextHAlignment::CENTER);
    _panel->addChild(_descLabel);

    // 费用
    _costLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _costLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 120));
    _costLabel->setTextColor(Color4B::GREEN);
    _panel->addChild(_costLabel);

    // 时间
    _timeLabel = Label::createWithSystemFont("", "Microsoft YaHei", 14);
    _timeLabel->setPosition(Vec2(panelWidth / 2, panelHeight - 145));
    _timeLabel->setTextColor(Color4B(150, 200, 255, 255));
    _panel->addChild(_timeLabel);

    // ========== 按钮布局（根据是否为兵营调整） ==========
    if (isBarracks)
    {
        // 兵营：三个按钮（训练、升级、关闭）
        
        // 训练按钮（顶部）
        _trainButton = Button::create();
        _trainButton->setTitleText("🎖 训练士兵");
        _trainButton->setTitleFontSize(18);
        _trainButton->setContentSize(Size(200, 40));
        _trainButton->setPosition(Vec2(panelWidth / 2, 85));
        _trainButton->addClickEventListener([this](Ref*) { onTrainClicked(); });
        
        auto trainBg = LayerColor::create(Color4B(50, 100, 200, 200), 200, 40);
        trainBg->setPosition(Vec2(-100, -20));
        _trainButton->addChild(trainBg, -1);
        _panel->addChild(_trainButton);
        
        // 升级按钮（左下）
        _upgradeButton = Button::create();
        _upgradeButton->setTitleText("升级");
        _upgradeButton->setTitleFontSize(18);
        _upgradeButton->setContentSize(Size(100, 40));
        _upgradeButton->setPosition(Vec2(panelWidth / 2 - 60, 35));
        _upgradeButton->addClickEventListener([this](Ref*) { onUpgradeClicked(); });

        auto upgradeBg = LayerColor::create(Color4B(0, 150, 0, 200), 100, 40);
        upgradeBg->setPosition(Vec2(-50, -20));
        _upgradeButton->addChild(upgradeBg, -1);
        _panel->addChild(_upgradeButton);

        // 关闭按钮（右下）
        _closeButton = Button::create();
        _closeButton->setTitleText("关闭");
        _closeButton->setTitleFontSize(18);
        _closeButton->setContentSize(Size(100, 40));
        _closeButton->setPosition(Vec2(panelWidth / 2 + 60, 35));
        _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });

        auto closeBg = LayerColor::create(Color4B(150, 0, 0, 200), 100, 40);
        closeBg->setPosition(Vec2(-50, -20));
        _closeButton->addChild(closeBg, -1);
        _panel->addChild(_closeButton);
    }
    else
    {
        // 普通建筑：两个按钮（升级、关闭）
        
        // 升级按钮
        _upgradeButton = Button::create();
        _upgradeButton->setTitleText("升级");
        _upgradeButton->setTitleFontSize(18);
        _upgradeButton->setContentSize(Size(100, 40));
        _upgradeButton->setPosition(Vec2(panelWidth / 2 - 60, 35));
        _upgradeButton->addClickEventListener([this](Ref*) { onUpgradeClicked(); });

        auto upgradeBg = LayerColor::create(Color4B(0, 150, 0, 200), 100, 40);
        upgradeBg->setPosition(Vec2(-50, -20));
        _upgradeButton->addChild(upgradeBg, -1);
        _panel->addChild(_upgradeButton);

        // 关闭按钮
        _closeButton = Button::create();
        _closeButton->setTitleText("关闭");
        _closeButton->setTitleFontSize(18);
        _closeButton->setContentSize(Size(100, 40));
        _closeButton->setPosition(Vec2(panelWidth / 2 + 60, 35));
        _closeButton->addClickEventListener([this](Ref*) { onCloseClicked(); });

        auto closeBg = LayerColor::create(Color4B(150, 0, 0, 200), 100, 40);
        closeBg->setPosition(Vec2(-50, -20));
        _closeButton->addChild(closeBg, -1);
        _panel->addChild(_closeButton);
    }
}

// 1. 修改 updateUI：只有满级才禁用按钮，否则让用户点，点了再报错
void BuildingUpgradeUI::updateUI()
{
    if (!_building) return;

    _titleLabel->setString(_building->getDisplayName());

    // --- 满级检查 ---
    if (_building->isMaxLevel())
    {
        _levelLabel->setString(StringUtils::format("等级: %d (MAX)", _building->getLevel()));
        _costLabel->setString("已达最高等级");
        _timeLabel->setString("");
        _upgradeButton->setEnabled(false); // 只有满级才彻底禁用
        _upgradeButton->setTitleColor(Color3B::GRAY);
    }
    else
    {
        // --- 显示升级信息 ---
        _levelLabel->setString(StringUtils::format("等级: %d → %d",
            _building->getLevel(),
            _building->getLevel() + 1));

        std::string resName = getResourceTypeName(_building->getUpgradeCostType());
        _costLabel->setString(StringUtils::format("费用: %d %s",
            _building->getUpgradeCost(),
            resName.c_str()));

        // 这里直接显示10秒，或者读取配置
        _timeLabel->setString(StringUtils::format("时间: %s",
            formatTime(_building->getUpgradeTime()).c_str()));

        // --- 按钮状态逻辑 ---
        // 始终启用按钮，允许玩家点击（点击后检查条件并提示）
        _upgradeButton->setEnabled(true);
        _upgradeButton->setTitleColor(Color3B::WHITE);

        // 仅仅改变文字颜色来提示资源是否足够（视觉反馈，不阻断操作）
        bool hasResource = ResourceManager::getInstance().hasEnough(
            _building->getUpgradeCostType(),
            _building->getUpgradeCost()
        );
        _costLabel->setTextColor(hasResource ? Color4B::GREEN : Color4B::RED);
    }

    _descLabel->setString(_building->getBuildingDescription());
}

void BuildingUpgradeUI::setPositionNearBuilding(BaseBuilding* building)
{
    if (!building)
        return;

    Vec2 worldPos = building->getParent()->convertToWorldSpace(building->getPosition());
    Vec2 offset(0, building->getContentSize().height * building->getScale() * 0.5f + 20);
    this->setPosition(worldPos + offset);
}

void BuildingUpgradeUI::show()
{
    this->setVisible(true);
    this->setScale(0.0f);
    auto scaleIn = EaseBackOut::create(ScaleTo::create(0.2f, 1.0f));
    this->runAction(scaleIn);
}

void BuildingUpgradeUI::hide()
{
    auto scaleOut = EaseBackIn::create(ScaleTo::create(0.15f, 0.0f));
    auto remove = CallFunc::create([this]() {
        this->removeFromParent();
    });
    this->runAction(Sequence::create(scaleOut, remove, nullptr));
}

// 2. 修改 onUpgradeClicked：获取详细失败原因并提示
void BuildingUpgradeUI::onUpgradeClicked()
{
    if (!_building) return;

    // 直接调用 Service 的 tryUpgrade，获取详细结果
    auto result = BuildingUpgradeService::getInstance().tryUpgrade(_building);

    if (result.success)
    {
        // === 成功 ===
        // 播放音效等...

        // 通知回调
        if (_resultCallback) {
            _resultCallback(true, _building->getLevel());
        }

        // 刷新UI（通常升级开始后会关闭此窗口，或者变为"升级中"状态）
        updateUI();

        // 成功后关闭窗口
        onCloseClicked();
    }
    else
    {
        // === 失败 ===
        // 根据错误类型显示具体的提示
        std::string hintMsg = result.message; // Service已经返回了很好的错误信息

        // 如果想自定义提示，可以判断 result.error
        switch (result.error) {
        case UpgradeError::kNoAvailableBuilder:
            hintMsg = "没有空闲的建筑工人！";
            break;
        case UpgradeError::kNotEnoughGold:
            hintMsg = "金币不足，无法升级！";
            break;
        case UpgradeError::kNotEnoughElixir:
            hintMsg = "圣水不足，无法升级！";
            break;
        default:
            break;
        }

        // 调用场景控制器显示提示 (假设 SceneUIController 是单例或你能获取到)
        // 如果 SceneUIController 不是单例，你需要通过 getParent() 找到它，或者在 Scene 中设置全局访问点
        // 这里假设你在 SceneUIController 加了一个 getInstance() 或者通过场景查找
        auto scene = Director::getInstance()->getRunningScene();
        if (scene) {
            // 尝试查找 SceneUIController，根据之前的代码它是一个 Node
            // 你可能需要修改 SceneUIController 增加 getInstance() 静态方法
            // 这里演示通过 name 查找或者直接 new 一个 Label 提示

            // 简单提示方案：
            auto label = Label::createWithSystemFont(hintMsg, "Arial", 24);
            label->setPosition(Director::getInstance()->getVisibleSize() / 2);
            label->setTextColor(Color4B::RED);
            scene->addChild(label, 10000);

            // 向上飘动并消失动画
            auto move = MoveBy::create(1.0f, Vec2(0, 50));
            auto fade = FadeOut::create(1.0f);
            auto seq = Sequence::create(Spawn::create(move, fade, nullptr), RemoveSelf::create(), nullptr);
            label->runAction(seq);
        }

        CCLOG("升级失败: %s", hintMsg.c_str());
    }
}

void BuildingUpgradeUI::onCloseClicked()
{
    if (_closeCallback) _closeCallback(); // 通知拥有者先清理引用
    hide();
}

void BuildingUpgradeUI::onTrainClicked()
{
    // 检查是否为兵营建筑
    if (!_building || _building->getBuildingType() != BuildingType::kArmy)
        return;
    
    // 转换为兵营类型
    auto barracks = dynamic_cast<ArmyBuilding*>(_building);
    if (!barracks)
        return;
    
    CCLOG("打开训练UI：%s", barracks->getDisplayName().c_str());
    
    // 先关闭当前升级UI
    if (_closeCallback) _closeCallback();
    
    // 创建训练UI
    auto trainingUI = TrainingUI::create(barracks);
    if (trainingUI)
    {
        // 获取场景根节点
        auto scene = Director::getInstance()->getRunningScene();
        if (scene)
        {
            scene->addChild(trainingUI, 2000);  // 高层级显示
        }
    }
    
    // 隐藏升级UI
    hide();
}

std::string BuildingUpgradeUI::getResourceTypeName(ResourceType type) const
{
    switch (type)
    {
        case ResourceType::kGold:
            return "金币";
        case ResourceType::kElixir:
            return "圣水";
        case ResourceType::kGem:
            return "宝石";
        default:
            return "未知";
    }
}

std::string BuildingUpgradeUI::formatTime(int seconds) const
{
    if (seconds < 60)
    {
        return StringUtils::format("%d秒", seconds);
    }
    else if (seconds < 3600)
    {
        return StringUtils::format("%d分钟", seconds / 60);
    }
    else if (seconds < 86400)
    {
        return StringUtils::format("%d小时%d分", seconds / 3600, (seconds % 3600) / 60);
    }
    else
    {
        return StringUtils::format("%d天%d小时", seconds / 86400, (seconds % 86400) / 3600);
    }
}