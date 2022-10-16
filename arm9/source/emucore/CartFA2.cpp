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
#include <string.h>
#include "CartFA2.hxx"
#include "Random.hxx"
#include "System.hxx"
#include "../StellaDS.h"
#include <iostream>

extern char my_filename[MAX_FILE_NAME_LEN+1];
char flash_filename[MAX_FILE_NAME_LEN+5];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA2::CartridgeFA2(const uInt8* image, uInt32 size)
{
  // Copy the ROM image into my buffer - just reuse the existing buffer
  if (size <= (28 * 1024))
  {
    myImage = (uInt8 *)image;
  }
  else
  {
      myImage = (uInt8 *) (image + 1024);  // Offset past ARM junk in first 1K block for 29K/32K versions of this...
  }

  // Initialize RAM with random values
  Random random;
  for(uInt32 i = 0; i < 256; ++i)
  {
    myRAM[i] = random.next();
  }
    
  // Save the Flash backing filename for the extra RAM
  strncpy(flash_filename,my_filename, MAX_FILE_NAME_LEN);
  flash_filename[MAX_FILE_NAME_LEN] = 0;
  strcat(flash_filename, ".fla");
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA2::~CartridgeFA2()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeFA2::name() const
{
  return "CartridgeFA2";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::reset()
{
  // Upon reset we switch to bank 0
  myCurrentBank = 99;
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

    
  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1100 & mask) == 0) && ((0x1200 & mask) == 0));

  // Set the page accessing methods for the hot spots
  for(uInt32 i = (0x1FF8 & ~mask); i < 0x2000; i += (1 << shift))
  {
    page_access.directPeekBase = 0;
    page_access.directPokeBase = 0;
    page_access.device = this;
    mySystem->setPageAccess(i >> shift, page_access);
  }

  // Set the page accessing method for the RAM writing pages
  for(uInt32 j = 0x1000; j < 0x1100; j += (1 << shift))
  {
    page_access.device = this;
    page_access.directPeekBase = 0;
    page_access.directPokeBase = &myRAM[j & 0x00FF];
    mySystem->setPageAccess(j >> shift, page_access);
  }
 
  // Set the page accessing method for the RAM reading pages
  for(uInt32 k = 0x1100; k < 0x1200; k += (1 << shift))
  {
    page_access.device = this;
    page_access.directPeekBase = &myRAM[k & 0x00FF];
    page_access.directPokeBase = 0;
    mySystem->setPageAccess(k >> shift, page_access);
  }

  // Install pages for bank 0
  myCurrentBank = 99;
  bank(0);
}

void CartridgeFA2::handle_fa2_flash_backing(void)
{

  if (myRAM[255] == 1) // 1=Read Request
  {
      FILE *fp = fopen(flash_filename, "rb");
      if (fp == NULL)
      {
          memset(myRAM, 0x00, sizeof(myRAM));
      }
      else
      {
          fread(myRAM, sizeof(myRAM), 1, fp);
      }
      fclose(fp);
  }
  else if (myRAM[255] == 2)  // 2=Write Request
  {
      FILE *fp = fopen(flash_filename, "wb+");
      fwrite(myRAM, sizeof(myRAM), 1, fp);
      fclose(fp);
  }
    
  myRAM[255] = 0;   // Indicate success
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFA2::peek(uInt16 address)
{
  address = address & 0x0FFF;

  // Switch banks if necessary
  if (address >= 0x0FF5 && address <= 0x0FFB)
  {
      bank(address - 0x0FF5);
  }
  else if (address == 0x0FF4)   // Read or Write the 256 bytes of extra RAM to flash
  {
      handle_fa2_flash_backing();
      return 0x00;
  }
    
  // NOTE: This does not handle accessing RAM, however, this function
  // should never be called for RAM because of the way page accessing
  // has been setup
  return myImage[(myCurrentBank * 4096) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::poke(uInt16 address, uInt8)
{
  address = address & 0x0FFF;

  // Switch banks if necessary
  if (address >= 0x0FF5 && address <= 0x0FFB)
  {
      bank(address - 0x0FF5);
  }
  else if (address == 0x0FF4)   // Read or Write the 256 bytes of extra RAM to flash
  {
      handle_fa2_flash_backing();
  }

  // NOTE: This does not handle accessing RAM, however, this function
  // should never be called for RAM because of the way page accessing
  // has been setup
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2::bank(uInt16 bank)
{
  // Remember what bank we're in
  if (myCurrentBank != bank)
  {
      myCurrentBank = bank;
      uInt16 offset = myCurrentBank * 4096;

      // Setup the page access methods for the current bank
      page_access.device = this;
      page_access.directPokeBase = 0;

      // Setup the page access methods for the current bank
      uInt32 access_num = 0x1200 >> MY_PAGE_SHIFT;

      // Map ROM image into the system
      for(uInt32 address = 0x1200; address < 0x1F80; address += (1 << MY_PAGE_SHIFT))
      {
          page_access.directPeekBase = &myImage[offset + (address & 0xFFF)];
          mySystem->setPageAccess(access_num++, page_access);
      }
  }
}
