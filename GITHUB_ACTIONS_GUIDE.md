# GitHub Actions 构建配置说明

## 概述
本文档说明了为 ConnectTool 项目配置的跨平台自动编译和发布系统。

## 主要修改

### 1. CMakeLists.txt 的关键改动

#### ✅ Steamworks SDK 路径
- **修改前**: `steamworks/`
- **修改后**: `sdk/`
- **原因**: 匹配您实际下载的 Steamworks SDK 目录结构

#### ✅ ImGui 集成方式
- **修改前**: 使用 `add_subdirectory(imgui)` (ImGui 默认没有 CMakeLists.txt)
- **修改后**: 手动收集 ImGui 源文件并创建静态库
```cmake
file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    ...
)
add_library(imgui STATIC ${IMGUI_SOURCES})
```

#### ✅ 跨平台 Steamworks 库链接
```cmake
if(WIN32)
    set(STEAM_LIB "${STEAMWORKS_DIR}/redistributable_bin/win64/steam_api64.lib")
elseif(APPLE)
    set(STEAM_LIB "${STEAMWORKS_DIR}/redistributable_bin/osx/libsteam_api.dylib")
else()
    set(STEAM_LIB "${STEAMWORKS_DIR}/redistributable_bin/linux64/libsteam_api.so")
endif()
```

#### ✅ 自动复制 Steam 动态库
添加了 POST_BUILD 命令，自动将 Steam API 库复制到可执行文件目录：
- Windows: `steam_api64.dll`
- macOS: `libsteam_api.dylib`
- Linux: `libsteam_api.so`

#### ✅ 禁用 TUN 示例
将 `BUILD_TUN_EXAMPLE` 默认值改为 `OFF`，避免在 CI 环境中构建不必要的示例。

### 2. GitHub Actions 工作流 (.github/workflows/build.yml)

#### 支持的平台
| 平台 | 架构 | 制品名称 |
|------|------|----------|
| Windows | x64 | ConnectTool-Windows-x64 |
| Linux (Ubuntu) | x64 | ConnectTool-Linux-x64 |
| macOS | ARM64 | ConnectTool-MacOS-arm64 |

#### 工作流步骤

1. **依赖安装**
   - **Linux**: `libglfw3-dev`, `libboost-system-dev`, `libgl1-mesa-dev`
   - **macOS**: `glfw`, `boost` (通过 Homebrew)
   - **Windows**: `glfw3:x64-windows`, `boost-system:x64-windows` (通过 vcpkg)

2. **克隆子模块**
   - ImGui (从 GitHub)
   - nanoid_cpp (从 GitHub)

3. **检查 Steamworks SDK**
   - 验证 `sdk/` 目录是否存在
   - 如果不存在则报错并终止构建

4. **CMake 配置**
   - **Windows**: 使用 Visual Studio 2022 生成器
   - **Linux/macOS**: 使用默认生成器

5. **构建**
   - Release 配置

6. **上传制品**
   - 可执行文件: `OnlineGameTool.exe` / `OnlineGameTool`
   - Steam 库文件

## 重要注意事项

### ⚠️ Steamworks SDK 必须提交到仓库
由于 Steamworks SDK 需要登录才能下载，您需要：

1. **选项 A**: 将 `sdk/` 目录提交到 Git 仓库
   ```bash
   git add sdk/
   git commit -m "Add Steamworks SDK"
   ```

2. **选项 B**: 使用 Git LFS (推荐用于大文件)
   ```bash
   git lfs track "sdk/**"
   git add .gitattributes sdk/
   git commit -m "Add Steamworks SDK with LFS"
   ```

3. **选项 C**: 使用私有子模块或 GitHub Secrets
   - 将 SDK 上传到私有仓库
   - 在工作流中使用 Personal Access Token 克隆

### 📁 必需的目录结构
```
ConnectTool/
├── .github/
│   └── workflows/
│       └── build.yml
├── ConnectTool/
│   ├── online_game_tool.cpp
│   ├── steam/
│   └── net/
├── sdk/                          # Steamworks SDK
│   ├── public/
│   │   └── steam/               # 头文件
│   └── redistributable_bin/
│       ├── win64/
│       │   ├── steam_api64.lib
│       │   └── steam_api64.dll
│       ├── linux64/
│       │   └── libsteam_api.so
│       └── osx/
│           └── libsteam_api.dylib
├── tun/
├── imgui/                        # 由 CI 自动克隆
├── nanoid_cpp/                   # 由 CI 自动克隆
└── CMakeLists.txt
```

## 本地构建指南

### Windows (需要 Visual Studio)
```powershell
# 安装依赖
vcpkg install glfw3:x64-windows boost-system:x64-windows

# 克隆子模块
git clone --depth 1 https://github.com/ocornut/imgui.git imgui
git clone --depth 1 https://github.com/mcmikecreations/nanoid_cpp.git nanoid_cpp

# 配置和构建
cmake -G "Visual Studio 17 2022" -A x64 -B build -S . -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

### Linux
```bash
# 安装依赖
sudo apt-get install libglfw3-dev libboost-system-dev libgl1-mesa-dev

# 克隆子模块
git clone --depth 1 https://github.com/ocornut/imgui.git imgui
git clone --depth 1 https://github.com/mcmikecreations/nanoid_cpp.git nanoid_cpp

# 配置和构建
cmake -B build -S .
cmake --build build --config Release
```

### macOS
```bash
# 安装依赖
brew install glfw boost

# 克隆子模块
git clone --depth 1 https://github.com/ocornut/imgui.git imgui
git clone --depth 1 https://github.com/mcmikecreations/nanoid_cpp.git nanoid_cpp

# 配置和构建
cmake -B build -S .
cmake --build build --config Release
```

## CI/CD 触发条件

工作流会在以下情况下自动运行：
- 推送到 `main` 或 `master` 分支
- 创建针对 `main` 或 `master` 的 Pull Request
- 手动触发 (workflow_dispatch)

## 制品下载

构建完成后，可以在 GitHub Actions 页面下载制品：
1. 进入仓库的 "Actions" 标签
2. 选择最新的工作流运行
3. 在 "Artifacts" 部分下载对应平台的制品

## 故障排除

### 问题: CMake 找不到编译器
**解决方案**: 确保安装了 Visual Studio (Windows) 或 GCC/Clang (Linux/macOS)

### 问题: 找不到 Steamworks SDK
**解决方案**: 确保 `sdk/` 目录存在且包含正确的文件结构

### 问题: ImGui 编译错误
**解决方案**: 确保 ImGui 仓库已正确克隆到 `imgui/` 目录

### 问题: Boost 库未找到
**解决方案**: 
- Windows: 使用 vcpkg 安装
- Linux: `sudo apt-get install libboost-all-dev`
- macOS: `brew install boost`

## 下一步优化建议

1. **添加自动发布**
   - 在推送 tag 时自动创建 GitHub Release
   - 自动上传编译好的二进制文件

2. **添加代码签名**
   - Windows: 使用证书签名 .exe
   - macOS: 使用 Apple Developer 证书签名

3. **添加测试**
   - 单元测试
   - 集成测试

4. **缓存优化**
   - 缓存 vcpkg 依赖
   - 缓存 CMake 构建文件

5. **多架构支持**
   - Windows ARM64
   - Linux ARM64
   - macOS x64 (Intel)
