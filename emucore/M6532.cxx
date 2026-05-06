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

#include <cassert>

#include "Console.hxx"
#include "Settings.hxx"
#include "Switches.hxx"
#include "System.hxx"

#include "M6532.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const Console& console, const Settings& settings)
  : myConsole(console),
    mySettings(settings)
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::~M6532()
{
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::reset()
{
  // Initialize the 128 bytes of memory
  if(mySettings.getBool("ramrandom"))
    for(uInt32 t = 0; t < 128; ++t)
      myRAM[t] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, 128);

  // The timer absolutely cannot be initialized to zero; some games will
  // loop or hang (notably Solaris and H.E.R.O.)
  myTimer = (0xff - (mySystem->randGenerator().next() % 0xfe)) << 10;
  myIntervalShift = 10;
  myCyclesWhenTimerSet = 0;

  // Zero the I/O registers
  myDDRA = myDDRB = myOutA = myOutB = 0x00;

  // Zero the timer registers
  myOutTimer[0] = myOutTimer[1] = myOutTimer[2] = myOutTimer[3] = 0x00;

  // Zero the interrupt flag register and mark D7 as invalid
  myInterruptFlag = 0x00;
  myTimerFlagValid = false;

  // Edge-detect set to negative (high to low)
  myEdgeDetectPositive = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::systemCyclesReset()
{
  // System cycles are being reset to zero so we need to adjust
  // the cycle count we remembered when the timer was last set
  myCyclesWhenTimerSet -= mySystem->cycles();

  // We should also inform any 'smart' controllers as well
  myConsole.controller(Controller::Left).systemCyclesReset();
  myConsole.controller(Controller::Right).systemCyclesReset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::update()
{
  Controller& port0 = myConsole.controller(Controller::Left);
  Controller& port1 = myConsole.controller(Controller::Right);

  // Get current PA7 state
  bool prevPA7 = port0.myDigitalPinState[Controller::Four];

  // Update entire port state
  port0.update();
  port1.update();
  myConsole.switches().update();

  // Get new PA7 state
  bool currPA7 = port0.myDigitalPinState[Controller::Four];

  // PA7 Flag is set on active transition in appropriate direction
  if((!myEdgeDetectPositive && prevPA7 && !currPA7) ||
     (myEdgeDetectPositive && !prevPA7 && currPA7))
    myInterruptFlag |= PA7Bit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system)
{
  install(system, *this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system, Device& device)
{
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  //assert((0x1080 & mask) == 0);
  
  // All accesses are to the given device
  System::PageAccess access(0, 0, 0, &device, System::PA_READWRITE);

  // We're installing in a 2600 system
  for(int address = 0; address < 8192; address += (1 << shift))
    if((address & 0x1080) == 0x0080)
      mySystem->setPageAccess(address >> shift, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::peekSpecial(uInt16 addr)
{
   switch (addr & 0x07)
   {
   case 0x04: 
   case 0x06:
   {
      myInterruptFlag &= ~TimerBit;
      Int32 timer = timerClocks();
   
      if (!(timer & 0x40000)) {
         return (timer >> myIntervalShift) & 0xff;
      }

      uInt8 divByOne = timer & 0xff;
      if (divByOne != 0 && divByOne != 255)
         myTimerFlagValid = true;
      return divByOne;
   }

   case 0x05: 
   case 0x07:
   {
      if (!myTimerFlagValid && timerClocks() < 0)
      {
         myInterruptFlag |= TimerBit;
         myTimerFlagValid = true;
      }
      uInt8 result = myInterruptFlag;
      myInterruptFlag &= ~PA7Bit;
      return result;
   }

   case 0x00: 
   {
      uInt8 value = (myConsole.controller(Controller::Left).read() << 4) |
         myConsole.controller(Controller::Right).read();
      return (myOutA | ~myDDRA) & value;
   }

   case 0x02: 
   {
      return (myOutB | ~myDDRB) & (myConsole.switches().read() | myDDRB);
   }

   case 0x01:  
      return myDDRA;

   case 0x03: 
      return myDDRB;

   default:
      return 0;
   }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::pokeSpecial(uInt16 addr, uInt8 value)
{
  if((addr & 0x04) != 0)
  {
    if((addr & 0x10) != 0)
      setTimerRegister(value, addr & 0x03);
    else
      myEdgeDetectPositive = addr & 0x01;
  }
  else
  {
    switch(addr & 0x03)
    {
      case 0:     // SWCHA - Port A I/O Register (Joystick)
      {
        myOutA = value;
        setPinState(true);
        break;
      }

      case 1:     // SWACNT - Port A Data Direction Register 
      {
        myDDRA = value;
        setPinState(false);
        break;
      }

      case 2:     // SWCHB - Port B I/O Register (Console switches)
      {
        myOutB = value;
        break;
      }

      case 3:     // SWBCNT - Port B Data Direction Register 
      {
        myDDRB = value;
        break;
      }
    }
  }
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::save(Serializer& out) const
{
    out.putString(name());

    out.putByteArray(myRAM, 128);

    out.putInt(myTimer);
    out.putInt(myIntervalShift);
    out.putInt(myCyclesWhenTimerSet);

    out.putByte(myDDRA);
    out.putByte(myDDRB);
    out.putByte(myOutA);
    out.putByte(myOutB);

    out.putByte(myInterruptFlag);
    out.putBool(myTimerFlagValid);
    out.putBool(myEdgeDetectPositive);
    out.putByteArray(myOutTimer, 4);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::load(Serializer& in)
{
    if(in.getString() != name())
      return false;

    in.getByteArray(myRAM, 128);

    myTimer = in.getInt();
    myIntervalShift = in.getInt();
    myCyclesWhenTimerSet = in.getInt();

    myDDRA = in.getByte();
    myDDRB = in.getByte();
    myOutA = in.getByte();
    myOutB = in.getByte();

    myInterruptFlag = in.getByte();
    myTimerFlagValid = in.getBool();
    myEdgeDetectPositive = in.getBool();
    in.getByteArray(myOutTimer, 4);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::intim() const
{
  // This method is documented in ::peek(0x284), and exists so that the
  // debugger can read INTIM without changing the state of the system

  // Get number of clocks since timer was set
  Int32 timer = timerClocks();  
  if(!(timer & 0x40000))
    return (timer >> myIntervalShift) & 0xff;
  else
    return timer & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::timint() const
{
  // This method is documented in ::peek(0x285), and exists so that the
  // debugger can read TIMINT without changing the state of the system

  // Update timer flag if it is invalid and timer has expired
  uInt8 interrupt = myInterruptFlag;
  if(timerClocks() < 0)
    interrupt |= TimerBit;

  return interrupt;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 M6532::intimClocks() const
{
  // This method is similar to intim(), except instead of giving the actual
  // INTIM value, it will give the current number of clocks between one
  // INTIM value and the next

  // Get number of clocks since timer was set
  Int32 timer = timerClocks();  
  if(!(timer & 0x40000))
    return timerClocks() & ((1 << myIntervalShift) - 1);
  else
    return timer & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const M6532& c)
  : myConsole(c.myConsole),
    mySettings(c.mySettings)
{
  //assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532& M6532::operator = (const M6532&)
{
  //assert(false);
  return *this;
}
