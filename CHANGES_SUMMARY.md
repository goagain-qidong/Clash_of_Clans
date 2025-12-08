# 代码更改总结

## 完成时间
2025年1月

## 更改目标
1. 删除所有 gridDev 相关代码 ?
2. 完善切换账号功能：每个账号随机分配一张地图 ?
3. 删除切换地图功能 ?
4. **修复切换账号Bug：彻底重新加载场景** ? (新增)

---

## 最新更新 (切换账号完善)

### 问题描述
之前切换账号后，虽然账号数据切换了，但场景内的显示（HUD 资源、地图、建筑等）没有更新，导致显示的还是旧账号的信息。

### 解决方案
**彻底重新创建场景**，而不是部分更新，确保所有状态都是干净的。

### 实现细节

#### 切换账号完整流程
```
用户点击切换账号
    ↓
1. 保存目标账号ID到 UserDefault (临时存储)
    ↓
2. 触发 onAccountSwitched 回调
    ↓
3. 保存当前账号的建筑状态和资源
    ↓
4. 从 UserDefault 读取目标账号ID
    ↓
5. 调用 AccountManager.switchAccount() 切换账号
   - 加载新账号的游戏数据
   - 同步资源到 ResourceManager
    ↓
6. 清除临时存储的目标账号ID
    ↓
7. 重新创建整个 DraggableMapScene
   - 自动加载新账号的分配地图
   - 重新初始化所有管理器
   - 重新创建 HUD 并显示新账号资源
   - 加载新账号的建筑
```

#### 修改的文件

##### SettingsPanel.cpp
```cpp
// 点击账号时保存目标账号ID
UserDefault::getInstance()->setStringForKey("switching_to_account", account.userId);
_onAccountSwitched(); // 触发回调
```

##### DraggableMapScene.cpp
```cpp
void DraggableMapScene::onAccountSwitched()
{
    // 1. 保存当前账号状态
    _buildingManager->saveCurrentState();
    
    // 2. 获取目标账号ID
    std::string targetUserId = UserDefault::getInstance()->getStringForKey("switching_to_account", "");
    
    // 3. 切换账号
    AccountManager::getInstance().switchAccount(targetUserId);
    
    // 4. 清除临时数据
    UserDefault::getInstance()->setStringForKey("switching_to_account", "");
    
    // 5. 重新创建场景
    auto newScene = DraggableMapScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(0.3f, newScene));
}
```

---

## 主要更改

### 1. 账号系统增强 (AccountManager)

#### AccountManager.h
- **新增字段**: `AccountInfo` 结构体增加 `std::string assignedMapName` 字段
  - 每个账号都有自己专属的地图
  - 默认值为 `"map/Map1.png"`

#### AccountManager.cpp
- **随机分配地图**: 在 `upsertAccount()` 方法中，新账号创建时从 `{"map/Map1.png", "map/Map2.png", "map/Map3.png"}` 中随机选择一张地图
- **保存/加载地图**: `save()` 和 `loadFromStorage()` 方法现在会保存和加载 `assignedMapName` 字段

```cpp
// 新账号创建时随机分配地图
const std::vector<std::string> availableMaps = {"map/Map1.png", "map/Map2.png", "map/Map3.png"};
int randomIndex = rand() % availableMaps.size();
newAcc.assignedMapName = availableMaps[randomIndex];
```

---

### 2. 删除 GridDev 功能

#### SceneUIController.h/cpp
- **删除**: 所有 GridDev 相关的成员变量和方法
  - `_gridMap`
  - `_mapConfigManager`
  - `_currentMapName`
  - `setGridMap()`
  - `setMapConfigManager()`
  - `setCurrentMapName()`

- **删除**: Map 按钮和地图列表相关功能
  - `_mapButton`
  - `_mapListUI`
  - `_isMapListVisible`
  - `_mapNames`
  - `setMapList()`
  - `createMapListUI()`
  - `toggleMapList()`
  - `onMapSelected` 回调

#### SettingsPanel.h/cpp
- **删除**: 所有 GridDev 相关的功能
  - `_gridDevButton`
  - `_isGridDevVisible`
  - `_gridMap`
  - `_mapConfigManager`
  - `_gridDevUI`
  - `_currentMapName`
  - `setGridMap()`
  - `setMapConfigManager()`
  - `setCurrentMapName()`
  - `setupGridDevButton()`
  - `onGridDevClicked()`
  - `toggleGridDev()`
  - `showGridDevUI()`
  - `hideGridDevUI()`
  - `updateGridDisplay()`

---

### 3. 主场景改造 (DraggableMapScene)

#### DraggableMapScene.h
- **删除**: `MapConfigManager* _mapConfigManager` 成员变量

#### DraggableMapScene.cpp

##### initializeManagers()
- **新功能**: 根据当前账号的 `assignedMapName` 加载地图
  ```cpp
  std::string assignedMap = "map/Map1.png"; // 默认
  const auto* currentAccount = accMgr.getCurrentAccount();
  if (currentAccount && !currentAccount->assignedMapName.empty()) {
      assignedMap = currentAccount->assignedMapName;
  }
  _mapController->loadMap(assignedMap);
  ```
- **删除**: MapConfigManager 的初始化和相关设置

##### setupCallbacks()
- **删除**: `onMapSelected` 回调的设置

##### initBuildingData()
- **删除**: `_uiController->setMapList()` 调用

##### UI 回调
- **删除**: `onMapSelected()` 方法

##### onAccountSwitched() (重构)
- **新功能**: 完整的账号切换流程
  1. 保存当前账号状态
  2. 切换到新账号
  3. 重新创建整个场景

---

## 保留但不使用的文件

以下文件保留在项目中但不再被使用（可能在未来开发中有用）：
- `MapConfigManager.h`
- `MapConfigManager.cpp`

---

## 用户体验变化

### 之前
1. 用户可以通过 Map 按钮切换地图
2. 用户可以通过 Settings 面板打开 GridDev 模式调试网格
3. 所有账号共享同一张地图
4. ? 切换账号后显示不更新

### 之后
1. ? **每个账号创建时随机分配一张地图**（Map1/Map2/Map3）
2. ? **切换账号时自动加载该账号的专属地图**
3. ? **不再有地图切换按钮**
4. ? **不再有 GridDev 调试功能**
5. ? **界面更简洁，UI 按钮减少**
6. ? **切换账号后场景完全重新加载，显示正确的账号信息**
   - 地图切换到新账号的专属地图
   - HUD 显示新账号的资源
   - 建筑加载新账号的布局
   - 所有状态都是干净的

---

## 技术细节

### 为什么要重新创建场景？

**问题**：部分更新容易遗漏状态
- HUDLayer 的资源显示
- 地图缩放和位置
- BuildingManager 的内部状态
- 各种 UI 的临时状态
- 回调函数的闭包引用

**解决**：重新创建场景
- 所有管理器重新初始化
- 所有 UI 重新创建
- 所有状态重新加载
- 确保没有旧账号的残留数据

### 数据流向

```
点击账号 → SettingsPanel
    ↓
保存目标ID到 UserDefault
    ↓
触发回调 → DraggableMapScene::onAccountSwitched()
    ↓
保存当前账号状态 → BuildingManager::saveCurrentState()
    ↓
切换账号 → AccountManager::switchAccount()
    ↓ (自动执行)
加载新账号数据 → AccountManager::loadGameStateFromFile()
    ↓ (自动执行)
同步资源 → ResourceManager::SetResourceCount()
    ↓
重新创建场景 → DraggableMapScene::createScene()
    ↓ (自动执行)
加载新账号地图 → initializeManagers()
    ↓ (自动执行)
创建 HUD → HUDLayer::create()
    ↓ (自动执行)
显示新账号资源 → HUDLayer::updateDisplay()
    ↓ (自动执行)
加载新账号建筑 → BuildingManager::loadCurrentAccountState()
```

---

## 测试建议

1. **创建新账号**: 验证新账号是否被随机分配地图 ?
2. **切换账号**: 验证切换账号时所有内容是否正确更新 ?
   - HUD 资源显示
   - 地图切换
   - 建筑布局
   - 没有残留数据
3. **保存/加载**: 验证账号的 `assignedMapName` 是否正确保存和加载 ?
4. **编译**: 确保所有代码编译无错误 ? (已通过)
5. **多次切换**: 验证在多个账号之间反复切换是否稳定 (建议测试)

---

## 日志输出

新增的关键日志：
```cpp
// 新账号创建时
CCLOG("? Assigned map %s to new account %s", newAcc.assignedMapName.c_str(), newAcc.userId.c_str());

// 加载账号地图时
CCLOG("? Loading assigned map for account %s: %s", currentAccount->username.c_str(), assignedMap.c_str());

// 切换账号时
CCLOG("? Account switch initiated...");
CCLOG("? Saved current account state");
CCLOG("? Account switched successfully, reloading scene...");
```

---

## 未来改进建议

1. **地图解锁系统**: 可以根据大本营等级解锁更多地图
2. **地图编辑器**: 如果需要调试网格，可以添加开发者模式重新启用 GridDev
3. **地图预览**: 在账号选择界面显示该账号分配的地图缩略图
4. **地图主题**: 可以添加不同主题的地图（沙漠、雪地、森林等）
5. **账号切换动画优化**: 可以添加加载进度条或过渡动画

---

## 总结

本次更新成功：
- ? 删除了所有 gridDev 相关代码
- ? 实现了每个账号随机分配专属地图
- ? 删除了手动切换地图功能
- ? 简化了 UI 界面
- ? **修复了切换账号后显示不更新的 Bug**
- ? **实现了彻底的账号切换：场景完全重新加载**
- ? 编译通过

所有更改向后兼容，旧账号在加载时会默认使用 Map1.png。
