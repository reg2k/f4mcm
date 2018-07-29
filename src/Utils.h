#pragma once

#include "Globals.h"
#include "f4se/GameForms.h"
#include "f4se/PapyrusValue.h"
#include "f4se/PapyrusVM.h"

namespace MCMUtils {
	// 40
	struct PropertyInfo {
		BSFixedString		scriptName;		// 00
		BSFixedString		propertyName;	// 08
		UInt64				unk10;			// 10
		void*				unk18;			// 18
		void*				unk20;			// 20
		void*				unk28;			// 28
		SInt32				index;			// 30	-1 if not found
		UInt32				unk34;			// 34
		BSFixedString		unk38;			// 38
	};
	STATIC_ASSERT(offsetof(PropertyInfo, index) == 0x30);
	STATIC_ASSERT(sizeof(PropertyInfo) == 0x40);

	// Form Management
	TESForm * GetFormFromIdentifier(const std::string & identifier);
	std::string GetIdentifierFromForm(const TESForm & form);
	std::string GetIdentifierFromFormID(UInt32 formID);

	// Console Commands
	void ExecuteCommand(const char* cmd);

	// MCM Operation
	void DisableProcessUserEvent(bool disable);

	// Papyrus Properties
	void GetPropertyInfo(VMObjectTypeInfo* objectTypeInfo, PropertyInfo* outInfo, BSFixedString* propertyName);
	bool GetPropertyValue(const char* formIdentifier, const char* scriptName, const char* propertyName, VMValue* valueOut);
	bool SetPropertyValue(const char* formIdentifier, const char* scriptName, const char* propertyName, VMValue* valueIn);

    // Utilities
    template<typename T>
    T* GetOffsetPtr(const void * baseObject, int offset)
    {
        return reinterpret_cast<T*>((uintptr_t)baseObject + offset);
    }

	BSFixedString GetDescription(TESForm * thisForm);

	class VMScript
	{
	public:
		VMScript(TESForm* form, const char* scriptName = "ScriptObject") {
			if (!scriptName || scriptName[0] == '\0') scriptName = "ScriptObject";
			VirtualMachine* vm = (*G::gameVM)->m_virtualMachine;
			UInt64 handle = vm->GetHandlePolicy()->Create(form->formType, form);
			vm->GetObjectIdentifier(handle, scriptName, 1, &m_identifier, 0);
		}

		~VMScript() {
			if (m_identifier && !m_identifier->DecrementLock())
				m_identifier->Destroy();
		}

		VMIdentifier* m_identifier = nullptr;
	};
}