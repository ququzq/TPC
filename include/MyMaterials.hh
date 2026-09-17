#ifndef MyMaterials_hh
#define MyMaterials_hh 1

#include "G4Material.hh"
#include "G4OpticalSurface.hh"
#include "G4String.hh"

class MyMaterials
{
public:
    static void Construct();
    static G4Material* GetMaterial(const G4String& name);
    static G4OpticalSurface* GetOpticalSurface(const G4String& name);

private:
    static void ConstructCustomMaterials();
    static void ConstructSurfaces();
};

#endif