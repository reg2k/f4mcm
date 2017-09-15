#include "MCMTranslator.h"

#include "common/IFileStream.h"
#include <shlobj.h>
#include <string>
#include "f4se/GameStreams.h"
#include "f4se/GameSettings.h"

#include "f4se/ScaleformState.h"
#include "f4se/ScaleformTranslator.h"

// This file is adapted from f4se/Translation.cpp.

namespace MCMTranslator
{
	void LoadTranslations(BSScaleformTranslator * translator)
	{
		Setting	* setting = GetINISetting("sLanguage:General");

		// Load EN strings first to ensure that no strings are unsubstituted if the locale-specific translation is not present.
		ParseTranslation(translator, "mcm", "en");

		if (strcmp(setting->data.s, "en") != 0) {
			ParseTranslation(translator, "mcm", setting->data.s);
		}
	}

	bool ParseTranslation(BSScaleformTranslator * translator, std::string modName, std::string langCode)
	{
		std::string filePath = "Interface\\Translations\\";
		filePath += modName;
		filePath += "_";
		filePath += langCode;
		filePath += ".txt";
		
		BSResourceNiBinaryStream fileStream(filePath.c_str());
		if (!fileStream.IsValid()) {
			_WARNING("Warning: No translation file available. Locale: %s", langCode.c_str());
			return false;
		}

		// Check if file is empty, if not check if the BOM is UTF-16
		UInt16	bom = 0;
		UInt32	ret = fileStream.Read(&bom, sizeof(UInt16));
		if (ret == 0) {
			_MESSAGE("Empty translation file.");
			return false;
		}
		if (bom != 0xFEFF) {
			_MESSAGE("BOM Error, file must be encoded in UCS-2 LE.");
			return false;
		}

		while (true)
		{
			wchar_t buf[512];
			UInt32	len = fileStream.ReadLine_w(buf, sizeof(buf) / sizeof(buf[0]), '\n');
			if (len == 0) // End of file
				return false;

			// at least $ + wchar_t + \t + wchar_t
			if (len < 4 || buf[0] != '$')
				continue;

			wchar_t last = buf[len - 1];
			if (last == '\r')
				len--;

			// null terminate
			buf[len] = 0;

			UInt32 delimIdx = 0;
			for (UInt32 i = 0; i < len; i++)
				if (buf[i] == '\t')
					delimIdx = i;

			// at least $ + wchar_t
			if (delimIdx < 2)
				continue;

			// replace \t by \0
			buf[delimIdx] = 0;

			BSFixedString key(buf);
			BSFixedStringW translation(&buf[delimIdx + 1]);

			TranslationTableItem* existing = translator->translations.Find(&key);
			if (existing) {
				existing->translation = translation;
			} else {
				TranslationTableItem item(key, translation);
				translator->translations.Add(&item);
			}
			
		}

		return true;
	}
}
