#pragma once
#include <vector>
#include "f4se/PapyrusEvents.h"
#include "f4se/GameTypes.h"

// Forward-declaration
namespace Json {
	class Value;
}

class Keybind
{
public:
	UInt32	keycode;
	UInt8	modifiers;

	enum Modifiers {
		kModifier_Shift		= (1 << 0),
		kModifier_Control	= (1 << 1),
		kModifier_Alt		= (1 << 2),
	};

	bool operator<(const Keybind& rhs) const {
		if (keycode != rhs.keycode) {
			return keycode < rhs.keycode;
		} else {
			return modifiers < rhs.modifiers;
		}
	}

	bool operator==(const Keybind& rhs) const {
		return (keycode == rhs.keycode && modifiers == rhs.modifiers);
	}
};

struct ActionParameters
{
	enum ParamType {
		kType_None,
		kType_Int,
		kType_Float,
		kType_String,
		kType_Bool,
	};

	ParamType paramType;
	union {
		UInt32			iValue;
		bool			bValue;
		float			fValue;
		BSFixedString	sValue;
	};

	ActionParameters() {
		paramType = kType_None;
		iValue = 0;
	}
};

class KeybindParameters
{
public:
	BSFixedString   keybindID;		// A unique identifier for this keybind.
	BSFixedString   keybindDesc;	// A short description of the keybind to be shown for conflict resolution.
	BSFixedString   modName;		// The name of the mod that owns this keybind.
	UInt8			type;			// Action type.
	UInt8			flags;			// Reserved for future use.
	UInt32			targetFormID;	// The form ID of the quest form to use
	BSFixedString	callbackName;	// The function to be invoked when this keybind is activated.
	BSFixedString	scriptName;		// The script name to call the function on, for global function invocations.
	std::vector<ActionParameters> actionParams;

	enum Type {
		kType_CallFunction,
		kType_CallGlobalFunction,
		kType_RunConsoleCommand,
		kType_SendEvent,
	};

	enum Flags {
		kFlag_OnKeyDown		= (1 << 0),
		kFlag_OnKeyUp		= (1 << 1),
		kFlag_UserDefined	= (1 << 2),
	};

	/*bool operator<(const KeybindParameters& rhs) const {
		return (modifiers < rhs.modifiers);
	}*/
};

// Used to return keybind information to Papyrus or Scaleform.
// If modifying this structure, also update:
// - GetKeybind(BSFixedString modName, BSFixedString keybindID);
// - GetKeybind(Keybind kb);
// - Scaleform GetKeybind
// - Serialization functions (toJSON and fromJSON)
struct KeybindInfo
{
	UInt32			keycode;
	UInt8			modifiers;
	UInt8			keybindType;	// A value in the KeybindType enum. Represents the source of this keybind.
	BSFixedString   keybindID;		// A unique identifier for this keybind.
	BSFixedString   keybindDesc;	// A short description of the keybind to be shown for conflict resolution.
	BSFixedString   modName;		// The name of the mod that owns this keybind.
	UInt8			type;			// Action type.
	UInt8			flags;			// Reserved for future use.
	BSFixedString	callTarget;		// Form identifier for CallFunction, Script name for CallGlobalFunction.
	BSFixedString	callbackName;	// The function to be invoked when this keybind is activated.

	enum KeybindType {
		kType_Invalid,
		kType_MCM,
		kType_Game,
		kType_F4SE,
	};
};

class KeybindManager : public SafeDataHolder<std::map<Keybind, KeybindParameters>>
{
	typedef std::map<Keybind, KeybindParameters> RegMap;

public:
	// Thread-safe
	void Register(Keybind key, KeybindParameters & params);
	void Clear(void);

	bool RegisterKeybind(Keybind kb, BSFixedString modName, BSFixedString keybindID);	// Returns true if the keybind was registered. False if keybind definition could not be located.

	// Serialization
	std::string ToJSON();
	bool FromJSON(std::string jsonStr);
	void CommitKeybinds();	// Saves registered keybinds to disk if data was changed. (m_keybindsDirty)
	bool GetKeybindData(std::string modName, std::string keybindID, KeybindParameters* kp);		// Retrieves keybind data from Config\ModName\keybinds.json
	void SetActionParams(Json::Value & actionParams, KeybindParameters & kp);

	// Not thread-safe. Explicitly lock before calling these.
	KeybindInfo GetKeybind(BSFixedString modName, BSFixedString keybindID);
	KeybindInfo GetKeybind(Keybind kb);
	std::vector<KeybindInfo> GetAllKeybinds();
	
	// Returns true if successfully cleared. False if the keybind did not exist.
	bool ClearKeybind(BSFixedString modName, BSFixedString keybindID);
	bool ClearKeybind(Keybind kb);

	// Returns true if the keybind was remapped. False if the keybind was not remapped. e.g. if old and new keybinds were the same, or the keybind did not exist.
	bool RemapKeybind(BSFixedString modName, BSFixedString keybindID, Keybind newKeybind);

	// Whether or not we should save the keybind information to JSON.
	bool m_keybindsDirty = false;

private:
	// Maps a concatentation of modName+keybindID to keybind parameters.
	// Data is lazy-loaded. Mod keybind data is loaded from disk into this map when first requested and cached here for future fast lookup.
	std::map<std::string, KeybindParameters> m_keybindData;

};

extern KeybindManager g_keybindManager;