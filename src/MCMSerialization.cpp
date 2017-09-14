#include <fstream>

#include "json/json.h"

#include "MCMSerialization.h"
#include "MCMKeybinds.h"

const char* KEYBIND_LOCATION = "Data\\MCM\\Settings\\Keybinds.json";

namespace MCMSerialization
{
	enum SaveVersion
	{
		kVersion_1,
		kCurrentVersion = kVersion1
	};

	void RevertCallback(const F4SESerializationInterface * intfc)
	{
		_DMESSAGE("Clearing MCM co-save internal state.");
		g_keybindManager.Clear();
	}

	void LoadCallback(const F4SESerializationInterface * intfc)
	{
		_DMESSAGE("Loading MCM data.");

		LARGE_INTEGER countStart, countEnd, frequency;
		QueryPerformanceCounter(&countStart);
		QueryPerformanceFrequency(&frequency);

		// Load keybind registrations.
		std::ifstream file(KEYBIND_LOCATION);
		std::stringstream ss;
		if (file.is_open()) {
			ss << file.rdbuf();
			file.close();
			g_keybindManager.FromJSON(ss.str());
		} else {
			_MESSAGE("Keybind storage could not be opened or does not exist.");
		}

		QueryPerformanceCounter(&countEnd);
		_MESSAGE("Elapsed: %llu ms.", (countEnd.QuadPart - countStart.QuadPart) / (frequency.QuadPart / 1000));
	}

	void SaveCallback(const F4SESerializationInterface * intfc)
	{
		g_keybindManager.CommitKeybinds();
	}
	
}