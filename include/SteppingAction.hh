#ifndef B1SteppingAction_hh
#define B1SteppingAction_hh 1

#include "G4UserSteppingAction.hh"

namespace B1
{

class RunAction;

class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction(RunAction* runAction);
    virtual ~SteppingAction();

    virtual void UserSteppingAction(const G4Step* step) override;

private:
    RunAction* fRunAction;
};

} // namespace B1

#endif