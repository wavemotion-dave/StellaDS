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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// This file has been modified by Dave Bernazzani (wavemotion-dave)
// for optimized execution on the DS/DSi platform. Please seek the
// official Stella source distribution which is far cleaner, newer,
// and better maintained.
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <nds.h>

#include <assert.h>
#include "Console.hxx"
#include "M6532.hxx"
#include "Random.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include <iostream>

// An amazing 128 bytes of RAM... plus 128 more for the Super Carts (SC)
uInt8 myRAM[256] __attribute__ ((aligned (16))) __attribute__((section(".dtcm")));

// Current value of my Timer
uInt32 myTimer __attribute__((section(".dtcm")));

// Log base 2 of the number of cycles in a timer interval
uInt8 myIntervalShift __attribute__((section(".dtcm")));

// Indicates the number of cycles when the timer was last set
Int32 myCyclesWhenTimerSet __attribute__((section(".dtcm")));

// Indicates when the timer was read after timer interrupt occured
Int32 myCyclesWhenInterruptReset __attribute__((section(".dtcm")));

// Indicates if a read from timer has taken place after interrupt occured
uInt8 myTimerReadAfterInterrupt __attribute__((section(".dtcm")));

// Data Direction Register for Port A
uInt8 myDDRA __attribute__((section(".dtcm")));

// Data Direction Register for Port B
uInt8 myDDRB __attribute__((section(".dtcm"))); 

// Data Out for Port A
uInt8 myOutA __attribute__((section(".dtcm")));

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const Console& console)
    : myConsole(console)
{
  // Randomize the 128 bytes of memory
  Random random;

  for(uInt32 t = 0; t < 128; ++t)
  {
      myRAM[t] = (myCartInfo.clearRAM ? 0x00:random.next());
  }

  // Initialize other data members
  reset();
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::~M6532()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* M6532::name() const
{
  return "M6532";
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::reset()
{
  Random random;

  myTimer = 25 + (random.next() % 75);
  myIntervalShift = 6;
  myCyclesWhenTimerSet = 0;
  myCyclesWhenInterruptReset = 0;
  myTimerReadAfterInterrupt = false;

  // Zero the I/O registers
  myDDRA = 0x00;
  myDDRB = 0x00;
  myOutA = 0x00;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::systemCyclesReset()
{
  // System cycles are being reset to zero so we need to adjust
  // the cycle count we remembered when the timer was last set
  myCyclesWhenTimerSet -= gSystemCycles;
  myCyclesWhenInterruptReset -= gSystemCycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system)
{
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1080 & mask) == 0);
  
  // All accesses are to this device
  PageAccess access;
  access.device = this;

  // We're installing in a 2600 system
  for(int address = 0; address < 8192; address += (1 << shift))
  {
    if((address & 0x1080) == 0x0080)
    {
      if((address & 0x0200) == 0x0000)
      {
        access.directPeekBase = &myRAM[address & 0x007f];
        access.directPokeBase = &myRAM[address & 0x007f];
        mySystem->setPageAccess(address >> shift, access);
      }
      else
      {
        access.directPeekBase = 0; 
        access.directPokeBase = 0;
        mySystem->setPageAccess(address >> shift, access);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE uInt8 M6532::peek(uInt16 addr)
{
  switch(addr & 0x07)
  {
    case 0x04:    // Timer Read
    case 0x06:
    {
      uInt32 delta = (gSystemCycles-1) - myCyclesWhenTimerSet;
      uInt32 delta_shift = delta >> myIntervalShift;        
      Int32 timer = (Int32)myTimer - (Int32)delta_shift - 1;

      // See if the timer has expired yet?
      if(timer >= 0)
      {
        return (uInt8)timer; 
      }
      else
      {
        timer = (Int32)(myTimer << myIntervalShift) - (Int32)delta - 1;

        if((timer <= -2) && !myTimerReadAfterInterrupt)
        {
          // Indicate that timer has been read after interrupt occured
          myTimerReadAfterInterrupt = true;
          myCyclesWhenInterruptReset = gSystemCycles;
        }

        if(myTimerReadAfterInterrupt)
        {
          Int32 offset = myCyclesWhenInterruptReset - 
              (myCyclesWhenTimerSet + (myTimer << myIntervalShift));

          timer = (Int32)myTimer - (Int32)(delta_shift) - offset;
        }
      }
      return (uInt8)timer;    
    }

    case 0x05:    // Interrupt Flag
    case 0x07:
    {
      uInt32 cycles = gSystemCycles - 1;
      uInt32 delta = cycles - myCyclesWhenTimerSet;
      Int32 timer = (Int32)myTimer - (Int32)(delta >> myIntervalShift) - 1;

      if((timer >= 0) || myTimerReadAfterInterrupt)
        return 0x00;
      else
        return 0x80;
    }
	  
	case 0x00:    // Port A I/O Register (Joystick)
    {
      uInt8 value = 0x00;

      if(myConsole.controller(Controller::Left).read(Controller::One))
        value |= 0x10;
      if(myConsole.controller(Controller::Left).read(Controller::Two))
        value |= 0x20;
      if(myConsole.controller(Controller::Left).read(Controller::Three))
        value |= 0x40;
      if(myConsole.controller(Controller::Left).read(Controller::Four))
        value |= 0x80;

      if(myConsole.controller(Controller::Right).read(Controller::One))
        value |= 0x01;
      if(myConsole.controller(Controller::Right).read(Controller::Two))
        value |= 0x02;
      if(myConsole.controller(Controller::Right).read(Controller::Three))
        value |= 0x04;
      if(myConsole.controller(Controller::Right).read(Controller::Four))
        value |= 0x08;
      return value;
    }

    case 0x01:    // Port A Data Direction Register 
    {
      return myDDRA;
    }

    case 0x02:    // Port B I/O Register (Console switches)
    {
      return myConsole.switches().read();
    }

    case 0x03:    // Port B Data Direction Register
    {
      return myDDRB;
    }

    default:
    {    
      return 0x00;
    }
  }
}

void M6532::setPinState()
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
    uInt8 a = myOutA | ~myDDRA;

    myConsole.controller(Controller::Left).write(Controller::One,   a & 0x10);
    myConsole.controller(Controller::Left).write(Controller::Two,   a & 0x20);
    myConsole.controller(Controller::Left).write(Controller::Three, a & 0x40);
    myConsole.controller(Controller::Left).write(Controller::Four,  a & 0x80);
    
    myConsole.controller(Controller::Right).write(Controller::One,   a & 0x01);
    myConsole.controller(Controller::Right).write(Controller::Two,   a & 0x02);
    myConsole.controller(Controller::Right).write(Controller::Three, a & 0x04);
    myConsole.controller(Controller::Right).write(Controller::Four,  a & 0x08);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void M6532::poke(uInt16 addr, uInt8 value)
{
  if((addr & 0x07) == 0x00)         // Port A I/O Register (Joystick)
  {
    myOutA = value;
    setPinState();
  }
  else if((addr & 0x07) == 0x01)    // Port A Data Direction Register 
  {
    myDDRA = value;
    setPinState();
  }
  else if((addr & 0x07) == 0x02)    // Port B I/O Register (Console switches)
  {
    return;
  }
  else if((addr & 0x07) == 0x03)    // Port B Data Direction Register
  {
    // Fixed Input - can't change it...
    return;
  }
  else if((addr & 0x17) == 0x14)    // Write timer divide by 1 
  {
    myTimer = value;
    myIntervalShift = 0;
    myCyclesWhenTimerSet = gSystemCycles;
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x17) == 0x15)    // Write timer divide by 8
  {
    myTimer = value;
    myIntervalShift = 3;
    myCyclesWhenTimerSet = gSystemCycles;
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x17) == 0x16)    // Write timer divide by 64
  {
    myTimer = value;
    myIntervalShift = 6;
    myCyclesWhenTimerSet = gSystemCycles;
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x17) == 0x17)    // Write timer divide by 1024
  {
    myTimer = value;
    myIntervalShift = 10;
    myCyclesWhenTimerSet = gSystemCycles;
    myTimerReadAfterInterrupt = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const M6532& c)
    : myConsole(c.myConsole)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532& M6532::operator = (const M6532&)
{
  assert(false);

  return *this;
}
 
