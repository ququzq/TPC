#include "SteppingAction.hh"
#include "RunAction.hh"
#include "MyElectricFieldMap.hh" // 你的 COMSOL 电场地图类

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4Electron.hh"

// ===== 修改: 用于读取光学边界过程的状态 =====
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
// ============================================

namespace B1
{

// ===== 修改: 判断某个边界状态是否属于"反射" =====
namespace
{
G4bool IsReflectionStatus(G4OpBoundaryProcessStatus status)
{
    switch (status)
    {
        case FresnelReflection:
        case TotalInternalReflection:
        case LambertianReflection:
        case LobeReflection:
        case SpikeReflection:
        case BackScattering:
        case PolishedLumirrorAirReflection:
        case PolishedLumirrorGlueReflection:
        case PolishedAirReflection:
        case PolishedTeflonAirReflection:
        case PolishedTiOAirReflection:
        case PolishedTyvekAirReflection:
        case PolishedVM2000AirReflection:
        case PolishedVM2000GlueReflection:
        case EtchedLumirrorAirReflection:
        case EtchedLumirrorGlueReflection:
        case EtchedAirReflection:
        case EtchedTeflonAirReflection:
        case EtchedTiOAirReflection:
        case EtchedTyvekAirReflection:
        case EtchedVM2000AirReflection:
        case EtchedVM2000GlueReflection:
        case GroundLumirrorAirReflection:
        case GroundLumirrorGlueReflection:
        case GroundAirReflection:
        case GroundTeflonAirReflection:
        case GroundTiOAirReflection:
        case GroundTyvekAirReflection:
        case GroundVM2000AirReflection:
        case GroundVM2000GlueReflection:
        case CoatedDielectricReflection:
            return true;
        default:
            return false;
    }
}
}  // namespace
// ============================================

SteppingAction::SteppingAction(RunAction* runAction)
    : G4UserSteppingAction(), fRunAction(runAction) {}

SteppingAction::~SteppingAction() {}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    G4Track* track = step->GetTrack();
    G4StepPoint* postPoint = step->GetPostStepPoint();
    G4VPhysicalVolume* postVolume = postPoint->GetPhysicalVolume();

    // ===== 修改: 统计在光学边界发生反射的光子能量 =====
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition() &&
        postPoint->GetStepStatus() == fGeomBoundary)
    {
        G4OpBoundaryProcess* boundary = nullptr;
        G4ProcessManager* pm = track->GetDefinition()->GetProcessManager();
        G4ProcessVector* pv = pm->GetProcessList();

        G4int nProcesses = pv->entries();
        for (G4int i = 0; i < nProcesses; ++i)
        {
            G4VProcess* proc = (*pv)[i];
            if (proc->GetProcessName() == "OpBoundary")
            {
                boundary = static_cast<G4OpBoundaryProcess*>(proc);
                break;
            }
        }

        if (boundary && IsReflectionStatus(boundary->GetStatus()))
        {
            if (fRunAction)
            {
                fRunAction->RecordReflectedPhoton(track->GetKineticEnergy());
            }
        }
    }
    // ==============================================

if (track->GetDefinition() ==G4OpticalPhoton::OpticalPhotonDefinition())
//if (track->GetDefinition() == G4Electron::ElectronDefinition())
{
    if (track->GetCurrentStepNumber() == 1)
    {
        G4VPhysicalVolume* volume = track->GetVolume();

        if (volume)
        {
            G4String volName = volume->GetName();

            if (volName == "physInnerLAr" ||
                volName == "physTransLAr" ||
                volName == "physOuterLAr1" ||
                volName == "physOuterLAr2")
            {
                G4double energy =
                    track->GetKineticEnergy();

                if (fRunAction)
                {
                    fRunAction->RecordPhoton(energy);
                }
            }
        }
    }
}


    if (postVolume && postVolume->GetName() == "World") {
       track->SetTrackStatus(fStopAndKill);
        return;
    }
}

}
 // namespace B1