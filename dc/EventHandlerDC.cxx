//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// ---------------------------------------------------------------------------
// Sega Dreamcast Port (2025-2026):
// Adapted, optimized, and maintained by Alessandro dos Anjos (Ale-DC)
// ---------------------------------------------------------------------------
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <set>
#include <map>

#include "bspf.hxx"

#include "Base.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PropsSet.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "M6532.hxx"

#include "EventHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(OSystem *osystem)
	 : myOSystem(osystem),
		myState(S_NONE),
		myAllowAllDirectionsFlag(false),
		myFryingFlag(false),
		myNumJoysticks(0)
{
	myEvent = new Event();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
	delete myEvent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::initialize()
{
	// Make sure the event/action mappings are correctly set,
	// and fill the ActionList structure with valid values

	// Integer to string conversions (for HEX) use upper or lower-case
	Common::Base::setHexUppercase(myOSystem->settings().getBool("dbg.uhex"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::reset(State state)
{
	setEventState(state);
	myOSystem->state().reset();
}

void EventHandler::poll(uInt64 time)
{
	maple_device_t *joysticA = maple_enum_dev(0, 0);
	maple_device_t *joysticB = maple_enum_dev(1, 0);

	mapJoystic(joysticA);
	mapJoystic(joysticB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::quit()
{
	myOSystem->quit();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::mapJoystic(maple_device_t *device)
{
	if (device && (device->info.functions && MAPLE_FUNC_CONTROLLER))
	{
		cont_state_t *contState = (cont_state_t *)maple_dev_status(device);

		if (contState)
		{
			bool start = (contState->buttons & CONT_START);
			bool l = (contState->ltrig > 120);
			bool r = (contState->rtrig > 120);

			int dz = 32; // Analog dead zone
			bool up = (contState->buttons & CONT_DPAD_UP) | (contState->joyy < -dz);
			bool down = (contState->buttons & CONT_DPAD_DOWN) | (contState->joyy > dz);
			bool left = (contState->buttons & CONT_DPAD_LEFT) | (contState->joyx < -dz);
			bool right = (contState->buttons & CONT_DPAD_RIGHT) | (contState->joyx > dz);

			bool a = (contState->buttons & CONT_A);
			bool b = (contState->buttons & CONT_B);
			bool x = (contState->buttons & CONT_X);
			bool y = (contState->buttons & CONT_Y);

			if (device->port == 0)
			{
				if (l && r && start)
					myOSystem->quit();

				if (start && (!l && !r))
					myOSystem->pause();

				myEvent->set(Event::JoystickZeroUp, up);
				myEvent->set(Event::JoystickZeroDown, down);
				myEvent->set(Event::JoystickZeroLeft, left);
				myEvent->set(Event::JoystickZeroRight, right);
				myEvent->set(Event::JoystickZeroFire, a);

				myEvent->set(Event::ConsoleReset, y);
				myEvent->set(Event::ConsoleSelect, x);
			}
			else if (device->port = 1)
			{
				myEvent->set(Event::JoystickOneUp, up);
				myEvent->set(Event::JoystickOneDown, down);
				myEvent->set(Event::JoystickOneLeft, left);
				myEvent->set(Event::JoystickOneRight, right);
				myEvent->set(Event::JoystickOneFire, a);
			}
		}
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type EventHandler::eventAtIndex(int idx, EventMode mode) const
{
	switch (mode)
	{
	case kEmulationMode:
		if (idx < 0 || idx >= kEmulActionListSize)
			return Event::NoType;
		else
			return ourEmulActionList[idx].event;
		break;
	default:
		return Event::NoType;
		break;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::actionAtIndex(int idx, EventMode mode) const
{
	switch (mode)
	{
	case kEmulationMode:
		if (idx < 0 || idx >= kEmulActionListSize)
			return EmptyString;
		else
			return ourEmulActionList[idx].action;
		break;
	default:
		return EmptyString;
		break;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::keyAtIndex(int idx, EventMode mode) const
{
	switch (mode)
	{
	case kEmulationMode:
		if (idx < 0 || idx >= kEmulActionListSize)
			return EmptyString;
		else
			return ourEmulActionList[idx].key;
		break;
	default:
		return EmptyString;
		break;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::enterMenuMode(State state)
{
	setEventState(state);
	myOSystem->sound().mute(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::leaveMenuMode()
{
	setEventState(S_EMULATE);
	myOSystem->sound().mute(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setEventState(State state)
{
	myState = state;

	// Normally, the usage of Control key is determined by 'ctrlcombo'
	// For certain ROMs it may be forced off, whatever the setting
	// myUseCtrlKeyFlag = myOSystem->settings().getBool("ctrlcombo");

	// Only enable Unicode in GUI modes, since there we need it for ascii data
	// Otherwise, it causes a performance hit, so leave it off
	switch (myState)
	{
	case S_EMULATE:
		myOSystem->sound().mute(false);
		break;

	case S_PAUSE:
		myOSystem->sound().mute(true);
		break;

		// case S_MENU:
		//   myOverlay = &myOSystem->menu();
		//   SDL_EnableUNICODE(1);
		//   break;

		// case S_CMDMENU:
		//	//myOverlay = &myOSystem->commandMenu();
		//	break;

		// case S_LAUNCHER:
		//   myOverlay = &myOSystem->launcher();
		//   break;

		break;
	}

	// Inform various subsystems about the new state
	myOSystem->stateChanged(myState);
	if (&myOSystem->frameBuffer())
	{
		myOSystem->frameBuffer().stateChanged(myState);
	}

	// Always clear any pending events when changing states
	myEvent->clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 EventHandler::resetEventsCallback(uInt32 interval, void *param)
{
	((EventHandler *)param)->myEvent->clear();
	return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::ActionList EventHandler::ourEmulActionList[kEmulActionListSize] = {
	 {Event::ConsoleSelect, "Select", 0, true},
	 {Event::ConsoleReset, "Reset", 0, true},
	 {Event::ConsoleColor, "Color TV", 0, true},
	 {Event::ConsoleBlackWhite, "Black & White TV", 0, true},
	 {Event::ConsoleLeftDiffA, "P0 Difficulty A", 0, true},
	 {Event::ConsoleLeftDiffB, "P0 Difficulty B", 0, true},
	 {Event::ConsoleRightDiffA, "P1 Difficulty A", 0, true},
	 {Event::ConsoleRightDiffB, "P1 Difficulty B", 0, true},
	 {Event::SaveState, "Save State", 0, false},
	 {Event::ChangeState, "Change State", 0, false},
	 {Event::LoadState, "Load State", 0, false},
	 {Event::TakeSnapshot, "Snapshot", 0, false},
	 {Event::Fry, "Fry cartridge", 0, false},
	 {Event::VolumeDecrease, "Decrease volume", 0, false},
	 {Event::VolumeIncrease, "Increase volume", 0, false},
	 {Event::PauseMode, "Pause", 0, false},
	 {Event::MenuMode, "Enter options menu mode", 0, false},
	 {Event::CmdMenuMode, "Toggle command menu mode", 0, false},
	 {Event::DebuggerMode, "Toggle debugger mode", 0, false},
	 {Event::LauncherMode, "Enter ROM launcher", 0, false},
	 {Event::Quit, "Quit", 0, false},

	 {Event::JoystickZeroUp, "P0 Joystick Up", 0, true},
	 {Event::JoystickZeroDown, "P0 Joystick Down", 0, true},
	 {Event::JoystickZeroLeft, "P0 Joystick Left", 0, true},
	 {Event::JoystickZeroRight, "P0 Joystick Right", 0, true},
	 {Event::JoystickZeroFire, "P0 Joystick Fire", 0, true},
	 {Event::JoystickZeroFire5, "P0 Booster Top Trigger", 0, true},
	 {Event::JoystickZeroFire9, "P0 Booster Handle Grip", 0, true},

	 {Event::JoystickOneUp, "P1 Joystick Up", 0, true},
	 {Event::JoystickOneDown, "P1 Joystick Down", 0, true},
	 {Event::JoystickOneLeft, "P1 Joystick Left", 0, true},
	 {Event::JoystickOneRight, "P1 Joystick Right", 0, true},
	 {Event::JoystickOneFire, "P1 Joystick Fire", 0, true},
	 {Event::JoystickOneFire5, "P1 Booster Top Trigger", 0, true},
	 {Event::JoystickOneFire9, "P1 Booster Handle Grip", 0, true},

	 {Event::KeyboardZero1, "P0 Keyboard 1", 0, true},
	 {Event::KeyboardZero2, "P0 Keyboard 2", 0, true},
	 {Event::KeyboardZero3, "P0 Keyboard 3", 0, true},
	 {Event::KeyboardZero4, "P0 Keyboard 4", 0, true},
	 {Event::KeyboardZero5, "P0 Keyboard 5", 0, true},
	 {Event::KeyboardZero6, "P0 Keyboard 6", 0, true},
	 {Event::KeyboardZero7, "P0 Keyboard 7", 0, true},
	 {Event::KeyboardZero8, "P0 Keyboard 8", 0, true},
	 {Event::KeyboardZero9, "P0 Keyboard 9", 0, true},
	 {Event::KeyboardZeroStar, "P0 Keyboard *", 0, true},
	 {Event::KeyboardZero0, "P0 Keyboard 0", 0, true},
	 {Event::KeyboardZeroPound, "P0 Keyboard #", 0, true},

	 {Event::KeyboardOne1, "P1 Keyboard 1", 0, true},
	 {Event::KeyboardOne2, "P1 Keyboard 2", 0, true},
	 {Event::KeyboardOne3, "P1 Keyboard 3", 0, true},
	 {Event::KeyboardOne4, "P1 Keyboard 4", 0, true},
	 {Event::KeyboardOne5, "P1 Keyboard 5", 0, true},
	 {Event::KeyboardOne6, "P1 Keyboard 6", 0, true},
	 {Event::KeyboardOne7, "P1 Keyboard 7", 0, true},
	 {Event::KeyboardOne8, "P1 Keyboard 8", 0, true},
	 {Event::KeyboardOne9, "P1 Keyboard 9", 0, true},
	 {Event::KeyboardOneStar, "P1 Keyboard *", 0, true},
	 {Event::KeyboardOne0, "P1 Keyboard 0", 0, true},
	 {Event::KeyboardOnePound, "P1 Keyboard #", 0, true}};
