import os

import numpy as np
import matplotlib.pyplot as plt

# ============================================================
# 1. 文件路径
# ============================================================

filename = "photons_in_LAr.txt"
reflected_filename = "photon_reflected.txt"

# ============================================================
# 2. 读取数据
#    第一列：Photon ID
#    第二列：Energy (eV)
# ============================================================

data = np.loadtxt(filename, comments="#")

photon_id = data[:, 0]
energy_eV = data[:, 1]

# ============================================================
# 3. 能量 -> 波长
#
# lambda(nm) = 1239.841984 / E(eV)
# ============================================================

wavelength_nm = 1239.841984 / energy_eV

# ============================================================
# 4. 输出基本信息
# ============================================================

print("============================================")
print(f"光子总数: {len(wavelength_nm)}")
print(f"平均能量: {np.mean(energy_eV):.4f} eV")
print(f"平均波长: {np.mean(wavelength_nm):.4f} nm")
print(f"波长最小值: {np.min(wavelength_nm):.4f} nm")
print(f"波长最大值: {np.max(wavelength_nm):.4f} nm")
print(f"波长标准差: {np.std(wavelength_nm):.4f} nm")
print("============================================")

# ============================================================
# 5. 保存波长数据
# ============================================================

output_data = np.column_stack((photon_id, wavelength_nm))

np.savetxt(
    "photon_wavelength.txt",
    output_data,
    fmt=["%d", "%.6f"],
    header="Photon_ID\tWavelength(nm)"
)

print("波长数据已保存到 photon_wavelength.txt")

# ============================================================
# 6. 读取被反射光子并转换波长 (新增)
#    第一列：Reflected Photon ID
#    第二列：Energy (eV)
# ============================================================

ref_wavelength_nm = None

if os.path.exists(reflected_filename):
    ref_data = np.loadtxt(reflected_filename, comments="#")

    # 只有一条记录时保证是二维数组
    if ref_data.ndim == 1:
        ref_data = ref_data.reshape(1, -1)

    ref_id = ref_data[:, 0]
    ref_energy_eV = ref_data[:, 1]
    ref_wavelength_nm = 1239.841984 / ref_energy_eV

    print("============================================")
    print(f"被反射光子(反射事件)总数: {len(ref_wavelength_nm)}")
    print(f"平均能量: {np.mean(ref_energy_eV):.4f} eV")
    print(f"平均波长: {np.mean(ref_wavelength_nm):.4f} nm")
    print(f"波长最小值: {np.min(ref_wavelength_nm):.4f} nm")
    print(f"波长最大值: {np.max(ref_wavelength_nm):.4f} nm")
    print(f"波长标准差: {np.std(ref_wavelength_nm):.4f} nm")
    print("============================================")

    # 保存反射光子波长数据
    np.savetxt(
        "photon_reflected_wavelength.txt",
        np.column_stack((ref_id, ref_wavelength_nm)),
        fmt=["%d", "%.6f"],
        header="Reflected_Photon_ID\tWavelength(nm)"
    )

    print("反射光子波长数据已保存到 photon_reflected_wavelength.txt")
else:
    print(f"警告: 未找到 {reflected_filename}，跳过反射光子分布")

# ============================================================
# 7. 绘制波长分布
#    (新增右侧: 被反射光子的波长分布)
# ============================================================

ncols = 2 if ref_wavelength_nm is not None else 1
fig, axes = plt.subplots(1, ncols, figsize=(7 * ncols, 6))

if ncols == 1:
    axes = [axes]

# 7.1 进入 LAr 的光子
axes[0].hist(
    wavelength_nm,
    bins=100,
    edgecolor="black"
)
axes[0].set_xlabel("Wavelength (nm)")
axes[0].set_ylabel("Number of Photons")
axes[0].set_title("Photon Wavelength Distribution (in LAr)")
axes[0].grid(alpha=0.3)

# 7.2 被反射的光子
if ref_wavelength_nm is not None:
    axes[1].hist(
        ref_wavelength_nm,
        bins=100,
        edgecolor="black",
        color="tab:orange"
    )
    axes[1].set_xlabel("Wavelength (nm)")
    axes[1].set_ylabel("Number of Reflections")
    axes[1].set_title("Reflected Photon Wavelength Distribution")
    axes[1].grid(alpha=0.3)

plt.tight_layout()
plt.show()
