#include "f4se/PluginAPI.h"
#include "f4se/GameAPI.h"
#include <shlobj.h>

#include "f4se/PluginManager.h"

#include "f4se_common/f4se_version.h"

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformCallbacks.h"

// Translation
#include "f4se/ScaleformLoader.h"
#include "f4se/Translation.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "Config.h"
#include "rva/RVA.h"

#include "Globals.h"
#include "PapyrusMCM.h"
#include "ScaleformMCM.h"
#include "SettingStore.h"
#include "MCMInput.h"
#include "MCMSerialization.h"
#include "MCMTranslator.h"

IDebugLog gLog;
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

F4SEScaleformInterface		*g_scaleform = NULL;
F4SEPapyrusInterface		*g_papyrus = NULL;
F4SEMessagingInterface		*g_messaging = NULL;
F4SESerializationInterface	*g_serialization = NULL;

//-------------------------
// Event Handlers
//-------------------------

bool RegisterPapyrus(VirtualMachine *vm) {
	PapyrusMCM::RegisterFuncs(vm);
	_MESSAGE("Registered Papyrus native functions.");

	return true;
}

void OnF4SEMessage(F4SEMessagingInterface::Message* msg) {
    switch (msg->type) {
        case F4SEMessagingInterface::kMessage_GameLoaded:
            MCMInput::GetInstance().RegisterForInput(true);

            // Inject translations
            BSScaleformTranslator* translator = (BSScaleformTranslator*)(*G::scaleformManager)->stateBag->GetStateAddRef(GFxState::kInterface_Translator);
            if (translator) {
                MCMTranslator::LoadTranslations(translator);
            }
            break;
    }
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\MCM.log");

	_MESSAGE("MCM v%s", PLUGIN_VERSION_STRING);
	_MESSAGE("MCM query");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name      = PLUGIN_NAME_SHORT;
	info->version   = PLUGIN_VERSION;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	// Check game version
    if (f4se->runtimeVersion != SUPPORTED_RUNTIME_VERSION) {
        char str[512];
        sprintf_s(str, sizeof(str), "Your game version: v%d.%d.%d.%d\nExpected version: v%d.%d.%d.%d\n%s will be disabled.",
            GET_EXE_VERSION_MAJOR(f4se->runtimeVersion),
            GET_EXE_VERSION_MINOR(f4se->runtimeVersion),
            GET_EXE_VERSION_BUILD(f4se->runtimeVersion),
            GET_EXE_VERSION_SUB(f4se->runtimeVersion),
            GET_EXE_VERSION_MAJOR(SUPPORTED_RUNTIME_VERSION),
            GET_EXE_VERSION_MINOR(SUPPORTED_RUNTIME_VERSION),
            GET_EXE_VERSION_BUILD(SUPPORTED_RUNTIME_VERSION),
            GET_EXE_VERSION_SUB(SUPPORTED_RUNTIME_VERSION),
            PLUGIN_NAME_LONG
        );

        MessageBox(NULL, str, PLUGIN_NAME_LONG, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

	// Get the scaleform interface
	g_scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
	if(!g_scaleform) {
		_MESSAGE("couldn't get scaleform interface");
		return false;
	}

	// Get the papyrus interface
	g_papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
	if (!g_papyrus) {
		_MESSAGE("couldn't get papyrus interface");
		return false;
	}

	// Get the messaging interface
	g_messaging = (F4SEMessagingInterface *)f4se->QueryInterface(kInterface_Messaging);
	if (!g_messaging) {
		_MESSAGE("couldn't get messaging interface");
		return false;
	}

	// Get the serialization interface
	g_serialization = (F4SESerializationInterface *)f4se->QueryInterface(kInterface_Serialization);
	if (!g_serialization) {
		_MESSAGE("couldn't get serialization interface");
		return false;
	}

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface *f4se)
{
    _MESSAGE("MCM load");

    // Initialize globals and addresses
    G::Init();
    RVAManager::UpdateAddresses(f4se->runtimeVersion);

    g_scaleform->Register("f4mcm", ScaleformMCM::RegisterScaleform);
    g_papyrus->Register(RegisterPapyrus);
    g_messaging->RegisterListener(g_pluginHandle, "F4SE", OnF4SEMessage);

    g_serialization->SetUniqueID(g_pluginHandle, 'MCM');
    g_serialization->SetRevertCallback(g_pluginHandle, MCMSerialization::RevertCallback);
    g_serialization->SetLoadCallback(g_pluginHandle, MCMSerialization::LoadCallback);
    g_serialization->SetSaveCallback(g_pluginHandle, MCMSerialization::SaveCallback);

    // Create Data/MCM if it doesn't already exist.
    if (GetFileAttributes("Data\\MCM") == INVALID_FILE_ATTRIBUTES)
        CreateDirectory("Data\\MCM", NULL);

    SettingStore::GetInstance().ReadSettings();

    return true;
}

};
