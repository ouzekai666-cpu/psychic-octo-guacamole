import tkinter as tk
from tkinter import ttk, messagebox
import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier

# ══════════════════════════════════════════════
#  1. 加载数据 & 训练模型（启动时自动执行）
# ══════════════════════════════════════════════
try:
    df = pd.read_csv("heart.csv")
except FileNotFoundError:
    print("未找到 heart.csv，请先运行 download_data.py 生成数据。")
    raise SystemExit(1)

feature_cols = [c for c in df.columns if c != "target"]
X = df[feature_cols]
y = df["target"]

# 存储各特征的中位数，给未显式填写的字段当默认值
medians = X.median().to_dict()

model = RandomForestClassifier(n_estimators=200, random_state=42)
model.fit(X, y)

print(f"模型已就绪 | 训练样本: {len(X)} | 特征数: {len(feature_cols)}")

# ══════════════════════════════════════════════
#  2. 构建界面
# ══════════════════════════════════════════════
class PredictApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("❤️ 心脏病风险预测系统")
        self.geometry("520x560")
        self.resizable(False, False)

        # ── 全局字体参数（纯英文，避免中文报错）──
        self._font_family = "Segoe UI"
        self._setup_styles()

        self._build_header()
        self._build_form()
        self._build_button()
        self._build_result()

    # ──────────────── 样式设定 ────────────────
    def _setup_styles(self):
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TLabel", font=(self._font_family, 10), padding=2)
        style.configure("Header.TLabel", font=(self._font_family, 16, "bold"),
                        foreground="#2c3e50", padding=(0, 10))
        style.configure("ResultOK.TLabel", font=(self._font_family, 18, "bold"),
                        foreground="#27ae60")
        style.configure("ResultWarn.TLabel", font=(self._font_family, 18, "bold"),
                        foreground="#e74c3c")
        style.configure("TButton", font=(self._font_family, 11, "bold"),
                        padding=(20, 6))
        style.configure("TFrame", background="#f0f4f8")
        style.configure("Card.TFrame", background="#ffffff",
                        relief="groove", borderwidth=1)

    # ──────────────── 标题 ────────────────
    def _build_header(self):
        container = tk.Frame(self, bg="#ffffff", padx=20, pady=14)
        container.pack(fill="x")
        tk.Label(container, text="❤️ 心脏病风险预测",
                 font=(self._font_family, 20, "bold"),
                 fg="#2c3e50", bg="#ffffff").pack()
        tk.Label(container, text="输入指标后点击按钮，模型将自动评估风险",
                 font=(self._font_family, 10),
                 fg="#7f8c8d", bg="#ffffff").pack()

    # ──────────────── 表单 ────────────────
    def _build_form(self):
        main_frame = tk.Frame(self, bg="#f0f4f8", padx=30, pady=10)
        main_frame.pack(fill="x")

        fields = [
            ("年龄 (岁)", "age_entry", None),
            ("静息血压 (mmHg)", "trestbps_entry", None),
            ("胆固醇 (mg/dl)", "chol_entry", None),
            ("最大心率 (bpm)", "thalach_entry", None),
        ]

        for i, (label_text, attr, _) in enumerate(fields):
            row_frame = tk.Frame(main_frame, bg="#f0f4f8")
            row_frame.pack(fill="x", pady=4)
            tk.Label(row_frame, text=label_text, width=18, anchor="e",
                     font=(self._font_family, 10), bg="#f0f4f8").pack(side="left", padx=(0, 8))
            entry = tk.Entry(row_frame, font=(self._font_family, 10),
                             width=20, relief="solid", bd=1)
            entry.pack(side="left")
            setattr(self, attr, entry)

        # ── 性别 ──
        row_frame = tk.Frame(main_frame, bg="#f0f4f8")
        row_frame.pack(fill="x", pady=6)
        tk.Label(row_frame, text="性别", width=18, anchor="e",
                 font=(self._font_family, 10), bg="#f0f4f8").pack(side="left", padx=(0, 8))
        self.sex_var = tk.IntVar(value=1)
        tk.Radiobutton(row_frame, text="男", variable=self.sex_var,
                       value=1, font=(self._font_family, 10),
                       bg="#f0f4f8", activebackground="#f0f4f8").pack(side="left", padx=4)
        tk.Radiobutton(row_frame, text="女", variable=self.sex_var,
                       value=0, font=(self._font_family, 10),
                       bg="#f0f4f8", activebackground="#f0f4f8").pack(side="left", padx=4)

    # ──────────────── 预测按钮 ────────────────
    def _build_button(self):
        container = tk.Frame(self, bg="#f0f4f8", pady=12)
        container.pack(fill="x")
        btn = tk.Button(container, text="🔍 评估心脏病风险",
                        font=(self._font_family, 12, "bold"),
                        bg="#3498db", fg="white",
                        activebackground="#2980b9", activeforeground="white",
                        padx=28, pady=8, relief="flat", cursor="hand2",
                        command=self._predict)
        btn.pack()
        # hover 效果
        btn.bind("<Enter>", lambda e: btn.config(bg="#2980b9"))
        btn.bind("<Leave>", lambda e: btn.config(bg="#3498db"))

    # ──────────────── 结果区 ────────────────
    def _build_result(self):
        self.result_frame = tk.Frame(self, bg="#ffffff",
                                     highlightbackground="#dcdde1",
                                     highlightthickness=1, padx=20, pady=16)
        self.result_frame.pack(fill="x", padx=30, pady=(0, 20))

        self.result_label = tk.Label(self.result_frame,
                                     text="等待评估…",
                                     font=(self._font_family, 16, "bold"),
                                     fg="#bdc3c7", bg="#ffffff")
        self.result_label.pack()

        self.prob_label = tk.Label(self.result_frame,
                                   text="",
                                   font=(self._font_family, 12),
                                   fg="#7f8c8d", bg="#ffffff")
        self.prob_label.pack(pady=(6, 0))

    # ══════════════════════════════════════════
    #  3. 预测逻辑
    # ══════════════════════════════════════════
    def _predict(self):
        # ── 读取用户输入 ──
        try:
            age_val       = float(self.age_entry.get().strip())
            trestbps_val  = float(self.trestbps_entry.get().strip())
            chol_val      = float(self.chol_entry.get().strip())
            thalach_val   = float(self.thalach_entry.get().strip())
        except ValueError:
            messagebox.showwarning("输入错误",
                "请确保年龄、血压、胆固醇和最大心率均为有效数字。")
            return

        sex_val = self.sex_var.get()

        # ── 构建特征向量（顺序必须与训练时一致）──
        input_dict = {
            "age":      age_val,
            "sex":      sex_val,
            "cp":       medians["cp"],
            "trestbps": trestbps_val,
            "chol":     chol_val,
            "fbs":      medians["fbs"],
            "restecg":  medians["restecg"],
            "thalach":  thalach_val,
            "exang":    medians["exang"],
            "oldpeak":  medians["oldpeak"],
            "slope":    medians["slope"],
            "ca":       medians["ca"],
            "thal":     medians["thal"],
        }

        input_df = pd.DataFrame([input_dict])[feature_cols]

        # ── 预测 ──
        prob = model.predict_proba(input_df)[0][1]   # 患病概率
        pred = int(round(prob))

        # ── 展示 ──
        if pred == 0:
            self.result_label.config(
                text="🟢 低风险 / 健康",
                font=(self._font_family, 18, "bold"),
                fg="#27ae60")
        else:
            self.result_label.config(
                text="🔴 高风险 / 建议复查",
                font=(self._font_family, 18, "bold"),
                fg="#e74c3c")

        self.prob_label.config(
            text=f"患病可能性为 {prob:.0%}",
            font=(self._font_family, 14),
            fg="#2c3e50")


# ══════════════════════════════════════════════
#  启动
# ══════════════════════════════════════════════
if __name__ == "__main__":
    app = PredictApp()
    app.mainloop()
