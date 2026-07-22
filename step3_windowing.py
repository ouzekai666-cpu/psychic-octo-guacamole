import pydicom
import numpy as np
import matplotlib.pyplot as plt


def apply_window(hu_image, window_center, window_width):
    """将 HU 值映射到 0~255 灰度区间"""
    lower = window_center - window_width / 2
    upper = window_center + window_width / 2

    # 钳位并线性映射
    import numpy as np
    clipped = np.clip(hu_image, lower, upper)
    result = ((clipped - lower) / (upper - lower) * 255).astype(np.uint8)
    return result


# 1. 读取 DICOM 文件
ds = pydicom.dcmread("test.dcm")
modality = getattr(ds, "Modality", "未知")
pixels = ds.pixel_array

# 2. 如果是三维体积，取中间一层
if pixels.ndim == 3:
    mid = pixels.shape[0] // 2
    pixels = pixels[mid]

# 3. 转换为 HU 值
has_hu = hasattr(ds, "RescaleSlope") and hasattr(ds, "RescaleIntercept")

if has_hu:
    slope = ds.RescaleSlope
    intercept = ds.RescaleIntercept
    # 先转为浮点数，避免 uint8 溢出
    hu = pixels.astype(float) * slope + intercept
    print(f"CT 数据：已转换为 HU 值（范围 {hu.min():.0f} ~ {hu.max():.0f}）")
else:
    # 非 CT（如 XA、MR 等），直接使用原始像素值
    hu = pixels.astype(float)
    print(f"非 CT 数据（Modality={modality}），直接在原始像素值上做窗口映射")

print(f"当前数据类型：{modality}，像素范围 {pixels.min()} ~ {pixels.max()}")

# 4. 应用两种调窗
if modality == "CT":
    soft_tissue = apply_window(hu, 40, 400)
    bone = apply_window(hu, 400, 1000)
    title1 = "软组织模式（窗位 40 / 窗宽 400）"
    title2 = "骨骼模式（窗位 400 / 窗宽 1000）"
else:
    # XA 等非 CT：使用基于数据范围的自动窗口
    lo, hi = hu.min(), hu.max()
    mid_val = (lo + hi) / 2
    width = hi - lo
    wide = apply_window(hu, mid_val, width * 1.2)
    narrow = apply_window(hu, mid_val, width * 0.4)
    soft_tissue = wide
    bone = narrow
    title1 = f"宽窗口（窗位 {mid_val:.0f} / 窗宽 {width*1.2:.0f}）"
    title2 = f"窄窗口（窗位 {mid_val:.0f} / 窗宽 {width*0.4:.0f}）"

# 5. 并排显示
plt.figure(figsize=(10, 5))

plt.subplot(1, 2, 1)
plt.imshow(soft_tissue, cmap="gray")
plt.title(title1)
plt.axis("off")

plt.subplot(1, 2, 2)
plt.imshow(bone, cmap="gray")
plt.title(title2)
plt.axis("off")

plt.tight_layout()
plt.show()
