import pydicom
import matplotlib.pyplot as plt

# 读取 DICOM 文件
ds = pydicom.dcmread("test.dcm")

# 提取像素数据（自动转为 numpy 数组）
pixels = ds.pixel_array

# 判断图像维度
if pixels.ndim == 3:
    # 三维体积 (切片数, 高, 宽)，取中间一层展示
    mid = pixels.shape[0] // 2
    img = pixels[mid]
    print(f"检测到三维体积数据：共 {pixels.shape[0]} 层，{pixels.shape[1]}×{pixels.shape[2]}，显示第 {mid} 层")
else:
    img = pixels

# 显示为灰度图像
plt.imshow(img, cmap="gray")
plt.title("医疗 CT/MRI 原始图像")
plt.axis("off")          # 隐藏坐标轴，让图像更干净
plt.show()
