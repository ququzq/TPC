import numpy as np
import matplotlib.pyplot as plt

# ============================================================
# 1. 文件路径
# ============================================================

filename = "photons_in_LAr.txt"

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
# 6. 绘制波长分布
# ============================================================

plt.figure(figsize=(8, 6))

plt.hist(
    wavelength_nm,
    bins=100,
    edgecolor="black"
)

plt.xlabel("Wavelength (nm)")
plt.ylabel("Number of Photons")
plt.title("Photon Wavelength Distribution")

plt.grid(alpha=0.3)

plt.tight_layout()
plt.show()