#include "stdafx.h"
#include "DS2Fix64.h"

#include "Utils\SigScan.h"
#include "Core\Signatures.h"
#include "Fixes\Durability.h"
#include "Fixes\PlusFourteen.h"

#include "MinHook.h"

BOOL Begin()
{
    if (!GetImageInfo())
    {
        log_err("GetImageInfo() failed");
        return false;
    }

    // Retrieve injections points by signature

    // This one is a complete hook, we'll return to the trampoline
    // and back to the original
    oApplyDurabilityDamage = (ApplyDurabilityDamage)(FindSignature(&fsApplyDurabilityDamage));
    
    // +14 patching is mid-function hooking, so we have to manage
    // the return address differently. The first hook returns 8 bytes
    // below the injection point.
    oPlusFourteen_1 = (PlusFourteen_1)(FindSignature(&fsPlusFourteenCrash_1));
    bPlusFourteen_1 = (DWORD64)oPlusFourteen_1 + fsPlusFourteenCrash_1.ret;

    // The second hook needs 11 bytes. Got a better way? Create a pull request!
    oPlusFourteen_2 = (PlusFourteen_2)(FindSignature(&fsPlusFourteenCrash_2));
    bPlusFourteen_2 = (DWORD64)oPlusFourteen_2 + fsPlusFourteenCrash_2.ret;

    if (!(oApplyDurabilityDamage && oPlusFourteen_1 && oPlusFourteen_2))
    {
        log_err("failed to locate injection points");
        return false;
    }
    
    // Enable hooks
    if (ApplyDetours() == false)
    {
        log_err("detouring failed");
        return false;
    }
    else {
        debug("oApplyDurabilityDamage() @ 0x%p t-> 0x%p b-> 0x%p", oApplyDurabilityDamage, tApplyDurabilityDamage, bApplyDurabilityDamage);
        debug("oPlusFourteen_1 @ 0x%p t-> 0x%p b-> 0x%p", oPlusFourteen_1, tPlusFourteen_1, bPlusFourteen_1);
        debug("oPlusFourteen_2 @ 0x%p t-> 0x%p b-> 0x%p", oPlusFourteen_2, tPlusFourteen_2, bPlusFourteen_2);
    }

    return true;
}

BOOL End()
{
    if (RemoveDetours() == false)
    {
        log_err("failed to remove detours");
        return false;
    }

    return true;
}

BOOL ApplyDetours()
{
    if (MH_Initialize() != MH_OK)
    {
        log_err("failed to initialize MinHook");
        return false;
    }

    if (MH_CreateHook((LPVOID*)oApplyDurabilityDamage, (LPVOID)tApplyDurabilityDamage, reinterpret_cast<LPVOID*>(&bApplyDurabilityDamage)) != MH_OK ||
        MH_CreateHook((LPVOID*)oPlusFourteen_1, (LPVOID)tPlusFourteen_1, NULL) != MH_OK ||
        MH_CreateHook((LPVOID*)oPlusFourteen_2, (LPVOID)tPlusFourteen_2, NULL) != MH_OK)
    {
        log_err("failed to create hooks");
        return false;
    }
    
    if (MH_EnableHook(oApplyDurabilityDamage) != MH_OK ||
        MH_EnableHook(oPlusFourteen_1) != MH_OK ||
        MH_EnableHook(oPlusFourteen_2) != MH_OK)
    {
        log_err("failed to enable hooks");
        return false;
    }

    return true;
}

BOOL RemoveDetours()
{
    if (MH_DisableHook(oApplyDurabilityDamage) != MH_OK ||
        MH_DisableHook(oPlusFourteen_1) != MH_OK ||
        MH_DisableHook(oPlusFourteen_2) != MH_OK)
    {
        log_err("failed to remove hooks");
        return false;
    }

    if (MH_Uninitialize() != MH_OK)
    {
        log_err("failed to uninitialize MinHook");
        return false;
    }

    return true;
}