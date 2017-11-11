#include "Globals.h"
#include "f4se_common/Relocation.h"
#include "f4se_common/f4se_version.h"

#define GET_RVA(relocPtr) relocPtr.GetUIntPtr() - RelocationManager::s_baseAddr

/*
This file makes globals version-independent.

Initialization order is important for this file.

Since RelocPtrs are static globals with constructors they are initialized during the dynamic initialization phase.
Static initialization order is undefined for variables in different translation units, so it is not possible to deterministically obtain the value of a RelocPtr during static init.

Initialization must thus be done explicitly:
Call G::Init() in the plugin load routine before calling RVAManager::UpdateAddresses().

Doing so ensures that all RelocPtrs have been initialized and can be used to initialize an RVA.
*/

#include "f4se/GameData.h"
#include "f4se/GameInput.h"
#include "f4se/GameMenus.h"
#include "f4se/PapyrusVM.h"
#include "f4se/ScaleformLoader.h"

namespace G
{
    RVA<BSScaleformManager*>    scaleformManager;
    RVA<MenuControls*>          menuControls;
    RVA<UI*>                    ui;
    RVA<InputManager*>          inputMgr;
    RVA<GameVM*>                gameVM;
    RVA<DataHandler*>           dataHandler;

    void Init()
    {
        scaleformManager    = RVA<BSScaleformManager*>  (GET_RVA(g_scaleformManager),   "48 8B 0D ? ? ? ? 48 8D 05 ? ? ? ? 48 8B D3", 0, 3, 7);
        menuControls        = RVA<MenuControls*>        (GET_RVA(g_menuControls),       "48 8B 0D ? ? ? ? E8 ? ? ? ? 80 3D ? ? ? ? ? 0F B6 F8", 0, 3, 7);
        ui                  = RVA<UI*>                  (GET_RVA(g_ui),                 "48 8B 0D ? ? ? ? BA ? ? ? ? 8B 1C 16", 0, 3, 7);
        inputMgr            = RVA<InputManager*>        (GET_RVA(g_inputMgr),           "48 83 3D ? ? ? ? ? 74 3F 48 83 C1 40", 0, 3, 8);
        gameVM              = RVA<GameVM*>              (GET_RVA(g_gameVM),             "4C 8B 05 ? ? ? ? 48 8B F9", 0, 3, 7);
        dataHandler         = RVA<DataHandler*>         (GET_RVA(g_dataHandler),        "48 8B 05 ? ? ? ? 8B 13", 0, 3, 7);
    }

    // mov     rcx, cs:qq_g_scaleformManager
    // mov     rcx, cs:qq_g_menuControls
    // mov     rcx, cs:qq_g_ui
    // cmp     cs:qq_g_inputMgr, 0
    // mov     r8, cs:qq_g_gameVM
    // mov     rax, cs:qq_g_dataHandler
}