#include "MCMInput.h"
#include "ScaleformMCM.h"

#include "f4se/GameInput.h"
#include "f4se/GameMenus.h"
#include "f4se/InputMap.h"
#include "f4se/PapyrusUtilities.h"

#include "MCMInput.h"
#include "MCMKeybinds.h"

#include "Globals.h"
#include "Utils.h"

void MCMInput::RegisterForInput(bool bRegister)
{
	tArray<BSInputEventUser*>* inputEvents = &((*G::menuControls)->inputEvents);
	BSInputEventUser* inputHandler = this;
	int idx = inputEvents->GetItemIndex(inputHandler);
	if (idx > -1) {
		if (!bRegister) {
			inputEvents->Remove(idx);
		}
	} else {
		if (bRegister) {
			inputEvents->Push(inputHandler);
			_MESSAGE("Registered for input events.");
		}
	}
}

void PackArgs(VMArray<VMVariable> & arguments, KeybindParameters & kp) {
	for (int i = 0; i < kp.actionParams.size(); i++) {
		VMVariable var;
		switch (kp.actionParams[i].paramType) {
			case ActionParameters::kType_Int:
				var.Set(&kp.actionParams[i].iValue);
				break;
			case ActionParameters::kType_Bool:
				var.Set(&kp.actionParams[i].bValue);
				break;
			case ActionParameters::kType_Float:
				var.Set(&kp.actionParams[i].fValue);
				break;
			case ActionParameters::kType_String:
				var.Set(&kp.actionParams[i].sValue);
				break;
		}
		arguments.Push(&var);
	}
}

void MCMInput::OnButtonEvent(ButtonEvent * inputEvent)
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

	float timer		= inputEvent->timer;
	bool  isDown	= inputEvent->isDown == 1.0f && timer == 0.0f;
	bool  isUp		= inputEvent->isDown == 0.0f && timer != 0.0f;

	if (isDown) {
		switch (keyCode) {
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
				break;	// Shift, Ctrl, Alt modifiers

			default: {
				if ((*G::ui)->numPauseGame == 0) {
					Keybind kb = {};
					kb.keycode = keyCode;
					if (GetAsyncKeyState(VK_SHIFT) & 0x8000)		kb.modifiers |= Keybind::kModifier_Shift;
					if (GetAsyncKeyState(VK_CONTROL) & 0x8000)	kb.modifiers |= Keybind::kModifier_Control;
					if (GetAsyncKeyState(VK_MENU) & 0x8000)		kb.modifiers |= Keybind::kModifier_Alt;

					g_keybindManager.Lock();
					if (g_keybindManager.m_data.count(kb) > 0) {
						KeybindParameters kp = g_keybindManager.m_data[kb];
						g_keybindManager.Release();

						switch (kp.type) {
							case KeybindParameters::kType_CallFunction:
							{
								TESForm* form = LookupFormByID(kp.targetFormID);
								if (form) {
									VMArray<VMVariable> arguments;
									PackArgs(arguments, kp);
									CallFunctionNoWait<TESForm>(form, kp.callbackName, arguments);
								} else {
									_WARNING("Warning: Cannot call a function on a None form.");
								}
								break;
							}
							case KeybindParameters::kType_CallGlobalFunction:
							{
								if (strlen(kp.callbackName.c_str()) > 0) {
									VirtualMachine * vm = (*G::gameVM)->m_virtualMachine;
									VMArray<VMVariable> arguments;
									PackArgs(arguments, kp);
									VMValue args;
									PackValue(&args, &arguments, vm);
									CallGlobalFunctionNoWait_Internal(vm, 0, 0, &kp.scriptName, &kp.callbackName, &args);
								}
								break;
							}
							case KeybindParameters::kType_RunConsoleCommand:
							{
								if (strlen(kp.callbackName.c_str()) > 0) {
									MCMUtils::ExecuteCommand(kp.callbackName.c_str());
								}
								break;
							}
							case KeybindParameters::kType_SendEvent:
							{
								TESForm* form = LookupFormByID(kp.targetFormID);
								if (form) {
									UInt64 handle = PapyrusVM::GetHandleFromObject(form, TESForm::kTypeID);
									SendPapyrusEvent1<BSFixedString>(handle, "ScriptObject", "OnControlDown", kp.keybindID);
								} else {
									_WARNING("Warning: Cannot send an event to a None form.");
								}
								break;
							}
							default:
								_WARNING("Warning: Cannot execute a keybind with unknown action type.");
								break;
						}
						
					} else {
						g_keybindManager.Release();
					}
				}

				break;
			}
		}
		

	} else if (isUp) {
		switch (keyCode) {
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
				break;	// Shift, Ctrl, Alt modifiers

			default: {
				if ((*G::ui)->numPauseGame == 0) {
					Keybind kb = {};
					kb.keycode = keyCode;
					kb.modifiers = 0;

					g_keybindManager.Lock();
					if (g_keybindManager.m_data.count(kb) > 0) {
						KeybindParameters kp = g_keybindManager.m_data[kb];
						g_keybindManager.Release();

						switch (kp.type) {
							case KeybindParameters::kType_SendEvent:
							{
								TESForm* form = LookupFormByID(kp.targetFormID);
								if (form) {
									UInt64 handle = PapyrusVM::GetHandleFromObject(form, TESForm::kTypeID);
									SendPapyrusEvent2<BSFixedString, float>(handle, "ScriptObject", "OnControlUp", kp.keybindID, timer);
								} else {
									_WARNING("Warning: Cannot send an event to a None form.");
								}
								break;
							}
						}
					} else {
						g_keybindManager.Release();
					}
				}

				break;
			}
		}
	}
}

