#include "ScaleformMCM.h"

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/PapyrusScaleformAdapter.h"

#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusUtilities.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameMenus.h"
#include "f4se/GameInput.h"
#include "f4se/InputMap.h"

#include "Globals.h"
#include "Config.h"
#include "Utils.h"
#include "SettingStore.h"
#include "MCMKeybinds.h"

namespace ScaleformMCM {

	// function GetMCMVersionString():String;
	class GetMCMVersionString : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetString(PLUGIN_VERSION_STRING);
		}
	};

	// function GetMCMVersionCode():int;
	class GetMCMVersionCode : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetInt(PLUGIN_VERSION);
		}
	};

	// function GetConfigList(fullPath:Boolean=false, filename:String="config.json"):Array;
	// Returns: ["Mod1", "Mod2", "Mod3"] (fullPath = false), or ["Data\MCM\Config\Mod1\config.json", ...] (fullPath = true)
	class GetConfigList : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			bool wantFullPath		= (args->numArgs > 0 && args->args[0].GetType() == GFxValue::kType_Bool) ? args->args[0].GetBool() : false;
			const char* filename	= (args->numArgs > 1 && args->args[1].GetType() == GFxValue::kType_String) ? args->args[1].GetString() : "config.json";

			args->movie->movieRoot->CreateArray(args->result);

			HANDLE hFind;
			WIN32_FIND_DATA data;

			hFind = FindFirstFile("Data\\MCM\\Config\\*", &data);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
					if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, "..")) continue;

					char fullPath[MAX_PATH];
					snprintf(fullPath, MAX_PATH, "%s%s%s%s", "Data\\MCM\\Config\\", data.cFileName, "\\", filename);

					if (GetFileAttributes(fullPath) == INVALID_FILE_ATTRIBUTES) continue;

					GFxValue filePath;
					filePath.SetString(wantFullPath ? fullPath : data.cFileName);
					args->result->PushBack(&filePath);
				} while (FindNextFile(hFind, &data));
				FindClose(hFind);
			}
		}
	};

	// function OnMCMOpen();
	class OnMCMOpen : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			// Start key handler
			RegisterForInput(true);
		}
	};

	// function OnMCMClose();
	class OnMCMClose : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			// Save modified keybinds.
			g_keybindManager.CommitKeybinds();
			RegisterForInput(false);
		}
	};

	// function DisableMenuInput(disable:Boolean);
	class DisableMenuInput : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			if (args->numArgs < 1) return;
			if (args->args[0].GetType() != GFxValue::kType_Bool) return;
			bool disable = args->args[0].GetBool();

			MCMUtils::DisableProcessUserEvent(disable);
		}
	};

	// function GetGlobalValue(formIdentifier:String):Number
	class GetGlobalValue : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetNumber(-1);

			if (args->numArgs != 1) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;

			TESForm* targetForm = MCMUtils::GetFormFromIdentifier(args->args[0].GetString());
			TESGlobal* global = nullptr;
			global = DYNAMIC_CAST(targetForm, TESForm, TESGlobal);

			if (global) {
				args->result->SetNumber(global->value);
			}
		}
	};

	// function SetGlobalValue(formIdentifier:String, newValue:Number):Boolean
	class SetGlobalValue : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs != 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_Number) return;

			TESForm* targetForm = MCMUtils::GetFormFromIdentifier(args->args[0].GetString());
			TESGlobal* global = nullptr;
			global = DYNAMIC_CAST(targetForm, TESForm, TESGlobal);

			if (global) {
				global->value = args->args[1].GetNumber();
				args->result->SetBool(true);
			}
		}
	};

	// function GetPropertyValue(formIdentifier:String, propertyName:String):*
	// Returns null if the property doesn't exist.
	class GetPropertyValue : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetNull();

			if (args->numArgs < 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;
			
			const char* formIdentifier	= args->args[0].GetString();
			const char*	propertyName	= args->args[1].GetString();

			VMValue valueOut;
			bool getOK = MCMUtils::GetPropertyValue(formIdentifier, propertyName, &valueOut);
			if (getOK)
				PlatformAdapter::ConvertPapyrusValue(args->result, &valueOut, args->movie->movieRoot);
		}
	};

	// function SetPropertyValue(formIdentifier:String, propertyName:String, newValue:*):Boolean
	class SetPropertyValue : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs < 3) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			const char* formIdentifier	= args->args[0].GetString();
			const char*	propertyName	= args->args[1].GetString();
			GFxValue*	newValue		= &args->args[2];

			VirtualMachine* vm = (*G::gameVM)->m_virtualMachine;

			VMValue newVMValue;
			PlatformAdapter::ConvertScaleformValue(&newVMValue, newValue, vm);
			bool setOK = MCMUtils::SetPropertyValue(formIdentifier, propertyName, &newVMValue);

			args->result->SetBool(setOK);
		}
	};

	// function CallQuestFunction(formID:String, functionName:String, ...arguments);
	// e.g. CallQuestFunction("MyMod.esp|F99", "MyFunction", 0.1, 0.2, true);
	// Note: this function has been updated to accept any Form type.
	class CallQuestFunction : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs < 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			TESForm* targetForm = MCMUtils::GetFormFromIdentifier(args->args[0].GetString());

			if (!targetForm) {
				_MESSAGE("WARNING: %s is not a valid form.", args->args[0].GetString());
			} else {

				VirtualMachine* vm = (*G::gameVM)->m_virtualMachine;

				VMValue senderValue;
				PackValue(&senderValue, &targetForm, vm);

				if (!senderValue.IsIdentifier()) {
					_MESSAGE("WARNING: %s cannot be resolved to a Papyrus script object.", args->args[0].GetString());
				} else {

					VMIdentifier* identifier = senderValue.data.id;
					BSFixedString funcName(args->args[1].GetString());

					VMValue packedArgs;
					UInt32 length = args->numArgs - 2;
					VMValue::ArrayData* arrayData = nullptr;
					vm->CreateArray(&packedArgs, length, &arrayData);

					packedArgs.type.value = VMValue::kType_VariableArray;
					packedArgs.data.arr = arrayData;

					for (UInt32 i = 0; i < length; i++)
					{
						VMValue* var = new VMValue;
						PlatformAdapter::ConvertScaleformValue(var, &args->args[i + 2], vm);
						arrayData->arr.entries[i].SetVariable(var);
					}

					CallFunctionNoWait_Internal(vm, 0, identifier, &funcName, &packedArgs);

					args->result->SetBool(true);
				}
			}
		}
	};

	// function CallGlobalFunction(scriptName:String, funcName:String, ...arguments);
	// e.g. CallGlobalFunction("Debug", "MessageBox", "Hello world!");
	class CallGlobalFunction : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs < 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			VirtualMachine* vm = (*G::gameVM)->m_virtualMachine;

			BSFixedString scriptName(args->args[0].GetString());
			BSFixedString funcName(args->args[1].GetString());
			
			VMValue packedArgs;
			UInt32 length = args->numArgs - 2;
			VMValue::ArrayData* arrayData = nullptr;
			vm->CreateArray(&packedArgs, length, &arrayData);

			packedArgs.type.value = VMValue::kType_VariableArray;
			packedArgs.data.arr = arrayData;

			for (UInt32 i = 0; i < length; i++)
			{
				VMValue * var = new VMValue;
				PlatformAdapter::ConvertScaleformValue(var, &args->args[i + 2], vm);
				arrayData->arr.entries[i].SetVariable(var);
			}

			CallGlobalFunctionNoWait_Internal(vm, 0, 0, &scriptName, &funcName, &packedArgs);

			args->result->SetBool(true);
		}
	};

	// GetModSettingInt(modName:String, settingName:String):int;
	class GetModSettingInt : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetNumber(-1);

			if (args->numArgs != 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			args->result->SetInt(SettingStore::GetInstance().GetModSettingInt(args->args[0].GetString(), args->args[1].GetString()));
		}
	};

	// GetModSettingBool(modName:String, settingName:String):Boolean;
	class GetModSettingBool : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs != 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			args->result->SetBool(SettingStore::GetInstance().GetModSettingBool(args->args[0].GetString(), args->args[1].GetString()));
		}
	};

	// GetModSettingFloat(modName:String, settingName:String):Number;
	class GetModSettingFloat : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetNumber(-1);

			if (args->numArgs != 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			args->result->SetNumber(SettingStore::GetInstance().GetModSettingFloat(args->args[0].GetString(), args->args[1].GetString()));
		}
	};

	// GetModSettingString(modName:String, settingName:String):String;
	class GetModSettingString : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetString("");

			if (args->numArgs != 2) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;

			args->result->SetString(SettingStore::GetInstance().GetModSettingString(args->args[0].GetString(), args->args[1].GetString()));
		}
	};

	// SetModSettingInt(modName:String, settingName:String, value:int):Boolean;
	class SetModSettingInt : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs != 3) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;
			if (args->args[2].GetType() != GFxValue::kType_Int) return;

			SettingStore::GetInstance().SetModSettingInt(args->args[0].GetString(), args->args[1].GetString(), args->args[2].GetInt());

			args->result->SetBool(true);
		}
	};

	// SetModSettingBool(modName:String, settingName:String, value:Boolean):Boolean;
	class SetModSettingBool : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs != 3) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;
			if (args->args[2].GetType() != GFxValue::kType_Bool) return;

			SettingStore::GetInstance().SetModSettingBool(args->args[0].GetString(), args->args[1].GetString(), args->args[2].GetBool());

			args->result->SetBool(true);
		}
	};

	// SetModSettingFloat(modName:String, settingName:String, value:Number):Boolean;
	class SetModSettingFloat : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs != 3) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;
			if (args->args[2].GetType() != GFxValue::kType_Number) return;

			SettingStore::GetInstance().SetModSettingFloat(args->args[0].GetString(), args->args[1].GetString(), args->args[2].GetNumber());

			args->result->SetBool(true);
		}
	};

	// SetModSettingString(modName:String, settingName:String, value:String):Boolean;
	class SetModSettingString : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs != 3) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;
			if (args->args[1].GetType() != GFxValue::kType_String) return;
			if (args->args[2].GetType() != GFxValue::kType_String) return;

			SettingStore::GetInstance().SetModSettingString(args->args[0].GetString(), args->args[1].GetString(), args->args[2].GetString());

			args->result->SetBool(true);
		}
	};

	// IsPluginInstalled(modName:String):Boolean;
	class IsPluginInstalled : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs < 1) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;

			const ModInfo* mi = (*G::dataHandler)->LookupModByName(args->args[0].GetString());
			// modIndex == -1 for mods that are present in the Data directory but not active.
			if (mi && mi->modIndex != 0xFF) {
				args->result->SetBool(true);
			}
		}
	};

	class GetKeybind : public GFxFunctionHandler {
	public:
		enum CallType {
			kCallType_ID,
			kCallType_Keycode,
		};

		virtual void Invoke(Args* args) {
			CallType callType = kCallType_ID;

			if (args->numArgs < 2) return;
			if (args->args[0].GetType() == GFxValue::kType_String && args->args[1].GetType() == GFxValue::kType_String) {
				callType = kCallType_ID;
			} else if (args->args[0].GetType() == GFxValue::kType_Int && args->args[1].GetType() == GFxValue::kType_Int) {
				callType = kCallType_Keycode;
			} else {
				// Invalid parameter type
				return;
			}

			GFxValue keycode, modifiers, keybindType, keybindID, keybindName, modName, type, flags, targetForm, callbackName;
			
			g_keybindManager.Lock();
			KeybindInfo ki;
			if (callType == kCallType_ID) {
				ki = g_keybindManager.GetKeybind(args->args[0].GetString(), args->args[1].GetString());
			} else {
				Keybind kb = {};
				kb.keycode = args->args[0].GetInt();
				kb.modifiers = args->args[1].GetInt();
				ki = g_keybindManager.GetKeybind(kb);
			}
			g_keybindManager.Release();

			SetKeybindInfo(ki, args->movie->movieRoot, args->result);
		}
	};

	class GetAllKeybinds : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			g_keybindManager.Lock();
			std::vector<KeybindInfo> keybinds = g_keybindManager.GetAllKeybinds();
			g_keybindManager.Release();

			args->movie->movieRoot->CreateArray(args->result);

			for (int i = 0; i < keybinds.size(); i++) {
				GFxValue keybindInfoValue;
				SetKeybindInfo(keybinds[i], args->movie->movieRoot, &keybindInfoValue);
				args->result->PushBack(&keybindInfoValue);
			}
		}
	};

	// function SetKeybind(modName:String, keybindID:String, keycode:int, modifiers:int):Boolean
	class SetKeybind : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs < 4) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;	// modName
			if (args->args[1].GetType() != GFxValue::kType_String) return;	// keybindID
			if (args->args[2].GetType() != GFxValue::kType_Int) return;		// keycode
			if (args->args[3].GetType() != GFxValue::kType_Int) return;		// modifiers

			std::string modName		= args->args[0].GetString();
			std::string keybindID	= args->args[1].GetString();

			Keybind kb = {};
			kb.keycode		= args->args[2].GetInt();
			kb.modifiers	= args->args[3].GetInt();

			if (g_keybindManager.RegisterKeybind(kb, modName.c_str(), keybindID.c_str())) {
				args->result->SetBool(true);
			}
		}
	};

	// function SetKeybindEx(modName:String, keybindID:String, keybindName:String, keycode:int, modifiers:int, type:int, params:Array)
	class SetKeybindEx : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			if (args->numArgs < 5) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;	// modName
			if (args->args[1].GetType() != GFxValue::kType_String) return;	// keybindID
			if (args->args[2].GetType() != GFxValue::kType_String) return;	// keybindDesc
			if (args->args[3].GetType() != GFxValue::kType_Int) return;		// keycode
			if (args->args[4].GetType() != GFxValue::kType_Int) return;		// modifiers
			if (args->args[5].GetType() != GFxValue::kType_Int) return;		// type
			if (args->args[6].GetType() != GFxValue::kType_Array) return;	// params

			Keybind				kb = {};
			KeybindParameters	kp = {};
			kp.modName		= args->args[0].GetString();
			kp.keybindID	= args->args[1].GetString();
			kp.keybindDesc	= args->args[2].GetString();
			kb.keycode		= args->args[3].GetInt();
			kb.modifiers	= args->args[4].GetInt();
			kp.type			= args->args[5].GetInt();
			GFxValue params = args->args[6];
			int paramSize	= params.GetArraySize();

			switch (kp.type) {
				case KeybindParameters::kType_CallFunction:
				{
					if (paramSize < 2) return;
					GFxValue targetFormIdentifier, callbackName;
					params.GetElement(0, &targetFormIdentifier);
					params.GetElement(1, &callbackName);

					if (targetFormIdentifier.GetType()	!= GFxValue::kType_String) return;
					if (callbackName.GetType()			!= GFxValue::kType_String) return;

					TESForm* targetForm = MCMUtils::GetFormFromIdentifier(targetFormIdentifier.GetString());
					if (!targetForm) {
						_WARNING("Cannot register a None form as a call target.");
						return;
					}

					kp.targetFormID = targetForm->formID;
					kp.callbackName = callbackName.GetString();

					g_keybindManager.Register(kb, kp);

					_MESSAGE("Succesfully registered kType_CallFunction keybind for keycode %d.", kb.keycode);

					break;
				}
				case KeybindParameters::kType_CallGlobalFunction:
				{
					if (paramSize < 2) return;
					GFxValue scriptName, functionName;
					params.GetElement(0, &scriptName);
					params.GetElement(1, &functionName);

					if (scriptName.GetType() != GFxValue::kType_String) return;
					if (functionName.GetType() != GFxValue::kType_String) return;

					kp.scriptName	= scriptName.GetString();
					kp.callbackName = functionName.GetString();

					g_keybindManager.Register(kb, kp);

					_MESSAGE("Succesfully registered kType_CallGlobalFunction keybind for keycode %d.", kb.keycode);

					break;
				}
				case KeybindParameters::kType_RunConsoleCommand:
				{
					if (paramSize < 1) return;
					GFxValue consoleCommandValue;
					params.GetElement(0, &consoleCommandValue);

					if (consoleCommandValue.GetType() != GFxValue::kType_String) return;

					kp.callbackName = consoleCommandValue.GetString();

					g_keybindManager.Register(kb, kp);

					_MESSAGE("Succesfully registered kType_RunConsoleCommand keybind for keycode %d.", kb.keycode);

					break;
				}
				case KeybindParameters::kType_SendEvent:
				{
					_MESSAGE("Not implemented.");
					break;
				}
				default:
					_WARNING("Failed to register keybind. Unknown keybind type.");
			}
		}
	};

	class ClearKeybind : public GFxFunctionHandler {
	public:
		enum CallType {
			kCallType_ID,
			kCallType_Keycode,
		};

		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			CallType callType = kCallType_ID;
			if (args->numArgs < 2) return;
			if (args->args[0].GetType() == GFxValue::kType_String && args->args[1].GetType() == GFxValue::kType_String) {
				callType = kCallType_ID;
			} else if (args->args[0].GetType() == GFxValue::kType_Int && args->args[1].GetType() == GFxValue::kType_Int) {
				callType = kCallType_Keycode;
			} else {
				// Invalid parameter type
				return;
			}

			g_keybindManager.Lock();
			if (callType == kCallType_ID) {
				if (g_keybindManager.ClearKeybind(args->args[0].GetString(), args->args[1].GetString()))
					args->result->SetBool(true);
			} else {
				Keybind kb = {};
				kb.keycode		= args->args[0].GetInt();
				kb.modifiers	= args->args[1].GetInt();
				if (g_keybindManager.ClearKeybind(kb))
					args->result->SetBool(true);
			}
			g_keybindManager.Release();
		}
	};

	class RemapKeybind : public GFxFunctionHandler {
	public:
		virtual void Invoke(Args* args) {
			args->result->SetBool(false);

			if (args->numArgs < 4) return;
			if (args->args[0].GetType() != GFxValue::kType_String) return;	// modName
			if (args->args[1].GetType() != GFxValue::kType_String) return;	// keybindID
			if (args->args[2].GetType() != GFxValue::kType_Int) return;		// newKeycode
			if (args->args[3].GetType() != GFxValue::kType_Int) return;		// newModifiers

			Keybind kb		= {};
			kb.keycode		= args->args[2].GetInt();
			kb.modifiers	= args->args[3].GetInt();

			g_keybindManager.Lock();
			if (g_keybindManager.RemapKeybind(args->args[0].GetString(), args->args[1].GetString(), kb))
				args->result->SetBool(true);
			g_keybindManager.Release();
		}
	};
}

void ScaleformMCM::RegisterFuncs(GFxValue* codeObj, GFxMovieRoot* movieRoot) {
	// MCM Data
	RegisterFunction<GetMCMVersionString>(codeObj, movieRoot, "GetMCMVersionString");
	RegisterFunction<GetMCMVersionCode>(codeObj, movieRoot, "GetMCMVersionCode");
	RegisterFunction<GetConfigList>(codeObj, movieRoot, "GetConfigList");

	// MCM Events
	RegisterFunction<OnMCMOpen>(codeObj, movieRoot, "OnMCMOpen");
	RegisterFunction<OnMCMClose>(codeObj, movieRoot, "OnMCMClose");

	// MCM Utilities
	RegisterFunction<DisableMenuInput>(codeObj, movieRoot, "DisableMenuInput");

	// Actions
	RegisterFunction<GetGlobalValue>(codeObj, movieRoot, "GetGlobalValue");
	RegisterFunction<SetGlobalValue>(codeObj, movieRoot, "SetGlobalValue");
	RegisterFunction<GetPropertyValue>(codeObj, movieRoot, "GetPropertyValue");
	RegisterFunction<SetPropertyValue>(codeObj, movieRoot, "SetPropertyValue");
	RegisterFunction<CallQuestFunction>(codeObj, movieRoot, "CallQuestFunction");
	RegisterFunction<CallGlobalFunction>(codeObj, movieRoot, "CallGlobalFunction");

	// Mod Settings
	RegisterFunction<GetModSettingInt>(codeObj, movieRoot, "GetModSettingInt");
	RegisterFunction<GetModSettingBool>(codeObj, movieRoot, "GetModSettingBool");
	RegisterFunction<GetModSettingFloat>(codeObj, movieRoot, "GetModSettingFloat");
	RegisterFunction<GetModSettingString>(codeObj, movieRoot, "GetModSettingString");

	RegisterFunction<SetModSettingInt>(codeObj, movieRoot, "SetModSettingInt");
	RegisterFunction<SetModSettingBool>(codeObj, movieRoot, "SetModSettingBool");
	RegisterFunction<SetModSettingFloat>(codeObj, movieRoot, "SetModSettingFloat");
	RegisterFunction<SetModSettingString>(codeObj, movieRoot, "SetModSettingString");

	// Mod Info
	RegisterFunction<IsPluginInstalled>(codeObj, movieRoot, "IsPluginInstalled");

	// Keybinds
	RegisterFunction<GetKeybind>(codeObj, movieRoot, "GetKeybind");
	RegisterFunction<GetAllKeybinds>(codeObj, movieRoot, "GetAllKeybinds");
	RegisterFunction<SetKeybind>(codeObj, movieRoot, "SetKeybind");
	RegisterFunction<ClearKeybind>(codeObj, movieRoot, "ClearKeybind");
	RegisterFunction<RemapKeybind>(codeObj, movieRoot, "RemapKeybind");
}

//-------------------------
// Input Handler
//-------------------------
class F4SEInputHandler : public BSInputEventUser
{
public:
	F4SEInputHandler() : BSInputEventUser(true) { }

	virtual void OnButtonEvent(ButtonEvent * inputEvent)
	{
		UInt32	keyCode;
		UInt32	deviceType = inputEvent->deviceType;
		UInt32	keyMask = inputEvent->keyMask;

		if (deviceType == InputEvent::kDeviceType_Mouse) {
			// Mouse
			if (keyMask < 2 || keyMask > 7) return;	// Disallow Mouse1, Mouse2, MouseWheelUp and MouseWheelDown
			keyCode = InputMap::kMacro_MouseButtonOffset + keyMask;
		} else if (deviceType == InputEvent::kDeviceType_Gamepad) {
			// Gamepad
			keyCode = InputMap::GamepadMaskToKeycode(keyMask);
		} else {
			// Keyboard
			keyCode = keyMask;
		}

		float timer	 = inputEvent->timer;
		bool  isDown = inputEvent->isDown == 1.0f && timer == 0.0f;
		bool  isUp   = inputEvent->isDown == 0.0f && timer != 0.0f;

		BSFixedString* control = inputEvent->GetControlID();

		if (isDown) {
			ScaleformMCM::ProcessKeyEvent(keyCode, true);
			ScaleformMCM::ProcessUserEvent(control->c_str(), true, deviceType);
		} else if (isUp) {
			ScaleformMCM::ProcessKeyEvent(keyCode, false);
			ScaleformMCM::ProcessUserEvent(control->c_str(), false, deviceType);
		}
	}
};
F4SEInputHandler g_scaleformInputHandler;

void ScaleformMCM::ProcessKeyEvent(UInt32 keyCode, bool isDown)
{
	BSFixedString mainMenuStr("PauseMenu");
	if ((*G::ui)->IsMenuOpen(mainMenuStr)) {
		IMenu* menu = (*G::ui)->GetMenu(mainMenuStr);
		GFxMovieRoot* movieRoot = menu->movie->movieRoot;
		GFxValue args[2];
		args[0].SetInt(keyCode);
		args[1].SetBool(isDown);
		movieRoot->Invoke("root.mcm_loader.content.ProcessKeyEvent", nullptr, args, 2);
	}
}

void ScaleformMCM::ProcessUserEvent(const char * controlName, bool isDown, int deviceType)
{
	BSFixedString mainMenuStr("PauseMenu");
	if ((*G::ui)->IsMenuOpen(mainMenuStr)) {
		IMenu* menu = (*G::ui)->GetMenu(mainMenuStr);
		GFxMovieRoot* movieRoot = menu->movie->movieRoot;
		GFxValue args[3];
		args[0].SetString(controlName);
		args[1].SetBool(isDown);
		args[2].SetInt(deviceType);
		movieRoot->Invoke("root.mcm_loader.content.ProcessUserEvent", nullptr, args, 3);
	}
}

void ScaleformMCM::RefreshMenu()
{
	BSFixedString mainMenuStr("PauseMenu");
	if ((*G::ui)->IsMenuOpen(mainMenuStr)) {
		IMenu* menu = (*G::ui)->GetMenu(mainMenuStr);
		GFxMovieRoot* movieRoot = menu->movie->movieRoot;
		movieRoot->Invoke("root.mcm_loader.content.RefreshMCM", nullptr, nullptr, 0);
	}
}

void ScaleformMCM::SetKeybindInfo(KeybindInfo ki, GFxMovieRoot * movieRoot, GFxValue * kiValue)
{
	GFxValue keycode, modifiers, keybindType, keybindID, keybindName, modName, type, flags, targetForm, callbackName;

	keycode.SetInt(ki.keycode);
	modifiers.SetInt(ki.modifiers);
	keybindType.SetInt(ki.keybindType);
	keybindID.SetString(ki.keybindID);
	keybindName.SetString(ki.keybindDesc);
	modName.SetString(ki.modName);
	type.SetInt(ki.type);
	flags.SetInt(ki.flags);
	targetForm.SetString(ki.callTarget);
	callbackName.SetString(ki.callbackName);

	movieRoot->CreateObject(kiValue);
	kiValue->SetMember("keycode", &keycode);
	kiValue->SetMember("modifiers", &modifiers);
	kiValue->SetMember("keybindType", &keybindType);
	kiValue->SetMember("keybindID", &keybindID);
	kiValue->SetMember("keybindName", &keybindName);
	kiValue->SetMember("modName", &modName);
	kiValue->SetMember("type", &type);
	kiValue->SetMember("flags", &flags);
	kiValue->SetMember("targetForm", &targetForm);
	kiValue->SetMember("callbackName", &callbackName);
}

void ScaleformMCM::RegisterForInput(bool bRegister) {
	if (bRegister) {
		g_scaleformInputHandler.enabled = true;
		tArray<BSInputEventUser*>* inputEvents = &((*G::menuControls)->inputEvents);
		BSInputEventUser* inputHandler = &g_scaleformInputHandler;
		int idx = inputEvents->GetItemIndex(inputHandler);
		if (idx == -1) {
			inputEvents->Push(&g_scaleformInputHandler);
			_MESSAGE("Registered for input events.");
		}
	} else {
		g_scaleformInputHandler.enabled = false;
	}
}

bool ScaleformMCM::RegisterScaleform(GFxMovieView * view, GFxValue * f4se_root)
{
	GFxMovieRoot* movieRoot = view->movieRoot;

	GFxValue currentSWFPath;
	const char* currentSWFPathString = nullptr;

	if (movieRoot->GetVariable(&currentSWFPath, "root.loaderInfo.url")) {
		currentSWFPathString = currentSWFPath.GetString();
	} else {
		_MESSAGE("WARNING: Scaleform registration failed.");
	}

	// Look for the menu that we want to inject into.
	if (strcmp(currentSWFPathString, "Interface/MainMenu.swf") == 0) {
        GFxValue root; movieRoot->GetVariable(&root, "root");

        // Register native code object
        GFxValue mcm; movieRoot->CreateObject(&mcm);
        root.SetMember("mcm", &mcm);
        ScaleformMCM::RegisterFuncs(&mcm, movieRoot);

		// Inject MCM menu
		GFxValue loader, urlRequest;
		movieRoot->CreateObject(&loader, "flash.display.Loader");
		movieRoot->CreateObject(&urlRequest, "flash.net.URLRequest", &GFxValue("MCM.swf"), 1);

		root.SetMember("mcm_loader", &loader);
		bool injectionSuccess = movieRoot->Invoke("root.mcm_loader.load", nullptr, &urlRequest, 1);

		movieRoot->Invoke("root.Menu_mc.addChild", nullptr, &loader, 1);

		if (!injectionSuccess) {
			_MESSAGE("WARNING: MCM injection failed.");
		}
	}

	return true;
}