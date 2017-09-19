#include "PapyrusMCM.h"
#include "Config.h"
#include "SettingStore.h"
#include "ScaleformMCM.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#define MCM_NAME "MCM"

namespace PapyrusMCM
{
	bool IsInstalled(StaticFunctionTag* base) {
		return true;
	}

	UInt32 GetVersionCode(StaticFunctionTag* base) {
		return PLUGIN_VERSION;
	}

	void RefreshMenu(StaticFunctionTag* base) {
		ScaleformMCM::RefreshMenu();
	}

	SInt32 GetModSettingInt(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting) {
		return SettingStore::GetInstance().GetModSettingInt(asModName.c_str(), asModSetting.c_str());
	}

	bool GetModSettingBool(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting) {
		return SettingStore::GetInstance().GetModSettingBool(asModName.c_str(), asModSetting.c_str());
	}

	float GetModSettingFloat(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting) {
		return SettingStore::GetInstance().GetModSettingFloat(asModName.c_str(), asModSetting.c_str());
	}

	BSFixedString GetModSettingString(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting) {
		BSFixedString str(SettingStore::GetInstance().GetModSettingString(asModName.c_str(), asModSetting.c_str()));
		return str;
	}

	void SetModSettingInt(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting, SInt32 abValue) {
		SettingStore::GetInstance().SetModSettingInt(asModName.c_str(), asModSetting.c_str(), abValue);
	}

	void SetModSettingBool(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting, bool abValue) {
		SettingStore::GetInstance().SetModSettingBool(asModName.c_str(), asModSetting.c_str(), abValue);
	}

	void SetModSettingFloat(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting, float abValue) {
		SettingStore::GetInstance().SetModSettingFloat(asModName.c_str(), asModSetting.c_str(), abValue);
	}

	void SetModSettingString(StaticFunctionTag* base, BSFixedString asModName, BSFixedString asModSetting, BSFixedString abValue) {
		SettingStore::GetInstance().SetModSettingString(asModName.c_str(), asModSetting.c_str(), abValue.c_str());
	}
}

void PapyrusMCM::RegisterFuncs(VirtualMachine* vm) {
	vm->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, bool>("IsInstalled", MCM_NAME, PapyrusMCM::IsInstalled, vm));

	vm->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, UInt32>("GetVersionCode", MCM_NAME, PapyrusMCM::GetVersionCode, vm));

	vm->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, void>("RefreshMenu", MCM_NAME, PapyrusMCM::RefreshMenu, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, SInt32, BSFixedString, BSFixedString>("GetModSettingInt", MCM_NAME, PapyrusMCM::GetModSettingInt, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, bool, BSFixedString, BSFixedString>("GetModSettingBool", MCM_NAME, PapyrusMCM::GetModSettingBool, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, float, BSFixedString, BSFixedString>("GetModSettingFloat", MCM_NAME, PapyrusMCM::GetModSettingFloat, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, BSFixedString, BSFixedString, BSFixedString>("GetModSettingString", MCM_NAME, PapyrusMCM::GetModSettingString, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, BSFixedString, BSFixedString, SInt32>("SetModSettingInt", MCM_NAME, PapyrusMCM::SetModSettingInt, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, BSFixedString, BSFixedString, bool>("SetModSettingBool", MCM_NAME, PapyrusMCM::SetModSettingBool, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, BSFixedString, BSFixedString, float>("SetModSettingFloat", MCM_NAME, PapyrusMCM::SetModSettingFloat, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, BSFixedString, BSFixedString, BSFixedString>("SetModSettingString", MCM_NAME, PapyrusMCM::SetModSettingString, vm));

	vm->SetFunctionFlags(MCM_NAME, "IsInstalled", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags(MCM_NAME, "GetVersionCode", IFunction::kFunctionFlag_NoWait);
}