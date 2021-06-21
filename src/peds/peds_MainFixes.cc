#include <CPed.hh>
#include <Utils.hh>
#include <CTheScripts.hh>
#include <CutSceneManager.hh>

#include "peds_Compatibility.hh"
#include "scrThread.hh"

using namespace NativeLiterals;

/*******************************************************/
/* Cutscene and Script Fixes - Fixes required for Ped Randomizer to function
 * properly without softlocking the game in major ways. The fixes include
 * register entity fixes for cutscene ped randomizer and returning the original
 * ped model for scripts. */
/*******************************************************/
class PedRandomizer_MainFixes
{
    /*******************************************************/
    static uint32_t
    FixupScriptEntityModel (uint32_t guid)
    {
        CPed *ped = static_cast<CPed *> (fwScriptGuid::GetBaseFromGuid (guid));

        if (!ped || !ped->m_pModelInfo)
            return 0;

        return PedRandomizerCompatibility::GetOriginalModel (ped)->m_nHash;
    }

    /*******************************************************/
    static void
    FixupScriptEntityModel (scrThread::Info *info)
    {
        info->GetReturn () = FixupScriptEntityModel (info->GetArg (0));
    }

    /*******************************************************/
    template <auto &CutSceneManager_RegisterEntity>
    static void
    CorrectRegisterEntity (CutSceneManager *mgr, CEntity *entity,
                           uint32_t *handle, uint32_t *modelHash, bool p5,
                           bool p6, bool p7, uint32_t p8)
    {
        if (entity
            && entity->m_pModelInfo->GetType ()
                   == eModelInfoType::MODEL_INFO_PED)
            {
                *modelHash = PedRandomizerCompatibility::GetOriginalModel (
                                 static_cast<CPed *> (entity))
                                 ->m_nHash;
            }

        CutSceneManager_RegisterEntity (mgr, entity, handle, modelHash, p5, p6,
                                        p7, p8);
    }

    /*******************************************************/
    static void
    FixIsPedModelNative (scrThread::Info *info)
    {
        info->GetReturn ()
            = FixupScriptEntityModel (info->GetArg (0)) == info->GetArg (1);
    }

    /*******************************************************/
    static void
    AdjustAiBlipNoticeRange (scrThread::Info *info)
    {
        info->GetArg<float> (1) = -1.0f;
    }

    /*******************************************************/
    static void
    AdjustAiBlipForcedOn (scrThread::Info *info)
    {
        info->GetArg<bool> (1) = true;
    }

public:
    /*******************************************************/
    PedRandomizer_MainFixes ()
    {
        // Fix for scripts where a certain hash is required for a certain ped.
        "GET_ENTITY_MODEL"_n.Hook (FixupScriptEntityModel);
        "IS_PED_MODEL"_n.Hook (FixIsPedModelNative);

#define HOOK(native, func)                                                     \
    NativeCallbackMgr::InitCallback<native##_joaat, func, true> ()

        HOOK ("SET_PED_AI_BLIP_NOTICE_RANGE", AdjustAiBlipNoticeRange);
        HOOK ("SET_PED_AI_BLIP_FORCED_ON", AdjustAiBlipForcedOn);

        REGISTER_HOOK (
            "c6 44 ? ? 00 ? 8d ? f0 ? 8d ? f4 ? 8b c8 e8 ? ? ? ? ? 8b 5c", 16,
            CorrectRegisterEntity, void, CutSceneManager *, CEntity *,
            uint32_t *, uint32_t *, bool, bool, bool, uint32_t);

#undef HOOK
    }

} peds_MainFixes;
