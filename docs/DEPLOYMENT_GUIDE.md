# 渠沟清理标定与数字孪生监控系统 部署流程

> 适用版本：2.0.0（Qt 6.8.2 / C++17 / MinGW / CMake）
> 文档用途：面向团队成员的编译、打包与目标机器部署说明。

## 1. 部署目标与环境

目标环境：

- Windows 10/11（64 位）；
- 免安装运行：将 `deploy/` 发布目录整体拷贝到目标机器即可，无需安装 Qt。

构建机建议环境：

- Qt 6.8.2（MinGW 64-bit 套件）；
- MinGW-w64（g++ 支持 C++17）；
- CMake 3.16+；
- Git（可选，用于获取源码）。

## 2. 源码获取

仓库分支：`docs/ditch-cleaning-readme`

```bash
git clone -b docs/ditch-cleaning-readme \
  https://github.com/ouzekai666-cpu/psychic-octo-guacamole.git
cd psychic-octo-guacamole/ChannelInspectionCalibration
```

源码结构：

```text
ChannelInspectionCalibration/
├── main.cpp
├── CMakeLists.txt
├── config/DeviceConfig.h
├── models/
├── hardware/
├── services/
├── database/
├── visualization/
└── ui/
```

## 3. 编译构建

### 3.1 配置

```bash
cmake -S . -B build -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.2/mingw_64
```

> 如 Qt 安装路径不同，请将 `CMAKE_PREFIX_PATH` 替换为实际路径。

### 3.2 构建

```bash
cmake --build build --parallel 8
```

构建产物：

```text
build/ChannelInspectionCalibration.exe
```

### 3.3 本地运行验证

```bash
./build/ChannelInspectionCalibration.exe
```

确认：窗口标题为“渠沟清理标定”，主界面正常显示，点击“开始模拟”无崩溃。

## 4. 发布打包

### 4.1 使用 windeployqt

```bash
windeployqt build/ChannelInspectionCalibration.exe
```

该命令会自动收集 Qt DLL、平台插件、图片格式插件、翻译文件等到 exe 同级目录。

### 4.2 整理发布目录

将以下内容放入发布目录（示例：`deploy/`）：

```text
deploy/
├── ChannelInspectionCalibration.exe
├── logo.png
├── Qt6Core.dll
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6Network.dll
├── Qt6Svg.dll
├── libgcc_s_seh-1.dll
├── libstdc++-6.dll
├── libwinpthread-1.dll
├── platforms/
│   └── qwindows.dll
├── imageformats/
├── iconengines/
├── styles/
├── tls/
├── translations/
└── (其它 windeployqt 自动生成的依赖)
```

> `logo.png` 需与 exe 位于同一目录，否则侧边栏 Logo 无法显示（程序会回退到内置资源路径）。

## 5. 目标机器部署

1. 将整个 `deploy/` 目录拷贝到目标机器（如 `C:\Program Files\DitchCalibration\`）；
2. 目录内所有文件保持同层，不要只拷贝 exe；
3. 双击 `ChannelInspectionCalibration.exe` 启动；
4. 验证功能：界面加载、随机生成现场、开始模拟、导出 JSON。

## 6. 版本发布检查清单

- [ ] 最新源码已编译通过（Release）；
- [ ] 本地运行无崩溃、无布局异常；
- [ ] 数字孪生、报警联动、视觉监控功能正常；
- [ ] `windeployqt` 已执行，缺失 DLL/插件已补齐；
- [ ] `logo.png` 已放入发布目录；
- [ ] 发布目录在干净机器（无 Qt）上启动成功；
- [ ] JSON 导出文件可正常打开；
- [ ] 版本号与 README/操作手册一致。

## 7. 常见部署问题

**Q1：提示缺少 Qt6Core.dll 等文件**
发布目录未收集完整运行时，请重新执行 `windeployqt`，或从构建机 Qt `bin/` 目录补齐同名 DLL。

**Q2：提示“could not find or load the Qt platform plugin windows”**
缺少 `platforms/qwindows.dll`，请确保发布目录存在 `platforms/` 文件夹且与 exe 同级。

**Q3：目标机器没有安装 VC++ 运行库**
本项目使用 MinGW 工具链，需携带 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`。

**Q4：界面中文乱码**
源码与资源均为 UTF-8，请勿使用旧编码编辑器改写；发布目录保留 `translations/` 不强制，但建议保留。

## 8. 备份与回滚

- 每次发布前备份上一版 `deploy/` 目录；
- 保留上一版 exe 与对应 DLL 组合，避免混用不同 Qt 版本的运行时；
- 源码发布以 Git 分支标签为准，重要改动请打 Tag 后发布。
