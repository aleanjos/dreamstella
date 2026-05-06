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

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <map>

class Console;
class OSystem;
class StringList;
class VariantList;

#include "Array.hxx"
#include "Event.hxx"
#include "StellaKeys.hxx"
#include "bspf.hxx"

enum MouseButton
{
  EVENT_LBUTTONDOWN,
  EVENT_LBUTTONUP,
  EVENT_RBUTTONDOWN,
  EVENT_RBUTTONUP,
  EVENT_WHEELDOWN,
  EVENT_WHEELUP
};

enum JoyHat
{
  EVENT_HATUP = 0,   // make sure these are set correctly,
  EVENT_HATDOWN = 1, // since they'll be used as array indices
  EVENT_HATLEFT = 2,
  EVENT_HATRIGHT = 3,
  EVENT_HATCENTER = 4
};

enum EventMode
{
  kEmulationMode = 0, // make sure these are set correctly,
  kMenuMode = 1,      // since they'll be used as array indices
  kNumModes = 2
};

/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontend will send translated events here, and the handler will
  check to see what the current 'mode' is.

  If in emulation mode, events received from the frontend are remapped and
  sent to the emulation core.  If in menu mode, the events are sent
  unchanged to the menu class, where (among other things) changing key
  mapping can take place.

  @author  Stephen Anthony
  @version $Id$
*/
class EventHandler
{
public:
  /**
    Create a new event handler object
  */
  EventHandler(OSystem *osystem);

  /**
    Destructor
  */
  virtual ~EventHandler();

  // Enumeration representing the different states of operation
  enum State
  {
    S_NONE,
    S_EMULATE,
    S_PAUSE,
    S_LAUNCHER,
    S_MENU,
    S_CMDMENU,
    S_DEBUGGER
  };

  /**
    Returns the event object associated with this handler class.

    @return The event object
  */
  Event &event() { return *myEvent; }

  // Global Event object
  Event *myEvent;

  // Global OSystem object
  OSystem *myOSystem;

  /**
    Initialize state of this eventhandler.
  */
  void initialize();

  /**
    Collects and dispatches any pending events.  This method should be
    called regularly (at X times per second, where X is the game framerate).

    @param time  The current time in microseconds.
  */
  void poll(uInt64 time);

  /**
    Returns the current state of the EventHandler

    @return The State type
  */
  State state() const { return myState; }

  /**
    Resets the state machine of the EventHandler to the defaults

    @param state  The current state to set
  */
  void reset(State state);

  void quit();

  /**
    Set the number of seconds between taking a snapshot in
    continuous snapshot mode.  Setting an interval of 0 disables
    continuous snapshots.

    @param interval  Interval in seconds between snapshots
  */
  void setContinuousSnapshots(uInt32 interval);

  void enterMenuMode(State state);
  void leaveMenuMode();

  bool frying() const { return myFryingFlag; }

  void getActionList(EventMode mode, StringList &list) const;

  Event::Type eventForKey(StellaKey key, EventMode mode) const
  {
    return myKeyTable[key][mode];
  }

  Event::Type eventAtIndex(int idx, EventMode mode) const;
  string actionAtIndex(int idx, EventMode mode) const;
  string keyAtIndex(int idx, EventMode mode) const;

  /**
    Bind a key to an event/action and regenerate the mapping array(s)

    @param event  The event we are remapping
    @param mode   The mode where this event is active
    @param key    The key to bind to this event
  */
  bool addKeyMapping(Event::Type event, EventMode mode, StellaKey key);

  /**
    Bind a joystick axis direction to an event/action and regenerate
    the mapping array(s)

    @param event  The event we are remapping
    @param mode   The mode where this event is active
    @param stick  The joystick number
    @param axis   The joystick axis
    @param value  The value on the given axis
    @param updateMenus  Whether to update the action mappings (normally
                        we want to do this, unless there are a batch of
                        'adds', in which case it's delayed until the end
  */

  /**
    Bind a joystick button to an event/action and regenerate the
    mapping array(s)

    @param event  The event we are remapping
    @param mode   The mode where this event is active
    @param stick  The joystick number
    @param button The joystick button
    @param updateMenus  Whether to update the action mappings (normally
                        we want to do this, unless there are a batch of
                        'adds', in which case it's delayed until the end
  */

  /**
    Bind a joystick hat direction to an event/action and regenerate
    the mapping array(s)

    @param event  The event we are remapping
    @param mode   The mode where this event is active
    @param stick  The joystick number
    @param axis   The joystick hat
    @param value  The value on the given hat
    @param updateMenus  Whether to update the action mappings (normally
                        we want to do this, unless there are a batch of
                        'adds', in which case it's delayed until the end
  */

  /**
    Erase the specified mapping

    @param event  The event for which we erase all mappings
    @param mode   The mode where this event is active
  */
  void eraseMapping(Event::Type event, EventMode mode);

  /**
    Resets the event mappings to default values

    @param event  The event which to (re)set (Event::NoType resets all)
    @param mode   The mode for which the defaults are set
  */
  void setDefaultMapping(Event::Type event, EventMode mode);

  /**
    Joystick emulates 'impossible' directions (ie, left & right
    at the same time)

    @param allow  Whether or not to allow impossible directions
  */
  void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }

private:
  enum
  {
    kComboSize = 16,
    kEventsPerCombo = 8,
    kEmulActionListSize = 75 + kComboSize,
    kMenuActionListSize = 14
  };

  void setKeymap();

  void mapKeyboard(maple_device_t *mapleDevice);
  void mapJoystic(maple_device_t *mapleDevice);

  void setMouseAsPaddle(int paddle, const string &message);

  /**
    Tests if a given event should use continuous/analog values.

    @param event  The event to test for analog processing
    @return       True if analog, else false
  */
  bool eventIsAnalog(Event::Type event) const;

  void setEventState(State state);

  // Callback function invoked by the event-reset SDL Timer
  static uInt32 resetEventsCallback(uInt32 interval, void *param);

private:
  // Structure used for action menu items
  struct ActionList
  {
    Event::Type event;
    const char *action;
    char *key;
    bool allow_combo;
  };

  struct JoyMouse
  { // Used for joystick to mouse emulation
    bool active;
    int x, y, x_amt, y_amt, amt, val, old_val;
  };

  // Used for Dreamcast Menus
  struct EventSwitches
  {
    bool isBlackAndWhite;
    bool isAdvancedDifficultyP1;
    bool isAdvancedDifficultyP2;
  };

  EventSwitches myEventSwitches;

  // Array of key events, indexed by StellaKey
  Event::Type myKeyTable[KBDK_LAST][kNumModes];

  // The event(s) assigned to each combination event
  Event::Type myComboTable[kComboSize][kEventsPerCombo];

  // Array of strings which correspond to the given StellaKey
  string ourKBDKMapping[KBDK_LAST];

  // Indicates the current state of the system (ie, which mode is current)
  State myState;

  // Indicates whether the joystick emulates 'impossible' directions
  bool myAllowAllDirectionsFlag;

  // Indicates whether or not we're in frying mode
  bool myFryingFlag;

  // Holds static strings for the remap menu (emulation and menu events)
  static ActionList ourEmulActionList[kEmulActionListSize];

  // Static lookup tables for Stelladaptor/2600-daptor axis/button support
  static const Event::Type SA_Axis[2][2];
  static const Event::Type SA_Button[2][4];
  static const Event::Type SA_Key[2][12];

  uInt32 myNumJoysticks;
  map<string, string> myJoystickMap;

  public:
    EventSwitches& eventSwitches() { return myEventSwitches; };
};

#endif
