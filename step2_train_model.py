import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, precision_score, recall_score

# ── 1. 读取数据 ──
df = pd.read_csv("heart.csv")
print(f"数据集大小: {df.shape[0]} 行, {df.shape[1]} 列\n")

# ── 2. 划分特征与标签 ──
X = df.iloc[:, :-1]   # 所有行，除最后一列外的所有列
y = df.iloc[:, -1]    # 所有行，最后一列 target

print(f"特征列 ({X.shape[1]} 个): {list(X.columns)}")
print(f"标签列: target")
print(f"  0（无心脏病）: {(y == 0).sum()} 人")
print(f"  1（有心脏病）: {(y == 1).sum()} 人\n")

# ── 3. 划分训练集 / 测试集 ──
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42
)
print(f"训练集: {X_train.shape[0]} 条")
print(f"测试集: {X_test.shape[0]} 条\n")

# ── 4. 训练模型 & 评估 ──
models = {
    "逻辑回归 (Logistic Regression)": LogisticRegression(max_iter=1000, random_state=42),
    "随机森林 (Random Forest)":       RandomForestClassifier(n_estimators=100, random_state=42),
}

results = []
for name, model in models.items():
    model.fit(X_train, y_train)
    y_pred = model.predict(X_test)

    acc  = accuracy_score(y_test, y_pred)
    prec = precision_score(y_test, y_pred)
    rec  = recall_score(y_test, y_pred)
    results.append((name, acc, prec, rec))

print("=" * 60)
print(f"{'模型':<30s} {'准确率':>8s} {'精确率':>8s} {'召回率':>8s}")
print("=" * 60)
for name, acc, prec, rec in results:
    print(f"{name:<30s} {acc:>8.2%} {prec:>8.2%} {rec:>8.2%}")
print("=" * 60)
print()

# 选出更优的模型
best = max(results, key=lambda r: r[1])
print(f"准确率最高的模型: {best[0]} ({best[1]:.2%})")
