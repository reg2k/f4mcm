#pragma once
#include "rva/RVA.h"

class BSScaleformManager;
class MenuControls;
class UI;
class InputManager;
class GameVM;
class DataHandler;

namespace G
{
    void Init();
    extern RVA<BSScaleformManager*> scaleformManager;
    extern RVA<MenuControls*>       menuControls;
    extern RVA<UI*>                 ui;
    extern RVA<InputManager*>       inputMgr;
    extern RVA<GameVM*>             gameVM;
    extern RVA<DataHandler*>        dataHandler;
}