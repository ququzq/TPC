#include "PhysicsList.hh"

// 工厂模式及电磁物理包
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT.hh"

// 光学科
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"

#include "G4SystemOfUnits.hh"

PhysicsList::PhysicsList()
    : G4VModularPhysicsList()
{
    // 1. 标准电磁物理
    RegisterPhysics(new G4EmStandardPhysics_option4());

    // 2. 衰变物理
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4RadioactiveDecayPhysics());

    // 3. 强子物理
    RegisterPhysics(new G4HadronPhysicsFTFP_BERT());

    // 4. 光学物理模块
    G4OpticalPhysics* opticalPhysics = new G4OpticalPhysics();
    G4OpticalParameters* params = G4OpticalParameters::Instance();

    // 开启物理过程
    params->SetProcessActivation("Scintillation", true);
    params->SetProcessActivation("OpAbsorption", true);
    params->SetProcessActivation("OpRayleigh", true);
    params->SetProcessActivation("OpBoundary", true);

    // 修正后的 API 函数名：
    params->SetScintByParticleType(false); // 使用正确的方法名
    params->SetScintTrackSecondariesFirst(true);
    params->SetScintStackPhotons(true);

    params->SetScintStackPhotons(true);
    RegisterPhysics(opticalPhysics);
}
PhysicsList::~PhysicsList()
{
}

void PhysicsList::SetCuts()
{
    // 设置全局生产截断值 (Secondary Cut)
    G4VModularPhysicsList::SetCuts();
    SetDefaultCutValue(0.1 * mm);
}