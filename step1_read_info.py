import pydicom

# 读取 DICOM 文件
ds = pydicom.dcmread("test.dcm")

# 逐个提取信息，缺失时显示"未提供"
patient_name   = getattr(ds, "PatientName", "未提供")
patient_id     = getattr(ds, "PatientID", "未提供")
modality       = getattr(ds, "Modality", "未提供")
rows           = getattr(ds, "Rows", "未提供")
columns        = getattr(ds, "Columns", "未提供")

# 输出
print("=" * 36)
print("  DICOM 文件基本信息")
print("=" * 36)
print(f"  病人姓名：{patient_name}")
print(f"  病人 ID ：{patient_id}")
print(f"  检查类型：{modality}")
print(f"  图像宽度：{columns}")
print(f"  图像高度：{rows}")
print("=" * 36)
