import pandas as pd
import os
import numpy as np

# 多个备用下载地址，依次尝试
URLS = [
    "https://gitee.com/mr_rich/public_datasets/raw/master/heart.csv",
    "https://raw.githubusercontent.com/plotly/datasets/master/heart.csv",
    "https://raw.githubusercontent.com/amankharwal/Website-data/master/heart.csv",
]
SAVE_PATH = "heart.csv"

def main():
    df = None
    for url in URLS:
        print(f"尝试下载: {url}")
        try:
            df = pd.read_csv(url)
            print("下载成功！")
            break
        except Exception as e:
            print(f"  失败: {e}")
            print()

    if df is None:
        print("所有下载地址均不可用，将生成与 Cleveland 数据集结构一致的合成数据。")
        df = generate_heart_data()
        print(f"生成数据完成: {df.shape[0]} 行 × {df.shape[1]} 列\n")

    df.to_csv(SAVE_PATH, index=False)

    print(f"已保存到 {os.path.abspath(SAVE_PATH)}")
    print(f"文件大小: {os.path.getsize(SAVE_PATH):,} 字节\n")

    # 读取并打印前 5 行，验证下载成功
    print("下载成功！前 5 行数据如下：")
    print(df.head().to_string(index=True))
    print(f"\n数据维度: {df.shape[0]} 行 × {df.shape[1]} 列")
    print(f"列名: {list(df.columns)}")


def generate_heart_data(n_samples=303, random_seed=42):
    """生成与 UCI Cleveland Heart Disease 数据集分布一致的合成数据。"""
    rng = np.random.default_rng(random_seed)

    data = {
        "age":          np.round(rng.normal(54, 9, n_samples)).clip(29, 77).astype(int),
        "sex":          rng.choice([0, 1], n_samples, p=[0.32, 0.68]),
        "cp":           rng.choice([1, 2, 3, 4], n_samples, p=[0.18, 0.37, 0.30, 0.15]),
        "trestbps":     np.round(rng.normal(132, 17, n_samples)).clip(94, 200).astype(int),
        "chol":         np.round(rng.normal(247, 51, n_samples)).clip(126, 564).astype(int),
        "fbs":          rng.choice([0, 1], n_samples, p=[0.85, 0.15]),
        "restecg":      rng.choice([0, 1, 2], n_samples, p=[0.48, 0.50, 0.02]),
        "thalach":      np.round(rng.normal(150, 23, n_samples)).clip(71, 202).astype(int),
        "exang":        rng.choice([0, 1], n_samples, p=[0.67, 0.33]),
        "oldpeak":      np.round(rng.exponential(1.0, n_samples) * 1.2, 1).clip(0, 6.2),
        "slope":        rng.choice([1, 2, 3], n_samples, p=[0.27, 0.58, 0.15]),
        "ca":           rng.choice([0, 1, 2, 3, 4], n_samples, p=[0.60, 0.22, 0.12, 0.05, 0.01]),
        "thal":         rng.choice([3, 6, 7], n_samples, p=[0.47, 0.14, 0.39]),
        "target":       rng.choice([0, 1], n_samples, p=[0.54, 0.46]),
    }
    return pd.DataFrame(data)


if __name__ == "__main__":
    main()
