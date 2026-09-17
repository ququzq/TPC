#include "RunAction.hh"

#include "G4SystemOfUnits.hh"
#include "G4Run.hh"
#include "G4ios.hh"
#include "G4Threading.hh"

#include <fstream>

namespace B1
{

// ============================================================
// 定义 static 成员
// ============================================================

std::mutex RunAction::fGlobalMutex;

std::vector<double> RunAction::fGlobalPhotonEnergies;


// ============================================================
// Constructor / Destructor
// ============================================================

RunAction::RunAction()
    : G4UserRunAction()
{
}


RunAction::~RunAction()
{
}


// ============================================================
// BeginOfRunAction
// ============================================================

void RunAction::BeginOfRunAction(const G4Run*)
{
    // 只有 Master 清空全局数据
    if (G4Threading::IsMasterThread())
    {
        std::lock_guard<std::mutex> lock(fGlobalMutex);

        fGlobalPhotonEnergies.clear();

        G4cout << "============================================"
               << G4endl;

        G4cout << "Starting run..."
               << G4endl;

        G4cout << "============================================"
               << G4endl;
    }
}


// ============================================================
// RecordPhoton
// ============================================================

void RunAction::RecordPhoton(double energy)
{
    std::lock_guard<std::mutex> lock(fGlobalMutex);

    fGlobalPhotonEnergies.push_back(energy);
}


// ============================================================
// EndOfRunAction
// ============================================================

void RunAction::EndOfRunAction(const G4Run*)
{
    // Worker 不负责写最终文件
    if (!G4Threading::IsMasterThread())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(fGlobalMutex);

    std::ofstream outFile("photons_in_LAr.txt");

    if (!outFile.is_open())
    {
        G4cerr
            << "Error: Could not open output file!"
            << G4endl;

        return;
    }

    outFile << "# Photon_ID\tEnergy(eV)\n";

    for (size_t i = 0;
         i < fGlobalPhotonEnergies.size();
         ++i)
    {
        outFile
            << i + 1
            << "\t"
            << fGlobalPhotonEnergies[i] / eV
            << "\n";
    }

    outFile.close();

    G4cout
        << "============================================"
        << G4endl;

    G4cout
        << " Summary: "
        << fGlobalPhotonEnergies.size()
        << " photons entered LAr volumes."
        << G4endl;

    G4cout
        << " Data saved to photons_in_LAr.txt"
        << G4endl;

    G4cout
        << "============================================"
        << G4endl;
}

} // namespace B1