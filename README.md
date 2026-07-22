# 🏥 DICOM Medical Image Viewer & Windowing Tool

> 基于 Python 的轻量级医疗 DICOM 图像解析与交互式调窗工具
>
> *DICOM Medical Image Analysis & Interactive Windowing Toolkit*

---

## 📖 项目简介

DICOM Medical Image Viewer 是一个面向医疗影像领域的轻量级解析与可视化工具。它能够读取标准 DICOM 格式文件（CT、X 射线血管造影等），自动提取患者元数据，将像素值转换为 Hounsfield Unit（HU），并基于 **窗宽 / 窗位（Window Width / Window Center）** 算法实现灰度图像的动态映射，帮助医生、研究人员或学习者快速浏览和对比不同组织窗口下的影像细节。

项目采用 **纯 Python** 实现，GUI 基于 **Tkinter** 原生界面库，无需安装额外大型依赖即可运行。

---

## ✨ 核心功能

| 功能 | 说明 |
|------|------|
| 📂 **DICOM 文件解析** | 读取标准 `.dcm` 文件，支持单帧与多帧（三维体积自动取中间层） |
| 📋 **元数据提取** | 自动提取并展示患者姓名、患者 ID、检查类型（Modality）、图像尺寸（Rows × Columns） |
| 🔢 **HU 值转换** | 利用 `RescaleSlope` / `RescaleIntercept` 标签将像素值转换为 CT 标准 Hounsfield Unit |
| 🎛️ **动态调窗算法** | 基于 `Window Center ± Width/2` 的线性映射函数，将指定范围的灰度值映射到 0–255 |
| 🖼️ **多窗口预设** | 软组织模式（C=40, W=400）与骨骼模式（C=400, W=1000） |
| 🎯 **实时交互调窗** | Tkinter 滑动条拖动即时更新图像对比度，支持任意 Center / Width 组合 |
| 🔄 **非 CT 自适应** | 自动判断 Modality，对 XA（血管造影）等非 CT 模态直接在原始像素域进行窗口映射 |

---

## 🧱 技术栈

| 技术 | 用途 |
|------|------|
| **Python 3.14+** | 开发语言 |
| **pydicom** | DICOM 文件读写与标签解析 |
| **NumPy** | 像素矩阵运算、HU 值转换、窗口映射 |
| **OpenCV** | 图像处理与格式转换辅助 |
| **Matplotlib** | 图像显示与对比验证 |
| **Pillow (PIL)** | Tkinter 界面中的图像渲染 |
| **Tkinter** | 原生 GUI 界面框架 |

> 除 Python 内置模块外，所有依赖均列于 `requirements.txt`，一行命令即可安装。

---

## 🚀 快速开始

### 1️⃣ 环境准备

确保已安装 Python 3.9+（推荐 3.12+），并在终端中进入项目目录：

```bash
cd "C:\Users\29166\Desktop\medical tool"
```

### 2️⃣ 安装依赖

```bash
pip install -r requirements.txt
```

### 3️⃣ 运行程序

**启动 GUI 交互界面：**

```bash
python main_app.py
```

**查看 DICOM 基本信息：**

```bash
python step1_read_info.py
```

**显示原始图像（Matplotlib）：**

```bash
python step2_show_image.py
```

**对比软组织 / 骨骼双窗口：**

```bash
python step3_windowing.py
```

---

## 🖼️ 界面展示

> *以下为应用主界面效果预览，替换 `screenshot.png` 为实际截图即可。*

![App Screenshot](screenshot.png)

---

## 📂 项目结构

```
medical tool/
├── main_app.py              # Tkinter 桌面主程序（整合所有功能）
├── step1_read_info.py       # 步骤 1：读取 DICOM 元数据
├── step2_show_image.py      # 步骤 2：显示原始图像
├── step3_windowing.py       # 步骤 3：HU 转换与双窗口对比
├── requirements.txt         # Python 依赖清单
├── test.dcm                 # 示例 DICOM 文件（XA 血管造影）
└── README.md                # 本文件
```

---

## ⚙️ 技术细节

### HU 值转换公式

```
HU = Pixel_Value × RescaleSlope + RescaleIntercept
```

### 调窗（Windowing）算法

```
LowerBound = Center - Width / 2
UpperBound = Center + Width / 2

Output = clip((Input - Lower) / (Upper - Lower) × 255, 0, 255)
```

### 支持的模态

| Modality | 说明 | HU 转换 |
|----------|------|---------|
| CT | 计算机断层扫描 | ✅ 自动转换 |
| XA | X 射线血管造影 | ❌ 直接处理原始像素 |
| MR | 磁共振成像 | ❌ 直接处理原始像素 |
| 其他 .dcm | 通用 DICOM | 视标签决定 |

---

## 📜 许可证

本项目仅供学习与科研参考使用，不构成医疗诊断依据。

---

*Made with ❤️ for medical imaging learners & researchers*
