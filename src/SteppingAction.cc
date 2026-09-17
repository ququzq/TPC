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

namespace B1
{

SteppingAction::SteppingAction(RunAction* runAction)
    : G4UserSteppingAction(), fRunAction(runAction) {}

SteppingAction::~SteppingAction() {}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    G4Track* track = step->GetTrack();
    G4StepPoint* postPoint = step->GetPostStepPoint();
    G4VPhysicalVolume* postVolume = postPoint->GetPhysicalVolume();

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