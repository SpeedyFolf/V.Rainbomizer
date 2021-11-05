#include <CHud.hh>
#include <Utils.hh>
#include "CARGB.hh"
#include "common/config.hh"
#include "HSL.hh"
#include "common/logger.hh"
#include <CVehicle.hh>
#include <cstdint>
#include <map>
#include <stdexcept>
#include "common/events.hh"
#include <common/minhook.hh>

void (*CHud__SetHudColour) (int, int, int, int, int);
uint32_t (*CCustomShaderEffectVehicle_SetForVehicle_134) (
    CCustomShaderEffectVehicle *, CVehicle *);

class ColoursRandomizer
{
    struct VehicleColourData
    {
        CARGB OriginalColours[4];
        CARGB RandomColours[4];
    };

    inline static std::map<CVehicle *, VehicleColourData> mColourData;
    inline static std::map<uint32_t, CARGB>               mHudCols;

    RB_C_CONFIG_START
    {
        bool RandomizeHudColours = true;
        bool RandomizeCarColours = true;
    }
    RB_C_CONFIG_END

    /*******************************************************/
    static void
    SetNewHudColour (int index, int r, int g, int b, int a)
    {
        using Rainbomizer::HSL;

        mHudCols[index] = CARGB (a, r, g, b);

        HSL colour (CARGB (a, r, g, b));
        colour.h = RandomFloat (360);
        colour.s = RandomFloat (0.5, 1);
        colour.l = RandomFloat (std::max (0.0f, colour.l - 0.25f),
                                std::min (1.0f, colour.l + 0.25f));

        CARGB newColour = colour.ToARGB ();

        CHud__SetHudColour (index, newColour.r, newColour.g, newColour.b, a);
    }

    /*******************************************************/
    static void
    RestoreVehicleColourData (CCustomShaderEffectVehicle *shader, CVehicle *veh)
    {
        if (auto *data = LookupMap (mColourData, veh))
            {
                CARGB *colours = shader->GetColours ();

                for (int i = 0; i < 4; i++)
                    colours[i] = data->OriginalColours[i];
            }
    }

    /*******************************************************/
    static bool
    StoreVehicleColourData (CCustomShaderEffectVehicle *shader, CVehicle *veh)
    {
        auto & data    = mColourData[veh];
        CARGB *colours = shader->GetColours ();

        bool changed = false;
        for (int i = 0; i < 4; i++)
            if (std::exchange (data.OriginalColours[i], colours[i])
                != colours[i])
                changed = true;

        return changed;
    }

    /*******************************************************/
    static uint32_t
    RandomizeVehicleColour (CCustomShaderEffectVehicle *shader, CVehicle *veh)
    {
        RestoreVehicleColourData (shader, veh);

        uint32_t ret
            = CCustomShaderEffectVehicle_SetForVehicle_134 (shader, veh);
        CARGB *colours = shader->GetColours ();

        if (StoreVehicleColourData (shader, veh))
            {
                using Rainbomizer::HSL;
                for (int i = 0; i < 4; i++)
                    mColourData[veh].RandomColours[i]
                        = HSL (RandomFloat (360.0f), 1.0f, RandomFloat (1.0f));
            }

        for (int i = 0; i < 4; i++)
            colours[i] = mColourData[veh].RandomColours[i];

        return ret;
    }

    /*******************************************************/
    static void
    RandomizeOnFade ()
    {
        for (const auto &[idx, col] : mHudCols)
            SetNewHudColour (idx, col.r, col.g, col.b, col.a);
    }

    /*******************************************************/
    void
    InitialiseCarColourHooks ()
    {
        MinHookWrapper::HookBranchDestination (
            "85 c9 74 ? ? 8b d3 e8 ? ? ? ? 84 c0 74 ? ? 84 ff 74", 7,
            CCustomShaderEffectVehicle_SetForVehicle_134,
            RandomizeVehicleColour);
    }

public:
    /*******************************************************/
    ColoursRandomizer ()
    {
        RB_C_DO_CONFIG ("ColourRandomizer", RandomizeHudColours,
                        RandomizeCarColours);

        InitialiseAllComponents ();

        // Hud Colours
        // ----------
        if (Config ().RandomizeHudColours)
            RegisterHook ("8b ? ? ? ? ? 8b ? ? ? ? ? 8b cb 89 44 ? ? e8", 18,
                          CHud__SetHudColour, SetNewHudColour);

        // Car Colours
        // ---------
        if (Config ().RandomizeCarColours)
            InitialiseCarColourHooks ();

        Rainbomizer::Events ().OnFade += RandomizeOnFade;
    }
} _cols;