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
#include <iostream>
#include "CartDPC.hxx"
#include "System.hxx"

// The 2K display ROM image of the cartridge
uInt8 myDisplayImage[2048];

// The top registers for the data fetchers
extern uInt8 myTops[8];

// The bottom registers for the data fetchers
extern uInt8 myBottoms[8];

extern uInt16 myCounters[8];

// The flag registers for the data fetchers
uInt8 myFlags[8] __attribute__((section(".dtcm")));

// The random number generator register
uInt8 myRandomNumber __attribute__((section(".dtcm")));

// The music mode DF5, DF6, & DF7 enabled flags
bool myMusicMode[3] __attribute__((section(".dtcm")));

// System cycle count when the last update to music data fetchers occurred
uInt32 mySystemCycles __attribute__((section(".dtcm"))) = 0; 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPC::CartridgeDPC(const uInt8* image, uInt32 size)
{
  uInt32 addr;

  // Copy the program ROM image into my buffer
  for(addr = 0; addr < 8192; ++addr)
  {
    fast_cart_buffer[addr] = image[addr];
  }

  // Copy the display ROM image into my buffer
  for(addr = 0; addr < 2048; ++addr)
  {
    myDisplayImage[addr] = (uInt16)image[8192+2047 - addr]; // Yes, load it in backwards - it helps with speed when indexing later...
  }

  // Initialize the DPC data fetcher registers
  for(uInt16 i = 0; i < 8; ++i)
  {
    myTops[i] = myBottoms[i] = myCounters[i] = myFlags[i] = 0;
  }
    
  // None of the data fetchers are in music mode
  myMusicMode[0] = myMusicMode[1] = myMusicMode[2] = false;

  // Initialize the DPC's random number generator register (must be non-zero)
  myRandomNumber = 1;

  // Initialize the system cycles counter & fractional clock values
  mySystemCycles = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPC::~CartridgeDPC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeDPC::name() const
{
  return "CartridgeDPC";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::reset()
{
  // Update cycles to the current system cycles
  mySystemCycles = mySystem->cycles();

  // Upon reset we switch to bank 1
  myCurrentOffset = 4096;
  bank(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust the cycle counter so that it reflects the new value
  mySystemCycles -= cycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1080 & mask) == 0) && ((0x1100 & mask) == 0));

  page_access.directPeekBase = 0;
  page_access.directPokeBase = 0;
  page_access.device = this;
  
  // Set the page accessing methods for the hot spots
  for(uInt32 i = (0x1FF8 & ~mask); i < 0x2000; i += (1 << shift))
  {
    mySystem->setPageAccess(i >> shift, page_access);
  }

  // Set the page accessing method for the DPC reading & writing pages
  for(uInt32 j = 0x1000; j < 0x1080; j += (1 << shift))
  {
    mySystem->setPageAccess(j >> shift, page_access);
  }

  // Install pages for bank 1
  myCurrentOffset = 4096;
  bank(1);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeDPC::bank(uInt16 bank)
{ 
  uInt32 access_num = 0x1080 >> MY_PAGE_SHIFT;
  // Map Program ROM image into the system
  for(uInt32 address = 0x0080; address < (0x0FF8U & ~MY_PAGE_MASK); address += (2 << MY_PAGE_SHIFT))
  {
    myPageAccessTable[access_num++].directPeekBase = &fast_cart_buffer[myCurrentOffset + address];
    myPageAccessTable[access_num++].directPeekBase = &fast_cart_buffer[myCurrentOffset + (address + 0x80)];
  }
}

#define DPC_MUSIC_PITCH 64

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::updateMusicModeDataFetchers(uInt32 delta)
{
  Int32 wholeClocks = delta / DPC_MUSIC_PITCH;

  // Let's update counters and flags of the music mode data fetchers
  for(int x = 5; x <= 7; ++x)
  {
    // Update only if the data fetcher is in music mode
    if(myMusicMode[x - 5])
    {
      Int32 top = myTops[x] + 1;
      Int32 newLow = (Int32)(myCounters[x] & 0x00ff);

      if(myTops[x] != 0)
      {
        newLow -= (wholeClocks % top);
        if(newLow < 0)
        {
          newLow += top;
        }
      }
      else
      {
        newLow = 0;
      }

      // Update flag register for this data fetcher
      if(newLow <= myBottoms[x])
      {
        myFlags[x] = 0x00;
      }
      else if(newLow <= myTops[x])
      {
        myFlags[x] = 0xff;
      }

      myCounters[x] = (myCounters[x] & 0x0700) | (uInt16)newLow;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE uInt8 CartridgeDPC::peek(uInt16 address)
{
  address = address & 0x0FFF;

  if(address < 0x0040)
  {
    uInt8 result = 0;

    // Get the index of the data fetcher that's being accessed
    uInt8 index = address & 0x07;
    uInt8 function = (address >> 3);

    // Update flag register for selected data fetcher
    if((myCounters[index] & 0x00ff) == myTops[index])
    {
      myFlags[index] = 0xff;
    }
    else if((myCounters[index] & 0x00ff) == myBottoms[index])
    {
      myFlags[index] = 0x00;
    }
    
    switch(function)
    {
      extern uInt32 gTotalAtariFrames;
      case 0x00:
        if (index < 4) result = (uInt8)gTotalAtariFrames; // Not really random but good enough to flash the eel in Pitfall II (it's only use)
        else    // No... it's a music fetcher
        {
          if (myCartInfo.soundQuality == SOUND_WAVE)
          {
              static const uInt8 musicAmplitudes[8] = {
                  0x00, 0x04, 0x05, 0x09, 0x06, 0x0a, 0x0b, 0x0f
              };

              uInt32 delta = gSystemCycles - mySystemCycles;
              if (delta >= DPC_MUSIC_PITCH)
              {              
                  // Update the music data fetchers (counter & flag)
                  updateMusicModeDataFetchers(delta);
                  mySystemCycles = gSystemCycles - (delta % DPC_MUSIC_PITCH);
              }

              uInt8 i = 0;
              if(myMusicMode[0] && myFlags[5])
              {
                i |= 0x01;
              }
              if(myMusicMode[1] && myFlags[6])
              {
                i |= 0x02;
              }
              if(myMusicMode[2] && myFlags[7])
              {
                i |= 0x04;
              }

              result = musicAmplitudes[i];
          }
          else
          {
              result = 0x00;
          }
        }
        break;
            
      // DFx display data read
      case 0x01:
      {
        result = myDisplayImage[myCounters[index]];
        break;
      }

      // DFx display data read AND'd w/flag
      case 0x02:
      {
        result = myDisplayImage[myCounters[index]] & myFlags[index];
        break;
      } 

      // DFx flag
      case 0x07:
      {
        result = myFlags[index];
        break;
      }
    }

    // Clock the selected data fetcher's counter if needed
    if((index < 5) || ((index >= 5) && (!myMusicMode[index - 5])))
    {
      myCounters[index] = (myCounters[index] - 1) & 0x07ff;
    }
      
    return result;
  }
  else
  {
    // Switch banks if necessary
    if (address == 0x0FF8) {  myCurrentOffset = 0; bank(0);}
    else if (address == 0x0FF9) {  myCurrentOffset = 4096;bank(1);}
    return fast_cart_buffer[myCurrentOffset + address];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::poke(uInt16 address, uInt8 value)
{
  address = address & 0x0FFF;

  if((address >= 0x0040) && (address < 0x0080))
  {
    // Get the index of the data fetcher that's being accessed
    uInt32 index = address & 0x07;    
    uInt32 function = (address >> 3) & 0x07;

    switch(function)
    {
      // DFx top count
      case 0x00:
      {
        myTops[index] = value;
        myFlags[index] = 0x00;
        break;
      }

      // DFx bottom count
      case 0x01:
      {
        myBottoms[index] = value;
        break;
      }

      // DFx counter low
      case 0x02:
      {
        if((index >= 5) && myMusicMode[index - 5])
        {
          // Data fecther is in music mode so its low counter value
          // should be loaded from the top register not the poked value
          myCounters[index] = (myCounters[index] & 0x0700) | (uInt16)myTops[index];
        }
        else
        {
            // Data fecther is either not a music mode data fecther or it
            // isn't in music mode so it's low counter value should be loaded
            // with the poked value
            myCounters[index] = (myCounters[index] & 0x0700) | (uInt16)value;
        }
        break;
      }

      // DFx counter high
      case 0x03:
      {
        myCounters[index] = (((uInt16)value & 0x07) << 8) | (myCounters[index] & 0x00ff);
        // Execute special code for music mode data fetchers
        if(index >= 5)
        {
          myMusicMode[index - 5] = (value & 0x10);
        }
          
        break;
      }
    } 
  }
  else
  {
    // Switch banks if necessary
    if (address == 0x0FF8) { myCurrentOffset = 0; bank(0);}
    else if (address == 0x0FF9) {  myCurrentOffset = 4096;bank(1);}
  }
}
