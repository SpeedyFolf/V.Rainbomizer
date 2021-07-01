#include <common/logger.hh>
#include <exceptions/exceptions_Mgr.hh>
#include <common/events.hh>
#include <common/config.hh>

#include <Natives.hh>
#include <CutSceneManager.hh>
#include <CTheScripts.hh>
#include <CLoadingScreens.hh>
#include <Utils.hh>
#include <scrThread.hh>
#include <rage.hh>

#include "missions.hh"

#include <map>
#include <random>
#include <cstdint>

using namespace NativeLiterals;

class fwAssetStore;
class CutSceneManager;

eScriptState (*scrThread_Runff6) (uint64_t *, uint64_t *, scrProgram *,
                                  scrThreadContext *);

class MissionRandomizer
{
    using Components = MissionRandomizer_Components;

    static auto &
    Config ()
    {
        return Components::Config ();
    }

    /*******************************************************/
    static eScriptState
    RunThreadHook (uint64_t *stack, uint64_t *globals, scrProgram *program,
                   scrThreadContext *ctx)
    {
        eScriptState state    = ctx->m_nState;
        static bool  blocking = false;
        if (Components::Process (program, ctx))
            {
                if (blocking)
                    Rainbomizer::Logger::LogMessage ("Stopped blocking");
                blocking = false;
                Components::sm_Flow.SetVariables (ctx);
                state = scrThread_Runff6 (stack, globals, program, ctx);
                Components::sm_Flow.ClearVariables (ctx);
            }
        else if (!blocking)
            {
                Rainbomizer::Logger::LogMessage ("Started blocking");
                blocking = true;
            }

        Components::Process (program, ctx);

        return state;
    }

public:
    /*******************************************************/
    MissionRandomizer ()
    {
        if (!ConfigManager::ReadConfig (
                "MissionRandomizer", std::pair ("Seed", &Config ().Seed),
                std::pair ("ForceSeedOnSaves", &Config ().ForceSeedOnSaves),
                std::pair ("ForcedMission", &Config ().ForcedMission),
                std::pair ("EnableFastSkips", &Config ().EnableFastSkips),
                std::pair ("LogMissionOrder", &Config ().LogMissionOrder)))
            return;

        InitialiseAllComponents ();

        Components::sm_Globals.Initialise ();

        RegisterHook ("8d 15 ? ? ? ? ? 8b c0 e8 ? ? ? ? ? 85 ff ? 89 1d", 9,
                      scrThread_Runff6, RunThreadHook);

        Rainbomizer::Events ().OnInit +=
            [] (bool) { Components::sm_Flow.Reset (); };
    }
} missions;
