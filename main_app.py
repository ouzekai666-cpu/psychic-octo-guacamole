import tkinter as tk
from tkinter import filedialog
import pydicom
import numpy as np
from PIL import Image, ImageTk


def apply_window(image, center, width):
    """将像素值映射到 0~255 灰度区间"""
    lower = center - width / 2
    upper = center + width / 2
    clipped = np.clip(image, lower, upper)
    return ((clipped - lower) / (upper - lower) * 255).astype(np.uint8)


class DICOMViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("DICOM 医学图像查看器")
        self.root.geometry("1100x700")

        # 数据存储
        self.pixels = None
        self.hu_data = None
        self.ds = None
        self.photo = None          # 防止 PhotoImage 被 Python 回收

        # ---------- 顶部：打开文件按钮 ----------
        top_frame = tk.Frame(root)
        top_frame.pack(fill=tk.X, padx=10, pady=(10, 0))

        btn_open = tk.Button(top_frame, text="打开 DICOM 文件", command=self.open_file)
        btn_open.pack(side=tk.LEFT)

        # ---------- 中间：左侧信息 + 右侧图像 ----------
        mid_frame = tk.Frame(root)
        mid_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # 左侧信息面板
        info_frame = tk.LabelFrame(mid_frame, text="文件信息", width=220)
        info_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))
        info_frame.pack_propagate(False)

        self.info_text = tk.Text(info_frame, width=25, height=10,
                                 font=("Microsoft YaHei", 10), state=tk.DISABLED)
        self.info_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 右侧图像显示区
        img_frame = tk.LabelFrame(mid_frame, text="图像显示")
        img_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.img_label = tk.Label(img_frame, bg="#1a1a1a")
        self.img_label.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # ---------- 底部：调窗滑动条 ----------
        bottom_frame = tk.Frame(root)
        bottom_frame.pack(fill=tk.X, padx=10, pady=(0, 10))

        tk.Label(bottom_frame, text="窗位 (Center):").pack(anchor=tk.W)
        self.scale_center = tk.Scale(bottom_frame, from_=-1000, to=1000,
                                     orient=tk.HORIZONTAL,
                                     command=self.update_image)
        self.scale_center.set(40)
        self.scale_center.pack(fill=tk.X)

        tk.Label(bottom_frame, text="窗宽 (Width):").pack(anchor=tk.W)
        self.scale_width = tk.Scale(bottom_frame, from_=1, to=2000,
                                    orient=tk.HORIZONTAL,
                                    command=self.update_image)
        self.scale_width.set(400)
        self.scale_width.pack(fill=tk.X)

    # ------------------------------------------------------------------
    #  打开 DICOM 文件
    # ------------------------------------------------------------------
    def open_file(self):
        path = filedialog.askopenfilename(
            filetypes=[("DICOM 文件", "*.dcm"), ("所有文件", "*.*")])
        if not path:
            return

        self.ds = pydicom.dcmread(path)
        pixels = self.ds.pixel_array

        # 三维体积 → 取中间层
        if pixels.ndim == 3:
            pixels = pixels[pixels.shape[0] // 2]

        self.pixels = pixels

        # 判断是否有 CT 的 HU 转换标签
        has_hu = (hasattr(self.ds, "RescaleSlope") and
                  hasattr(self.ds, "RescaleIntercept"))
        if has_hu:
            self.hu_data = pixels.astype(float) * float(self.ds.RescaleSlope) + \
                           float(self.ds.RescaleIntercept)
        else:
            self.hu_data = pixels.astype(float)

        # 根据数据范围自动设置合适的初始窗位/窗宽
        lo, hi = float(self.hu_data.min()), float(self.hu_data.max())
        mid = (lo + hi) / 2
        w = hi - lo
        self.scale_center.set(mid)
        self.scale_width.set(max(w, 1))

        # 更新界面
        self.update_info()
        self.update_image()

    # ------------------------------------------------------------------
    #  更新文件信息
    # ------------------------------------------------------------------
    def update_info(self):
        if self.ds is None:
            return

        name = str(getattr(self.ds, "PatientName", "未提供"))
        pid  = str(getattr(self.ds, "PatientID", "未提供"))
        mod  = str(getattr(self.ds, "Modality", "未提供"))
        rows = str(getattr(self.ds, "Rows", "未提供"))
        cols = str(getattr(self.ds, "Columns", "未提供"))

        info = (
            f"病人姓名: {name}\n"
            f"病人 ID:  {pid}\n"
            f"检查类型: {mod}\n"
            f"图像尺寸: {cols} × {rows}"
        )

        self.info_text.config(state=tk.NORMAL)
        self.info_text.delete("1.0", tk.END)
        self.info_text.insert("1.0", info)
        self.info_text.config(state=tk.DISABLED)

    # ------------------------------------------------------------------
    #  重新绘制图像（滑动条拖动时自动触发）
    # ------------------------------------------------------------------
    def update_image(self, event=None):
        if self.hu_data is None:
            return

        center = self.scale_center.get()
        width  = self.scale_width.get()
        img_array = apply_window(self.hu_data, center, width)

        # numpy → PIL → PhotoImage
        pil_img = Image.fromarray(img_array, mode="L")

        # 缩放以适配显示区域
        disp_w = max(self.img_label.winfo_width(), 400)
        disp_h = max(self.img_label.winfo_height(), 400)
        pil_img.thumbnail((disp_w, disp_h), Image.LANCZOS)

        self.photo = ImageTk.PhotoImage(pil_img)
        self.img_label.config(image=self.photo)


if __name__ == "__main__":
    root = tk.Tk()
    app = DICOMViewer(root)
    root.mainloop()
