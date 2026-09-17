#ifndef MyElectricFieldMap_hh
#define MyElectricFieldMap_hh 1

#include "G4ElectricField.hh"
#include "globals.hh"
#include <vector>
#include <string>

/**
 * @brief 自定义 3D 电场映射类 (继承自 G4ElectricField)
 * 用于加载从 COMSOL 导出的 3D 规则网格电场文件，并在 Geant4 中进行三线性插值
 */
class MyElectricFieldMap : public G4ElectricField
{
public:
    // 构造函数：filename 为转换后的 3D 文本文件路径，lenUnit 为长度单位转换系数，fieldUnit 为场强单位转换系数
    MyElectricFieldMap(const std::string& filename, G4double lenUnit = 1.0, G4double fieldUnit = 1.0);
    virtual ~MyElectricFieldMap();

    /**
     * @brief Geant4 核心纯虚函数实现
     * @param Point[4] 粒子当前的时空位置: Point[0]=x, Point[1]=y, Point[2]=z, Point[3]=t
     * @param field[6] 输出数组: field[0..2] 为 Bx,By,Bz (纯电场填0), field[3..5] 为 Ex,Ey,Ez
     */
    virtual void GetFieldValue(const G4double Point[4], G4double* field) const override;

private:
    // 读取 3D 网格文本文件
    void LoadFieldMap(const std::string& filename);

private:
    // 网格节点数量
    int fNx, fNy, fNz;

    // 空间边界与网格步长 (自动换算为 Geant4 内部单位，如 mm)
    G4double fMinX, fMaxX, fDx;
    G4double fMinY, fMaxY, fDy;
    G4double fMinZ, fMaxZ, fDz;

    // 单位换算系数
    G4double fLenUnit;   // 坐标换算 (例如: m -> mm)
    G4double fFieldUnit; // 电场强度换算 (例如: V/m -> volt/m)

    // 存储三维网格节点上的电场数据 Ex, Ey, Ez
    std::vector<G4double> fExMap;
    std::vector<G4double> fEyMap;
    std::vector<G4double> fEzMap;
};

#endif