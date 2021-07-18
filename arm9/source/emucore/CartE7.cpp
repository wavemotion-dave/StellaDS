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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartE7.cxx,v 1.5 2005/02/13 19:17:02 stephena Exp $
//============================================================================

#include <assert.h>
#include "CartE7.hxx"
#include "Random.hxx"
#include "System.hxx"
#include <iostream>

uInt16 myCurrentSlice __attribute__((section(".dtcm"))) = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7::CartridgeE7(const uInt8* image)
{
  // Copy the ROM image into my buffer
  for(uInt32 addr = 0; addr < 16384; ++addr)
  {
    myImage[addr] = image[addr];
  }

  myRAM = fast_cart_buffer;
    
  // Initialize RAM with random values
  Random random;
  for(uInt32 i = 0; i < 2048; ++i)
  {
    myRAM[i] = random.next();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7::~CartridgeE7()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeE7::name() const
{
  return "CartridgeE7";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7::reset()
{
  // Install some default banks for the RAM and first segment
  bankRAM(0);
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1400 & mask) == 0) && ((0x1800 & mask) == 0) &&
      ((0x1900 & mask) == 0) && ((0x1A00 & mask) == 0));

  // Set the page accessing methods for the hot spots
  for(uInt32 i = (0x1FE0 & ~mask); i < 0x2000; i += (1 << shift))
  {
    page_access.directPeekBase = 0;
    page_access.directPokeBase = 0;
    page_access.device = this;
    mySystem->setPageAccess(i >> shift, page_access);
  }

  // Setup the second segment to always point to the last ROM slice
  for(uInt32 j = 0x1A00; j < (0x1FE0U & ~mask); j += (1 << shift))
  {
    page_access.device = this;
    page_access.directPeekBase = &myImage[7 * 2048 + (j & 0x07FF)];
    page_access.directPokeBase = 0;
    mySystem->setPageAccess(j >> shift, page_access);
  }

  // Install some default banks for the RAM and first segment
  bankRAM(0);
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeE7::peek(uInt16 address)
{
  address = address & 0x0FFF;

  if (address >= 0x0FE0)
  {
      // Switch banks if necessary
      if(address <= 0x0FE7)
      {
        bank(address & 0x0007);
      }
      else if(address <= 0x0FEB)
      {
        bankRAM(address & 0x0003);
      }
      return myImage[(7 << 11) + (address & 0x07FF)];
  }
  else
  {
      // NOTE: The following does not handle reading from RAM, however,
      // this function should never be called for RAM because of the
      // way page accessing has been setup
      if (address & 0x0800)
          return myImage[(7 << 11) + (address & 0x07FF)];
      else
          return myImage[myCurrentSlice + (address & 0x07FF)];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7::poke(uInt16 address, uInt8)
{
  address = address & 0x0FFF;

  if (address >= 0x0FE0)
  {
      // Switch banks if necessary
      if (address <= 0x0FE7)
      {
        bank(address & 0x0007);
      }
      else if (address <= 0x0FEB)
      {
        bankRAM(address & 0x0003);
      }
  }
  // NOTE: This does not handle writing to RAM, however, this 
  // function should never be called for RAM because of the
  // way page accessing has been setup
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7::bank(uInt16 slice)
{ 
  // Remember what bank we're in
  myCurrentSlice = slice<<11;
  uInt32 access_num = 0x1000 >> MY_PAGE_SHIFT;

  // Setup the page access methods for the current bank
  if(slice != 7)
  {
    page_access.directPokeBase = 0;

    // Map ROM image into first segment
    for(uInt32 address = 0x0000; address < 0x0800; address += (1 << MY_PAGE_SHIFT))
    {
      page_access.directPeekBase = &myImage[myCurrentSlice + address];
      mySystem->setPageAccess(access_num++, page_access);
    }
  }
  else
  {
    // Set the page accessing method for the 1K slice of RAM writing pages
    page_access.directPeekBase = 0;
    for(uInt32 j = 0x0000; j < 0x0400; j += (1 << MY_PAGE_SHIFT))
    {
      page_access.directPokeBase = &myRAM[j];
      mySystem->setPageAccess(access_num++, page_access);
    }

    // Set the page accessing method for the 1K slice of RAM reading pages
    page_access.directPokeBase = 0;
    for(uInt32 k = 0x0000; k < 0x0400; k += (1 << MY_PAGE_SHIFT))
    {
      page_access.directPeekBase = &myRAM[k];
      mySystem->setPageAccess(access_num++, page_access);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7::bankRAM(uInt16 bank)
{ 
  // Remember what bank we're in
  uInt16 offset = 1024 + (bank << 8);

  // Set the page accessing method for the 256 bytes of RAM writing pages
  page_access.directPeekBase = 0;
  uInt32 access_num = 0x1800 >> MY_PAGE_SHIFT;
  for(uInt32 j = 0x0000; j < 0x0100; j += (1 << MY_PAGE_SHIFT))
  {
    page_access.directPokeBase = &myRAM[offset + j];
    mySystem->setPageAccess(access_num++, page_access);
  }

  // Set the page accessing method for the 256 bytes of RAM reading pages
  page_access.directPokeBase = 0;
  for(uInt32 k = 0x0000; k < 0x0100; k += (1 << MY_PAGE_SHIFT))
  {
    page_access.directPeekBase = &myRAM[offset + k];
    mySystem->setPageAccess(access_num++, page_access);
  }
}
