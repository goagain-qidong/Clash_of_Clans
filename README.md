# Clash of Clans - 项目说明

这是一个基于 `cocos2d-x` 的 C++ 项目，实现了模拟类《部落冲突》风格的城镇构建系统。项目包含建筑管理、网格地图、资源管理、建筑移动/升级、序列化存档等子系统，适合作为学习和二次开发的基础。

## 主要特性

- 网格化地图与建筑放置系统 (`Classes/Managers/BuildingManager.cpp`)
- 多种建筑类型：资源建筑、军营、城墙、大本营等（`Classes/Buildings`）
- 资源与容量管理（`Classes/Managers/ResourceManager.*`）
- 保存/加载与账号管理（`Classes/Managers/AccountManager.*`）
- 支持建筑移动、升级倒计时 UI 等功能

## 目录结构（部分）

- `Classes/` - 游戏源码（场景、管理器、建筑等）
- `proj.win32/` - Windows 平台工程文件（Visual Studio / CMake）
- `CMakeLists.txt` - CMake 构建脚本
- `engine/` - 引擎与第三方库（已包含在仓库中）

## 环境与依赖

- `cocos2d-x` (兼容 3.x/4.x 的代码风格；请根据仓库中的 `engine/` 目录确认版本)
- Windows + Visual Studio 2019/2022 或等效支持 C++ 的 IDE
- CMake（用于跨平台构建）

## 构建与运行

1. 使用 Visual Studio 打开 `proj.win32` 下的工程，选择 `Debug` 或 `Release` 配置并编译运行。

2. 或使用 CMake（命令行示例）：

   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

3. 如果出现资源或引擎路径问题，请确认 `engine/` 目录在仓库中完整，或修改 CMake/工程的引擎引用路径。

## 常见问题

- README 出现乱码：请确保编辑器或 Git 使用 UTF-8 编码保存并查看该文件（推荐 UTF-8 无 BOM 或带 BOM，视您的编辑器而定）。
- 构建失败：检查 Visual Studio 的 C++ 工作负载是否已安装，确认 CMake 版本匹配。

## 贡献与开发指南

欢迎提交 issue 或 pull request。常见流程：

1. Fork 本仓库并创建功能分支
2. 实现并通过本地编译与基本测试
3. 提交 PR，说明更改目的与影响范围

## 许可证

本仓库默认采用 MIT 许可证；如需更改或补充许可证信息，请在根目录添加 `LICENSE` 文件。

---

# Clash of Clans - Project Description

This is a C++ project built on `cocos2d-x`, implementing a town-building system in the style of the simulation game Clash of Clans. The project includes subsystems for building management, grid-based maps, resource management, building movement/upgrades, and serialized saving, making it suitable as a foundation for learning and secondary development.

## Key Features

- Grid-based map and building placement system (`Classes/Managers/BuildingManager.cpp`)
- Multiple building types: resource buildings, barracks, walls, Town Hall, etc. (`Classes/Buildings`)
- Resource and capacity management (`Classes/Managers/ResourceManager.*`)
- Save/load and account management (`Classes/Managers/AccountManager.*`)
- Supports building movement, upgrade countdown UI, and more

## Directory Structure (Partial)

- `Classes/` - Game source code (scenes, managers, buildings, etc.)
- `proj.win32/` - Windows platform project files (Visual Studio / CMake)
- `CMakeLists.txt` - CMake build script
- `engine/` - Engine and third-party libraries (included in the repository)

## Environment and Dependencies

- `cocos2d-x` (code style compatible with 3.x/4.x; verify version based on the `engine/` directory in the repository)
- Windows + Visual Studio 2019/2022 or equivalent C++ IDE
- CMake (for cross-platform builds)

## Build and Run

1. Open the project under `proj.win32` in Visual Studio, select `Debug` or `Release` configuration, then compile and run.
2. Or use CMake (command line example):
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```
3. If resource or engine path issues occur, verify the `engine/` directory is intact in the repository or modify the engine reference path in CMake/project.

## Common Issues

- Garbled README: Ensure your editor or Git saves and views the file using UTF-8 encoding (recommended: UTF-8 without BOM or with BOM, depending on your editor).
- Build failure: Verify the Visual Studio C++ workload is installed and confirm CMake version compatibility.

## Contributing and Development Guide

We welcome issue submissions and pull requests. Standard workflow:
1. Fork this repository and create a feature branch
2. Implement changes and verify local compilation with basic tests
3. Submit a PR with clear descriptions of changes and their scope

## License

This repository defaults to the MIT License. To modify or supplement license information, add a `LICENSE` file to the root directory.

---