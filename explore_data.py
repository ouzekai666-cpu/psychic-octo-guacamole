import pandas as pd
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
import seaborn as sns

# ── 1. 读取数据 ──
df = pd.read_csv("heart.csv")

# ── 2. 基本信息 ──
print("=" * 50)
print("数据概览")
print("=" * 50)
print(f"行数: {df.shape[0]}")
print(f"列数: {df.shape[1]}")
print(f"列名: {list(df.columns)}")
print()

print("空值统计：")
null_count = df.isnull().sum()
null_total = null_count.sum()
if null_total == 0:
    print("  无空值 ✓")
else:
    print(null_count[null_count > 0])
print()

print("各列数据类型：")
print(df.dtypes.to_string())
print()

# ── 3. 各指标与 target 的相关系数 ──
print("=" * 50)
print("各特征与 target（患病）的相关系数")
print("=" * 50)
corr_with_target = df.corr(numeric_only=True)["target"].drop("target").sort_values(key=abs, ascending=False)
for col, val in corr_with_target.items():
    print(f"  {col:>10s}  {val:>8.4f}")
print()

# ── 4. 相关性热力图 ──
# 设置字体，避免中文报错 —— 指定系统存在的英文字体
sns.set_theme(style="white", font="DejaVu Sans")
plt.rcParams.update({
    "font.family": "DejaVu Sans",
    "axes.unicode_minus": False,
})

plt.figure(figsize=(10, 8))
corr_matrix = df.corr(numeric_only=True)
mask = None  # 显示完整矩阵

sns.heatmap(
    corr_matrix,
    annot=True,
    fmt=".2f",
    cmap="RdBu_r",
    center=0,
    square=True,
    linewidths=0.5,
    cbar_kws={"shrink": 0.75, "label": "相关系数"},
)

plt.title("Correlation Heatmap — Heart Disease Dataset", fontsize=14, pad=16)
plt.xticks(rotation=30, ha="right")
plt.yticks(rotation=0)
plt.tight_layout()
plt.show()
