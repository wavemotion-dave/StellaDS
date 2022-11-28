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

//#define CDFJ_MUSIC_DISABLE  1 // Comment this line out if you want to try music fetchers

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
#define MEM_32KB (1024 * 32)
#define MEM_64KB (1024 * 64)

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
uInt8 myMode __attribute__((section(".dtcm"))) = 0xFF;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// params:
//  - searchValue: uInt32 value to search for; assumes it is on a DWORD boundary
//
// returns:
//  - offset in image where value was found
//  - 0xFFFFFFFF if not found
uInt32 CartridgeCDF::scanCDFDriver(uInt32 searchValue)
{
  for (int i = 0; i < 2048; i += 4)
    if (getUInt32(cart_buffer, i) == searchValue)
      return i;

  return 0xFFFFFFFF;
}

u8 myLDXenabled = 0;
u8 myLDYenabled = 0;
uInt32 myFastFetcherOffset = 0;
uInt32 cBase = 0x00000000;
uInt32 cStart = 0x00000000;
uInt32 cStack = 0x00000000;

bool  isCDFJPlus      __attribute__((section(".dtcm"))) = 0;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::CartridgeCDF(const uInt8* image, uInt32 size)
{
  uInt8 subversion = 0;
    
  // isCDFJPlus is already set by the Cart auto-detect

  // get offset of CDFJPlus ID
  uInt32 cdfjOffset = 0;
    
  if ((cdfjOffset = scanCDFDriver(0x53554c50)) != 0xFFFFFFFF && // Plus
      getUInt32(image, cdfjOffset+4) == 0x4a464443 &&   // CDFJ
      getUInt32(image, cdfjOffset+8) == 0x00000001)     // V1
  {
    myAmplitudeStream = 0x23;
    myDatastreamBase = 0x0098;
    myDatastreamIncrementBase = 0x0124;
    myFastFetcherOffset = 0;
    myWaveformBase = 0x01b0;

    for (int i = 0; i < 2048; i += 4)
    {
      const uInt32 cdfjValue = getUInt32(image, i);
      if (cdfjValue == 0x135200A2)
        myLDXenabled = true;
      if (cdfjValue == 0x135200A0)
        myLDYenabled = true;
        
      // search for Fast Fetcher Offset (default is 0)
      if ((cdfjValue & 0xFFFFFF00) == 0xE2422000)
        myFastFetcherOffset = i;
        
      cBase = getUInt32(image, 0x17F8) & 0xFFFFFFFE;    // C Base Address
      cStart = cBase;                                   // C Start Address
      cStack = getUInt32(image, 0x17F4);                // C Stack        
    }
  }
  else
  {
      cStack = 0x40001fb4;
      cBase  = 0x00000800;
      cStart = 0x00000808;
      
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
  }
    
  // Pointer to the program ROM (28K @ 4K offset)
  myDPCptr = (uInt8 *)image + (isCDFJPlus ? MEM_2KB : MEM_4KB);
  memcpy(myDPC, myDPCptr, MEM_32KB); // For the 6502, we only need to copy 32K 
        
  memset(fast_cart_buffer, 0, 8192);    // Clear all of the "ARM Thumb" RAM
    
  // Copy intial driver into the CDF Harmony RAM
  memcpy(fast_cart_buffer, image, MEM_2KB);

  // Pointer to the display RAM
  myDisplayImageCDF = fast_cart_buffer + MEM_2KB;
    
  // Set the initial Music Waveform sizes
  for (int i=0; i < 3; ++i)
  {
    myMusicWaveformSize[i] = 27;
  }
    
  // CDF/CDFJ always starts in bank 6, CDFJ+ in bank 0
  myStartBank = (isCDFJPlus ? 0:6);
  bank(myStartBank);
    
  fastDataStreamBase = (uInt32)&fast_cart_buffer[myDatastreamBase];
  fastIncStreamBase = (uInt32)&fast_cart_buffer[myDatastreamIncrementBase];
    
  myMode = 0xFF;
  myDataStreamFetch = 0x00;
    
  // Create Thumbulator ARM emulator
  myThumbEmulator = new Thumbulator((uInt16*)image); // The Thumbulator gets the full image no matter how big...
  
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
#ifdef CDFJ_MUSIC_DISABLE
        myMusicCounters[value1] = 0xFFFFFFFF;   // TODO: Not ready for digital audio 
#endif
        return myMusicCounters[value1];
      // _SetWaveSize - set size of waveform buffer
    case 3:
      myMusicWaveformSize[value1] = value2;
      break;
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 CartridgeCDF::getWaveform(uInt8 index) const
{
  uInt32 *ptr = (uInt32*) ((uInt32)&fast_cart_buffer[myWaveformBase + (index << 2)]);
  uInt32 result = *ptr;

  result -= (0x40000000 + MEM_2KB);
  result &= 4095;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 CartridgeCDF::getSample()
{
  uInt32 *ptr = (uInt32*) ((uInt32)&fast_cart_buffer[myWaveformBase]);
  return *ptr;
}

ITCM_CODE uInt8 CartridgeCDF::peekMusic(void)
{
#ifdef CDFJ_MUSIC_DISABLE
    return 0x00;
#else    
  //static double myFractionalClocks=0.0;
  uInt8 peekvalue = 0;
 
  if (DIGITAL_AUDIO_ON)
  {
      uInt32 cyclesPassed = (gSystemCycles - myDPCPCycles);
      if (cyclesPassed >= 60)
      {
          myMusicCounters[0] += myMusicFrequencies[0] * (cyclesPassed/60);
          myDPCPCycles = (gSystemCycles - (cyclesPassed % 60));
      }
    // retrieve packed sample (max size is 2K, or 4K of unpacked data)
    const uInt32 sampleaddress = getSample() + (myMusicCounters[0] >> 21);

    if (sampleaddress & 0xF0000000) // check for RAM
      peekvalue = fast_cart_buffer[sampleaddress & 0xFFFF];
    else 
      peekvalue = cart_buffer[sampleaddress];

    // make sure current volume value is in the lower nybble
    if ((myMusicCounters[0] & (1<<20)) == 0)
    {
      peekvalue >>= 4;
    }
    peekvalue &= 0x0f;
  }
  else
  {
      uInt32 cyclesPassed = (gSystemCycles - myDPCPCycles);
      if (cyclesPassed >= 60)
      {
          myMusicCounters[0] += myMusicFrequencies[0] * (cyclesPassed/60);
          myMusicCounters[1] += myMusicFrequencies[1] * (cyclesPassed/60);
          myMusicCounters[2] += myMusicFrequencies[2] * (cyclesPassed/60);
          myDPCPCycles = (gSystemCycles - (cyclesPassed % 60));
      }
      peekvalue = myDisplayImageCDF[getWaveform(0) + (myMusicCounters[0] >> myMusicWaveformSize[0])]
                + myDisplayImageCDF[getWaveform(1) + (myMusicCounters[1] >> myMusicWaveformSize[1])]
                + myDisplayImageCDF[getWaveform(2) + (myMusicCounters[2] >> myMusicWaveformSize[2])];
  }
  return peekvalue;   
#endif    
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
          if (isCDFJPlus)
          {
              uInt32 *ptr = (uInt32*) ((uInt32)fastDataStreamBase + (COMMSTREAM << 2));
              myDisplayImageCDF[ *ptr >> 16 ] = value;
              *ptr  += 0x10000;  // always increment by 1 when writing
          }
          else          
          {
              uInt32 *ptr = (uInt32*) ((uInt32)fastDataStreamBase + (COMMSTREAM << 2));
              myDisplayImageCDF[ *ptr >> 20 ] = value;
              *ptr  += 0x100000;  // always increment by 1 when writing
          }
      }
      break;

    case 0xFF1:   // DSPTR
      {
          uInt32 *ptr = (uInt32*) ((uInt32)fastDataStreamBase + (COMMSTREAM << 2));
          *ptr <<=8;
          if (isCDFJPlus)
          {
              *ptr &= 0xf0000000;
              *ptr |= (value << 16);
          }
          else
          {
              *ptr &= 0xff000000;
              *ptr |= (value << 20);
          }
      }
      break;

    case 0xFF2:   // SETMODE
      myMode = value;
      if (myMode & 0x0F) myDataStreamFetch = 0x00; else myDataStreamFetch = (myAmplitudeStream+1);
      break;

    case 0xFF3:   // CALLFN
      callFunction(value);
      break;
          
    case 0xFF4: bank(isCDFJPlus ? 0 : 6); break;
    case 0xFF5: bank(isCDFJPlus ? 1 : 0); break;
    case 0xFF6: bank(isCDFJPlus ? 2 : 1); break;
    case 0xFF7: bank(isCDFJPlus ? 3 : 2); break;
    case 0xFF8: bank(isCDFJPlus ? 4 : 3); break;
    case 0xFF9: bank(isCDFJPlus ? 5 : 4); break;
    case 0xFFA: bank(isCDFJPlus ? 6 : 5); break;
    case 0xFFB: bank(isCDFJPlus ? 0 : 6); break;

    default:
      break;
  }
    
  return;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::bank(uInt16 bank)
{
    // Remember what bank we're in
  myCurrentOffset = bank << 12;
  myDPCptr = &myDPC[myCurrentOffset];
}
