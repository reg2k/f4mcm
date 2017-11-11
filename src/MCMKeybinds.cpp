#include "MCMKeybinds.h"
#include <fstream>
#include <algorithm>

#include "Globals.h"
#include "Utils.h"

#include "json/json.h"

#include "f4se/GameInput.h"
#include "f4se/InputMap.h"

KeybindManager g_keybindManager;

void KeybindManager::Register(Keybind key, KeybindParameters & params)
{
	Lock();
	m_data[key] = params;
	Release();
}

void KeybindManager::Clear(void)
{
	Lock();
	m_data.clear();
	Release();
}

//------------------------------
// Serialization
//------------------------------

std::string KeybindManager::ToJSON()
{
	Lock();
	std::vector<KeybindInfo> keybinds = GetAllKeybinds();
	Release();

	Json::Value json;
	json["version"] = 1;

	for (int i = 0; i < keybinds.size(); i++) {
		Json::Value keybind;
		keybind["keycode"]			= (int)	keybinds[i].keycode;
		keybind["modifiers"]		= (int)	keybinds[i].modifiers;
		keybind["modName"]			=		keybinds[i].modName.c_str();
		keybind["id"]				=		keybinds[i].keybindID.c_str();

		json["keybinds"][i] = keybind;
	}

	Json::FastWriter fast;
	std::string jsonStr = fast.write(json);
	return jsonStr;
}

bool KeybindManager::FromJSON(std::string jsonStr)
{
	try {
		Json::Value json, keybinds;
		Json::Reader reader;
		reader.parse(jsonStr, json);

		if (json["version"].asInt() < 1) return false;

		keybinds = json["keybinds"];
		if (!keybinds.isArray()) return false;

		for (int i = 0; i < keybinds.size(); i++) {
			const Json::Value& keybind = keybinds[i];

			Keybind kb = {};
			KeybindParameters kp = {};

			kb.keycode		= keybind["keycode"].asInt();
			kb.modifiers	= keybind["modifiers"].asInt();

			std::string modName		= keybind["modName"].asString();
			std::string keybindID	= keybind["id"].asString();

			if (GetKeybindData(modName, keybindID, &kp)) {
				Lock();
				m_data[kb] = kp;
				Release();
			} else {
				_MESSAGE("Warning: Failed to get keybind data for %s with keybind ID %s", modName.c_str(), keybindID.c_str());
				continue;
			}
		}

		return true;
	} catch (...) {
		_WARNING("Warning: Keybind storage deserialization failure. No keybinds will be loaded.");
		return false;
	}
}

void KeybindManager::CommitKeybinds()
{
	if (m_keybindsDirty) {
		// Save keybinds only if the data has changed.
		LARGE_INTEGER countStart, countEnd, frequency;
		QueryPerformanceCounter(&countStart);
		QueryPerformanceFrequency(&frequency);

		Lock();
		try {
			_MESSAGE("Serializing keybinds...");
			if (GetFileAttributes("Data\\MCM\\Settings") == INVALID_FILE_ATTRIBUTES)
				CreateDirectory("Data\\MCM\\Settings", NULL);
			std::ofstream file("Data\\MCM\\Settings\\Keybinds.json");
			std::string jsonStr = ToJSON();
			file << jsonStr;
			file.close();
			m_keybindsDirty = false;
		} catch (...) {
			_MESSAGE("Warning: An error occurred when serializing keybinds.");
		}
		Release();

		QueryPerformanceCounter(&countEnd);
		_MESSAGE("Elapsed: %llu ms.", (countEnd.QuadPart - countStart.QuadPart) / (frequency.QuadPart / 1000));
	}
}

bool KeybindManager::GetKeybindData(std::string modName, std::string keybindID, KeybindParameters * kp)
{
	// Check if we've already cached the data.
	std::string keyName = modName + keybindID;
	std::transform(keyName.begin(), keyName.end(), keyName.begin(), ::tolower);
	if (m_keybindData.count(keyName) == 0) {
		try {
			// Not in the cache. Load from disk.
			auto idx = modName.find_last_of('.');	// Strip trailing .esp / .esm
			std::string filePath = "Data\\MCM\\Config\\" + modName.substr(0, idx) + "\\keybinds.json";
			_MESSAGE("Loading keybind definitions for %s", modName.c_str());
			std::ifstream file(filePath);
			if (file.is_open()) {
				Json::Value json, keybinds;
				Json::Reader reader;
				reader.parse(file, json);

				keybinds = json["keybinds"];
				if (!keybinds.isArray()) return false;

				for (int i = 0; i < keybinds.size(); i++) {
					const Json::Value& keybind = keybinds[i];

					KeybindParameters kp = {};
					kp.modName		= json["modName"].asCString();
					kp.keybindID	= keybind["id"].asCString();
					kp.keybindDesc	= keybind["desc"].asCString();
					//kp.flags		= keybind["flags"].asInt();
					kp.type			= -1;

					std::string typeStr = keybind["action"]["type"].asString();
					if		(typeStr == "CallFunction")			kp.type = KeybindParameters::kType_CallFunction;
					else if (typeStr == "CallGlobalFunction")	kp.type = KeybindParameters::kType_CallGlobalFunction;
					else if (typeStr == "RunConsoleCommand")	kp.type = KeybindParameters::kType_RunConsoleCommand;
					else if (typeStr == "SendEvent")			kp.type = KeybindParameters::kType_SendEvent;

					switch (kp.type) {
						case KeybindParameters::kType_CallFunction:
						{
							TESForm* form = MCMUtils::GetFormFromIdentifier(keybind["action"]["form"].asString());
							if (form) {
								kp.targetFormID = form->formID;
								kp.callbackName = keybind["action"]["function"].asCString();

								Json::Value actionParams = keybind["action"]["params"];
								if (!actionParams.isNull()) SetActionParams(actionParams, kp);
							} else {
								// Invalid form.
								continue;
							}
							break;
						}
						case KeybindParameters::kType_CallGlobalFunction:
						{
							kp.scriptName = keybind["action"]["script"].asCString();
							kp.callbackName = keybind["action"]["function"].asCString();

							Json::Value actionParams = keybind["action"]["params"];
							if (!actionParams.isNull()) SetActionParams(actionParams, kp);
							
							break;
						}
						case KeybindParameters::kType_RunConsoleCommand:
						{
							kp.callbackName = keybind["action"]["command"].asCString();
							break;
						}
						case KeybindParameters::kType_SendEvent:
						{
							TESForm* form = MCMUtils::GetFormFromIdentifier(keybind["action"]["form"].asString());
							if (form) {
								kp.targetFormID = form->formID;
							} else {
								// Invalid form.
								continue;
							}
							break;
						}
						default:
						{
							_WARNING("Warning: Cannot deserialize invalid keybind action type %s.", typeStr.c_str());
							continue;
							break;
						}
					}

					std::string keyName = json["modName"].asString() + keybind["id"].asString();
					std::transform(keyName.begin(), keyName.end(), keyName.begin(), ::tolower);
					m_keybindData[keyName] = kp;
				}
			} else {
				// Keybinds file doesn't exist.
				return false;
			}
		} catch (...) {
			_WARNING("Warning: Failed to parse malformed keybind definition file for mod %s.", modName.c_str());
			return false;
		}
	}

	if (m_keybindData.count(keyName) > 0) {
		*kp = m_keybindData[keyName];
		return true;
	} else {
		// The keybind doesn't exist anymore.
		return false;
	}
}

void KeybindManager::SetActionParams(Json::Value & actionParams, KeybindParameters & kp)
{
	if (actionParams.isArray()) {
		for (int i = 0; i < actionParams.size(); i++) {
			ActionParameters ap;
			switch (actionParams[i].type()) {
			case Json::intValue:
				ap.paramType = ActionParameters::kType_Int;
				ap.iValue = actionParams[i].asInt();
				break;
			case Json::booleanValue:
				ap.paramType = ActionParameters::kType_Bool;
				ap.bValue = actionParams[i].asBool();
				break;
			case Json::realValue:
				ap.paramType = ActionParameters::kType_Float;
				ap.fValue = actionParams[i].asFloat();
				break;
			case Json::stringValue:
				ap.paramType = ActionParameters::kType_String;
				ap.sValue = actionParams[i].asCString();
				break;
			default:
				// Unknown value type
				_MESSAGE("Cannot register unknown parameter value type: %s", actionParams[i].type());
				break;
			}
			if (ap.paramType != ActionParameters::kType_None) {
				kp.actionParams.push_back(ap);
			}
		}
	}
}

KeybindInfo KeybindManager::GetKeybind(BSFixedString modName, BSFixedString keybindID)
{
	for (RegMap::iterator iter = m_data.begin(); iter != m_data.end(); iter++) {
		if (iter->second.modName == modName && iter->second.keybindID == keybindID) {
			return GetKeybind(iter->first);
		}
	}
	KeybindInfo ki = {};
	return ki;
}

KeybindInfo KeybindManager::GetKeybind(Keybind kb)
{
	KeybindInfo ki = {};
	ki.keybindType = KeybindInfo::kType_Invalid;
	ki.keycode = kb.keycode;
	ki.modifiers = kb.modifiers;

	if (m_data.find(kb) != m_data.end()) {
		ki.keybindType = KeybindInfo::kType_MCM;
		ki.keybindID = m_data[kb].keybindID;
		ki.keybindDesc = m_data[kb].keybindDesc;
		ki.modName = m_data[kb].modName;
		ki.type = m_data[kb].type;
		ki.flags = m_data[kb].flags;
		ki.callbackName = m_data[kb].callbackName;

		switch (ki.type) {
			case KeybindParameters::kType_CallFunction:
			case KeybindParameters::kType_SendEvent:
			{
				ki.callTarget = MCMUtils::GetIdentifierFromFormID(m_data[kb].targetFormID).c_str();
				break;
			}
			case KeybindParameters::kType_CallGlobalFunction:
			{
				ki.callTarget = m_data[kb].scriptName;
				break;
			}
			case KeybindParameters::kType_RunConsoleCommand:
			{
				ki.callTarget = m_data[kb].callbackName;
				break;
			}
		}
	} else {
		// Not in registered keybinds. Check game keybinds.
		if (kb.keycode > 0) {
			BSFixedString controlName("");

			if (kb.keycode < InputMap::kMacro_MouseButtonOffset) {
				// KB
				controlName = (*G::inputMgr)->GetMappedControl(kb.keycode, InputEvent::kDeviceType_Keyboard, InputManager::kContext_Gameplay);
				
			} else if (kb.keycode < InputMap::kMacro_GamepadOffset) {
				// Mouse
				controlName = (*G::inputMgr)->GetMappedControl(kb.keycode - InputMap::kMacro_MouseButtonOffset, InputEvent::kDeviceType_Mouse, InputManager::kContext_Gameplay);

			} else if (kb.keycode < InputMap::kMaxMacros) {
				// Gamepad
				controlName = (*G::inputMgr)->GetMappedControl(InputMap::GamepadKeycodeToMask(kb.keycode), InputEvent::kDeviceType_Gamepad, InputManager::kContext_Gameplay);
			}

			if (strcmp("", controlName.c_str()) != 0) {
				ki.keybindType = KeybindInfo::kType_Game;
				ki.keybindID = "";
				ki.keybindDesc = controlName;
				ki.modName = "Fallout4.esm";
				ki.type = 0;
				ki.flags = 0;
				ki.callTarget = "";
				ki.callbackName = "";
			}
		}
	}

	return ki;
}

std::vector<KeybindInfo> KeybindManager::GetAllKeybinds()
{
	std::vector<KeybindInfo> keybinds;
	for (RegMap::iterator iter = m_data.begin(); iter != m_data.end(); iter++) {
		KeybindInfo ki = GetKeybind(iter->first);
		keybinds.push_back(ki);
	}
	return keybinds;
}

bool KeybindManager::RegisterKeybind(Keybind kb, BSFixedString modName, BSFixedString keybindID)
{
	KeybindParameters kp = {};
	if (GetKeybindData(modName.c_str(), keybindID.c_str(), &kp)) {
		Lock();
		m_data[kb] = kp;
		Release();
		m_keybindsDirty = true;
		return true;
	} else {
		return false;
	}
}

bool KeybindManager::ClearKeybind(BSFixedString modName, BSFixedString keybindID)
{
	for (RegMap::iterator iter = m_data.begin(); iter != m_data.end(); iter++) {
		if (iter->second.modName == modName && iter->second.keybindID == keybindID) {
			m_data.erase(iter);
			m_keybindsDirty = true;
			return true;
		}
	}
	return false;
}

bool KeybindManager::ClearKeybind(Keybind kb)
{
	auto iter = m_data.find(kb);
	if (iter != m_data.end()) {
		m_data.erase(iter);
		m_keybindsDirty = true;
		return true;
	} else {
		return false;
	}
}

bool KeybindManager::RemapKeybind(BSFixedString modName, BSFixedString keybindID, Keybind newKeybind)
{
	for (RegMap::iterator iter = m_data.begin(); iter != m_data.end(); iter++) {
		if (iter->second.modName == modName && iter->second.keybindID == keybindID) {
			Keybind oldKeybind = iter->first;
			if (oldKeybind == newKeybind) return false;
			m_data[newKeybind] = iter->second;
			m_data.erase(iter);
			m_keybindsDirty = true;
			return true;
		}
	}
	return false;
}
