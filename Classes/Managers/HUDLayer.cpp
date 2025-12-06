#include "HUDLayer.h"
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
    // ====================================================

    // 注册资源变化监听
    ResourceManager::getInstance().SetOnResourceChangeCallback([this](ResourceType type, int amount) {
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
        icon->setScale(0.8f); // 根据素材大小调整
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

    for (auto& pair : _amountLabels) {
        ResourceType type = pair.first;
        Label* lbl = pair.second;

        int current = rm.GetResourceCount(type);
        int max = rm.GetResourceCapacity(type);

        if (type == ResourceType::kGem) {
            // 宝石通常不显示上限，或者上限很大
            lbl->setString(std::to_string(current));
            lbl->setTextColor(Color4B(0, 255, 0, 255)); // 亮绿色
        }
        else if (type == ResourceType::kBuilder) {
            // 建筑工人特殊显示：空闲/总数
            lbl->setString(StringUtils::format("%d / %d", current, max));
            // 如果 current > 0 表示有空闲工人，显示绿色；否则显示红色
            if (current > 0) {
                lbl->setTextColor(Color4B::GREEN);
            }
            else {
                lbl->setTextColor(Color4B::RED);
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