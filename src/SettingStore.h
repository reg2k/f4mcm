#pragma once

#include <unordered_map>

#include "f4se/GameSettings.h"

//struct ModSetting {
//	char* settingName;
//	union {
//		float floatValue;
//		bool boolValue;
//		SInt32 intValue;
//		char* strValue;
//	};
//};

// Singleton
class SettingStore
{
public:
	static SettingStore& GetInstance() {
		static SettingStore instance;
		return instance;
	}
	void ReadSettings();

	SInt32 GetModSettingInt(std::string modName, std::string settingName);
	void SetModSettingInt(std::string modName, std::string settingName, SInt32 newValue);
	
	bool GetModSettingBool(std::string modName, std::string settingName);
	void SetModSettingBool(std::string modName, std::string settingName, bool newValue);

	float GetModSettingFloat(std::string modName, std::string settingName);
	void SetModSettingFloat(std::string modName, std::string settingName, float newValue);

	char* GetModSettingString(std::string modName, std::string settingName);
	void SetModSettingString(std::string modName, std::string settingName, const char* newValue);

private:
	SettingStore();
	std::unordered_map<std::string, Setting*> m_settingStore;

	bool ReadINI(std::string modName, std::string iniLocation);
	void LoadDefaults();
	void LoadUserSettings();

	Setting* GetModSetting(std::string modName, std::string settingName);
	void RegisterModSetting(std::string modName, std::string settingName, std::string settingValue);
	void CommitModSetting(std::string modName, Setting* modSetting);

public:
	// Get rid of unwanted constructors
	SettingStore(SettingStore const&)	= delete;
	void operator=(SettingStore const&) = delete;
};

