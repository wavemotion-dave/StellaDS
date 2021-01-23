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
// $Id: CartAR.cxx,v 1.6 2005/02/13 19:17:02 stephena Exp $
//============================================================================
#include <nds.h>
#include <assert.h>
#include <string.h>
#include "CartAR.hxx"
#include "M6502Low.hxx"
#include "Random.hxx"
#include "System.hxx"
#include <iostream>

extern uInt32 NumberOfDistinctAccesses;
extern uInt8 fast_cart_buffer[];


uInt8 myWriteEnabled __attribute__((section(".dtcm")));
uInt8 myDataHoldRegister __attribute__((section(".dtcm")));
uInt32 myNumberOfDistinctAccesses __attribute__((section(".dtcm")));
uInt8 myWritePending __attribute__((section(".dtcm")));
uInt8 bPossibleLoad __attribute__((section(".dtcm")));    

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeAR::CartridgeAR(const uInt8* image, uInt32 size)
{
  // Create a load image buffer and copy the given image
  myLoadImages = new uInt8[size];
  myNumberOfLoadImages = size / 8448;
  memcpy(myLoadImages, image, size);

  myImage = (uInt8*)fast_cart_buffer; // Set this to the fast internal RAM... that buffer is otherwise unused at this point...
  myImage0 = myImage;
  myImage1 = myImage - 2048;
  bPossibleLoad=1;
        
  // Initialize SC BIOS ROM
  initializeROM(0);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeAR::~CartridgeAR()
{
  delete[] myLoadImages;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeAR::name() const
{
  return "CartridgeAR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::reset()
{
  // Initialize RAM to Zeros
  memset(myImage, 0x00, 6*1024);

  myPower = true;
  myWriteEnabled = false;

  myDataHoldRegister = 0;
  myNumberOfDistinctAccesses = 5;
  myWritePending = false;

  // Set bank configuration upon reset so ROM is selected and powered up
  bankConfiguration(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::systemCyclesReset()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1000 & mask) == 0);

  System::PageAccess access;
  for(uInt32 i = 0x1000; i < 0x2000; i += (1 << shift))
  {
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    access.device = this;
    mySystem->setPageAccess(i >> shift, access);
  }

  bankConfiguration(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeAR::peek(uInt16 addr)
{
  addr &= 0x0FFF;     // Map down to 4k...
  if (addr & 0x0800)  // If we are in the upper bank...
  {
      if (bPossibleLoad)
      {
          // Is the "dummy" SC BIOS hotspot for reading a load being accessed?
          if(addr == 0x0850)
          {
            // Get load that's being accessed (BIOS places load number at 0x80)
            uInt8 load = mySystem->peek(0x0080);

            // Read the specified load into RAM
            loadIntoRAM(load);

            return myImage1[addr];
          }
      }

      // Cancel any pending write if more than 5 distinct accesses have occurred
      if(myWritePending &&  (NumberOfDistinctAccesses > myNumberOfDistinctAccesses))
      {
        myWritePending = false;
      }

      // Is the bank configuration hotspot being accessed?
      if((addr) == 0x0FF8)
      {
        // Yes, so handle bank configuration
        myWritePending = false;
        bankConfiguration(myDataHoldRegister);
      }
      // Handle poke if writing enabled
      else if (myWritePending)
      {
        if (myWriteEnabled)
        {
            if ((NumberOfDistinctAccesses == (myNumberOfDistinctAccesses)))
            {
                if(!bPossibleLoad)    // Can't poke to ROM :-)
                   myImage1[addr] = myDataHoldRegister;
                myWritePending = false;
            }
        }
      }

      return myImage1[addr];
  }
  else // In lower bank
  {
      // Cancel any pending write if more than 5 distinct accesses have occurred
      if(myWritePending &&  (NumberOfDistinctAccesses > myNumberOfDistinctAccesses))
      {
        myWritePending = false;
      }

      // Is the data hold register being set?
      if(!(addr & 0x0F00) && (!myWriteEnabled || !myWritePending))
      {
        myDataHoldRegister = addr;
        myNumberOfDistinctAccesses = NumberOfDistinctAccesses+5;
        myWritePending = true;
      }
      // Handle poke if writing enabled
      else if (myWritePending)
      {
        if (myWriteEnabled)
        {
            if (NumberOfDistinctAccesses == myNumberOfDistinctAccesses)
            {
                myImage0[addr] = myDataHoldRegister;
                myWritePending = false;
            }
        }
      }

     return myImage0[addr];
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::poke(uInt16 addr, uInt8)
{
  addr &= 0x0FFF; // Map down to 4k...
  
  // Cancel any pending write if more than 5 distinct accesses have occurred
  if(myWritePending &&  (NumberOfDistinctAccesses > myNumberOfDistinctAccesses))
  {
    myWritePending = false;
  }

  // Is the data hold register being set?
  if(!(addr & 0x0F00) && (!myWriteEnabled || !myWritePending))
  {
    myDataHoldRegister = addr;
    myNumberOfDistinctAccesses = NumberOfDistinctAccesses+5;
    myWritePending = true;
  }
  // Is the bank configuration hotspot being accessed?
  else if (addr == 0x0FF8)
  {
    // Yes, so handle bank configuration
    myWritePending = false;
    bankConfiguration(myDataHoldRegister);
  }
  // Handle poke if writing enabled
  else if(myWriteEnabled && myWritePending &&  (NumberOfDistinctAccesses == (myNumberOfDistinctAccesses)))
  {
    if((addr & 0x0800) == 0)
        myImage0[addr] = myDataHoldRegister;
    else if(!bPossibleLoad)    // Can't poke to ROM :-)
      myImage1[addr] = myDataHoldRegister;
    myWritePending = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::bankConfiguration(uInt8 configuration)
{
  // D7-D5 of this byte: Write Pulse Delay (n/a for emulator)
  //
  // D4-D0: RAM/ROM configuration:
  //       $F000-F7FF    $F800-FFFF Address range that banks map into
  //  000wp     2            ROM
  //  001wp     0            ROM
  //  010wp     2            0      as used in Commie Mutants and many others
  //  011wp     0            2      as used in Suicide Mission
  //  100wp     2            ROM
  //  101wp     1            ROM
  //  110wp     2            1      as used in Killer Satellites
  //  111wp     1            2      as we use for 2k/4k ROM cloning
  // 
  //  w = Write Enable (1 = enabled; accesses to $F000-$F0FF cause writes
  //    to happen.  0 = disabled, and the cart acts like ROM.)
  //  p = ROM Power (0 = enabled, 1 = off.)  Only power the ROM if you're
  //    wanting to access the ROM for multiloads.  Otherwise set to 1.

  myCurrentBank = configuration & 0x1f; // remember for the bank() method

  myWriteEnabled = configuration & 0x02;

  switch((configuration >> 2) & 0x07)
  {
    case 0:
    {
      bPossibleLoad = 1;
      myImage0 = myImage + (2 * 2048);
      myImage1 = myImage + (3 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 1:
    {
      bPossibleLoad = 1;
      myImage0 = myImage + (0 * 2048);
      myImage1 = myImage + (3 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 2:
    {
      bPossibleLoad = 0;
      myImage0 = myImage + (2 * 2048);
      myImage1 = myImage + (0 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 3:
    {
      bPossibleLoad = 0;
      myImage0 = myImage + (0 * 2048);
      myImage1 = myImage + (2 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 4:
    {
      bPossibleLoad = 1;
      myImage0 = myImage + (2 * 2048);
      myImage1 = myImage + (3 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 5:
    {
      bPossibleLoad = 1;
      myImage0 = myImage + (1 * 2048);
      myImage1 = myImage + (3 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 6:
    {
      bPossibleLoad = 0;
      myImage0 = myImage + (2 * 2048);
      myImage1 = myImage + (1 * 2048);
      myImage1 -= 2048;
      break;
    }

    case 7:
    {
      bPossibleLoad = 0;
      myImage0 = myImage + (1 * 2048);
      myImage1 = myImage + (2 * 2048);
      myImage1 -= 2048;
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::initializeROM(bool fastbios)
{
  static uInt8 dummyROMCode[] = {
    0xa5, 0xfa, 0x85, 0x80, 0x4c, 0x18, 0xf8, 0xff, 
    0xff, 0xff, 0x78, 0xd8, 0xa0, 0x00, 0xa2, 0x00, 
    0x94, 0x00, 0xe8, 0xd0, 0xfb, 0x4c, 0x50, 0xf8, 
    0xa2, 0x00, 0xbd, 0x06, 0xf0, 0xad, 0xf8, 0xff, 
    0xa2, 0x00, 0xad, 0x00, 0xf0, 0xea, 0xbd, 0x00, 
    0xf7, 0xca, 0xd0, 0xf6, 0x4c, 0x50, 0xf8, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xa2, 0x03, 0xbc, 0x1d, 0xf9, 0x94, 0xfa, 0xca, 
    0x10, 0xf8, 0xa0, 0x00, 0xa2, 0x28, 0x94, 0x04, 
    0xca, 0x10, 0xfb, 0xa2, 0x1c, 0x94, 0x81, 0xca, 
    0x10, 0xfb, 0xa9, 0x00, 0x85, 0x1b, 0x85, 0x1c, 
    0x85, 0x1d, 0x85, 0x1e, 0x85, 0x1f, 0x85, 0x19, 
    0x85, 0x1a, 0x85, 0x08, 0x85, 0x01, 0xa9, 0x10, 
    0x85, 0x21, 0x85, 0x02, 0xa2, 0x07, 0xca, 0xca, 
    0xd0, 0xfd, 0xa9, 0x00, 0x85, 0x20, 0x85, 0x10, 
    0x85, 0x11, 0x85, 0x02, 0x85, 0x2a, 0xa9, 0x05, 
    0x85, 0x0a, 0xa9, 0xff, 0x85, 0x0d, 0x85, 0x0e, 
    0x85, 0x0f, 0x85, 0x84, 0x85, 0x85, 0xa9, 0xf0, 
    0x85, 0x83, 0xa9, 0x74, 0x85, 0x09, 0xa9, 0x0c, 
    0x85, 0x15, 0xa9, 0x1f, 0x85, 0x17, 0x85, 0x82, 
    0xa9, 0x07, 0x85, 0x19, 0xa2, 0x08, 0xa0, 0x00, 
    0x85, 0x02, 0x88, 0xd0, 0xfb, 0x85, 0x02, 0x85, 
    0x02, 0xa9, 0x02, 0x85, 0x02, 0x85, 0x00, 0x85, 
    0x02, 0x85, 0x02, 0x85, 0x02, 0xa9, 0x00, 0x85, 
    0x00, 0xca, 0x10, 0xe4, 0x06, 0x83, 0x66, 0x84, 
    0x26, 0x85, 0xa5, 0x83, 0x85, 0x0d, 0xa5, 0x84, 
    0x85, 0x0e, 0xa5, 0x85, 0x85, 0x0f, 0xa6, 0x82, 
    0xca, 0x86, 0x82, 0x86, 0x17, 0xe0, 0x0a, 0xd0, 
    0xc3, 0xa9, 0x02, 0x85, 0x01, 0xa2, 0x1c, 0xa0, 
    0x00, 0x84, 0x19, 0x84, 0x09, 0x94, 0x81, 0xca, 
    0x10, 0xfb, 0xa6, 0x80, 0xdd, 0x00, 0xf0, 0xa5, 
    0x80, 0x45, 0xfe, 0x45, 0xff, 0xa2, 0xff, 0xa0, 
    0x00, 0x9a, 0x4c, 0xfa, 0x00, 0xcd, 0xf8, 0xff, 0x4c
  };

  // If fastbios is enabled, set the wait time between vertical bars
  // to 0 (default is 8), which is stored at address 189 of the bios
  if(fastbios)
    dummyROMCode[189] = 0x0;

  uInt32 size = sizeof(dummyROMCode);

  // Initialize ROM with illegal 6502 opcode that causes a real 6502 to jam
  for(uInt32 i = 0; i < 2048; ++i)
  {
    myImage[3 * 2048 + i] = 0x02; 
  }

  // Copy the "dummy" Supercharger BIOS code into the ROM area
  for(uInt32 j = 0; j < size; ++j)
  {
    myImage[3 * 2048 + j] = dummyROMCode[j];
  }

  // Finally set 6502 vectors to point to initial load code at 0xF80A of BIOS
  myImage[3 * 2048 + 2044] = 0x0A;
  myImage[3 * 2048 + 2045] = 0xF8;
  myImage[3 * 2048 + 2046] = 0x0A;
  myImage[3 * 2048 + 2047] = 0xF8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeAR::checksum(uInt8* s, uInt16 length)
{
  uInt8 sum = 0;

  for(uInt32 i = 0; i < length; ++i)
  {
    sum += s[i];
  }

  return sum;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeAR::loadIntoRAM(uInt8 load)
{
  uInt16 image;

  // Scan through all of the loads to see if we find the one we're looking for
  for(image = 0; image < myNumberOfLoadImages; ++image)
  {
    // Is this the correct load?
    if(myLoadImages[(image * 8448) + 8192 + 5] == load)
    {
      // Copy the load's header
        uInt32 *dest = (uInt32 *) (myHeader);
        uInt32 *src2 = (uInt32 *) (myLoadImages + (image * 8448) + 8192);
        for (int i=0; i<64; i++)
        {
           *dest++ = *src2++;   
        }     

      // Load all of the pages from the load
      for(uInt32 j = 0; j < myHeader[3]; ++j)
      {
        uInt32 bank = myHeader[16 + j] & 0x03;
        uInt32 page = (myHeader[16 + j] >> 2) & 0x07;
        uInt8* src = myLoadImages + (image * 8448) + (j * 256);
        // Copy page to Supercharger RAM (don't allow a copy into ROM area)
        if(bank < 3)
        {
            uInt32 *dest = (uInt32 *) (myImage + (bank * 2048) + (page * 256));
            uInt32 *src2 = (uInt32 *) src;
            for (int i=0; i<64; i++)
            {
               *dest++ = *src2++;   
            }
        }
      }

      // Copy the bank switching byte and starting address into the 2600's
      // RAM for the "dummy" SC BIOS to access it
      mySystem->poke(0xfe, myHeader[0]);
      mySystem->poke(0xff, myHeader[1]);
      mySystem->poke(0x80, myHeader[2]);

      return;
    }
  }
}



