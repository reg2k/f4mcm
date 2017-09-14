#pragma once

class GFxMovieRoot;
class GFxMovieView;
class GFxValue;

struct KeybindInfo;

namespace ScaleformMCM
{
	bool RegisterScaleform(GFxMovieView* view, GFxValue* f4se_root);
	void RegisterFuncs(GFxValue* codeObj, GFxMovieRoot* movieRoot);

	void RegisterForInput(bool bRegister);
	void ProcessKeyEvent(UInt32 keyCode, bool isDown);
	void ProcessUserEvent(const char* controlName, bool isDown, int deviceType);

	void SetKeybindInfo(KeybindInfo ki, GFxMovieRoot* movieRoot, GFxValue* kiValue);
	void RefreshMenu();
}