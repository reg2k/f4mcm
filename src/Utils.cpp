#include "Utils.h"
#include "Config.h"

#include <string>

#include "f4se_common/SafeWrite.h"
#include "f4se/GameData.h"
#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusArgs.h"

//---------------------
// Function Signatures
//---------------------

// VM -> Unk23
typedef bool (*_SetPropertyValue)(VirtualMachine* vm, VMIdentifier** identifier, const char* propertyName, VMValue* newValue, UInt64* unk4);
const int Idx_SetPropertyValue = 0x23;

// VM -> Unk25
typedef bool (*_GetPropertyValueByIndex)(VirtualMachine* vm, VMIdentifier** identifier, int idx, VMValue* outValue);
const int Idx_GetPropertyValueByIndex = 0x25;

template <typename T>
T GetVirtualFunction(void* baseObject, int vtblIndex) {
	uintptr_t* vtbl = reinterpret_cast<uintptr_t**>(baseObject)[0];
	return reinterpret_cast<T>(vtbl[vtblIndex]);
}

template <typename T>
T GetOffset(const void* baseObject, int offset) {
	return *reinterpret_cast<T*>((uintptr_t)baseObject + offset);
}

//---------------------
// Addresses
//---------------------

typedef void (*_ExecuteCommand)(const char* str);
RelocAddr <_ExecuteCommand> ExecuteCommand_Internal(Addr_ExecuteCommand);

RelocAddr <uintptr_t> ProcessUserEvent_Check(Addr_ProcessUserEvent_Check);

typedef void* (*_GetPropertyInfo)(VMObjectTypeInfo* objectTypeInfo, void* outInfo, BSFixedString* propertyName, bool unk4); 	// unk4 = 1
RelocAddr <_GetPropertyInfo> GetPropertyInfo_Internal(Addr_GetPropertyInfo);

//---------------------
// Functions
//---------------------

TESForm * MCMUtils::GetFormFromIdentifier(const std::string & identifier)
{
	auto delimiter = identifier.find('|');
	if (delimiter != std::string::npos) {
		std::string modName = identifier.substr(0, delimiter);
		std::string modForm = identifier.substr(delimiter + 1);

		const ModInfo* mod = (*g_dataHandler)->LookupModByName(modName.c_str());
		if (mod && mod->modIndex != -1) {
			UInt32 formID = std::stoul(modForm, nullptr, 16) & 0xFFFFFF;
			UInt32 flags = GetOffset<UInt32>(mod, 0x334);
			if (flags & (1 << 9)) {
				// ESL
				formID &= 0xFFF;
				formID |= 0xFE << 24;
				formID |= GetOffset<UInt16>(mod, 0x372) << 12;	// ESL load order
			} else {
				formID |= (mod->modIndex) << 24;
			}
			return LookupFormByID(formID);
		}
	}
	return nullptr;
}

std::string MCMUtils::GetIdentifierFromForm(const TESForm & form)
{
	return GetIdentifierFromFormID(form.formID);
}

// Note: This function does not yet support ESLs.
// This result of this function is not currently being used in the MCM.
std::string MCMUtils::GetIdentifierFromFormID(UInt32 formID)
{
	UInt8 modIndex = formID >> 24;
	if (modIndex >= 0xFE) return "";	// ESL or user-created

	ModInfo* mod = (*g_dataHandler)->modList.loadedMods[modIndex];

	char formIDStr[9];

	if (mod) {
		std::string output = mod->name;
		output += "|";
		snprintf(formIDStr, sizeof(formIDStr), "%X", formID & 0xFFFFFF);
		output += formIDStr;
		return output;
	}

	return "";
}

void MCMUtils::ExecuteCommand(const char * cmd)
{
	ExecuteCommand_Internal(cmd);
}

void MCMUtils::DisableProcessUserEvent(bool disable)
{
	if (disable) {
		unsigned char data[] = { 0x90, 0xE9 };	// NOP, JMP
		SafeWriteBuf(ProcessUserEvent_Check.GetUIntPtr(), &data, sizeof(data));
	} else {
		unsigned char data[] = { 0x0F, 0x84 };	// JZ (original)
		SafeWriteBuf(ProcessUserEvent_Check.GetUIntPtr(), &data, sizeof(data));
	}
}

void MCMUtils::GetPropertyInfo(VMObjectTypeInfo * objectTypeInfo, PropertyInfo * outInfo, BSFixedString * propertyName)
{
	GetPropertyInfo_Internal(objectTypeInfo, outInfo, propertyName, 1);
}

bool MCMUtils::GetPropertyValue(const char * formIdentifier, const char * propertyName, VMValue * valueOut)
{
	TESForm* targetForm = MCMUtils::GetFormFromIdentifier(formIdentifier);
	if (!targetForm) {
		_WARNING("Warning: Cannot retrieve property value %s from a None form. (%s)", propertyName, formIdentifier);
		return false;
	}

	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
	VMValue targetScript;
	PackValue(&targetScript, &targetForm, vm);
	if (!targetScript.IsIdentifier()) {
		_WARNING("Warning: Cannot retrieve a property value %s from a form with no scripts attached. (%s)", propertyName, formIdentifier);
		return false;
	}

	// Find the property
	PropertyInfo pInfo = {};
	pInfo.index = -1;
	GetPropertyInfo(targetScript.data.id->m_typeInfo, &pInfo, &BSFixedString(propertyName));

	if (pInfo.index != -1) {
		//vm->Unk_25(&targetScript.data.id, pInfo.index, valueOut);
		GetVirtualFunction<_GetPropertyValueByIndex>(vm, Idx_GetPropertyValueByIndex)(vm, &targetScript.data.id, pInfo.index, valueOut);
		return true;
	} else {
		_WARNING("Warning: Property %s does not exist on script %s", propertyName, targetScript.data.id->m_typeInfo->m_typeName.c_str());
		return false;
	}
}

bool MCMUtils::SetPropertyValue(const char * formIdentifier, const char * propertyName, VMValue * valueIn)
{
	TESForm* targetForm = GetFormFromIdentifier(formIdentifier);
	if (!targetForm) {
		_WARNING("Warning: Cannot set property %s on a None form. (%s)", propertyName, formIdentifier);
		return false;
	}

	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
	VMValue targetScript;
	PackValue(&targetScript, &targetForm, vm);
	if (!targetScript.IsIdentifier()) {
		_WARNING("Warning: Cannot set a property value %s on a form with no scripts attached. (%s)", propertyName, formIdentifier);
		return false;
	}

	UInt64 unk4 = 0;
	//vm->Unk_23(&targetScript.data.id, propertyName, valueIn, &unk4);
	GetVirtualFunction<_SetPropertyValue>(vm, Idx_SetPropertyValue)(vm, &targetScript.data.id, propertyName, valueIn, &unk4);

	return true;
}
