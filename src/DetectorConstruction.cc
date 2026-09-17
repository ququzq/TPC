#include "DetectorConstruction.hh"
#include "MyMaterials.hh"
#include "MyElectricFieldMap.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

#include "G4ElectricField.hh"
#include "G4FieldManager.hh"
#include "G4EqMagElectricField.hh"
#include "G4ClassicalRK4.hh"
#include "G4ChordFinder.hh"
#include "G4MagIntegratorDriver.hh"
#include "G4TransportationManager.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalBorderSurface.hh"

#include "G4ios.hh"

// CADMesh
#include "CADMesh.hh"

namespace B1
{

// ============================================================
// DetectorConstruction 构造与析构
// ============================================================

DetectorConstruction::DetectorConstruction()
    : G4VUserDetectorConstruction()
{
}

DetectorConstruction::~DetectorConstruction()
{
}

// ============================================================
// Construct
// ============================================================

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    // ------------------------------------------------------------------------
    // 0. 材料定义
    // ------------------------------------------------------------------------
    G4NistManager* nist = G4NistManager::Instance();
    
    G4Material* LAr     = MyMaterials::GetMaterial("G4_lAr"); // 此时拿到的 LAr 包含了 40000 photons/MeV 属性！
    G4Material* GAr     = MyMaterials::GetMaterial("G4_Ar");
    G4Material* world_mat  = MyMaterials::GetMaterial("G4_AIR");
    G4Material* PMMA    = MyMaterials::GetMaterial("G4_PLEXIGLASS");
    G4Material* Cu      = MyMaterials::GetMaterial("G4_Cu");
    G4Material* Clevios = MyMaterials::GetMaterial("Clevios");
    G4Material* TPB     = MyMaterials::GetMaterial("TPB");
    
    if(!Clevios) Clevios  = PMMA; // 备用测试

    // ------------------------------------------------------------------------
    // World 逻辑与物理体积
    // ------------------------------------------------------------------------
    G4double world_size = 3.0 * m;
    G4Box* solidWorld = new G4Box("World", 0.5*world_size, 0.5*world_size, 0.5*world_size);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, true);

    // ------------------------------------------------------------------------
    // 1. 内部液氩 (Inner LAr Cylinder)
    // ------------------------------------------------------------------------
    G4double inLAr_r   = 0.184 * m;
    G4double inLAr_h   = 0.995 * m;
    
    G4Tubs* solidInnerLAr = new G4Tubs("solidInnerLAr", 0.0, inLAr_r, 0.5 * inLAr_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicInnerLAr = new G4LogicalVolume(solidInnerLAr, LAr, "logicInnerLAr");
    // 底面位于 z=0 -> 中心位于 z = inLAr_h / 2
    new G4PVPlacement(0, G4ThreeVector(0, 0, 0.5 * inLAr_h), logicInnerLAr, "physInnerLAr", logicWorld, false, 0, true);

    // ------------------------------------------------------------------------
    // 5. PMMA 容器构建 (利用母子嵌套关系解决环形挖空)
    // ------------------------------------------------------------------------
    // 5.1 PMMA 侧面圆柱壳 (包含液氩环与 Clevios 环)
    G4double pmmaSide_rmin = 0.184 * m;
    G4double pmmaSide_rmax = 0.190 * m;
    G4double pmmaSide_h    = 0.995 * m;

    G4Tubs* solidPMMASide = new G4Tubs("solidPMMASide", pmmaSide_rmin, pmmaSide_rmax, 0.5 * pmmaSide_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicPMMASide = new G4LogicalVolume(solidPMMASide, PMMA, "logicPMMASide");
    new G4PVPlacement(0, G4ThreeVector(0, 0, 0.5 * pmmaSide_h), logicPMMASide, "physPMMASide", logicWorld, false, 0, true);

    // 1.b & 2. 放置 79 个液氩环 / TPB 膜 / Clevios 环
    // (作为 PMMA 侧板的子体积放置，免去昂贵的布尔运算)
    G4double ring_w = 0.0001 * m;
    G4double ring_h = 0.0060 * m;

    // 液氩环：局部坐标 r in [0.184, 0.1841] -> 对应 PMMA 局部内半径 rmin 至 rmin + ring_w
    G4Tubs* solidLArRing = new G4Tubs("solidLArRing", pmmaSide_rmin, pmmaSide_rmin + ring_w, 0.5 * ring_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicLArRing = new G4LogicalVolume(solidLArRing, LAr, "logicLArRing");

    // TPB 波长位移膜：局部坐标 r in [0.1841, 0.1842]
    G4Tubs* solidTPBRing = new G4Tubs("solidTPBRing", pmmaSide_rmin + ring_w, pmmaSide_rmin + 2.0 * ring_w, 0.5 * ring_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicTPBRing = new G4LogicalVolume(solidTPBRing, TPB, "logicTPBRing");

    // Clevios 环：局部坐标 r in [0.1842, 0.1843]
    G4Tubs* solidCleviosRing = new G4Tubs("solidCleviosRing", pmmaSide_rmin + 2.0 * ring_w, pmmaSide_rmin + 3.0 * ring_w, 0.5 * ring_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicCleviosRing = new G4LogicalVolume(solidCleviosRing, Clevios, "logicCleviosRing");

    G4double z_step = 0.04975 * 0.25 * m;
    for (int i = 0; i < 79; ++i) {
        G4double z_bottom_abs = z_step * (i + 1);
        // 转换到 PMMA 侧板局部坐标 (PMMA 中心在 z = 0.5 * 0.995m)
        G4double z_local = (z_bottom_abs + 0.5 * ring_h) - (0.5 * pmmaSide_h);

        new G4PVPlacement(0, G4ThreeVector(0, 0, z_local), logicLArRing, "physLArRing", logicPMMASide, false, i, false);
        G4VPhysicalVolume* physTPBRing =
            new G4PVPlacement(0, G4ThreeVector(0, 0, z_local), logicTPBRing, "physTPBRing", logicPMMASide, false, i, false);
        G4VPhysicalVolume* physCleviosRing =
            new G4PVPlacement(0, G4ThreeVector(0, 0, z_local), logicCleviosRing, "physCleviosRing", logicPMMASide, false, i, false);

        // TPB 外侧镜面反射膜：把未被 TPB 吸收的光子反射回 TPB/LAr
        new G4LogicalBorderSurface("TPBReflector", physTPBRing, physCleviosRing,
                                   MyMaterials::GetOpticalSurface("TPBReflectorSurface"));
    }

    // 5.2 PMMA 底面
    G4double pmmaBot_r = 0.190 * m;
    G4double pmmaBot_h = 0.025 * m;
    G4Tubs* solidPMMABot = new G4Tubs("solidPMMABot", 0.0, pmmaBot_r, 0.5 * pmmaBot_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicPMMABot = new G4LogicalVolume(solidPMMABot, PMMA, "logicPMMABot");
    // 底面在 z = -0.025m -> 中心在 z = -0.0125m
    new G4PVPlacement(0, G4ThreeVector(0, 0, -0.025 * m + 0.5 * pmmaBot_h), logicPMMABot, "physPMMABot", logicWorld, false, 0, true);

    // ------------------------------------------------------------------------
    // 3. 液氩、气氩过渡区 (LAr Transition Zone)
    // ------------------------------------------------------------------------
    G4double transLAr_r = 0.250 * m;
    G4double transLAr_h = 0.005 * m;
    G4Tubs* solidTransLAr = new G4Tubs("solidTransLAr", 0.0, transLAr_r, 0.5 * transLAr_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicTransLAr = new G4LogicalVolume(solidTransLAr, LAr, "logicTransLAr");
    // 底面位于 z = 0.995m -> 中心位于 z = 0.995m + 0.0025m
    new G4PVPlacement(0, G4ThreeVector(0, 0, 0.995 * m + 0.5 * transLAr_h), logicTransLAr, "physTransLAr", logicWorld, false, 0, true);

    // ------------------------------------------------------------------------
    // 4. 气氩 (Gaseous Argon) 
    // ------------------------------------------------------------------------
    G4double gar_r = 0.250 * m;
    G4double gar_h = 0.008 * m;
    G4Tubs* solidGAr = new G4Tubs("solidGAr", 0.0, gar_r, 0.5 * gar_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicGAr = new G4LogicalVolume(solidGAr, GAr, "logicGAr");
    // 底面紧接过渡区 z = 1.000m -> 中心位于 z = 1.004m
    new G4PVPlacement(0, G4ThreeVector(0, 0, 1.000 * m + 0.5 * gar_h), logicGAr, "physGAr", logicWorld, false, 0, true);

    // 5.3 PMMA 顶面
    G4double pmmaTop_r = 0.250 * m;
    G4double pmmaTop_h = 0.025 * m;
    G4Tubs* solidPMMATop = new G4Tubs("solidPMMATop", 0.0, pmmaTop_r, 0.5 * pmmaTop_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicPMMATop = new G4LogicalVolume(solidPMMATop, PMMA, "logicPMMATop");
    // 底面紧接气氩顶端 z = 1.008m -> 中心位于 z = 1.0205m
    new G4PVPlacement(0, G4ThreeVector(0, 0, 1.008 * m + 0.5 * pmmaTop_h), logicPMMATop, "physPMMATop", logicWorld, false, 0, true);

    // ------------------------------------------------------------------------
    // 7. 外部液氩 1 (Outer LAr 1) & 6. Guarding Ring 嵌套放置
    // ------------------------------------------------------------------------
    G4double outLAr1_rmin = 0.190 * m;
    G4double outLAr1_rmax = 0.290 * m;
    G4double outLAr1_h    = 1.020 * m;

    G4Tubs* solidOuterLAr1 = new G4Tubs("solidOuterLAr1", outLAr1_rmin, outLAr1_rmax, 0.5 * outLAr1_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicOuterLAr1 = new G4LogicalVolume(solidOuterLAr1, LAr, "logicOuterLAr1");
    // 底面在 z = -0.025m -> 中心在 z = -0.025m + 0.51m = 0.485m
    new G4PVPlacement(0, G4ThreeVector(0, 0, -0.025 * m + 0.5 * outLAr1_h), logicOuterLAr1, "physOuterLAr1", logicWorld, false, 0, true);

    // 6. Guarding Ring 嵌套作为 Outer LAr 1 的子体积放置 (同样避免布尔减法)
    G4double gr_rmin = 0.190 * m;
    G4double gr_rmax = 0.215 * m;
    G4double gr_h    = 0.005 * m;
    G4Tubs* solidGR = new G4Tubs("solidGR", gr_rmin, gr_rmax, 0.5 * gr_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicGR = new G4LogicalVolume(solidGR, Cu, "logicGR");
    
    // Guarding Ring 全局底面 z = 0.990m, 中心 z = 0.9925m
    // 转换至 OuterLAr1 局部坐标 (OuterLAr1 中心位于 z = 0.485m)
    G4double gr_z_local = 0.9925 * m - (-0.025 * m + 0.5 * outLAr1_h);
    new G4PVPlacement(0, G4ThreeVector(0, 0, gr_z_local), logicGR, "physGuardingRing", logicOuterLAr1, false, 0, true);

    // ------------------------------------------------------------------------
    // 7.b 外部液氩 2 (Outer LAr 2)
    // ------------------------------------------------------------------------
    G4double outLAr2_rmin = 0.250 * m;
    G4double outLAr2_rmax = 0.290 * m;
    G4double outLAr2_h    = 0.038 * m;
    G4Tubs* solidOuterLAr2 = new G4Tubs("solidOuterLAr2", outLAr2_rmin, outLAr2_rmax, 0.5 * outLAr2_h, 0.0, 360.0 * deg);
    G4LogicalVolume* logicOuterLAr2 = new G4LogicalVolume(solidOuterLAr2, LAr, "logicOuterLAr2");
    // 底面位于 z = 0.995m -> 中心位于 z = 0.995m + 0.019m = 1.014m
    new G4PVPlacement(0, G4ThreeVector(0, 0, 0.995 * m + 0.5 * outLAr2_h), logicOuterLAr2, "physOuterLAr2", logicWorld, false, 0, true);

    return physWorld;
}

// ============================================================
// ConstructSDandField
// ============================================================

void DetectorConstruction::ConstructSDandField()
{
    // 1. 创建 COMSOL 三维电场 Map
    G4ElectricField* myField = new MyElectricFieldMap(
        "../file/geant4_3d_field.txt",
        m,
        volt/m
    );

    // 2. 建立运动方程
    G4EqMagElectricField* equation = new G4EqMagElectricField(myField);

    // 3. Runge-Kutta 积分器
    G4MagIntegratorStepper* stepper = new G4ClassicalRK4(equation, 8);

    // 4. Integration Driver
    G4double minStep = 0.01 * mm;
    G4MagInt_Driver* driver = new G4MagInt_Driver(minStep, stepper, stepper->GetNumberOfVariables());

    // 5. Chord Finder
    G4ChordFinder* chordFinder = new G4ChordFinder(driver);

    // 6. 设置为 Geant4 全局电场
    G4TransportationManager* transportationManager = G4TransportationManager::GetTransportationManager();
    G4FieldManager* globalFieldManager = transportationManager->GetFieldManager();

    globalFieldManager->SetDetectorField(myField);
    globalFieldManager->SetChordFinder(chordFinder);

    G4cout << "============================================" << G4endl;
    G4cout << "Global electric field has been installed." << G4endl;
    G4cout << "Minimum integration step = " << minStep / mm << " mm" << G4endl;
    G4cout << "============================================" << G4endl;
}

} // namespace B1