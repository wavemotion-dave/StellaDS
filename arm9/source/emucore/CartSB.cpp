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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
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

#include <assert.h>
#include <iostream>
#include "CartSB.hxx"
#include "Random.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeSB::CartridgeSB(const uInt8* image, uInt32 size)
{
  // Copy the ROM image into my buffer - just reuse the existing buffer
  myImage = (uInt8 *)image;
    
  myRomBankCount = (size / 4096) + ((size % 4096) ? 1:0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeSB::~CartridgeSB()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeSB::name() const
{
  return "SB";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::reset()
{
  sbLastBank = 999;
  // Upon reset we switch to the last bank
  bank(myRomBankCount-1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
    
  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0800 >> shift);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0900 >> shift);
  myHotSpotPageAccess[2] = mySystem->getPageAccess(0x0A00 >> shift);
  myHotSpotPageAccess[3] = mySystem->getPageAccess(0x0B00 >> shift);
  myHotSpotPageAccess[4] = mySystem->getPageAccess(0x0C00 >> shift);
  myHotSpotPageAccess[5] = mySystem->getPageAccess(0x0D00 >> shift);
  myHotSpotPageAccess[6] = mySystem->getPageAccess(0x0E00 >> shift);
  myHotSpotPageAccess[7] = mySystem->getPageAccess(0x0F00 >> shift);

  page_access.device = this;
  page_access.directPeekBase = 0;
  page_access.directPokeBase = 0;
  
  // Set the page accessing methods for the hot spots
  for(uInt32 j = 0x800; j < 0x1000; j += (1 << shift))
  {
    mySystem->setPageAccess(j >> shift, page_access);
  }
  
  sbLastBank = 999;
  // Install pages for the last bank
  bank(myRomBankCount-1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeSB::checkSwitchBank(uInt16 address)
{
  // Switch banks if necessary
  if((address & 0x1800) == 0x0800)
  {
      bank(address & (myRomBankCount-1));
      return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeSB::peek(uInt16 address)
{
  address &= 0x1FFF;

  checkSwitchBank(address);

  if(!(address & 0x1000))
  {
    // Because of the way we've set up accessing above, we can only
    // get here when the addresses are from 0x800 - 0xFFF
    const int hotspot = ((address & 0x0F00) >> 8) - 8;
    return myHotSpotPageAccess[hotspot].device->peek(address);    
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::poke(uInt16 address, uInt8 value)
{
  address &= 0x1FFF;

  checkSwitchBank(address);

  if(!(address & 0x1000))
  {
    // Because of the way we've set up accessing above, we can only
    // get here when the addresses are from 0x800 - 0xFFF
    const int hotspot = ((address & 0x0F00) >> 8) - 8;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeSB::bank(uInt16 bank)
{ 
  // Remember what bank we're in
  myCurrentOffset32 = bank * 4096;

  if (bank != sbLastBank)
  {
      // Setup the page access methods for the current bank
      uInt32 access_num = 0x1000 >> MY_PAGE_SHIFT;

      // Map ROM image into the system
      for(uInt32 address = 0x1000; address < 0x2000; address += (1 << MY_PAGE_SHIFT))
      {
          page_access.directPeekBase = &myImage[myCurrentOffset32 + (address & 0xFFF)];
          mySystem->setPageAccess(access_num++, page_access);
      }
      sbLastBank = bank;
  }
}
