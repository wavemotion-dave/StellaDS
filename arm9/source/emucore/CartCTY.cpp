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
#include "CartCTY.hxx"
#include "Random.hxx"
#include "System.hxx"
#include "../StellaDS.h"
#include <iostream>

extern char my_filename[MAX_FILE_NAME_LEN+1];
extern char flash_filename[MAX_FILE_NAME_LEN+5];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCTY::CartridgeCTY(const uInt8* image, uInt32 size)
{
  // Copy the ROM image into my buffer - just reuse the existing buffer
  myImage = (uInt8 *)image;

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
CartridgeCTY::~CartridgeCTY()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeCTY::name() const
{
  return "CartridgeCTY";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::reset()
{
  // Upon reset we switch to bank 1
  myCurrentBank = 99;
  bank(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Set the page accessing methods for the hot spots
  for(uInt32 i = (0x1FF4 & ~mask); i < 0x2000; i += (1 << shift))
  {
    page_access.directPeekBase = 0;
    page_access.directPokeBase = 0;
    page_access.device = this;
    mySystem->setPageAccess(i >> shift, page_access);
  }

  // Set the page accessing method for the RAM read/writing pages
  for(uInt32 j = 0x1000; j < 0x1080; j += (1 << shift))
  {
    page_access.directPeekBase = 0;
    page_access.directPokeBase = 0;
    page_access.device = this;
    mySystem->setPageAccess(j >> shift, page_access);
  }
 
  // Install pages for bank 1 
  myCurrentBank = 99;
  bank(1);
}

void CartridgeCTY::handle_cty_flash_backing(void)
{
    u8 op = myRAM[0] & 0x0F;
    u8 index = (myRAM[0] >> 4) & 0x0F;
    switch (op)
    {
        case 2: // Load Score Table
        {
            FILE *fp = fopen(flash_filename, "rb");
            if (fp == NULL)
                memset(myEE, 0x00, sizeof(myEE));
            else
                fread(myEE, sizeof(myEE), 1, fp);
            
            // Grab 60B slice @ given index (first 4 bytes are ignored)
            memcpy(myRAM+4, myEE + (index << 6) + 4, 60);
            
            fclose(fp);
        }
        break;
        
        case 3: // Write Score Table
        {
            // Add 60B RAM to score table @ given index (first 4 bytes are ignored)
            memcpy(myEE + (index << 6) + 4, myRAM+4, 60);

            FILE *fp = fopen(flash_filename, "wb+");
            fwrite(myEE, sizeof(myEE), 1, fp);
            fclose(fp);
        }
        break;
            
        case 4: // Wipe Score Table
        {
            memset(myEE, 0x00, sizeof(myEE));
            memset(myRAM, 0x00, sizeof(myRAM));
            FILE *fp = fopen(flash_filename, "wb+");
            fwrite(myEE, sizeof(myEE), 1, fp);
            fclose(fp);
        }
        break;
    }
    
    myRAM[0] = 0;   // Mark this operation as GOOD
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCTY::peek(uInt16 address)
{
  address = address & 0x0FFF;

  if (address < 0x80)  // Are we reading the extra RAM?
  {
      return myRAM[address & 0x3F];
  }
  else if (address >= 0x0FF5 && address <= 0x0FFB)  // Switch banks if necessary
  {
      bank(address - 0x0FF4);
  }
  else if (address == 0x0FF4)   // Read or Write the 256 bytes of extra RAM to flash
  {
      handle_cty_flash_backing();
      return 0x00;
  }
  return myImage[(myCurrentBank * 4096) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::poke(uInt16 address, uInt8 value)
{
  address = address & 0x0FFF;

  if (address < 0x80)  // Are we reading the extra RAM?
  {
      myRAM[address & 0x3F] = value;
  }
  else if (address >= 0x0FF5 && address <= 0x0FFB) // Switch banks if necessary
  {
      bank(address - 0x0FF4);
  }
  else if (address == 0x0FF4)   // Read or Write the 256 bytes of extra RAM to flash
  {
      handle_cty_flash_backing();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCTY::bank(uInt16 bank)
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
      uInt32 access_num = 0x1080 >> MY_PAGE_SHIFT;

      // Map ROM image into the system
      for(uInt32 address = 0x1080; address < 0x1F80; address += (1 << MY_PAGE_SHIFT))
      {
          page_access.directPeekBase = &myImage[offset + (address & 0xFFF)];
          mySystem->setPageAccess(access_num++, page_access);
      }
  }
}
