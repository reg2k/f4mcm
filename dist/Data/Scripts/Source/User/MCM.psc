Scriptname MCM Native Hidden

; Checks to see whether the MCM is installed.
bool Function IsInstalled() native global

; Returns the version code of the MCM. This value is incremented for every public release of MCM.
int Function GetVersionCode() native global

; Refreshes currently displayed values in the MCM menu if it is currently open.
; Call this if you have changed values in response to a OnMCMSettingChange event.
Function RefreshMenu() native global

;-----------------
; Mod Settings
;-----------------

; Obtains the value of a mod setting.
int Function GetModSettingInt(string asModName, string asSetting) native global
bool Function GetModSettingBool(string asModName, string asSetting) native global
float Function GetModSettingFloat(string asModName, string asSetting) native global
string Function GetModSettingString(string asModName, string asSetting) native global

; Sets the value of a mod setting.
Function SetModSettingInt(string asModName, string asSettingName, int aiValue) native global
Function SetModSettingBool(string asModName, string asSettingName, bool abValue) native global
Function SetModSettingFloat(string asModName, string asSettingName, float afValue) native global
Function SetModSettingString(string asModName, string asSettingName, string asValue) native global

;-----------------
; Events
;-----------------

; Events dispatched by the MCM:
; - OnMCMSettingChange(string modName, string id)
; - OnMCMMenuOpen(string modName)
; - OnMCMMenuClose(string modName)

; To register for MCM events, use RegisterForExternalEvent.
; To receive events from a specified mod only, use the pipe symbol (|) followed by the mod name.

; e.g. RegisterForExternalEvent("OnMCMSettingChange|MyModName", "OnMCMSettingChange")

;------------------------------
; Example Event Registration
;------------------------------

; Event OnInit()
;     RegisterForExternalEvent("OnMCMSettingChange|MyModName", "OnMCMSettingChange")
; EndEvent

; Function OnMCMSettingChange(string modName, string id)
;     If (modName == "MyModName")
;         If (id == "MyControlID")
;             Debug.Notification("MyControlID's value was changed!")
;         EndIf
;     EndIf
; EndFunction
