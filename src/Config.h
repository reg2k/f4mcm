#pragma once
#include "f4se_common/f4se_version.h"

//-----------------------
// Plugin Information
//-----------------------
#define PLUGIN_VERSION              3
#define PLUGIN_VERSION_STRING       "1.11"
#define PLUGIN_NAME_SHORT           "F4MCM"
#define PLUGIN_NAME_LONG            "Mod Configuration Menu"
#define SUPPORTED_RUNTIME_VERSION   CURRENT_RELEASE_RUNTIME

// Addresses
#if SUPPORTED_RUNTIME_VERSION == RUNTIME_VERSION_1_10_20
	#define Addr_ExecuteCommand			0x01259430
	#define Addr_ProcessUserEvent_Check 0x0210DD1C
	#define Addr_GetPropertyInfo		0x02707500

#elif SUPPORTED_RUNTIME_VERSION == RUNTIME_VERSION_1_9_4
	#define Addr_ExecuteCommand			0x012416F0
	#define Addr_ProcessUserEvent_Check 0x020E745C
	#define Addr_GetPropertyInfo		0x026A52E0

#else
	#error "Addresses for runtime version missing."

#endif