#pragma once
#include <vector>

#include "f4se/GameInput.h"

class MCMInput : public BSInputEventUser
{
public:
	static MCMInput& GetInstance() {
		static MCMInput instance;
		return instance;
	}
	
	MCMInput(MCMInput const&)		= delete;
	void operator=(MCMInput const&) = delete;

	void RegisterForInput(bool bRegister);

	// Input Handlers
	virtual void OnButtonEvent(ButtonEvent * inputEvent);

private:
	MCMInput() : BSInputEventUser(true) { }
};

