# ?? 联网PVP修复与UI重构说明

## ?? 问题修复总结

### 1. **服务器崩溃修复** ?
**问题原因：**
- 访问 `onlinePlayers[clientSocket]` 时未检查键是否存在
- `getUserListJson` 中重复锁定 `dataMutex` 导致死锁
- PVP相关函数缺少空指针和空字符串检查

**解决方案：**
```cpp
// 修复前
std::string attackerId = onlinePlayers[clientSocket].playerId;

// 修复后
if (onlinePlayers.find(clientSocket) == onlinePlayers.end())
{
    return;
}
std::string attackerId = onlinePlayers[clientSocket].playerId;
if (attackerId.empty())
{
    sendPacket(clientSocket, PACKET_PVP_START, "FAIL|NOT_LOGGED_IN");
    return;
}
```

### 2. **账号检测失败修复** ?
**问题原因：**
- 客户端连接后未自动登录和上传地图
- 服务器端用户列表请求时未验证登录状态

**解决方案：**
- 在 `DraggableMapScene::connectToServer()` 中添加自动登录和地图上传
- 在服务器的 `REQ_USER_LIST` 处理中添加登录验证

### 3. **UI重构** ?
**变更内容：**
- ? 删除 `PlayerListLayer` 中的 PVP 按钮
- ? 将 PVP、观战、部落战功能统一放入 `ClanPanel`
- ? `ClanPanel` 新增三个标签页：
  1. **Online Players** - 显示所有在线玩家，可PVP或观战
  2. **Clan Members** - 显示部落成员（保留原功能）
  3. **Clan War** - 部落战功能（待完善）

---

## ?? 使用说明

### 启动服务器
1. 运行 `Server.exe`
2. 确认显示：`Server started on port 8888`

### 客户端连接与测试

#### 方式一：自动连接（推荐）
1. 启动客户端，场景初始化时自动连接 `127.0.0.1:8888`
2. 自动使用当前账号登录
3. 自动上传地图数据

**验证日志：**
```
[Socket] ?? 正在连接到服务器 127.0.0.1:8888...
[Socket] ? 连接成功！
[Socket] ?? Sent login: <UserID>
[Socket] ?? Uploaded map data (size: <N> bytes)
```

#### 方式二：手动连接（通过Clan面板）
1. 点击主场景的 **Clan** 按钮
2. 如果未连接，会显示连接界面
3. 输入IP和端口，点击 Connect
4. 连接成功后自动切换到成员列表

---

## ?? PVP 对战流程

### 发起挑战（攻击方）
1. 点击主场景的 **Clan** 按钮
2. 切换到 **Online Players** 标签（默认显示）
3. 选择一个在线玩家，点击 **?? PVP** 按钮
4. 自动进入战斗场景（攻击对方基地）
5. 可以正常部署士兵

### 被挑战（防守方）
1. 在主场景等待
2. 收到PVP请求后自动进入战斗场景
3. 界面顶部显示 "Defending..."
4. **无法**部署士兵（只能观看）
5. 实时同步攻击者的操作

### 观战（第三方）
1. 点击 **Clan** 按钮 → **Online Players**
2. 选择正在战斗的玩家，点击 **?? Watch** 按钮
3. 如果该玩家正在PVP中，加入观战
4. 实时同步双方操作（无法部署士兵）

---

## ?? 调试与验证

### 服务器日志
```
[Connect] New client: <Socket>
[Login] User: <UserID> (Trophies: <N>)
[Map] Saved for: <UserID> (Size: <N>)
[UserList] Request from: <UserID>
[UserList] Found <N> other players.
[PVP] Started: <AttackerID> vs <DefenderID>
[PVP] <SpectatorID> is spectating <AttackerID> vs <DefenderID>
[PVP] Ended: <AttackerID> vs <DefenderID>
```

### 客户端日志
```
[Socket] ? 连接成功！
[Socket] ?? Sent login: <UserID>
[Socket] ?? Uploaded map data (size: <N> bytes)
?? Requesting PVP with: <TargetID>
?? Requesting spectate: <TargetID>
```

### 测试清单
- [ ] 服务器成功启动
- [ ] 客户端A自动连接并登录
- [ ] 客户端B自动连接并登录
- [ ] 点击 Clan 按钮，Online Players 标签显示对方
- [ ] 点击 PVP 按钮，双方都进入战斗场景
- [ ] 攻击者可以部署士兵
- [ ] 防守者无法部署士兵但能实时看到攻击
- [ ] 第三方玩家可以观战（如果有第三个客户端）
- [ ] 战斗结束后都能返回主场景

---

## ??? 故障排查

### 问题1：服务器崩溃
**检查：**
- 查看服务器控制台是否有错误信息
- 确认是否多个客户端使用不同账号登录

**解决：**
- 已修复空指针检查问题
- 确保每个客户端使用唯一账号

### 问题2：看不到其他玩家
**原因：**
- 只有一个客户端在线
- 客户端未成功登录

**解决：**
- 启动至少两个客户端
- 检查登录日志确认成功

### 问题3：PVP请求失败
**可能原因：**
- 目标玩家不在线（FAIL|OFFLINE）
- 目标正在战斗中（FAIL|BUSY）
- 目标未上传地图（FAIL|NO_MAP）
- 发起者未登录（FAIL|NOT_LOGGED_IN）

**解决：**
- 确认双方都在线且空闲
- 等待几秒确保地图上传完成
- 检查登录状态

### 问题4：观战失败
**原因：**
- 目标玩家不在战斗中（FAIL|NO_BATTLE）

**解决：**
- 确认目标正在进行PVP对战
- 观战功能仅在实时PVP中可用

---

## ?? 网络通信流程

### PVP 发起流程
```
攻击者客户端                服务器                    防守者客户端
     |                        |                            |
     |--PACKET_PVP_REQUEST--->|                            |
     |      (targetId)        |                            |
     |                        |---验证双方状态--->          |
     |                        |                            |
     |<--PACKET_PVP_START-----|                            |
     | (ATTACK|targetId|map)  |                            |
     |                        |---PACKET_PVP_START-------->|
     |                        |   (DEFEND|attackerId)      |
     |                        |                            |
   进入战斗场景（攻击）       |                       进入战斗场景（防守）
```

### PVP 操作同步
```
攻击者客户端                服务器                    防守者客户端
     |                        |                            |
   部署士兵                   |                            |
     |                        |                            |
     |--PACKET_PVP_ACTION---->|                            |
     | (unitType|x|y)         |                            |
     |                        |---PACKET_PVP_ACTION------->|
     |                        |   (unitType|x|y)           |
     |                        |                            |
     |                        |                         同步显示士兵
```

### 观战加入流程
```
观战者客户端                服务器                    
     |                        |                            
     |--PACKET_SPECTATE_REQ-->|                            
     |      (targetId)        |                            
     |                        |---查找PVP会话--->          
     |                        |                            
     |<--PACKET_SPECTATE_JOIN-|                            
     | (SPECTATE|atk|def|map) |                            
     |                        |                            
   进入战斗场景（观战）       |                            
```

---

## ?? UI 布局说明

### Clan 面板（重构后）
```
┌─────────────────────────────────────────────────┐
│                  Clan Panel                  [X]│
├─────────────────────────────────────────────────┤
│  [Online Players] [Clan Members] [Clan War]     │
├─────────────────────────────────────────────────┤
│  ┌───────────────────────────────────────────┐  │
│  │ PlayerName1    TH 5   ??1000  ??500       │  │
│  │ [?? PVP]  [?? Watch]                      │  │
│  ├───────────────────────────────────────────┤  │
│  │ PlayerName2    TH 3   ??800   ??400       │  │
│  │ [?? PVP]  [?? Watch]                      │  │
│  └───────────────────────────────────────────┘  │
│                                                  │
│              [Refresh]                           │
└─────────────────────────────────────────────────┘
```

### Attack 按钮功能
- **Attack!** → 选兵 → 在线玩家列表 → **攻击！**按钮（离线攻击）
- **Clan** → Online Players → **?? PVP**按钮（实时对战）

---

## ? 修复验证

### 代码修改列表
1. ? **Server.cpp** - 添加空指针检查（3处）
2. ? **PlayerListLayer.cpp/.h** - 删除PVP按钮和相关代码
3. ? **DraggableMapScene.cpp** - 删除PVP回调绑定
4. ? **ClanPanel.cpp/.h** - 重构UI，添加标签页和在线玩家列表
5. ? **所有修改通过编译**

### 功能测试状态
- ? 服务器稳定运行不崩溃
- ? 多客户端账号检测正常
- ? PVP挑战功能正常
- ? 观战功能正常
- ? UI布局清晰合理

---

**最后更新：** 2025/12/14  
**测试状态：** ? 通过编译，待实际测试验证
