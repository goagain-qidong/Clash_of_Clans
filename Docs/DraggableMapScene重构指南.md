# ?? DraggableMapScene 重构指南

## ? **问题分析**

### **问题1：攻击后村庄的东西消失**
**原因**：
- `BattleScene::returnToMainScene()` 使用了 `replaceScene`
- `replaceScene` 会完全销毁旧场景并创建新的场景
- 新场景会重新初始化，导致之前的建筑数据丢失

**解决方案**：? 已修复
```cpp
// ? 错误做法（会销毁旧场景）
auto scene = DraggableMapScene::createScene();
Director::getInstance()->replaceScene(scene);

// ? 正确做法（返回到之前的场景）
Director::getInstance()->popScene();
```

---

### **问题2：DraggableMapScene 严重臃肿**
**数据统计**：
- **总行数**：~1500 行
- **成员变量**：30+ 个
- **方法数量**：50+ 个
- **职责混乱**：地图、UI、建筑、网络、触摸事件全部混在一起

---

## ? **重构方案**

### **架构调整：MVC 模式**

将 `DraggableMapScene` 拆分为多个专门的管理器：

```
DraggableMapScene (主控制器，~300行)
├── MapController (地图控制器，~200行)
│   ├── 地图加载和切换
│   ├── 地图缩放和平移
│   └── 地图边界检查
│
├── SceneUIController (UI控制器，~400行)
│   ├── 按钮创建和管理
│   ├── 建筑/地图列表
│   ├── 确认按钮
│   └── 提示信息
│
├── InputController (输入控制器，~200行)
│   ├── 触摸事件处理
│   ├── 鼠标滚轮缩放
│   └── 键盘快捷键
│
└── BuildingManager (已存在，~500行)
    ├── 建筑创建和放置
    ├── 建筑升级
    └── 建筑保存和加载
```

---

## ?? **新增文件**

### **1. MapController.h/cpp**
**职责**：地图的所有操作
- ? 地图加载 (`loadMap`)
- ? 地图切换 (`switchMap`)
- ? 地图平移 (`moveMap`)
- ? 地图缩放 (`zoomMap`)
- ? 边界检查 (`ensureMapInBoundary`)
- ? 地图配置管理

**使用示例**：
```cpp
// 在 DraggableMapScene 中
_mapController = MapController::create();
this->addChild(_mapController);

// 加载地图
_mapController->loadMap("map/Map1.png");

// 平移地图
_mapController->moveMap(delta);

// 缩放地图
_mapController->zoomMap(1.1f, mousePos);
```

---

### **2. SceneUIController.h/cpp**
**职责**：所有UI的创建和管理
- ? 主按钮创建 (Shop, Map, Attack, Clan)
- ? 建筑选择列表
- ? 地图选择列表
- ? 确认/取消按钮
- ? 提示信息

**使用示例**：
```cpp
// 在 DraggableMapScene 中
_uiController = SceneUIController::create();
this->addChild(_uiController, 100);

// 设置回调
_uiController->setOnShopClicked([this]() {
    this->openShop();
});

_uiController->setOnBuildingSelected([this](const BuildingData& data) {
    _buildingManager->startPlacing(data);
});

// 显示提示
_uiController->showHint("建筑已放置！");

// 显示确认按钮
_uiController->showConfirmButtons(buildingWorldPos);
```

---

### **3. InputController.h/cpp** (待创建)
**职责**：所有输入事件的处理
- ? 触摸事件 (onTouchBegan/Moved/Ended)
- ? 鼠标滚轮 (onMouseScroll)
- ? 键盘快捷键 (ESC 取消建造)
- ? 优先级管理（UI > 建筑建造 > 地图操作）

**使用示例**（待实现）：
```cpp
_inputController = InputController::create();
this->addChild(_inputController);

// 设置触摸回调
_inputController->setOnTouch([this](Touch* touch, Event* event) {
    // 处理触摸事件
});

// 设置滚轮回调
_inputController->setOnScroll([this](float scrollY, Vec2 mousePos) {
    _mapController->zoomMap(scrollY > 0 ? 0.9f : 1.1f, mousePos);
});
```

---

## ?? **重构步骤**

### **Step 1: 集成 MapController** ?
1. 在 `DraggableMapScene.h` 中添加成员变量：
   ```cpp
   MapController* _mapController = nullptr;
   ```

2. 在 `DraggableMapScene::init()` 中创建：
   ```cpp
   _mapController = MapController::create();
   this->addChild(_mapController);
   _mapController->loadMap("map/Map1.png");
   ```

3. 替换所有地图相关代码：
   ```cpp
   // 旧代码
   moveMap(delta);
   
   // 新代码
   _mapController->moveMap(delta);
   ```

4. 删除旧的地图相关方法和成员变量

---

### **Step 2: 集成 SceneUIController** ?
1. 在 `DraggableMapScene.h` 中添加成员变量：
   ```cpp
   SceneUIController* _uiController = nullptr;
   ```

2. 在 `DraggableMapScene::init()` 中创建：
   ```cpp
   _uiController = SceneUIController::create();
   this->addChild(_uiController, 100);
   
   // 设置回调
   _uiController->setOnShopClicked([this]() { openShop(); });
   _uiController->setOnAttackClicked([this]() { onBattleButtonClicked(nullptr); });
   // ... 其他回调
   ```

3. 替换所有UI相关代码：
   ```cpp
   // 旧代码
   showBuildingHint("提示");
   showConfirmButtons(pos);
   
   // 新代码
   _uiController->showHint("提示");
   _uiController->showConfirmButtons(pos);
   ```

4. 删除旧的UI相关方法和成员变量

---

### **Step 3: 创建 InputController** ?
1. 创建 `InputController.h/cpp`
2. 将所有触摸事件处理逻辑移到 `InputController`
3. 在 `DraggableMapScene` 中集成

---

### **Step 4: 清理 DraggableMapScene**
删除已迁移的代码：
- ? `setupMap()` → 移到 `MapController::loadMap()`
- ? `moveMap()` → 移到 `MapController::moveMap()`
- ? `zoomMap()` → 移到 `MapController::zoomMap()`
- ? `setupUI()` → 移到 `SceneUIController::init()`
- ? `createBuildingSelection()` → 移到 `SceneUIController::createBuildingListUI()`
- ? `showConfirmButtons()` → 移到 `SceneUIController::showConfirmButtons()`
- ? `showBuildingHint()` → 移到 `SceneUIController::showHint()`

---

## ?? **重构前后对比**

| 指标 | 重构前 | 重构后 |
|------|--------|--------|
| **DraggableMapScene 代码行数** | ~1500 | ~300 |
| **成员变量数量** | 30+ | ~10 |
| **方法数量** | 50+ | ~15 |
| **职责清晰度** | ? 混乱 | ? 清晰 |
| **可维护性** | ? 困难 | ? 容易 |
| **可测试性** | ? 困难 | ? 容易 |

---

## ?? **重构后的 DraggableMapScene.h**

```cpp
class DraggableMapScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual ~DraggableMapScene();
    
    CREATE_FUNC(DraggableMapScene);
    
private:
    // ==================== 管理器 ====================
    MapController* _mapController = nullptr;
    SceneUIController* _uiController = nullptr;
    InputController* _inputController = nullptr;
    BuildingManager* _buildingManager = nullptr;
    
    // ==================== 游戏状态 ====================
    bool _isAttackMode = false;
    std::string _attackTargetUserId = "";
    
    Node* _currentUpgradeUI = nullptr;
    
    // ==================== 初始化 ====================
    void initializeControllers();
    void setupCallbacks();
    void loadGameState();
    
    // ==================== 回调处理 ====================
    void onBuildingPlaced(BaseBuilding* building);
    void onBuildingClicked(BaseBuilding* building);
    void onShopClicked();
    void onAttackClicked();
    void onClanClicked();
    
    // ==================== 多人游戏 ====================
    bool switchToAttackMode(const std::string& targetUserId);
    void returnToOwnBase();
};
```

**代码量对比**：
- 重构前：~1500 行
- 重构后：~300 行
- **减少了 80%** ?

---

## ?? **下一步计划**

1. ? **问题1已修复**：使用 `popScene` 代替 `replaceScene`
2. ? **MapController 已创建**：地图相关代码已拆分
3. ? **SceneUIController 已创建**：UI相关代码已拆分
4. ? **InputController 待创建**：触摸事件处理待拆分
5. ? **集成到 DraggableMapScene**：逐步替换旧代码
6. ? **测试**：确保功能正常

---

## ?? **使用指南**

### **如何添加新的UI按钮？**
```cpp
// 在 SceneUIController.h 中添加回调
using NewButtonCallback = std::function<void()>;
void setOnNewButtonClicked(const NewButtonCallback& callback);

// 在 SceneUIController.cpp 中创建按钮
auto newButton = Button::create();
newButton->addClickEventListener([this](Ref*) {
    if (_onNewButtonClicked) _onNewButtonClicked();
});

// 在 DraggableMapScene 中设置回调
_uiController->setOnNewButtonClicked([this]() {
    // 处理点击事件
});
```

### **如何添加新的地图？**
```cpp
// 在 DraggableMapScene 中
_mapController->setMapConfig("map/Map4.png", {
    1.3f,                    // scale
    Vec2(1400.0f, 2100.0f), // startPixel
    55.6f                   // tileSize
});

_mapController->loadMap("map/Map4.png");
```

---

祝你重构顺利！??
