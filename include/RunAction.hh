#ifndef RunAction_h
#define RunAction_h

#include "G4UserRunAction.hh"
#include "globals.hh"

#include <vector>
#include <mutex>

namespace B1
{

class RunAction : public G4UserRunAction
{
public:

    RunAction();
    ~RunAction() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void RecordPhoton(double energy);

    // ===== 修改: 新增记录被反射光子能量的接口 =====
    void RecordReflectedPhoton(double energy);
    // ============================================

private:

    // 使用 static mutex 让所有线程共享
    static std::mutex fGlobalMutex;

    // 所有线程共享的光子能量
    static std::vector<double> fGlobalPhotonEnergies;

    // ===== 修改: 所有线程共享的被反射光子能量 =====
    static std::vector<double> fGlobalReflectedEnergies;
    // =============================================
};

}

#endif