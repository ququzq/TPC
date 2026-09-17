#ifndef PhysicsList_hh
#define PhysicsList_hh 1

#include "G4VModularPhysicsList.hh"

class PhysicsList : public G4VModularPhysicsList
{
public:
    PhysicsList();
    virtual ~PhysicsList();

    virtual void SetCuts() override;
};

#endif