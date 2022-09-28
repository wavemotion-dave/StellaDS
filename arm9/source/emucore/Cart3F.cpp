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
#include <assert.h>
#include "Cart3F.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include <iostream>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3F::Cartridge3F(const uInt8* image, uInt32 size)
    : mySize(size)
{
  // Copy the ROM image into my buffer - just reuse the existing buffer
  myImage = (uInt8 *)image;

  // Copy the ROM image into my buffer
  for(uInt32 addr = 0; addr < mySize; ++addr)
  {
    myImage[addr] = image[addr];
  }
        
  myBankMod = mySize / 2048;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3F::~Cartridge3F()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* Cartridge3F::name() const
{
  return "Cartridge3F";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::reset()
{
  // We'll map bank 0 into the first segment upon reset
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1800 & mask) == 0);

  // We're going to map the first 80 hex bytes here... we will chain call into TIA as needed
  for(uInt32 i = 0x00; i < 0x80; i += (1 << shift))
  {
    page_access.directPeekBase = 0;
    page_access.directPokeBase = 0;
    page_access.device = this;
    mySystem->setPageAccess(i >> shift, page_access);
  }

  // Setup the second segment to always point to the last ROM slice
  for(uInt32 j = 0x1800; j < 0x2000; j += (1 << shift))
  {
    page_access.device = this;
    page_access.directPeekBase = &myImage[(mySize - 2048) + (j & 0x07FF)];
    page_access.directPokeBase = 0;
    mySystem->setPageAccess(j >> shift, page_access);
  }
    
  // Leave these zero for faster bank switch
  page_access.directPeekBase = 0;
  page_access.directPokeBase = 0;

  // Install pages for bank 0 into the first segment
  bank(0);
}

extern TIA* theTIA; // 3F games are weird... they take over 40h bytes of the lower 80h so we need to chain to the real peek addresses...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3F::peek(uInt16 address)
{
  if ((address&0xFFF) < 0x80)
  {
      return theTIA->peek(address);
  }
  
  if((address&0x0FFF) < 0x0800)
  {
    return myImage[(address & 0x07FF) + myCurrentBank * 2048];
  }
  else
  {
    return myImage[(address & 0x07FF) + mySize - 2048];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::poke(uInt16 address, uInt8 value)
{
  address = address & 0x0FFF;

  // Switch banks if necessary
  if(address <= 0x003F)
  {
    bank(value);
  }
  else theTIA->poke(address, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3F::bank(uInt16 bank)
{ 
  myCurrentBank = bank % myBankMod;
  uInt32 offset = myCurrentBank * 2048;
  uInt16 shift = mySystem->pageShift();

  // Map ROM image into the system
  for(uInt32 address = 0x1000; address < 0x1800; address += (1 << shift))
  {
    page_access.directPeekBase = &myImage[offset + (address & 0x07FF)];
    mySystem->setPageAccess(address >> shift, page_access);
  }
}
