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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================
#include <nds.h>
#include <cassert>
#include <cstring>

#include "System.hxx"
#include "CartCDF.hxx"
#include "Thumbulator.hxx"

uInt32 myMusicWaveformSize[3];

#define FAST_FETCH_ON ((myMode & 0x0F) == 0)
#define DIGITAL_AUDIO_ON ((myMode & 0xF0) == 0)

static uInt16 myStartBank = 6;

#define MEM_2KB  (1024 * 2)
#define MEM_3KB  (1024 * 3)
#define MEM_4KB  (1024 * 4)
#define MEM_5KB  (1024 * 5)
#define MEM_24KB (1024 * 24)
#define MEM_28KB (1024 * 28)

extern Thumbulator *myThumbEmulator;
uInt8 myDataStreamFetch __attribute__((section(".dtcm"))) = 0x00;

// The counter registers for the data fetchers
uInt8* myDisplayImageCDF __attribute__((section(".dtcm")));  // Pointer to the 4K display ROM image of the cartridge

uInt32 fastDataStreamBase  __attribute__((section(".dtcm")));
uInt32 fastIncStreamBase   __attribute__((section(".dtcm")));
uInt32 myWaveformBase      __attribute__((section(".dtcm")));

CartridgeCDF *myCartCDF __attribute__((section(".dtcm")));

// System cycle count when the last update to music data fetchers occurred

uInt16 myAmplitudeStream                __attribute__((section(".dtcm"))) = 0x22;
uInt16 myDatastreamBase                 __attribute__((section(".dtcm"))) = 0x00a0;
uInt16 myDatastreamIncrementBase        __attribute__((section(".dtcm"))) = 0x0128;

// Pointer to the 28K program ROM image of the cartridge
extern uInt8 myDPC[];
extern uInt8 *myDPCptr;
extern Int32 myDPCPCycles;

extern uInt32 myMusicCounters[3];
extern uInt32 myMusicFrequencies[3];

// Controls mode, lower nybble sets Fast Fetch, upper nybble sets audio
// -0 = Fast Fetch ON
// -F = Fast Fetch OFF
// 0- = Packed Digital Sample
// F- = 3 Voice Music
uInt8 myMode = 0xFF;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::CartridgeCDF(const uInt8* image, uInt32 size)
{
  // Pointer to the program ROM (28K @ 4K offset)
  myDPCptr = (uInt8 *)image + MEM_4KB;
  memcpy(myDPC, myDPCptr, MEM_28KB);
        
  memset(fast_cart_buffer, 0, 8192);    // Clear all of the "ARM Thumb" RAM
    
  // Pointer to the display RAM
  myDisplayImageCDF = fast_cart_buffer + MEM_2KB;

  // Copy intial driver into the CDF Harmony RAM
  memcpy(fast_cart_buffer, image, MEM_2KB);
  
  for (int i=0; i < 3; ++i)
    myMusicWaveformSize[i] = 27;
    
  // DPC+ always starts in bank 6
  myStartBank = 6;
      
  uInt8 subversion = 0;

  for(uInt32 i = 0; i < 2048; i += 4)
  {
    // CDF signature occurs 3 times in a row, i+3 (+7 or +11) is version
    if (    image[i+0] == 0x43 && image[i + 4] == 0x43 && image[i + 8] == 0x43) // C
      if (  image[i+1] == 0x44 && image[i + 5] == 0x44 && image[i + 9] == 0x44) // D
        if (image[i+2] == 0x46 && image[i + 6] == 0x46 && image[i +10] == 0x46) // F
        {
          subversion = image[i+3];
          break;
        }
  }

  switch (subversion) {
    case 0x4a: //CDFJ
      myAmplitudeStream = 0x23;
      myDatastreamBase = 0x0098;
      myDatastreamIncrementBase = 0x0124;
      myWaveformBase = 0x01b0;
      break;

    case 0: //V0
      myAmplitudeStream = 0x22;
      myDatastreamBase = 0x06e0;
      myDatastreamIncrementBase = 0x0768;
      myWaveformBase = 0x07f0;
      break;

    default: //V1
      myAmplitudeStream = 0x22;
      myDatastreamBase = 0x00a0;
      myDatastreamIncrementBase = 0x0128;
      myWaveformBase = 0x01b0;
      break;
 }    
    
 fastDataStreamBase = (uInt32)&fast_cart_buffer[myDatastreamBase];
 fastIncStreamBase = (uInt32)&fast_cart_buffer[myDatastreamIncrementBase];
    
  myMode = 0xFF;
  myDataStreamFetch = 0x00;
    
  // Create Thumbulator ARM emulator
  myThumbEmulator = new Thumbulator((uInt16*)(image));
  
  myCartCDF = this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::~CartridgeCDF()
{
   if (myThumbEmulator) delete myThumbEmulator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartridgeCDF::name() const
{
  return "CartridgeCDF";
}    

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::reset()
{
  // Upon reset we switch to the startup bank
  myDPCPCycles = mySystem->cycles();
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust the cycle counter so that it reflects the new value
  myDPCPCycles -= cycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert(((0x1080 & mask) == 0) && ((0x1100 & mask) == 0));

  // Set the page accessing methods for the hot spots
  PageAccess access;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  access.device = this;

  // Set page accessing method for the DPC reading & writing pages
  for(uInt32 j = 0x1000; j < 0x2000; j += (1 << shift))
  {
    mySystem->setPageAccess(j >> shift, access);
  }

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeCDF::callFunction(uInt8 value)
{
  // myParameter
  switch (value)
  {
    case 254:
    case 255:
      // Call user written ARM code (most likely be C compiled for ARM)
      myThumbEmulator->run();
      break;
  }
}


uInt32 CDFCallback(uInt8 function, uInt32 value1, uInt32 value2)
{
  switch (function)
  {
    case 0:
      // _SetNote - set the note/frequency
      myMusicFrequencies[value1] = value2;
      break;

      // _ResetWave - reset counter,
      // used to make sure digital samples start from the beginning
    case 1:
      myMusicCounters[value1] = 0;
      break;

      // _GetWavePtr - return the counter
    case 2:
        myMusicCounters[value1] = 0xFFFFFFFF;       // TODO: we don't yet support digital waveforms so just tell the caller we're done with the sample
        return myMusicCounters[value1];

      // _SetWaveSize - set size of waveform buffer
    case 3:
      myMusicWaveformSize[value1] = value2;
      break;
  }

  return 0;
}

uInt8 CartridgeCDF::peekMusic(void)
{
    return 0x00;    // We don't yet support digital waveforms (timing is wrong... sounds like crap... too slow for the poor DSi)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDF::peek(uInt16 address)  
{
  return myDPCptr[address & 0xFFF];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void CartridgeCDF::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  switch(address)
  {
    case 0xFF0:   // DSWRITE
      {
          uInt32 *ptr = (uInt32*) ((uInt32)fastDataStreamBase + (COMMSTREAM << 2));
          myDisplayImageCDF[ *ptr >> 20 ] = value;
          *ptr  += 0x100000;  // always increment by 1 when writing
      }
      break;

    case 0xFF1:   // DSPTR
      {
          uInt32 *ptr = (uInt32*) ((uInt32)fastDataStreamBase + (COMMSTREAM << 2));
          *ptr <<=8;
          *ptr &= 0xf0000000;
          *ptr |= (value << 20);
      }
      break;

    case 0xFF2:   // SETMODE
      myMode = value;
      if (myMode & 0x0F) myDataStreamFetch = 0x00; else myDataStreamFetch = (myAmplitudeStream+1);
      break;

    case 0xFF3:   // CALLFN
      callFunction(value);
      break;

    case 0xFF5:
      // Set the current bank to the first 4k bank
      bank(0);
      break;

    case 0x0FF6:
      // Set the current bank to the second 4k bank
      bank(1);
      break;

    case 0x0FF7:
      // Set the current bank to the third 4k bank
      bank(2);
      break;

    case 0x0FF8:
      // Set the current bank to the fourth 4k bank
      bank(3);
      break;

    case 0x0FF9:
      // Set the current bank to the fifth 4k bank
      bank(4);
      break;

    case 0x0FFA:
      // Set the current bank to the sixth 4k bank
      bank(5);
      break;

    case 0x0FFB:
      // Set the current bank to the last 4k bank
      bank(6);
      break;

    default:
      break;
  }
    
  return;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeCDF::getImage(int& size) const
{
  size = mySize;
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCDF::readFromDatastream(uInt8 index)
{
  // Pointers are stored as:
  // PPPFF---
  //
  // Increments are stored as
  // ----IIFF
  //
  // P = Pointer
  // I = Increment
  // F = Fractional

  uInt32 pointer = getDatastreamPointer(index);
  uInt16 increment = getDatastreamIncrement(index);

  uInt8 value;
  value = myDisplayImageCDF[ pointer >> 20 ];
  pointer += (increment << 12);   

  setDatastreamPointer(index, pointer);
  return value;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::bank(uInt16 bank)
{
    // Remember what bank we're in
  myCurrentOffset = bank << 12;
  myDPCptr = &myDPC[myCurrentOffset];
}
