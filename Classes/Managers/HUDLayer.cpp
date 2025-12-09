#include "HUDLayer.h"
#include "Managers/UpgradeManager.h" // 引入升级管理器
/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     WallBuilding.cpp
 * File Function:  全新顶部资源栏
 * Author:        刘相成
 * Update Date:   2025/12/06
 * License:       MIT License
 ****************************************************************/
USING_NS_CC;

HUDLayer* HUDLayer::create() {
    HUDLayer* ret = new (std::nothrow) HUDLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HUDLayer::init() {
    if (!Layer::init()) return false;

    // 创建资源栏背景 (可选)
    // auto bg = LayerColor::create(Color4B(0,0,0,50), Director::getInstance()->getVisibleSize().width, 60);
    // bg->setPosition(0, Director::getInstance()->getVisibleSize().height - 60);
    // this->addChild(bg);

    // 从右向左排列：宝石 -> 圣水 -> 金币 -> (最左侧可以放工人)
    createResourceNode(ResourceType::kGem, "icon/Gem.png", 0);
    createResourceNode(ResourceType::kElixir, "icon/Elixir.png", 1);
    createResourceNode(ResourceType::kGold, "icon/Gold.png", 2);
    // ================= 新增：显示建筑工人 =================
    // 请确保 Resources/icon/Builder.png 存在
    // 这里的 '3' 是排序索引，会排在金币的左边
    createResourceNode(ResourceType::kBuilder, "icon/Builder.png", 3);
    // ================= 新增：显示人口 =================
    createResourceNode(ResourceType::kTroopPopulation, "units/barbarian_select_button_active.png", 4);
    // ====================================================
    UpgradeManager::getInstance()->setOnAvailableBuilderChanged([this](int available) {
        this->updateDisplay();
        });
    // 注册资源变化监听
    ResourceManager::getInstance().setOnResourceChangeCallback([this](ResourceType type, int amount) {
        this->updateDisplay();
        });

    updateDisplay();
    return true;
}

void HUDLayer::createResourceNode(ResourceType type, const std::string& iconFile, int orderIndex) {
auto visibleSize = Director::getInstance()->getVisibleSize();
float topY = visibleSize.height - 30;
float rightMargin = 20;
float itemWidth = 200; // 每个资源块的宽度

// 容器节点
auto node = Node::create();
// 从右往左排
float xPos = visibleSize.width - rightMargin - (orderIndex * itemWidth) - (itemWidth / 2);
node->setPosition(Vec2(xPos, topY));
this->addChild(node);

// 资源图标
auto icon = Sprite::create(iconFile);
if (icon) {
    // 军队人口图标（野蛮人）缩小到一半大小
    float scale = (type == ResourceType::kTroopPopulation) ? 0.4f : 0.8f;
    icon->setScale(scale);
    icon->setPosition(Vec2(itemWidth / 2 - 20, 0));
    node->addChild(icon, 1);
}
else {
    // 如果找不到图片，用色块代替测试
    auto box = LayerColor::create(Color4B::MAGENTA, 30, 30);
    box->setPosition(Vec2(itemWidth / 2 - 20, -15));
    node->addChild(box, 1);
}

    // 进度条背景
    auto barBg = LayerColor::create(Color4B(0, 0, 0, 150), 140, 24);
    barBg->setPosition(Vec2(-80, -12));
    node->addChild(barBg, 0);

    // 文字 Label: 1000/3000
    auto label = Label::createWithSystemFont("0/0", "Arial", 16);
    label->setPosition(Vec2(-10, 0));
    label->setAnchorPoint(Vec2(0.5f, 0.5f));
    node->addChild(label, 2);

    _amountLabels[type] = label;
}

void HUDLayer::updateDisplay() {
    auto& rm = ResourceManager::getInstance();
    auto* um = UpgradeManager::getInstance(); // 获取升级管理器
    for (auto& pair : _amountLabels) {
        ResourceType type = pair.first;
        Label* lbl = pair.second;

        int current = rm.getResourceCount(type);
        int max = rm.getResourceCapacity(type);

        if (type == ResourceType::kGem) {
            // 宝石通常不显示上限，或者上限很大
            lbl->setString(std::to_string(current));
            lbl->setTextColor(Color4B(0, 255, 0, 255)); // 亮绿色
        }
        else if (type == ResourceType::kBuilder) {
            int totalBuilders = rm.getResourceCapacity(ResourceType::kBuilder);
            // 或者是 getResourceCount(kBuilder)，取决于你初始化的逻辑。
            // 根据之前的 ResourceManager.cpp，addCapacity(kBuilder, 1) 是加在 Capacity 上。
            // 但 init() 里 setResourceCount(kBuilder, 2)。
            // 建议：总量用 Capacity，空闲用 UpgradeManager 计算。

            int max = rm.getResourceCount(ResourceType::kBuilder); // 假设这是拥有的工人总数

            // 获取空闲工人数 (从UpgradeManager)
            int available = um->getAvailableBuilders();

            lbl->setString(StringUtils::format("建筑工人：%d / %d", available, max));

            if (available > 0) {
                lbl->setTextColor(Color4B::GREEN);
            }
            else {
                lbl->setTextColor(Color4B::RED);
            }
        }
        else if (type == ResourceType::kTroopPopulation) {
            // 人口特殊显示：当前人口/人口上限
            lbl->setString(StringUtils::format("军队人口: %d / %d", current, max));
            // 如果人口已满，显示红色；否则显示白色
            if (current >= max) {
                lbl->setTextColor(Color4B::RED);
            }
            else {
                lbl->setTextColor(Color4B::WHITE);
            }
        }
        else {
            lbl->setString(StringUtils::format("%d / %d", current, max));

            // 如果满仓，显示红色，否则白色
            if (current >= max) {
                lbl->setTextColor(Color4B::RED);
            }
            else {
                lbl->setTextColor(Color4B::WHITE);
            }
        }
    }
}