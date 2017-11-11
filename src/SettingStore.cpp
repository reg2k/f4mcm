#include "SettingStore.h"

#include <string>

// reg2k
Setting::~Setting() {
    delete name;
    if (GetType() == kType_String) {
        delete data.s;
    }
}

SettingStore::SettingStore()
{
	_MESSAGE("ModSettingStore initializing.");
}

SInt32 SettingStore::GetModSettingInt(std::string modName, std::string settingName)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms) {
		return ms->data.s32;
	}
	return -1;
}

void SettingStore::SetModSettingInt(std::string modName, std::string settingName, SInt32 newValue)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms && ms->data.s32 != newValue) {
		ms->data.s32 = newValue;
		CommitModSetting(modName, ms);
	}
}

bool SettingStore::GetModSettingBool(std::string modName, std::string settingName)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms) {
		return (ms->data.u8) > 0;
	}
	return false;
}

void SettingStore::SetModSettingBool(std::string modName, std::string settingName, bool newValue)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms && ms->data.u8 != (newValue ? 1 : 0)) {
		ms->data.u8 = newValue;
		CommitModSetting(modName, ms);
	}
}

float SettingStore::GetModSettingFloat(std::string modName, std::string settingName)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms) {
		return ms->data.f32;
	}
	return -1;
}

void SettingStore::SetModSettingFloat(std::string modName, std::string settingName, float newValue)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms && ms->data.f32 != newValue) {
		ms->data.f32 = newValue;
		CommitModSetting(modName, ms);
	}
}

char* SettingStore::GetModSettingString(std::string modName, std::string settingName)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms) {
		return ms->data.s;
	}
	return nullptr;
}

void SettingStore::SetModSettingString(std::string modName, std::string settingName, const char* newValue)
{
	Setting* ms = GetModSetting(modName, settingName);
	if (ms && strcmp(ms->data.s, newValue) != 0) {
		if (ms->data.s) delete ms->data.s;
		ms->data.s = new char[strlen(newValue)+1];
		strcpy_s(ms->data.s, strlen(newValue) + 1, newValue);
		CommitModSetting(modName, ms);
	}
}

// Read ModSettings from filesystem.
void SettingStore::ReadSettings() {
	// - Load defaults from MCM\Config\Mod\defaults.ini
	// - Load user settings from MCM\Settings\Mod.ini
	// - For each file:
	// 		- Get all section names with GetPrivateProfileSectionNames
	//		- Get all key/value pairs in each section with GetPrivateProfileSection
	// - Store them in m_settingStore

	LARGE_INTEGER countStart, countEnd, frequency;
	QueryPerformanceCounter(&countStart);
	QueryPerformanceFrequency(&frequency);

	LoadDefaults();
	LoadUserSettings();

	QueryPerformanceCounter(&countEnd);
	long long int elapsed = (countEnd.QuadPart - countStart.QuadPart) / (frequency.QuadPart / 1000);
	_MESSAGE("Registered %d mod settings in %llu ms.", m_settingStore.size(), elapsed);
}

//----------------------
// Private Functions
//----------------------

void SettingStore::LoadDefaults() {
	// Find all defaults.ini files.
	HANDLE hFind;
	WIN32_FIND_DATA data;

	hFind = FindFirstFile("Data\\MCM\\Config\\*", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, "..")) continue;

			char fullPath[MAX_PATH];
			snprintf(fullPath, MAX_PATH, "%s%s%s", "Data\\MCM\\Config\\", data.cFileName, "\\settings.ini");

			if (GetFileAttributes(fullPath) == INVALID_FILE_ATTRIBUTES) continue;

			//_MESSAGE("name %s path %s", data.cFileName, fullPath);

			// Read into settings
			ReadINI(data.cFileName, fullPath);

		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}
}

void SettingStore::LoadUserSettings() {
	char* modSettingsDirectory = "Data\\MCM\\Settings\\*.ini";

	HANDLE hFind;
	WIN32_FIND_DATA data;
	std::vector<WIN32_FIND_DATA> modSettingFiles;

	hFind = FindFirstFile(modSettingsDirectory, &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			modSettingFiles.push_back(data);
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

	_MESSAGE("Number of mod setting files: %d", modSettingFiles.size());

	for (int i = 0; i < modSettingFiles.size(); i++) {
		std::string iniLocation = "./Data/MCM/Settings/";
		iniLocation += modSettingFiles[i].cFileName;

		// Extract mod name
		std::string modName(modSettingFiles[i].cFileName);
		modName = modName.substr(0, modName.find_last_of('.'));

		ReadINI(modName, iniLocation);
	}
}

bool SettingStore::ReadINI(std::string modName, std::string iniLocation) {

	//_MESSAGE("Loading mod settings for %s.", modName.c_str());

	// Extract all sections
	std::vector<std::string> sections;
	LPTSTR lpszReturnBuffer = new TCHAR[1024];
	DWORD sizeWritten = GetPrivateProfileSectionNames(lpszReturnBuffer, 1024, iniLocation.c_str());
	if (sizeWritten == (1024 - 2)) {
		_WARNING("Warning: Too many sections. Settings will not be read.");
		delete lpszReturnBuffer;
		return false;
	}

	for (LPTSTR p = lpszReturnBuffer; *p; p++) {
		std::string sectionName(p);
		sections.push_back(sectionName);
		p += strlen(p);
		ASSERT(*p == 0);
	}

	delete lpszReturnBuffer;

	//_MESSAGE("Number of sections: %d", sections.size());

	for (int j = 0; j < sections.size(); j++) {
		// Extract all keys within section
		int len = 1024;
		LPTSTR lpReturnedString = new TCHAR[len];
		DWORD sizeWritten = GetPrivateProfileSection(sections[j].c_str(), lpReturnedString, len, iniLocation.c_str());
		while (sizeWritten == (len - 2)) {
			// Buffer too small to contain all entries; expand buffer and try again.
			delete lpReturnedString;
			len <<= 1;
			lpReturnedString = new TCHAR[len];
			sizeWritten = GetPrivateProfileSection(sections[j].c_str(), lpReturnedString, len, iniLocation.c_str());
			_MESSAGE("Expanded buffer to %d bytes.", len);
		}

		for (LPTSTR p = lpReturnedString; *p; p++) {
			std::string valuePair(p);

			//_MESSAGE("%s", p);
			auto delimiter = valuePair.find_first_of('=');
			std::string settingName = valuePair.substr(0, delimiter) + ":" + sections[j];
			/*if (sections[j] != "Main" && sections[j] != modName) {
			settingName += ":" + sections[j];
			}*/
			std::string settingValue = valuePair.substr(delimiter + 1);
			RegisterModSetting(modName, settingName, settingValue);

			p += strlen(p);
			ASSERT(*p == 0);
		}

		delete lpReturnedString;
	}

	return true;
}

Setting * SettingStore::GetModSetting(std::string modName, std::string settingName)
{
	auto itr = m_settingStore.find(modName + ":" + settingName);
	if (itr != m_settingStore.end()) {
		return itr->second;
	}
	return nullptr;
}

void SettingStore::RegisterModSetting(std::string modName, std::string settingName, std::string settingValue)
{
	Setting* ms = new Setting;

	char* nameCopy = new char[settingName.size()+1];
	std::copy(settingName.begin(), settingName.end(), nameCopy);
	nameCopy[settingName.size()] = '\0';
	ms->name = nameCopy;
	
	switch (ms->GetType()) {
		case Setting::kType_Bool:
			ms->data.u8 = settingValue != "0";
			break;

		case Setting::kType_Float:
			ms->data.f32 = std::stof(settingValue);
			break;

		case Setting::kType_Integer:
			ms->data.s32 = std::stoi(settingValue);
			break;

		case Setting::kType_String: {
			ms->data.s = new char[settingValue.size() + 1];
			std::copy(settingValue.begin(), settingValue.end(), ms->data.s);
			ms->data.s[settingValue.size()] = '\0';
			break;
		}

		default:
			_WARNING("WARNING: ModSetting %s from mod %s has an unknown type and cannot be registered.", settingName.c_str(), modName.c_str());
			delete ms;
			return;
	}

	m_settingStore[modName + ":" + settingName] = ms;
}

void SettingStore::CommitModSetting(std::string modName, Setting* modSetting)
{
	std::string modSettingName(modSetting->name);
	auto delimiter = modSettingName.find_first_of(':');
	if (delimiter != std::string::npos) {
		std::string settingName = modSettingName.substr(0, delimiter);
		std::string sectionName = modSettingName.substr(delimiter+1);
		std::string value;

		switch (modSetting->GetType()) {
		case Setting::kType_Bool:
			value = std::to_string(modSetting->data.u8 & 1);
			break;
		case Setting::kType_Integer:
			value = std::to_string(modSetting->data.s32);
			break;
		case Setting::kType_Float:
			value = std::to_string(modSetting->data.f32);
			break;
		case Setting::kType_String:
			value = modSetting->data.s;
			break;
		default:
			_WARNING("WARNING: ModSetting %s from mod %s has an unknown type and cannot be saved.", settingName.c_str(), modName.c_str());
			return;
		}

		std::string iniPath = "./Data/MCM/Settings/" + modName + ".ini";
		if (GetFileAttributes("Data\\MCM\\Settings") == INVALID_FILE_ATTRIBUTES)
			CreateDirectory("Data\\MCM\\Settings", NULL);
		WritePrivateProfileString(sectionName.c_str(), settingName.c_str(), value.c_str(), iniPath.c_str());
	} else {
		_WARNING("Error: Section could not be resolved.");
	}

	
}
