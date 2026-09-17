#include "MyMaterials.hh"

#include "G4NistManager.hh"
#include "G4Element.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"

#include <map>

static std::map<G4String, G4OpticalSurface*> fSurfaceMap;

void MyMaterials::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();

    // 1. 加载 NIST 标准材料
    nist->FindOrBuildMaterial("G4_AIR");
    nist->FindOrBuildMaterial("G4_lAr");
    nist->FindOrBuildMaterial("G4_Ar");
    nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    nist->FindOrBuildMaterial("G4_Cu");
    nist->FindOrBuildMaterial("G4_Al");

    // 2. 创建自定义材料及添加闪烁光/光学属性
    ConstructCustomMaterials();

    // 3. 创建表面属性
    ConstructSurfaces();
}

void MyMaterials::ConstructCustomMaterials()
{
    G4NistManager* nist = G4NistManager::Instance();

    // ---------------------------------------------------------
    // 光子能量定义 (覆盖 VUV 128nm ~ 可见光 700nm)
    // ---------------------------------------------------------
    // 128 nm 对应约为 9.69 eV
    G4double photonEnergy_VUV[] = { 8.0 * eV,  9.69 * eV,  10.5 * eV }; 
    const G4int nEntries_VUV = sizeof(photonEnergy_VUV) / sizeof(G4double);

    // ---------------------------------------------------------
    // 1. 液氩 (G4_lAr) 的光学与闪烁属性
    // ---------------------------------------------------------
    G4Material* lAr = nist->FindOrBuildMaterial("G4_lAr");
    G4MaterialPropertiesTable* mptLAr = new G4MaterialPropertiesTable();

    // 液氩在 128nm 处的折射率约为 1.23，吸收长度设为 >1m，散射长度约 60cm
    G4double rIndexLAr[]  = { 1.23, 1.23, 1.23 };
    G4double absLAr[]     = { 2.0 * m, 2.0 * m, 2.0 * m };
    G4double rayleighLAr[]= { 60.0 * cm, 60.0 * cm, 60.0 * cm };
    
    // 液氩 128nm 闪烁光谱分布 (中心在 9.69 eV)
    G4double scintillLAr[]= { 0.1, 1.0, 0.1 };

    const int nPoints = 50;
    G4double e_min = 8.0 * eV;
    G4double e_max = 10.5 * eV;
    G4double e_center = 9.69 * eV;
    G4double sigma = 0.322 * eV; // 控制高斯峰宽

    std::vector<G4double> energies;
    std::vector<G4double> spectrum;

    for (int i = 0; i < nPoints; ++i) {
        G4double e = e_min + i * (e_max - e_min) / (nPoints - 1);
        G4double val = std::exp(-0.5 * std::pow((e - e_center) / sigma, 2));
        energies.push_back(e);
        spectrum.push_back(val);
    }

    // 光子能量点
    mptLAr->AddProperty("RINDEX", photonEnergy_VUV, rIndexLAr, nEntries_VUV);
    mptLAr->AddProperty("ABSLENGTH", photonEnergy_VUV, absLAr, nEntries_VUV);
    mptLAr->AddProperty("RAYLEIGH", photonEnergy_VUV, rayleighLAr, nEntries_VUV);

    // 闪烁光谱分布
    mptLAr->AddProperty("SCINTILLATIONCOMPONENT1", energies.data(), spectrum.data(), nPoints);
    mptLAr->AddProperty("SCINTILLATIONCOMPONENT2", energies.data(), spectrum.data(), nPoints);

    // 发光产额与衰减时间
    mptLAr->AddConstProperty("SCINTILLATIONYIELD", 40000.0 / MeV);
    mptLAr->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mptLAr->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 6.0 * ns);
    mptLAr->AddConstProperty("SCINTILLATIONTIMECONSTANT2", 1300.0 * ns);

    // 新版 Geant4 推荐的快慢成分相对权重设置：
    mptLAr->AddConstProperty("SCINTILLATIONRISETIME1", 0.0 * ns);
    mptLAr->AddConstProperty("SCINTILLATIONRISETIME2", 0.0 * ns);
    mptLAr->AddConstProperty("SCINTILLATIONYIELD1", 0.25);
    mptLAr->AddConstProperty("SCINTILLATIONYIELD2", 0.75);

    lAr->SetMaterialPropertiesTable(mptLAr);
    G4cout << "\n========== LAr Material Properties ==========\n";
    mptLAr->DumpTable();
    G4cout << "=============================================\n";

    // ---------------------------------------------------------
    // 2. 气氩 (G4_Ar) 的光学与闪烁属性
    // ---------------------------------------------------------
    G4Material* gAr = nist->FindOrBuildMaterial("G4_Ar");
    G4MaterialPropertiesTable* mptGAr = new G4MaterialPropertiesTable();

    // 气氩折射率接近 1.0
    G4double rIndexGAr[] = { 1.0003, 1.0003, 1.0003 };
    G4double absGAr[]    = { 10.0 * m, 10.0 * m, 10.0 * m };

    mptGAr->AddProperty("RINDEX", photonEnergy_VUV, rIndexGAr, nEntries_VUV);
    mptGAr->AddProperty("ABSLENGTH", photonEnergy_VUV, absGAr, nEntries_VUV);

    // 气氩闪烁光属性
    mptGAr->AddProperty("SCINTILLATIONCOMPONENT1", energies.data(), spectrum.data(), nPoints);
    mptGAr->AddProperty("SCINTILLATIONCOMPONENT2", energies.data(), spectrum.data(), nPoints);

    mptGAr->AddConstProperty("SCINTILLATIONYIELD", 15000.0 / MeV); // 常压气氩产额约为 15000 photons/MeV
    mptGAr->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mptGAr->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 10.0 * ns);
    mptGAr->AddConstProperty("SCINTILLATIONTIMECONSTANT2", 3200.0 * ns);
    mptGAr->AddConstProperty("SCINTILLATIONYIELD1", 0.1);
    mptGAr->AddConstProperty("SCINTILLATIONYIELD2", 0.9);

    gAr->SetMaterialPropertiesTable(mptGAr);

    // ---------------------------------------------------------
    // 2.b PMMA (G4_PLEXIGLASS) 的光学属性
    //     - 可见光区基本透明，VUV(128nm) 强吸收
    //     - 折射率用于 LAr/PMMA 界面的菲涅尔反射与折射
    // ---------------------------------------------------------
    G4Material* pmma = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    G4MaterialPropertiesTable* mptPMMA = new G4MaterialPropertiesTable();

    // 能量点 (eV): 可见光 700/550/400nm + VUV 155/128/118nm
    G4double photonEnergy_PMMA[] = { 1.77 * eV, 2.25 * eV, 3.10 * eV,
                                     8.00 * eV, 9.69 * eV, 10.50 * eV };
    const G4int nEntries_PMMA = sizeof(photonEnergy_PMMA) / sizeof(G4double);

    // PMMA 折射率: 可见光约 1.49, VUV 约 1.60
    G4double rIndexPMMA[] = { 1.49, 1.49, 1.49, 1.60, 1.60, 1.60 };

    // 吸收长度: 可见光近乎透明(10m), VUV 强吸收(约 1um)
    G4double absPMMA[] = { 10.0 * m, 10.0 * m, 10.0 * m,
                           1.0 * um, 1.0 * um, 1.0 * um };

    mptPMMA->AddProperty("RINDEX", photonEnergy_PMMA, rIndexPMMA, nEntries_PMMA);
    mptPMMA->AddProperty("ABSLENGTH", photonEnergy_PMMA, absPMMA, nEntries_PMMA);

    pmma->SetMaterialPropertiesTable(mptPMMA);
    G4cout << "\n========== PMMA Material Properties ==========\n";
    mptPMMA->DumpTable();
    G4cout << "==============================================\n";

    // ---------------------------------------------------------
    // 3. 基础元素定义
    // ---------------------------------------------------------
    G4Element* elH = nist->FindOrBuildElement("H");
    G4Element* elC = nist->FindOrBuildElement("C");
    G4Element* elO = nist->FindOrBuildElement("O");
    G4Element* elS = nist->FindOrBuildElement("S");

    // ---------------------------------------------------------
    // 4. 自定义材料：CLEVIOS™ FE T
    // ---------------------------------------------------------
    G4double cleviosDensity = 1.06 * g/cm3;
    G4Material* Clevios = new G4Material("Clevios", cleviosDensity, 4);
    Clevios->AddElement(elC, 14);
    Clevios->AddElement(elH, 16);
    Clevios->AddElement(elO, 6);
    Clevios->AddElement(elS, 2);

    G4MaterialPropertiesTable* mptClevios = new G4MaterialPropertiesTable();
    G4double photonEnergy_Vis[] = { 1.77 * eV, 2.25 * eV, 3.10 * eV }; // 700nm, 550nm, 400nm
    G4double rIndexClevios[]    = { 1.53, 1.53, 1.53 };
    G4double absClevios[]       = { 51.5 * um, 51.5 * um, 51.5 * um };

    mptClevios->AddProperty("RINDEX", photonEnergy_Vis, rIndexClevios, 3);
    mptClevios->AddProperty("ABSLENGTH", photonEnergy_Vis, absClevios, 3);

    Clevios->SetMaterialPropertiesTable(mptClevios);

    // ---------------------------------------------------------
    // 5. 自定义材料：TPB (四联苯) 波长位移涂层
    //    吸收 128nm VUV，再各向同性发射 ~420nm 可见光
    // ---------------------------------------------------------
    G4double tpbDensity = 1.079 * g/cm3;
    G4Material* TPB = new G4Material("TPB", tpbDensity, 2);
    TPB->AddElement(elC, 28);
    TPB->AddElement(elH, 22);

    G4MaterialPropertiesTable* mptTPB = new G4MaterialPropertiesTable();

    // 能量点同时覆盖 VUV(128nm) 与 TPB 发射(420nm)
    G4double photonEnergy_TPB[] = { 2.58 * eV, 2.95 * eV, 3.26 * eV,
                                    8.00 * eV, 9.69 * eV, 10.50 * eV };
    const G4int nEntries_TPB = sizeof(photonEnergy_TPB) / sizeof(G4double);

    // TPB 折射率: 可见光约 1.60, VUV 约 1.70
    G4double rIndexTPB[] = { 1.60, 1.60, 1.60, 1.70, 1.70, 1.70 };

    // 普通吸收: TPB 内可忽略（真正的 VUV 吸收交给 WLS 过程）
    G4double absTPB[] = { 10.0 * m, 10.0 * m, 10.0 * m,
                          10.0 * m, 10.0 * m, 10.0 * m };

    mptTPB->AddProperty("RINDEX", photonEnergy_TPB, rIndexTPB, nEntries_TPB);
    mptTPB->AddProperty("ABSLENGTH", photonEnergy_TPB, absTPB, nEntries_TPB);

    // WLS 吸收长度: 只对 128nm VUV 强吸收, 对 420nm 透明
    G4double wlsAbsTPB[] = { 10.0 * m, 10.0 * m, 10.0 * m,
                             1.0 * um, 1.0 * um, 1.0 * um };
    mptTPB->AddProperty("WLSABSLENGTH", photonEnergy_TPB, wlsAbsTPB, nEntries_TPB);

    // WLS 发射谱 (TPB 峰值 ~420nm / 2.95eV)
    G4double wlsEmissionEnergy[] = { 2.58 * eV, 2.76 * eV, 2.95 * eV, 3.10 * eV, 3.26 * eV };
    G4double wlsEmission[]       = { 0.10, 0.55, 1.00, 0.55, 0.10 };
    mptTPB->AddProperty("WLSCOMPONENT", wlsEmissionEnergy, wlsEmission, 5);

    // 每吸收一个 VUV 光子发射 1 个 420nm 光子, 延迟 1ns
    mptTPB->AddConstProperty("WLSMEANNUMBERPHOTONS", 1.0);
    mptTPB->AddConstProperty("WLSTIMECONSTANT", 1.0 * ns);

    TPB->SetMaterialPropertiesTable(mptTPB);
    G4cout << "\n========== TPB Material Properties ==========\n";
    mptTPB->DumpTable();
    G4cout << "=============================================\n";
}

void MyMaterials::ConstructSurfaces()
{
    G4OpticalSurface* pmmaSurface = new G4OpticalSurface(
        "PmmaReflectiveSurface",
        glisur,              // 表面模型
        ground,              // 表面粗糙度
        dielectric_metal,    // 介质-金属边界
        0.9                  // 抛光度
    );

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();
    G4double photonEnergy[] = { 2.0 * eV, 3.0 * eV, 10.0 * eV };
    G4double reflectivity[] = { 0.95,     0.95,     0.95 };

    mpt->AddProperty("REFLECTIVITY", photonEnergy, reflectivity, 3);
    pmmaSurface->SetMaterialPropertiesTable(mpt);
    fSurfaceMap["PmmaReflectiveSurface"] = pmmaSurface;

    // TPB 膜外侧镜面反射膜: 镜面反射未被 TPB 吸收的光子
    G4OpticalSurface* tpbReflector = new G4OpticalSurface(
        "TPBReflectorSurface",
        glisur,              // 表面模型
        polished,            // 镜面 (specular)
        dielectric_metal,    // 金属反射膜
        1.0                  // 抛光
    );

    G4MaterialPropertiesTable* mptReflector = new G4MaterialPropertiesTable();
    G4double reflEnergy[]        = { 2.0 * eV, 2.95 * eV, 10.5 * eV };
    G4double reflectivityRefl[]  = { 0.98,    0.98,     0.98 };

    mptReflector->AddProperty("REFLECTIVITY", reflEnergy, reflectivityRefl, 3);
    tpbReflector->SetMaterialPropertiesTable(mptReflector);
    fSurfaceMap["TPBReflectorSurface"] = tpbReflector;
}

G4Material* MyMaterials::GetMaterial(const G4String& name)
{
    Construct();
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* mat = nist->FindOrBuildMaterial(name);

    if (!mat) {
        G4Exception("MyMaterials::GetMaterial",
                    "MaterialNotFound",
                    FatalException,
                    ("Material not found: " + name).c_str());
    }

    return mat;
}

G4OpticalSurface* MyMaterials::GetOpticalSurface(const G4String& name)
{
    auto it = fSurfaceMap.find(name);
    if (it != fSurfaceMap.end()) {
        return it->second;
    }

    G4Exception("MyMaterials::GetOpticalSurface",
                "SurfaceNotFound",
                FatalException,
                ("Optical surface not found: " + name).c_str());
    return nullptr;
}