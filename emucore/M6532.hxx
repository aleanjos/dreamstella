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

#ifndef M6532_HXX
#define M6532_HXX

class Console;
class Settings;

#include "bspf.hxx"
#include "Device.hxx"
#include "System.hxx"
#include "Console.hxx"
#include "Control.hxx"

/**
  This class models the M6532 RAM-I/O-Timer (aka RIOT) chip in the 2600
  console.  Note that since the M6507 CPU doesn't contain an interrupt line,
  the following functionality relating to the RIOT IRQ line is not emulated:

    - A3 to enable/disable interrupt from timer to IRQ
    - A1 to enable/disable interrupt from PA7 to IRQ

  @author  Bradford W. Mott and Stephen Anthony
  @version $Id$
*/
class M6532 : public Device
{
public:
  /**
    Create a new 6532 for the specified console

    @param console  The console the 6532 is associated with
    @param settings The settings used by the system
  */
  M6532(const Console &console, const Settings &settings);

  /**
    Destructor
  */
  virtual ~M6532();

public:
  /**
    Reset cartridge to its power-on state
  */
  void reset();

  /**
    Notification method invoked by the system right before the
    system resets its cycle counter to zero.  It may be necessary
    to override this method for devices that remember cycle counts.
  */
  void systemCyclesReset();

  /**
    Update the entire digital and analog pin state of ports A and B.
  */
  void update();

  /**
    Install 6532 in the specified system.  Invoked by the system
    when the 6532 is attached to it.

    @param system The system the device should install itself in
  */
  void install(System &system);

  /**
    Install 6532 in the specified system and device.  Invoked by
    the system when the 6532 is attached to it.  All devices
    which invoke this method take responsibility for chaining
    requests back to *this* device.

    @param system The system the device should install itself in
    @param device The device responsible for this address space
  */
  void install(System &system, Device &device);

  /**
    Save the current state of this device to the given Serializer.

    @param out  The Serializer object to use
    @return  False on any errors, else true
  */
  bool save(Serializer &out) const;

  /**
    Load the current state of this device from the given Serializer.

    @param in  The Serializer object to use
    @return  False on any errors, else true
  */
  bool load(Serializer &in);

  /**
    Get a descriptor for the device name (used in error checking).

    @return The name of the object
  */
  string name() const { return "M6532"; }

public:
  /**
    Get the byte at the specified address

    @return The byte at the specified address
  */
  // uInt8 peek(uInt16 address);

  inline uInt8 peek(uInt16 addr) __attribute__((always_inline))
  {
    if (addr < 0x100)
    {
      return myRAM[addr & 0x7f];
    }
    if ((addr & 0x1080) == 0x0080 && (addr & 0x0200) == 0x0000)
    {
      return myRAM[addr & 0x007f];
    }

    return peekSpecial(addr);
  }

private:
  uInt8 peekSpecial(uInt16 addr);
  bool pokeSpecial(uInt16 addr, uInt8 value);

  /**
    Change the byte at the specified address to the given value

    @param address The address where the value should be stored
    @param value The value to be stored at the address

    @return  True if the poke changed the device address space, else false
  */
public:
  inline bool poke(uInt16 addr, uInt8 value) __attribute__((always_inline))
  {
    if (addr < 0x100)
    {
      myRAM[addr & 0x7F] = value;
      return true;
    }

    if ((addr & 0x1080) == 0x0080 && (addr & 0x0200) == 0x0000)
    {
      myRAM[addr & 0x007f] = value;
      return true;
    }

    return pokeSpecial(addr, value);
  }

private:
  inline Int32 timerClocks() const __attribute__((always_inline))
  {
    return myTimer - (mySystem->cycles() - myCyclesWhenTimerSet);
  }

  inline void setTimerRegister(uInt8 value, uInt8 interval) __attribute__((always_inline))
  {
    static const uInt8 shift[] = {0, 3, 6, 10};

    myIntervalShift = shift[interval];
    myOutTimer[interval] = value;
    myTimer = value << myIntervalShift;
    myCyclesWhenTimerSet = mySystem->cycles();

    // Interrupt timer flag is cleared (and invalid) when writing to the timer
    myInterruptFlag &= ~TimerBit;
    myTimerFlagValid = false;
  }

  inline void setPinState(bool swcha) __attribute__((always_inline))
{
  /*
    When a bit in the DDR is set as input, +5V is placed on its output
    pin.  When it's set as output, either +5V or 0V (depending on the
    contents of SWCHA) will be placed on the output pin.
    The standard macros for the AtariVox and SaveKey use this fact to
    send data to the port.  This is represented by the following algorithm:

      if(DDR bit is input)       set output as 1
      else if(DDR bit is output) set output as bit in ORA
  */
  Controller& port0 = myConsole.controller(Controller::Left);
  Controller& port1 = myConsole.controller(Controller::Right);

  uInt8 ioport = myOutA | ~myDDRA;

  port0.write(Controller::One,   ioport & 0x10);
  port0.write(Controller::Two,   ioport & 0x20);
  port0.write(Controller::Three, ioport & 0x40);
  port0.write(Controller::Four,  ioport & 0x80);
  port1.write(Controller::One,   ioport & 0x01);
  port1.write(Controller::Two,   ioport & 0x02);
  port1.write(Controller::Three, ioport & 0x04);
  port1.write(Controller::Four,  ioport & 0x08);

  if(swcha)
  {
    port0.controlWrite(ioport);
    port1.controlWrite(ioport);
  }
}

  // The following are used by the debugger to read INTIM/TIMINT
  // We need separate methods to do this, so the state of the system
  // isn't changed
  uInt8 intim() const;
  uInt8 timint() const;
  Int32 intimClocks() const;

private:
  // Accessible bits in the interrupt flag register
  // All other bits are always zeroed
  enum
  {
    TimerBit = 0x80,
    PA7Bit = 0x40
  };

  // Reference to the console
  const Console &myConsole;

  // Reference to the settings
  const Settings &mySettings;

  // An amazing 128 bytes of RAM
  uInt8 myRAM[128] __attribute__((aligned(32)));

  // Current value of the timer
  uInt32 myTimer;

  // Log base 2 of the number of cycles in a timer interval
  uInt32 myIntervalShift;

  // Indicates the number of cycles when the timer was last set
  Int32 myCyclesWhenTimerSet;

  // Data Direction Register for Port A
  uInt8 myDDRA;

  // Data Direction Register for Port B
  uInt8 myDDRB;

  // Last value written to Port A
  uInt8 myOutA;

  // Last value written to Port B
  uInt8 myOutB;

  // Interrupt Flag Register
  uInt8 myInterruptFlag;

  // Whether the timer flag (as currently set) can be used
  // If it isn't valid, it will be updated as required
  bool myTimerFlagValid;

  // Used to determine whether an active transition on PA7 has occurred
  // True is positive edge-detect, false is negative edge-detect
  bool myEdgeDetectPositive;

  // Last value written to the timer registers
  uInt8 myOutTimer[4];

private:
  // Copy constructor isn't supported by this class so make it private
  M6532(const M6532 &);

  // Assignment operator isn't supported by this class so make it private
  M6532 &operator=(const M6532 &);
};

#endif
