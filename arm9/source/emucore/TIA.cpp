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
// $Id: TIA.cxx,v 1.35 2005/01/05 02:57:58 bwmott Exp $
//============================================================================
#include <nds.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Console.hxx"
#include "Control.hxx"
#include "M6502.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "TIASound.hxx"
#include "Cart.hxx"
#define HBLANK 68

// ---------------------------------------------------------------------------------------------------------
// All of this used to be in the TIA class but for maximum speed, this is moved it out into fast memory...
// ---------------------------------------------------------------------------------------------------------
uint32  myBlendBk __attribute__((section(".dtcm"))) = 0;
uInt32  myPF                        __attribute__((section(".dtcm")));
uInt32  myColor[4]                  __attribute__((section(".dtcm")));
uInt32  myFrameYStart               __attribute__((section(".dtcm")));
uInt32  myFrameHeight               __attribute__((section(".dtcm")));
uInt32  myStartDisplayOffset        __attribute__((section(".dtcm")));
uInt32  myStopDisplayOffset         __attribute__((section(".dtcm")));
Int32   myVSYNCFinishClock          __attribute__((section(".dtcm")));
uInt8*  myCurrentFrameBuffer[2]     __attribute__((section(".dtcm")));
uInt8*  myFramePointer              __attribute__((section(".dtcm")));
uInt16* myDSFramePointer            __attribute__((section(".dtcm")));
Int32   myLastHMOVEClock            __attribute__((section(".dtcm")));
uInt32  myM0CosmicArkCounter        __attribute__((section(".dtcm")));
uInt32  ourPlayfieldTable[2][160]   __attribute__((section(".dtcm")));
Int32   myClockWhenFrameStarted     __attribute__((section(".dtcm")));
Int32   myCyclesWhenFrameStarted    __attribute__((section(".dtcm")));
Int32   myClockStartDisplay         __attribute__((section(".dtcm")));
Int32   myClockStopDisplay          __attribute__((section(".dtcm")));
Int32   myClockAtLastUpdate         __attribute__((section(".dtcm")));
Int32   myClocksToEndOfScanLine     __attribute__((section(".dtcm")));
uInt16  myMaximumNumberOfScanlines  __attribute__((section(".dtcm")));
uInt8*  myCurrentBLMask             __attribute__((section(".dtcm")));
uInt8*  myCurrentM0Mask             __attribute__((section(".dtcm")));
uInt8*  myCurrentM1Mask             __attribute__((section(".dtcm")));
uInt8*  myCurrentP0Mask             __attribute__((section(".dtcm")));
uInt8*  myCurrentP1Mask             __attribute__((section(".dtcm")));
uInt32* myCurrentPFMask             __attribute__((section(".dtcm")));
uInt16  ourCollisionTable[256]      __attribute__((section(".dtcm")));
uInt16  myCollision                 __attribute__((section(".dtcm")));    
Int16   myPOSP0                     __attribute__((section(".dtcm")));         
Int16   myPOSP1                     __attribute__((section(".dtcm")));         
Int16   myPOSM0                     __attribute__((section(".dtcm")));         
Int16   myPOSM1                     __attribute__((section(".dtcm")));         
Int16   myPOSBL                     __attribute__((section(".dtcm")));       
uInt8   myCurrentFrame              __attribute__((section(".dtcm")));
uInt8   dma_channel                 __attribute__((section(".dtcm")));
uInt8   myNUSIZ0                    __attribute__((section(".dtcm")));
uInt8   myNUSIZ1                    __attribute__((section(".dtcm")));
uInt8   myPlayfieldPriorityAndScore __attribute__((section(".dtcm")));
uInt8   myPriorityEncoder[2][256]   __attribute__((section(".dtcm")));
uInt8   myCTRLPF                    __attribute__((section(".dtcm")));
uInt8   myREFP0                     __attribute__((section(".dtcm")));
uInt8   myREFP1                     __attribute__((section(".dtcm")));
uInt8   myGRP0                      __attribute__((section(".dtcm")));
uInt8   myGRP1                      __attribute__((section(".dtcm")));
uInt8   myDGRP0                     __attribute__((section(".dtcm")));
uInt8   myDGRP1                     __attribute__((section(".dtcm")));
uInt8   myENAM0                     __attribute__((section(".dtcm")));
uInt8   myENAM1                     __attribute__((section(".dtcm")));
uInt8   myENABL                     __attribute__((section(".dtcm")));
uInt8   myDENABL                    __attribute__((section(".dtcm")));
Int8    myHMP0                      __attribute__((section(".dtcm")));
Int8    myHMP1                      __attribute__((section(".dtcm")));
Int8    myHMM0                      __attribute__((section(".dtcm")));
Int8    myHMM1                      __attribute__((section(".dtcm")));
Int8    myHMBL                      __attribute__((section(".dtcm")));
uInt8   myVDELP0                    __attribute__((section(".dtcm")));
uInt8   myVDELP1                    __attribute__((section(".dtcm")));
uInt8   myVDELBL                    __attribute__((section(".dtcm")));
uInt8   myRESMP0                    __attribute__((section(".dtcm")));
uInt8   myRESMP1                    __attribute__((section(".dtcm")));
uInt8   myEnabledObjects            __attribute__((section(".dtcm")));
uInt8   myCurrentGRP0               __attribute__((section(".dtcm")));
uInt8   myCurrentGRP1               __attribute__((section(".dtcm")));
uInt8   myVSYNC                     __attribute__((section(".dtcm")));
uInt8   myVBLANK                    __attribute__((section(".dtcm")));
uInt8   myHMOVEBlankEnabled         __attribute__((section(".dtcm")));
uInt8   myM0CosmicArkMotionEnabled  __attribute__((section(".dtcm")));
uInt8   ourPlayerReflectTable[256]  __attribute__((section(".dtcm")));

Int8 ourPokeDelayTable[64] __attribute__ ((aligned (4))) __attribute__((section(".dtcm"))) = {
   0,  1,  0,  0,  8,  8,  0,  0,  0,  0,  0,  1,  1, -1, -1, -1,
   0,  0,  8,  8,  8,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int8 delay_tab[] __attribute__ ((aligned (4))) __attribute__((section(".dtcm"))) = 
{
        4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 
        4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 
        4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 
        4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 
        4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 2, 2, 2, 3, 3, 3
};
   
// This 1k table is not worth putting in FAST memory - doesn't buy us much and is rather large...
uInt32 __attribute__ ((aligned (4))) color_repeat_table[]  = {
        0x00000000,  0x00000000,  0x02020202,  0x02020202,  0x04040404,  0x04040404,  0x06060606,  0x06060606,  
        0x08080808,  0x08080808,  0x0A0A0A0A,  0x0A0A0A0A,  0x0C0C0C0C,  0x0C0C0C0C,  0x0E0E0E0E,  0x0E0E0E0E,  
        0x10101010,  0x10101010,  0x12121212,  0x12121212,  0x14141414,  0x14141414,  0x16161616,  0x16161616,  
        0x18181818,  0x18181818,  0x1A1A1A1A,  0x1A1A1A1A,  0x1C1C1C1C,  0x1C1C1C1C,  0x1E1E1E1E,  0x1E1E1E1E,  
        0x20202020,  0x20202020,  0x22222222,  0x22222222,  0x24242424,  0x24242424,  0x26262626,  0x26262626,  
        0x28282828,  0x28282828,  0x2A2A2A2A,  0x2A2A2A2A,  0x2C2C2C2C,  0x2C2C2C2C,  0x2E2E2E2E,  0x2E2E2E2E,  
        0x30303030,  0x30303030,  0x32323232,  0x32323232,  0x34343434,  0x34343434,  0x36363636,  0x36363636,  
        0x38383838,  0x38383838,  0x3A3A3A3A,  0x3A3A3A3A,  0x3C3C3C3C,  0x3C3C3C3C,  0x3E3E3E3E,  0x3E3E3E3E,  
        0x40404040,  0x40404040,  0x42424242,  0x42424242,  0x44444444,  0x44444444,  0x46464646,  0x46464646,  
        0x48484848,  0x48484848,  0x4A4A4A4A,  0x4A4A4A4A,  0x4C4C4C4C,  0x4C4C4C4C,  0x4E4E4E4E,  0x4E4E4E4E,  
        0x50505050,  0x50505050,  0x52525252,  0x52525252,  0x54545454,  0x54545454,  0x56565656,  0x56565656,  
        0x58585858,  0x58585858,  0x5A5A5A5A,  0x5A5A5A5A,  0x5C5C5C5C,  0x5C5C5C5C,  0x5E5E5E5E,  0x5E5E5E5E,  
        0x60606060,  0x60606060,  0x62626262,  0x62626262,  0x64646464,  0x64646464,  0x66666666,  0x66666666,  
        0x68686868,  0x68686868,  0x6A6A6A6A,  0x6A6A6A6A,  0x6C6C6C6C,  0x6C6C6C6C,  0x6E6E6E6E,  0x6E6E6E6E,  
        0x70707070,  0x70707070,  0x72727272,  0x72727272,  0x74747474,  0x74747474,  0x76767676,  0x76767676,  
        0x78787878,  0x78787878,  0x7A7A7A7A,  0x7A7A7A7A,  0x7C7C7C7C,  0x7C7C7C7C,  0x7E7E7E7E,  0x7E7E7E7E,  
        0x80808080,  0x80808080,  0x82828282,  0x82828282,  0x84848484,  0x84848484,  0x86868686,  0x86868686,  
        0x88888888,  0x88888888,  0x8A8A8A8A,  0x8A8A8A8A,  0x8C8C8C8C,  0x8C8C8C8C,  0x8E8E8E8E,  0x8E8E8E8E,  
        0x90909090,  0x90909090,  0x92929292,  0x92929292,  0x94949494,  0x94949494,  0x96969696,  0x96969696,  
        0x98989898,  0x98989898,  0x9A9A9A9A,  0x9A9A9A9A,  0x9C9C9C9C,  0x9C9C9C9C,  0x9E9E9E9E,  0x9E9E9E9E,  
        0xA0A0A0A0,  0xA0A0A0A0,  0xA2A2A2A2,  0xA2A2A2A2,  0xA4A4A4A4,  0xA4A4A4A4,  0xA6A6A6A6,  0xA6A6A6A6,  
        0xA8A8A8A8,  0xA8A8A8A8,  0xAAAAAAAA,  0xAAAAAAAA,  0xACACACAC,  0xACACACAC,  0xAEAEAEAE,  0xAEAEAEAE,  
        0xB0B0B0B0,  0xB0B0B0B0,  0xB2B2B2B2,  0xB2B2B2B2,  0xB4B4B4B4,  0xB4B4B4B4,  0xB6B6B6B6,  0xB6B6B6B6,  
        0xB8B8B8B8,  0xB8B8B8B8,  0xBABABABA,  0xBABABABA,  0xBCBCBCBC,  0xBCBCBCBC,  0xBEBEBEBE,  0xBEBEBEBE,  
        0xC0C0C0C0,  0xC0C0C0C0,  0xC2C2C2C2,  0xC2C2C2C2,  0xC4C4C4C4,  0xC4C4C4C4,  0xC6C6C6C6,  0xC6C6C6C6,  
        0xC8C8C8C8,  0xC8C8C8C8,  0xCACACACA,  0xCACACACA,  0xCCCCCCCC,  0xCCCCCCCC,  0xCECECECE,  0xCECECECE,  
        0xD0D0D0D0,  0xD0D0D0D0,  0xD2D2D2D2,  0xD2D2D2D2,  0xD4D4D4D4,  0xD4D4D4D4,  0xD6D6D6D6,  0xD6D6D6D6,  
        0xD8D8D8D8,  0xD8D8D8D8,  0xDADADADA,  0xDADADADA,  0xDCDCDCDC,  0xDCDCDCDC,  0xDEDEDEDE,  0xDEDEDEDE,  
        0xE0E0E0E0,  0xE0E0E0E0,  0xE2E2E2E2,  0xE2E2E2E2,  0xE4E4E4E4,  0xE4E4E4E4,  0xE6E6E6E6,  0xE6E6E6E6,  
        0xE8E8E8E8,  0xE8E8E8E8,  0xEAEAEAEA,  0xEAEAEAEA,  0xECECECEC,  0xECECECEC,  0xEEEEEEEE,  0xEEEEEEEE,  
        0xF0F0F0F0,  0xF0F0F0F0,  0xF2F2F2F2,  0xF2F2F2F2,  0xF4F4F4F4,  0xF4F4F4F4,  0xF6F6F6F6,  0xF6F6F6F6,  
        0xF8F8F8F8,  0xF8F8F8F8,  0xFAFAFAFA,  0xFAFAFAFA,  0xFCFCFCFC,  0xFCFCFCFC,  0xFEFEFEFE,  0xFEFEFEFE  
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 ourHMOVEBlankEnableCycles[76] __attribute__((section(".dtcm"))) = {
  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,   // 00
  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,   // 10
  true,  false, false, false, false, false, false, false, false, false,  // 20
  false, false, false, false, false, false, false, false, false, false,  // 30
  false, false, false, false, false, false, false, false, false, false,  // 40
  false, false, false, false, false, false, false, false, false, false,  // 50
  false, false, false, false, false, false, false, false, false, false,  // 60
  false, false, false, false, false, true                                // 70
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int8 ourCompleteMotionTable[76][16] = {
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -6,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -5, -5,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -5, -5,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -4, -4, -4,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -3, -3, -3, -3,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -2, -2, -2, -2, -2,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -2, -2, -2, -2, -2,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -1, -1, -1, -1, -1, -1,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0,  0,  0,  0,  0,  0,  0,  0,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 1,  1,  1,  1,  1,  1,  1,  1,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 1,  1,  1,  1,  1,  1,  1,  1,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 2,  2,  2,  2,  2,  2,  2,  2,  8,  7,  6,  5,  4,  3,  2,  2}, // HBLANK
  { 3,  3,  3,  3,  3,  3,  3,  3,  8,  7,  6,  5,  4,  3,  3,  3}, // HBLANK
  { 4,  4,  4,  4,  4,  4,  4,  4,  8,  7,  6,  5,  4,  4,  4,  4}, // HBLANK
  { 4,  4,  4,  4,  4,  4,  4,  4,  8,  7,  6,  5,  4,  4,  4,  4}, // HBLANK
  { 5,  5,  5,  5,  5,  5,  5,  5,  8,  7,  6,  5,  5,  5,  5,  5}, // HBLANK
  { 6,  6,  6,  6,  6,  6,  6,  6,  8,  7,  6,  6,  6,  6,  6,  6}, // HBLANK
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0, -1, -2,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0, -1, -2, -3,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0, -1, -2, -3,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0,  0,  0,  0, -1, -2, -3, -4,  0,  0,  0,  0,  0,  0,  0,  0}, 
  { 0,  0,  0, -1, -2, -3, -4, -5,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0,  0, -1, -2, -3, -4, -5, -6,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0,  0, -1, -2, -3, -4, -5, -6,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0, -1, -2, -3, -4, -5, -6, -7,  0,  0,  0,  0,  0,  0,  0,  0},
  {-1, -2, -3, -4, -5, -6, -7, -8,  0,  0,  0,  0,  0,  0,  0,  0},
  {-2, -3, -4, -5, -6, -7, -8, -9,  0,  0,  0,  0,  0,  0,  0, -1},
  {-2, -3, -4, -5, -6, -7, -8, -9,  0,  0,  0,  0,  0,  0,  0, -1},
  {-3, -4, -5, -6, -7, -8, -9,-10,  0,  0,  0,  0,  0,  0, -1, -2}, 
  {-4, -5, -6, -7, -8, -9,-10,-11,  0,  0,  0,  0,  0, -1, -2, -3},
  {-5, -6, -7, -8, -9,-10,-11,-12,  0,  0,  0,  0, -1, -2, -3, -4},
  {-5, -6, -7, -8, -9,-10,-11,-12,  0,  0,  0,  0, -1, -2, -3, -4},
  {-6, -7, -8, -9,-10,-11,-12,-13,  0,  0,  0, -1, -2, -3, -4, -5},
  {-7, -8, -9,-10,-11,-12,-13,-14,  0,  0, -1, -2, -3, -4, -5, -6},
  {-8, -9,-10,-11,-12,-13,-14,-15,  0, -1, -2, -3, -4, -5, -6, -7},
  {-8, -9,-10,-11,-12,-13,-14,-15,  0, -1, -2, -3, -4, -5, -6, -7},
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}  // HBLANK
};



// -----------------------------------------------------------
// These two big buffers are our ping-pong video memory...
// -----------------------------------------------------------
uInt8 __attribute__ ((aligned (4))) videoBuf0[160 * 300];
uInt8 __attribute__ ((aligned (4))) videoBuf1[160 * 300];
#define MYCOLUBK  0
#define MYCOLUPF  1
#define MYCOLUP0  2
#define MYCOLUP1  3

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const Console& console)
    : myConsole(console),
      myColorLossEnabled(false)
{
  myMaximumNumberOfScanlines = ((myCartInfo.tv_type == PAL) ? 312:262); 
  // --------------------------------------------------------------------------------------
  // Allocate buffers for two frame buffers - Turns out Video Memory is actually slower
  // since we do a lot of 8-bit reads and the video memory is 16-bits wide. So we handle
  // our buffers in "slower" memory but it turns out to be a little faster to do it in 
  // main memory and then DMA copy into VRAM.
  // --------------------------------------------------------------------------------------
  myCurrentFrameBuffer[0] = videoBuf0; 
  myCurrentFrameBuffer[1] = videoBuf1; 

  for(uInt16 x = 0; x < 2; ++x)
  {
    for(uInt16 enabled = 0; enabled < 256; ++enabled)
    {
      if(enabled & PriorityBit)
      {
        uInt8 color = 0;

        if((enabled & (myP1Bit | myM1Bit)) != 0)
          color = 3;
        if((enabled & (myP0Bit | myM0Bit)) != 0)
          color = 2;
        if((enabled & myBLBit) != 0)
          color = 1;
        if((enabled & myPFBit) != 0)
          color = 1;  // NOTE: Playfield has priority so ScoreBit isn't used

        myPriorityEncoder[x][enabled] = color;
      }
      else
      {
        uInt8 color = 0;

        if((enabled & myBLBit) != 0)
          color = 1;
        if((enabled & myPFBit) != 0)
          color = (enabled & ScoreBit) ? ((x == 0) ? 2 : 3) : 1;
        if((enabled & (myP1Bit | myM1Bit)) != 0)
          color = 3;
        if((enabled & (myP0Bit | myM0Bit)) != 0)
          color = 2;

        myPriorityEncoder[x][enabled] = color;
      }
    }
  }

  for(uInt32 i = 0; i < 640; ++i)
  {
    ourDisabledMaskTable[i] = 0;
  }

  // Compute all of the mask tables
  computeBallMaskTable();
  computeCollisionTable();
  computeMissleMaskTable();
  computePlayerMaskTable();
  computePlayerPositionResetWhenTable();
  computePlayerReflectTable();
  computePlayfieldMaskTable();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::~TIA()
{
    // Frame Buffers are static now... nothing to delete...
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* TIA::name() const
{
  return "TIA";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::reset()
{
  // Clear frame buffers
  for(uInt32 i = 0; i < 160 * 300; ++i)
  {
    myCurrentFrameBuffer[0][i] = 0;
    myCurrentFrameBuffer[1][i] = 0;
  }
  myCurrentFrame = 0;
  dma_channel = 0;    

  // Reset pixel pointer and drawing flag
  myFramePointer = myCurrentFrameBuffer[0];
  myDSFramePointer = BG_GFX;

  // Calculate color clock offsets for starting and stoping frame drawing
  myStartDisplayOffset = 228 * myCartInfo.displayStartScanline;                              // Allow for some underscan lines on a per-cart basis
  myStopDisplayOffset = myStartDisplayOffset + (228 * (myCartInfo.displayStopScanline));     // Allow for some overscan lines on a per-cart basis

  // Reasonable values to start and stop the current frame drawing
  myCyclesWhenFrameStarted = gSystemCycles;
  myClockWhenFrameStarted = gSystemCycles * 3;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockWhenFrameStarted;
  myClocksToEndOfScanLine = 228;
  myVSYNCFinishClock = 0x7FFFFFFF;
    
  // Currently no objects are enabled
  myEnabledObjects = 0;

  // Some default values for the registers
  myVSYNC = 0;
  myVBLANK = 0;
  myNUSIZ0 = 0;
  myNUSIZ1 = 0;
  myPlayfieldPriorityAndScore = 0;
  myColor[MYCOLUP0] = 0;  
  myColor[MYCOLUP1] = 0;  
  myColor[MYCOLUPF] = 0;  
  myColor[MYCOLUBK] = 0;  
  myCTRLPF = 0;
  myREFP0 = false;
  myREFP1 = false;
  myPF = 0;
  myGRP0 = 0;
  myGRP1 = 0;
  myDGRP0 = 0;
  myDGRP1 = 0;
  myENAM0 = false;
  myENAM1 = false;
  myENABL = false;
  myDENABL = false;
  myHMP0 = 0;
  myHMP1 = 0;
  myHMM0 = 0;
  myHMM1 = 0;
  myHMBL = 0;
  myVDELP0 = false;
  myVDELP1 = false;
  myVDELBL = false;
  myRESMP0 = false;
  myRESMP1 = false;
  myCollision = 0;
  myPOSP0 = 0;
  myPOSP1 = 0;
  myPOSM0 = 0;
  myPOSM1 = 0;
  myPOSBL = 0;


  // Some default values for the "current" variables
  myCurrentGRP0 = 0;
  myCurrentGRP1 = 0;
  myCurrentBLMask = ourBallMaskTable[0][0];
  myCurrentM0Mask = ourMissleMaskTable[0][0][0];
  myCurrentM1Mask = ourMissleMaskTable[0][0][0];
  myCurrentP0Mask = ourPlayerMaskTable[0][0][0];
  myCurrentP1Mask = ourPlayerMaskTable[0][0][0];
  myCurrentPFMask = ourPlayfieldTable[0];

  myLastHMOVEClock = 0;
  myHMOVEBlankEnabled = false;
  myM0CosmicArkMotionEnabled = false;
  myM0CosmicArkCounter = 0;

  myDumpEnabled = false;
  myDumpDisabledCycle = 0;

  myFrameYStart = 34;
  myFrameHeight = 210;
    
  myColorLossEnabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void TIA::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = gSystemCycles;

  // Adjust the dump cycle
  myDumpDisabledCycle -= cycles;

  // Get the current color clock the system is using
  uInt32 clocks = cycles * 3;

  // Adjust the clocks by this amount since we're reseting the clock to zero
  myCyclesWhenFrameStarted -= cycles;
  myClockWhenFrameStarted -= clocks;
  myClockStartDisplay -= clocks;
  myClockStopDisplay -= clocks;
  myClockAtLastUpdate -= clocks;
  myVSYNCFinishClock -= clocks;
  myLastHMOVEClock -= clocks;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::install(System& system)
{
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  mySystem->resetCycles();


  // All accesses are to this device
  PageAccess access;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  access.device = this;

  // We're installing in a 2600 system
  for(uInt32 i = 0; i < 8192; i += (1 << shift))
  {
    if((i & 0x1080) == 0x0000)
    {
      mySystem->setPageAccess(i >> shift, access);
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void TIA::update()
{
    extern int gTotalAtariFrames;
  // We have processed another frame... used for true FPS indication
  gTotalAtariFrames++;
    
  // Remember the number of clocks which have passed on the current scanline
  // so that we can adjust the frame's starting clock by this amount.  This
  // is necessary since some games position objects during VSYNC and the
  // TIA's internal counters are not reset by VSYNC.
  uInt32 clocks = ((gSystemCycles * 3) - myClockWhenFrameStarted) % 228;

  // Ask the system to reset the cycle count so it doesn't overflow
  mySystem->resetCycles();

  // Setup clocks that'll be used for drawing this frame
  myCyclesWhenFrameStarted = -(clocks/3);
  myClockWhenFrameStarted = -clocks;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockStartDisplay;
  myClocksToEndOfScanLine = 228;

  // Reset frame buffer pointer
  myCurrentFrame = (myCurrentFrame + 1) % 2;
  myFramePointer = myCurrentFrameBuffer[myCurrentFrame];
  myDSFramePointer = BG_GFX;

  // Execute instructions until frame is finished
  mySystem->m6502().execute(25000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32* TIA::palette() const
{
    return ((myCartInfo.tv_type == PAL) ? ourPALPalette : ourNTSCPalette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::width() const 
{
  return 160; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::height() const 
{
  return myFrameHeight; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computeBallMaskTable()
{
  // First, calculate masks for alignment 0
  for(Int32 size = 0; size < 4; ++size)
  {
    Int32 x;

    // Set all of the masks to false to start with
    for(x = 0; x < 160; ++x)
    {
      ourBallMaskTable[0][size][x] = false;
    }

    // Set the necessary fields true
    for(x = 0; x < 160 + 8; ++x)
    {
      if((x >= 0) && (x < (1 << size)))
      {
        ourBallMaskTable[0][size][x % 160] = true;
      }
    }

    // Copy fields into the wrap-around area of the mask
    for(x = 0; x < 160; ++x)
    {
      ourBallMaskTable[0][size][x + 160] = ourBallMaskTable[0][size][x];
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(uInt32 align = 1; align < 4; ++align)
  {
    for(uInt32 size = 0; size < 4; ++size)
    {
      for(uInt32 x = 0; x < 320; ++x)
      {
        ourBallMaskTable[align][size][x] = 
            ourBallMaskTable[0][size][(x + 320 - align) % 320];
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computeCollisionTable()
{
  for(uInt8 i = 0; i < 64; ++i)
  { 
    ourCollisionTable[i] = 0;

    if((i & myM0Bit) && (i & myP1Bit))    // M0-P1
      ourCollisionTable[i] |= 0x0001;

    if((i & myM0Bit) && (i & myP0Bit))    // M0-P0
      ourCollisionTable[i] |= 0x0002;

    if((i & myM1Bit) && (i & myP0Bit))    // M1-P0
      ourCollisionTable[i] |= 0x0004;

    if((i & myM1Bit) && (i & myP1Bit))    // M1-P1
      ourCollisionTable[i] |= 0x0008;

    if((i & myP0Bit) && (i & myPFBit))    // P0-PF
      ourCollisionTable[i] |= 0x0010;

    if((i & myP0Bit) && (i & myBLBit))    // P0-BL
      ourCollisionTable[i] |= 0x0020;

    if((i & myP1Bit) && (i & myPFBit))    // P1-PF
      ourCollisionTable[i] |= 0x0040;

    if((i & myP1Bit) && (i & myBLBit))    // P1-BL
      ourCollisionTable[i] |= 0x0080;

    if((i & myM0Bit) && (i & myPFBit))    // M0-PF
      ourCollisionTable[i] |= 0x0100;

    if((i & myM0Bit) && (i & myBLBit))    // M0-BL
      ourCollisionTable[i] |= 0x0200;

    if((i & myM1Bit) && (i & myPFBit))    // M1-PF
      ourCollisionTable[i] |= 0x0400;

    if((i & myM1Bit) && (i & myBLBit))    // M1-BL
      ourCollisionTable[i] |= 0x0800;

    if((i & myBLBit) && (i & myPFBit))    // BL-PF
      ourCollisionTable[i] |= 0x1000;

    if((i & myP0Bit) && (i & myP1Bit))    // P0-P1
      ourCollisionTable[i] |= 0x2000;

    if((i & myM0Bit) && (i & myM1Bit))    // M0-M1
      ourCollisionTable[i] |= 0x4000;
  }
    
  for(int i = 64; i < 256; i++)
  { 
        ourCollisionTable[i] = ourCollisionTable[i%64];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computeMissleMaskTable()
{
  // First, calculate masks for alignment 0
  Int32 x, size, number;

  // Clear the missle table to start with
  for(number = 0; number < 8; ++number)
    for(size = 0; size < 4; ++size)
      for(x = 0; x < 160; ++x)
        ourMissleMaskTable[0][number][size][x] = false;

  for(number = 0; number < 8; ++number)
  {
    for(size = 0; size < 4; ++size)
    {
      for(x = 0; x < 160 + 72; ++x)
      {
        // Only one copy of the missle
        if((number == 0x00) || (number == 0x05) || (number == 0x07))
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - close
        else if(number == 0x01)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 16) >= 0) && ((x - 16) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - medium
        else if(number == 0x02)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Three copies - close
        else if(number == 0x03)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 16) >= 0) && ((x - 16) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - wide
        else if(number == 0x04)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 64) >= 0) && ((x - 64) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Three copies - medium
        else if(number == 0x06)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 64) >= 0) && ((x - 64) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
      }

      // Copy data into wrap-around area
      for(x = 0; x < 160; ++x)
        ourMissleMaskTable[0][number][size][x + 160] = 
          ourMissleMaskTable[0][number][size][x];
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(uInt32 align = 1; align < 4; ++align)
  {
    for(number = 0; number < 8; ++number)
    {
      for(size = 0; size < 4; ++size)
      {
        for(x = 0; x < 320; ++x)
        {
          ourMissleMaskTable[align][number][size][x] = 
            ourMissleMaskTable[0][number][size][(x + 320 - align) % 320];
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayerMaskTable()
{
  // First, calculate masks for alignment 0
  Int32 x, enable, mode;

  // Set the player mask table to all zeros
  for(enable = 0; enable < 2; ++enable)
    for(mode = 0; mode < 8; ++mode)
      for(x = 0; x < 160; ++x)
        ourPlayerMaskTable[0][enable][mode][x] = 0x00;

  // Now, compute the player mask table
  for(enable = 0; enable < 2; ++enable)
  {
    for(mode = 0; mode < 8; ++mode)
    {
      for(x = 0; x < 160 + 72; ++x)
      {
        if(mode == 0x00)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
        }
        else if(mode == 0x01)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 16) >= 0) && ((x - 16) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 16);
        }
        else if(mode == 0x02)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
        }
        else if(mode == 0x03)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 16) >= 0) && ((x - 16) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 16);
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
        }
        else if(mode == 0x04)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 64) >= 0) && ((x - 64) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 64);
        }
        else if(mode == 0x05)
        {
          // For some reason in double size mode the player's output
          // is delayed by one pixel thus we use > instead of >=
          if((enable == 0) && (x > 0) && (x <= 16))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> ((x - 1)/2);
        }
        else if(mode == 0x06)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
          else if(((x - 64) >= 0) && ((x - 64) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 64);
        }
        else if(mode == 0x07)
        {
          // For some reason in quad size mode the player's output
          // is delayed by one pixel thus we use > instead of >=
          if((enable == 0) && (x > 0) && (x <= 32))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> ((x - 1)/4);
        }
      }
  
      // Copy data into wrap-around area
      for(x = 0; x < 160; ++x)
      {
        ourPlayerMaskTable[0][enable][mode][x + 160] = 
            ourPlayerMaskTable[0][enable][mode][x];
      }
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(uInt32 align = 1; align < 4; ++align)
  {
    for(enable = 0; enable < 2; ++enable)
    {
      for(mode = 0; mode < 8; ++mode)
      {
        for(x = 0; x < 320; ++x)
        {
          ourPlayerMaskTable[align][enable][mode][x] =
              ourPlayerMaskTable[0][enable][mode][(x + 320 - align) % 320];
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayerPositionResetWhenTable()
{
  uInt32 mode, oldx, newx;

  // Loop through all player modes, all old player positions, and all new
  // player positions and determine where the new position is located:
  // 1 means the new position is within the display of an old copy of the
  // player, -1 means the new position is within the delay portion of an
  // old copy of the player, and 0 means it's neither of these two
  for(mode = 0; mode < 8; ++mode)
  {
    for(oldx = 0; oldx < 160; ++oldx)
    {
      // Set everything to 0 for non-delay/non-display section
      for(newx = 0; newx < 160; ++newx)
      {
        ourPlayerPositionResetWhenTable[mode][oldx][newx] = 0;
      }

      // Now, we'll set the entries for non-delay/non-display section
      for(newx = 0; newx < 160 + 72 + 5; ++newx)
      {
        if(mode == 0x00)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x01)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 16)) && (newx < (oldx + 16 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 16 + 4) && (newx < (oldx + 16 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x02)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x03)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 16)) && (newx < (oldx + 16 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 16 + 4) && (newx < (oldx + 16 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x04)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 64)) && (newx < (oldx + 64 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 64 + 4) && (newx < (oldx + 64 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x05)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 16)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x06)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 64)) && (newx < (oldx + 64 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 64 + 4) && (newx < (oldx + 64 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x07)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 32)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
      }

      // Let's do a sanity check on our table entries
      uInt32 s1 = 0, s2 = 0;
      for(newx = 0; newx < 160; ++newx)
      {
        if(ourPlayerPositionResetWhenTable[mode][oldx][newx] == -1)
          ++s1;
        if(ourPlayerPositionResetWhenTable[mode][oldx][newx] == 1)
          ++s2;
      }
      assert((s1 % 4 == 0) && (s2 % 8 == 0));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayerReflectTable()
{
  for(uInt16 i = 0; i < 256; ++i)
  {
    uInt8 r = 0;

    for(uInt16 t = 1; t <= 128; t *= 2)
    {
      r = (r << 1) | ((i & t) ? 0x01 : 0x00);
    }

    ourPlayerReflectTable[i] = r;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayfieldMaskTable()
{
  Int32 x;

  // Compute playfield mask table for non-reflected mode
  for(x = 0; x < 160; ++x)
  {
    if(x < 16)
    {
      int tmp = 0x00001 << (x / 4);
      ourPlayfieldTable[0][x] = tmp;
      ourPlayfieldTable[1][x] = tmp;
    }
    else if(x < 48)
    {
      int tmp = 0x00800 >> ((x - 16) / 4);
      ourPlayfieldTable[0][x] = tmp;
      ourPlayfieldTable[1][x] = tmp;
    }
    else if(x < 80) 
    {
      int tmp = 0x01000 << ((x - 48) / 4);
      ourPlayfieldTable[0][x] = tmp;
      ourPlayfieldTable[1][x] = tmp;
    }
    else if(x < 96) 
    {
      ourPlayfieldTable[0][x] = 0x00001 << ((x - 80) / 4);
    }
    else if(x < 128)
    {
      ourPlayfieldTable[0][x] = 0x00800 >> ((x - 96) / 4);
    }
    else if(x < 160) 
    {
      ourPlayfieldTable[0][x] = 0x01000 << ((x - 128) / 4);
    }
  }

  // Compute playfield mask table for reflected mode
  for(x = 80; x < 160; ++x)
  {
     if(x < 112) 
      ourPlayfieldTable[1][x] = 0x80000 >> ((x - 80) / 4);
    else if(x < 144) 
      ourPlayfieldTable[1][x] = 0x00010 << ((x - 112) / 4);
    else if(x < 160) 
      ourPlayfieldTable[1][x] = 0x00008 >> ((x - 144) / 4);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::handleObjectsAndCollisions(uInt32 clocksToUpdate, uInt32 hpos)
{
  // See if we're in the vertical blank region
  if(myVBLANK & 0x02)
  {
      // -------------------------------------------------------------------------------------------
      // Some games present a fairly static screen from frame to frame and so there is really no 
      // reason to blank the memory which can be time consuming... so check the flag for the cart
      // currently being emulated...
      // -------------------------------------------------------------------------------------------
      if (myCartInfo.vblankZero) 
      {
          memset(myFramePointer, 0x00, clocksToUpdate);
      }
      myFramePointer += clocksToUpdate;
  }
  else if (myEnabledObjects == 0x00)  // Background handling...
  {      
      memset(myFramePointer, myColor[MYCOLUBK], clocksToUpdate);
      myFramePointer += clocksToUpdate;
  }
  // Handle all other possible combinations - WE DO THIS IN A SPECIFIC ORDER to improve speed... 
  else
  {
      // Calculate the ending frame pointer value
      uInt8* ending = myFramePointer + clocksToUpdate;
      // --------------------------------------------------------------------------------
      // Ball and Playfield (very common) this is all stuff that starts with 0x30..0x3F
      // --------------------------------------------------------------------------------
      if ((myEnabledObjects & (myPFBit | myBLBit)) == (myPFBit | myBLBit)) 
      {
          if (myEnabledObjects == (myPFBit | myBLBit)) // Playfield and Ball only are enabled
          {
                uInt32* mPF = &myCurrentPFMask[hpos];
                uInt8* mBL = &myCurrentBLMask[hpos];

                while(myFramePointer < ending)
                {
                    if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL)
                    {
                        *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                        mPF += 4; mBL += 4; myFramePointer += 4;
                    }
                    else
                    {
                        *myFramePointer = ((myPF & *mPF) || *mBL) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                        if((myPF & *mPF) && *mBL) myCollision |= ourCollisionTable[myPFBit | myBLBit];
                        ++mPF; ++mBL; ++myFramePointer;
                    }
                }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit | myP0Bit)) // Playfield and Ball plus Player 1 and Player 0 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myP0Bit)) // Playfield and Ball plus Player 0 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM0Bit)) // Playfield and Ball plus Missile 0 enabled... (Elevators Amiss)
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit)) // Playfield and Ball plus Missile 1 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit | myM0Bit)) // Playfield and Ball plus Player 1 and Missile 0 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP1Bit)) // Playfield and Ball plus Missle 1 and Player 1
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myP0Bit | myM0Bit | myM1Bit)) // Playfield and Ball plus Player 0 and Missile 0 plus Missile 1 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP1Bit | myM0Bit)) 
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }       
          else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit | myM0Bit | myP0Bit)) // Playfield and Ball plus P1, M0 and P0
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }       
          else if (myEnabledObjects == (myPFBit | myBLBit | myP0Bit | myM0Bit)) // Playfield and Ball plus Player 0 and Missile 0 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit)) // Playfield and Ball plus Player 1 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM0Bit | myM1Bit)) // Playfield and Ball plus Missile 0 and Missile 1 enabled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(*mM0++)                        enabled |= myM0Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP0Bit)) // Playfield and Ball plus Missle 1 and Player 0
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP1Bit | myM0Bit | myP0Bit)) // Everything is enbaled...
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }       
          else  // Unsure... need to check them all... this is slow.
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            uInt8 meo1 = (myEnabledObjects & myBLBit);
            uInt8 meo2 = (myEnabledObjects & myM1Bit);
            uInt8 meo3 = (myEnabledObjects & myM0Bit);
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(meo1 && *mBL++)                enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(meo2 && *mM1++)                enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(meo3 && *mM0++)                enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }  
      }
      // -------------------------------------------------------------------------
      // Player 1 and Player 0 are both set (fairly common)
      // -------------------------------------------------------------------------
      else if ((myEnabledObjects & (myP0Bit | myP1Bit)) == (myP0Bit | myP1Bit)) 
      {
          if (myEnabledObjects == (myP0Bit | myP1Bit)) // Player 0 and 1 is enabled only...
          {
                uInt8* mP0 = &myCurrentP0Mask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0 && !*(uInt32*)mP1)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mP0 += 4; mP1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                        myColor[MYCOLUP0] : ((myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);

                    if((myCurrentGRP0 & *mP0) && (myCurrentGRP1 & *mP1))
                      myCollision |= ourCollisionTable[myP0Bit | myP1Bit];

                    ++mP0; ++mP1; ++myFramePointer;
                  }
                }
          }
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit))   // Player 0 and 1 plus Playfield
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit | myM1Bit)) // Player 0 and 1 plus Playfield plus Missile 1
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }          
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myBLBit | myM1Bit)) // Player 0 and 1 plus Ball plus Missile 1
          {
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];              
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }          
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myM1Bit)) // Player 0 and 1 plus Missile 1
          {
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }          
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit | myM0Bit)) // // Player 0 and 1 plus Playfield plus Missile 0
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }  
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myM0Bit | myBLBit | myPFBit)) // // Player 0 and 1 plus Playfield plus Missile 0 plus Ball
          {
                uInt32*mPF = &myCurrentPFMask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];
                uInt8* mP0 = &myCurrentP0Mask[hpos];
                uInt8* mM0 = &myCurrentM0Mask[hpos];
                uInt8* mBL = &myCurrentBLMask[hpos];
                for(; myFramePointer < ending; ++myFramePointer)
                {
                  uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
                  if(*mBL++)                        enabled |= myBLBit;
                  if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
                  if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
                  if(*mM0++)                        enabled |= myM0Bit;
                  myCollision |= ourCollisionTable[enabled];
                  if (hpos < 80)
                  {
                      *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                      hpos++;
                  }
                  else
                  {
                      *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
                  }
                }
          }
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myM0Bit | myM1Bit | myBLBit)) // Player 0/1 plus Missile 0/1 plus Ball enabled...
          {
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myBLBit)) // // Player 0 and 1 plus Ball
          {
                uInt8* mP1 = &myCurrentP1Mask[hpos];
                uInt8* mP0 = &myCurrentP0Mask[hpos];
                uInt8* mBL = &myCurrentBLMask[hpos];
                for(; myFramePointer < ending; ++myFramePointer)
                {
                  uInt8 enabled = myPlayfieldPriorityAndScore;
                  if(*mBL++)                        enabled |= myBLBit;
                  if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
                  if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
                  myCollision |= ourCollisionTable[enabled];
                  if (hpos < 80)
                  {
                      *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                      hpos++;
                  }
                  else
                  {
                      *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
                  }
                }
          }
          else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit | myM0Bit | myM1Bit)) // // Playfield plus M0, M1, P0, P1
          {
                uInt32*mPF = &myCurrentPFMask[hpos];
                uInt8* mP0 = &myCurrentP0Mask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];
                uInt8* mM0 = &myCurrentM0Mask[hpos];
                uInt8* mM1 = &myCurrentM1Mask[hpos];
                for(; myFramePointer < ending; ++myFramePointer)
                {
                  uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
                  if(*mM0++)                        enabled |= myM0Bit;
                  if(*mM1++)                        enabled |= myM1Bit;
                  if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
                  if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
                  myCollision |= ourCollisionTable[enabled];
                  if (hpos < 80)
                  {
                      *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                      hpos++;
                  }
                  else
                  {
                      *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
                  }
                }
          }
          else if (myEnabledObjects == (myP0Bit | myM0Bit | myP1Bit)) // Player 0 and Missile 0 and Missile 1 are enabled
          {
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];            
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }          
          else // Unsure... need to check them all... this is slow.
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            uInt8 meo1 = (myEnabledObjects & myBLBit);
            uInt8 meo2 = (myEnabledObjects & myM1Bit);
            uInt8 meo3 = (myEnabledObjects & myM0Bit);
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(meo1 && *mBL++)                enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(meo2 && *mM1++)                enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(meo3 && *mM0++)                enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
      }
      // -------------------------------------------------------------------------
      // Playfield Bit is set... (fairly common)
      // -------------------------------------------------------------------------
      else if ((myEnabledObjects & (myPFBit)) == myPFBit) // Playfield is Set (fairly common)
      {
          if (myEnabledObjects == myPFBit) // Playfield bit set... with or without score
          {
              uInt32* mask = &myCurrentPFMask[hpos];
              if (myPlayfieldPriorityAndScore & PriorityBit)   // Priority bit overrides the Score bit
              {
                // Update a uInt8 at a time until reaching a uInt32 boundary
                for(; ((uintptr_t)myFramePointer & 0x03) && (myFramePointer < ending); ++myFramePointer, ++mask)
                {
                  *myFramePointer = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                }

                // Now, update a uInt32 at a time
                for(; myFramePointer < ending; myFramePointer += 4, mask += 4)
                {
                  *((uInt32*)myFramePointer) = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                }
              }
              else if (myPlayfieldPriorityAndScore & ScoreBit)   // Playfield is enabled and the score bit is set 
              {
                // Update a uInt8 at a time until reaching a uInt32 boundary
                for(; ((uintptr_t)myFramePointer & 0x03) && (myFramePointer < ending); ++myFramePointer, ++mask, ++hpos)
                {
                  *myFramePointer = (myPF & *mask) ? (hpos < 80 ? myColor[MYCOLUP0] : myColor[MYCOLUP1]) : myColor[MYCOLUBK];
                }

                // Now, update a uInt32 at a time
                for(; myFramePointer < ending;  myFramePointer += 4, mask += 4, hpos += 4)
                {
                  *((uInt32*)myFramePointer) = (myPF & *mask) ? (hpos < 80 ? myColor[MYCOLUP0] : myColor[MYCOLUP1]) : myColor[MYCOLUBK];
                }
              }          
              else  // Playfield is enabled and the score bit is not set and priority clear...
              {
                // Update a uInt8 at a time until reaching a uInt32 boundary
                for(; ((uintptr_t)myFramePointer & 0x03) && (myFramePointer < ending); ++myFramePointer, ++mask)
                {
                  *myFramePointer = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                }

                // Now, update a uInt32 at a time
                for(; myFramePointer < ending; myFramePointer += 4, mask += 4)
                {
                  *((uInt32*)myFramePointer) = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                }
              }
          }      
          else if (myEnabledObjects == (myM0Bit | myPFBit)) // Playfield + Missile 0
          {
                uInt32*mPF = &myCurrentPFMask[hpos];
                uInt8* mM0 = &myCurrentM0Mask[hpos];
                uInt8 meo3 = (myEnabledObjects & myM0Bit);
                for(; myFramePointer < ending; ++myFramePointer)
                {
                  uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
                  if(meo3 && *mM0++)                enabled |= myM0Bit;
                  myCollision |= ourCollisionTable[enabled];
                  if (hpos < 80)
                  {
                      *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                      hpos++;
                  }
                  else
                  {
                      *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
                  }
                }
          }
          else if (myEnabledObjects == (myPFBit | myP0Bit)) // Playfield and Player 0 are enabled 
          {
              if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
              {
                uInt32* mPF = &myCurrentPFMask[hpos];
                uInt8* mP0 = &myCurrentP0Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
                  {
                    *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                    mPF += 4; mP0 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : 
                        ((myCurrentGRP0 & *mP0) ? myColor[MYCOLUP0] : myColor[MYCOLUBK]);

                    if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
                      myCollision |= ourCollisionTable[myPFBit | myP0Bit];

                    ++mPF; ++mP0; ++myFramePointer;
                  }
                }
              }
              else // Priority not set
              {
                uInt32* mPF = &myCurrentPFMask[hpos];
                uInt8* mP0 = &myCurrentP0Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
                  {
                    *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                    mPF += 4; mP0 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                          myColor[MYCOLUP0] : ((myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK]);

                    if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
                      myCollision |= ourCollisionTable[myPFBit | myP0Bit];

                    ++mPF; ++mP0; ++myFramePointer;
                  }
                }
              }
          }
          else if (myEnabledObjects == (myPFBit | myP1Bit)) // Playfield and Player 1 are enabled 
          {
              if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
              {
                uInt32* mPF = &myCurrentPFMask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
                  {
                    *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                    mPF += 4; mP1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : 
                        ((myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);

                    if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
                      myCollision |= ourCollisionTable[myPFBit | myP1Bit];

                    ++mPF; ++mP1; ++myFramePointer;
                  }
                }
              }
              else // Priority not set
              {
                uInt32* mPF = &myCurrentPFMask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
                  {
                    *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                    mPF += 4; mP1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (myCurrentGRP1 & *mP1) ? 
                          myColor[MYCOLUP1] : ((myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK]);

                    if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
                      myCollision |= ourCollisionTable[myPFBit | myP1Bit];

                    ++mPF; ++mP1; ++myFramePointer;
                  }
                }
              }
          }          
          else if (myEnabledObjects == (myPFBit | myP1Bit | myM0Bit)) // Playfield and Player 1 and Missile 0 are enabled 
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }      
          else if (myEnabledObjects == (myPFBit | myP0Bit | myM0Bit)) // Playfield and Player 0 and Missile 0 are enabled 
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myM1Bit | myM0Bit)) // Playfield, Missile 1, Missile 0
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mM1++)                        enabled |= myM1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myM1Bit)) // Playfield, Missile 1
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myM1Bit | myP1Bit)) // Playfield, Missile 1, Player 1
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mM1++)                        enabled |= myM1Bit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myM1Bit | myM0Bit | myP0Bit)) // Playfield, Missile 0, Missile 1, Player 1
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mM1++)                        enabled |= myM1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myPFBit | myM1Bit | myM0Bit | myP1Bit)) // Playfield, Missile 0, Missile 1, Player 1
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(*mM1++)                        enabled |= myM1Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else // Catch-all
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            uInt8 meo1 = (myEnabledObjects & myBLBit);
            uInt8 meo2 = (myEnabledObjects & myM1Bit);
            uInt8 meo3 = (myEnabledObjects & myM0Bit);
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(meo1 && *mBL++)                enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(meo2 && *mM1++)                enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(meo3 && *mM0++)                enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          } 
      }
      // -------------------------------------------------------------------------
      // Ball is set (fairly common) - but not playfield
      // -------------------------------------------------------------------------
      else if ((myEnabledObjects & (myBLBit)) == myBLBit) // Ball is set (fairly common)
      {
          if (myEnabledObjects == myBLBit) // Ball is enabled (Official Frogger)
          {
                uInt8* mBL = &myCurrentBLMask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = *mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                    ++mBL; ++myFramePointer;
                  }
                }
          }          
          else if (myEnabledObjects == (myBLBit | myP0Bit)) // Ball + Player 0 (freeway)
          {
                uInt8* mP0 = &myCurrentP0Mask[hpos];
                uInt8* mBL = &myCurrentBLMask[hpos];
                for(; myFramePointer < ending; ++myFramePointer)
                {
                  uInt8 enabled = myPlayfieldPriorityAndScore;
                  if(*mBL++)                        enabled |= myBLBit;
                  if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
                  myCollision |= ourCollisionTable[enabled];
                  if (hpos < 80)
                  {
                      *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                      hpos++;
                  }
                  else
                  {
                      *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
                  }
                }
          }
          else if (myEnabledObjects == (myBLBit | myP0Bit | myM0Bit)) // Ball plus Player 0 and Missile 0 are enabled (Super Breakout!!)
          {
            uInt8* mBL = &myCurrentBLMask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(*mM0++)                        enabled |= myM0Bit;
              if(*mBL++)                        enabled |= myBLBit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }
          else if (myEnabledObjects == (myBLBit | myM0Bit)) // Ball and Missle 0 are enabled
          {
              if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
              {
                uInt8* mBL = &myCurrentBLMask[hpos];
                uInt8* mM0 = &myCurrentM0Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; mM0 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (*mBL ? myColor[MYCOLUPF] : (*mM0 ? myColor[MYCOLUP0] : myColor[MYCOLUBK]));

                    if(*mBL && *mM0)
                      myCollision |= ourCollisionTable[myBLBit | myM0Bit];

                    ++mBL; ++mM0; ++myFramePointer;
                  }
                }
              }
              else  // Priority not set
              {
                uInt8* mBL = &myCurrentBLMask[hpos];
                uInt8* mM0 = &myCurrentM0Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; mM0 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (*mM0 ? myColor[MYCOLUP0] : (*mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK]));

                    if(*mBL && *mM0)
                      myCollision |= ourCollisionTable[myBLBit | myM0Bit];

                    ++mBL; ++mM0; ++myFramePointer;
                  }
                }
              }          
          }
          else if (myEnabledObjects == (myBLBit | myM1Bit)) // Ball and Missle 1 are enabled
          {
              if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
              {
                uInt8* mBL = &myCurrentBLMask[hpos];
                uInt8* mM1 = &myCurrentM1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && 
                      !*(uInt32*)mM1)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; mM1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (*mBL ? myColor[MYCOLUPF] : (*mM1 ? myColor[MYCOLUP1] : myColor[MYCOLUBK]));

                    if(*mBL && *mM1)
                      myCollision |= ourCollisionTable[myBLBit | myM1Bit];

                    ++mBL; ++mM1; ++myFramePointer;
                  }
                }
              }
              else  // Priority not set
              {
                uInt8* mBL = &myCurrentBLMask[hpos];
                uInt8* mM1 = &myCurrentM1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && 
                      !*(uInt32*)mM1)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; mM1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (*mM1 ? myColor[MYCOLUP1] : (*mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK]));

                    if(*mBL && *mM1)
                      myCollision |= ourCollisionTable[myBLBit | myM1Bit];

                    ++mBL; ++mM1; ++myFramePointer;
                  }
                }
              }
          }
          else if (myEnabledObjects == (myBLBit | myP1Bit)) // Ball and Player 1 are enabled
          {
              if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
              {
                uInt8* mBL = &myCurrentBLMask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; mP1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = *mBL ? myColor[MYCOLUPF] : 
                        ((myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);

                    if(*mBL && (myCurrentGRP1 & *mP1))
                      myCollision |= ourCollisionTable[myBLBit | myP1Bit];

                    ++mBL; ++mP1; ++myFramePointer;
                  }
                }
              }
              else // Priority not set
              {
                uInt8* mBL = &myCurrentBLMask[hpos];
                uInt8* mP1 = &myCurrentP1Mask[hpos];

                while(myFramePointer < ending)
                {
                  if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
                  {
                    *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                    mBL += 4; mP1 += 4; myFramePointer += 4;
                  }
                  else
                  {
                    *myFramePointer = (myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : 
                        (*mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK]);

                    if(*mBL && (myCurrentGRP1 & *mP1))
                      myCollision |= ourCollisionTable[myBLBit | myP1Bit];

                    ++mBL; ++mP1; ++myFramePointer;
                  }
                }
              }
          }      
          else if (myEnabledObjects == (myBLBit | myP1Bit | myM1Bit)) // Ball, Player 1, Missile 1
          {
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(*mBL++)                        enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(*mM1++)                        enabled |= myM1Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }          
          else if (myEnabledObjects == (myBLBit | myM0Bit | myM1Bit)) // Ball + Missile 0/1
          {
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = myPlayfieldPriorityAndScore;
              if(*mBL++)                enabled |= myBLBit;
              if(*mM1++)                enabled |= myM1Bit;
              if(*mM0++)                enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          }          
          else // Catch-all
          {
            uInt32*mPF = &myCurrentPFMask[hpos];
            uInt8* mP1 = &myCurrentP1Mask[hpos];
            uInt8* mP0 = &myCurrentP0Mask[hpos];
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];
            uInt8* mBL = &myCurrentBLMask[hpos];
            uInt8 meo1 = (myEnabledObjects & myBLBit);
            uInt8 meo2 = (myEnabledObjects & myM1Bit);
            uInt8 meo3 = (myEnabledObjects & myM0Bit);
            for(; myFramePointer < ending; ++myFramePointer)
            {
              uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
              if(meo1 && *mBL++)                enabled |= myBLBit;
              if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
              if(meo2 && *mM1++)                enabled |= myM1Bit;
              if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
              if(meo3 && *mM0++)                enabled |= myM0Bit;
              myCollision |= ourCollisionTable[enabled];
              if (hpos < 80)
              {
                  *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
                  hpos++;
              }
              else
              {
                  *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
              }
            }
          } 
      }
      // -------------------------------------------------------------------------
      // Otherwise we do the rest of the simple compares below... 
      // -------------------------------------------------------------------------
      else if (myEnabledObjects == myP0Bit) // Player 0 is enabled
      {
            uInt8* mP0 = &myCurrentP0Mask[hpos];

            while(myFramePointer < ending)
            {
              if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
              {
                *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                mP0 += 4; myFramePointer += 4;
              }
              else
              {
                *myFramePointer = (myCurrentGRP0 & *mP0) ? myColor[MYCOLUP0] : myColor[MYCOLUBK];
                ++mP0; ++myFramePointer;
              }
            }
      }
      else if (myEnabledObjects == myP1Bit) // Player 1 is enabled
      {
            uInt8* mP1 = &myCurrentP1Mask[hpos];

            while(myFramePointer < ending)
            {
              if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
              {
                *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                mP1 += 4; myFramePointer += 4;
              }
              else
              {
                *myFramePointer = (myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK];
                ++mP1; ++myFramePointer;
              }
            }
      }
      else if (myEnabledObjects == myM0Bit) // Missile 0 is enabled
      {
            uInt8* mM0 = &myCurrentM0Mask[hpos];

            while(myFramePointer < ending)
            {
              if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM0)
              {
                *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                mM0 += 4; myFramePointer += 4;
              }
              else
              {
                *myFramePointer = *mM0 ? myColor[MYCOLUP0] : myColor[MYCOLUBK];
                ++mM0; ++myFramePointer;
              }
            }
      }
      else if (myEnabledObjects == myM1Bit) // Missile 1 is enabled
      {
            uInt8* mM1 = &myCurrentM1Mask[hpos];

            while(myFramePointer < ending)
            {
              if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM1)
              {
                *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                mM1 += 4; myFramePointer += 4;
              }
              else
              {
                *myFramePointer = *mM1 ? myColor[MYCOLUP1] : myColor[MYCOLUBK];
                ++mM1; ++myFramePointer;
              }
            }
      }
      else if (myEnabledObjects == (myP0Bit | myM0Bit)) // Player 0 and Missile 0 are enabled (Super Breakout!!)
      {
        uInt8* mP0 = &myCurrentP0Mask[hpos];
        uInt8* mM0 = &myCurrentM0Mask[hpos];
        for(; myFramePointer < ending; ++myFramePointer)
        {
          uInt8 enabled = myPlayfieldPriorityAndScore;
          if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
          if(*mM0++)                        enabled |= myM0Bit;
          myCollision |= ourCollisionTable[enabled];
          if (hpos < 80)
          {
              *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
              hpos++;
          }
          else
          {
              *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
          }
        }
      }
      else if (myEnabledObjects == (myM0Bit | myM1Bit)) // Missile 0 and 1 is enabled
      {
            uInt8* mM0 = &myCurrentM0Mask[hpos];
            uInt8* mM1 = &myCurrentM1Mask[hpos];

            while(myFramePointer < ending)
            {
              if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM0 && !*(uInt32*)mM1)
              {
                *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                mM0 += 4; mM1 += 4; myFramePointer += 4;
              }
              else
              {
                *myFramePointer = *mM0 ? myColor[MYCOLUP0] : (*mM1 ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);

                if(*mM0 && *mM1)
                  myCollision |= ourCollisionTable[myM0Bit | myM1Bit];

                ++mM0; ++mM1; ++myFramePointer;
              }
            }
      }      
      else if (myEnabledObjects == (myP1Bit | myM1Bit)) // Player 1, Missile 1
      {
        uInt8* mP1 = &myCurrentP1Mask[hpos];
        uInt8* mM1 = &myCurrentM1Mask[hpos];
        for(; myFramePointer < ending; ++myFramePointer)
        {
          uInt8 enabled = myPlayfieldPriorityAndScore;
          if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
          if(*mM1++)                        enabled |= myM1Bit;
          myCollision |= ourCollisionTable[enabled];
          if (hpos < 80)
          {
              *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
              hpos++;
          }
          else
          {
              *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
          }
        }
      }
      else // Catch-all... this is a bit slow so do this only if no matches above...
      {
        uInt32*mPF = &myCurrentPFMask[hpos];
        uInt8* mP1 = &myCurrentP1Mask[hpos];
        uInt8* mP0 = &myCurrentP0Mask[hpos];
        uInt8* mM0 = &myCurrentM0Mask[hpos];
        uInt8* mM1 = &myCurrentM1Mask[hpos];
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8 meo1 = (myEnabledObjects & myBLBit);
        uInt8 meo2 = (myEnabledObjects & myM1Bit);
        uInt8 meo3 = (myEnabledObjects & myM0Bit);
        for(; myFramePointer < ending; ++myFramePointer)
        {
          uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
          if(meo1 && *mBL++)                enabled |= myBLBit;
          if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
          if(meo2 && *mM1++)                enabled |= myM1Bit;
          if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
          if(meo3 && *mM0++)                enabled |= myM0Bit;
          myCollision |= ourCollisionTable[enabled];
          if (hpos < 80)
          {
              *myFramePointer = myColor[myPriorityEncoder[0][enabled]];
              hpos++;
          }
          else
          {
              *myFramePointer = myColor[myPriorityEncoder[1][enabled]];
          }
        }
      }        
      myFramePointer = ending;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void TIA::updateFrame(Int32 clock)
{
  // See if we're in the nondisplayable portion of the screen or if
  // we've already updated this portion of the screen
  // We do these in the order of "most likely"
  if (clock < myClockStartDisplay)               return;
  if (myClockAtLastUpdate >= myClockStopDisplay) return;
  if (myClockAtLastUpdate >= clock)              return;

  // Truncate the number of cycles to update to the stop display point
  if(clock > myClockStopDisplay)
  {
    clock = myClockStopDisplay;
  }

  // Update frame one scanline at a time
  do
  {
    // Compute the number of clocks we're going to update
    Int32 clocksToUpdate = clock - myClockAtLastUpdate;

    // Remember how many clocks we are from the left side of the screen
    Int32 clocksFromStartOfScanLine = 228 - myClocksToEndOfScanLine;

    // See if we're updating more than the current scanline
    if(clock > (myClockAtLastUpdate + myClocksToEndOfScanLine))
    {
      // Yes, we have more than one scanline to update so finish current one
      clocksToUpdate = myClocksToEndOfScanLine;
      myClocksToEndOfScanLine = 228;
      myClockAtLastUpdate += clocksToUpdate;
    }
    else
    {
      // No, so do as much of the current scanline as possible
      myClocksToEndOfScanLine -= clocksToUpdate;
      myClockAtLastUpdate = clock;
    }

    // Skip over as many horizontal blank clocks as we can
    if(clocksFromStartOfScanLine < HBLANK)
    {
      uInt32 tmp;

      if((HBLANK - clocksFromStartOfScanLine) < clocksToUpdate)
        tmp = HBLANK - clocksFromStartOfScanLine;
      else
        tmp = clocksToUpdate;

      clocksFromStartOfScanLine += tmp;
      clocksToUpdate -= tmp;
    }

    // Remember frame pointer in case HMOVE blanks need to be handled
    uInt8* oldFramePointer = myFramePointer;

    // ----------------------------------------------------------------
    // Update as much of the scanline as we can. This is slow so
    // this routine has been optimized to handle objects in a
    // sort of "best" order...
    // ----------------------------------------------------------------
    handleObjectsAndCollisions(clocksToUpdate, clocksFromStartOfScanLine - HBLANK);

    // Handle HMOVE blanks if they are enabled
    if(myHMOVEBlankEnabled && (clocksFromStartOfScanLine < (HBLANK + 8)))
    {
        Int32 blanks = (HBLANK + 8) - clocksFromStartOfScanLine;
        memset(oldFramePointer, 0, blanks);

        if((clocksToUpdate + clocksFromStartOfScanLine) >= (HBLANK + 8))
        {
            myHMOVEBlankEnabled = false;
        }
    }
      
    // ------------------------------------------------------------------------
    // If we are mid-scanline... we record the background color for blending
    // ------------------------------------------------------------------------
    if (myClocksToEndOfScanLine < 190)
    {
        myBlendBk = myColor[MYCOLUBK];  
    }
    else  
    // See if we're at the end of a scanline
    if(myClocksToEndOfScanLine == 228)
    {
      // Yes, so set PF mask based on current CTRLPF reflection state 
      myCurrentPFMask = ourPlayfieldTable[myCTRLPF & 0x01];

      // TODO: These should be reset right after the first copy of the player
      // has passed.  However, for now we'll just reset at the end of the 
      // scanline since the other way would be to slow (01/21/99).
      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
        
      // Handle the "Cosmic Ark" TIA bug if it's enabled
      if(myM0CosmicArkMotionEnabled)
      {
        // Movement table associated with the bug
        static uInt32 m[4] = {18, 33, 0, 17};

        myM0CosmicArkCounter = (myM0CosmicArkCounter + 1) & 3;
        myPOSM0 -= m[myM0CosmicArkCounter];

        if(myPOSM0 >= 160)
          myPOSM0 -= 160;
        else if(myPOSM0 < 0)
          myPOSM0 += 160;

        if(myM0CosmicArkCounter == 1)
        {
          // Stretch this missle so it's at least 2 pixels wide
          myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][((myNUSIZ0 & 0x30) >> 4) | 0x01]
              [160 - (myPOSM0 & 0xFC)];
        }
        else if(myM0CosmicArkCounter == 2)
        {
          // Missle is disabled on this line 
          myCurrentM0Mask = &ourDisabledMaskTable[0];
        }
        else
        {
          myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
        }
      }
        
      // --------------------------------------------------------------------------
      // And now we must render this onto the DS screen... but first we check
      // if the cart info says we are any sort of flicker-free or flicker-reduce
      // and blend the frames... this is a bit slow so use with caution.
      // --------------------------------------------------------------------------
      if (myCartInfo.mode == MODE_FF)
      {
          int addr = (myFramePointer - myCurrentFrameBuffer[myCurrentFrame]);
          addr += 160;
          uInt32 *fp1 = (uInt32 *)(&myCurrentFrameBuffer[0][addr]);
          uInt32 *fp2 = (uInt32 *)(&myCurrentFrameBuffer[1][addr]);
          // "steal" a bit of VRAM which has no cache write... 
          uInt32 *fp_blend = (uInt32 *)0x0601E000;
          for (int i=0; i<40; i++)
          {
            *fp_blend++ = *fp1++ | *fp2++;
          }
          dma_channel = 1-dma_channel;
          dmaCopyWordsAsynch(dma_channel, (uInt32 *)0x0601E000, myDSFramePointer, 160);
      }
      else if (myCartInfo.mode == MODE_BACKG)
      {
          int addr = (myFramePointer - myCurrentFrameBuffer[myCurrentFrame]);
          addr += 160;
          uInt32 *fp1 = (uInt32 *)(&myCurrentFrameBuffer[myCurrentFrame][addr]);
          uInt32 *fp2 = (uInt32 *)(&myCurrentFrameBuffer[1-myCurrentFrame][addr]);
          // "steal" a bit of VRAM which has no cache write... 
          uInt32 *fp_blend = (uInt32 *)0x0601E000;
          for (int i=0; i<40; i++)
          {
            if (*fp1 == myBlendBk) *fp_blend++ = *fp2;          // mid-screen background - use previous frame
            else *fp_blend++ = *fp1;                            // Use current frame 
            fp1++;fp2++;
          }
          dma_channel = 1-dma_channel;
          dmaCopyWordsAsynch(dma_channel, (uInt32 *)0x0601E000, myDSFramePointer, 160);
      }
      else if (myCartInfo.mode == MODE_BLACK)
      {
          int addr = (myFramePointer - myCurrentFrameBuffer[myCurrentFrame]);
          addr += 160;
          uInt32 *fp1 = (uInt32 *)(&myCurrentFrameBuffer[myCurrentFrame][addr]);
          uInt32 *fp2 = (uInt32 *)(&myCurrentFrameBuffer[1-myCurrentFrame][addr]);
          // "steal" a bit of VRAM which has no cache write... 
          uInt32 *fp_blend = (uInt32 *)0x0601E000;
          for (int i=0; i<40; i++)
          {
            if (*fp1 == 0x000000) *fp_blend++ = *fp2;           // Black background - use previous frame
            else *fp_blend++ = *fp1;                            // Use current frame 
            fp1++;fp2++;
          }
          dma_channel = 1-dma_channel;
          dmaCopyWordsAsynch(dma_channel, (uInt32 *)0x0601E000, myDSFramePointer, 160);
      }
      else
      {
          // ------------------------------------------------------------------------------------------------------------------------
          // To help with caching issues and DMA transfers, we are actually copying the 160 pixel scanline of the previous frame.
          // By using slightly "stale" data, we ensure that we are outputting the right data and not something previously cached.
          // DMA and ARM9 is tricky stuff... I'll admit I don't fully understand it and there is some voodoo... but this works.
          // ------------------------------------------------------------------------------------------------------------------------
          dma_channel = 1-dma_channel;
          dmaCopyWordsAsynch(dma_channel, myFramePointer+160, myDSFramePointer, 160);   
      }
      myDSFramePointer += 128;  // 16-bit address... so this is 256 bytes
    }
  } 
  while(myClockAtLastUpdate < clock);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE uInt8 TIA::peek(uInt16 addr)
{
    uInt8 noise;
    if (myCartInfo.special == SPEC_CONMARS) noise = 0x02; //  [fix for games like Conquest of Mars which incorrectly assume the lower bits]
    else noise = myDataBusState & 0x3F; 
    
    addr &= 0x000F;
    
    if (addr < 8)
    {
         // Update frame to current color clock before we look at anything!
        updateFrame((3*gSystemCycles));
        switch(addr)
        {
        case 0x00:    // CXM0P
          if (myCollision & 0x0001) noise |= 0x80;
          if (myCollision & 0x0002) noise |= 0x40;
          return noise;
        case 0x01:    // CXM1P
          if (myCollision & 0x0004) noise |= 0x80;
          if (myCollision & 0x0008) noise |= 0x40;
          return noise;
        case 0x02:    // CXP0FB
          if (myCollision & 0x0010) noise |= 0x80;
          if (myCollision & 0x0020) noise |= 0x40;
          return noise;
        case 0x03:    // CXP1FB
          if (myCollision & 0x0040) noise |= 0x80;
          if (myCollision & 0x0080) noise |= 0x40;
          return noise;
        case 0x04:    // CXM0FB
          if (myCollision & 0x0100) noise |= 0x80;
          if (myCollision & 0x0200) noise |= 0x40;
          return noise;
        case 0x05:    // CXM1FB
          if (myCollision & 0x0400) noise |= 0x80;
          if (myCollision & 0x0800) noise |= 0x40;
          return noise;
        case 0x06:    // CXBLPF
          if (myCollision & 0x1000) noise |= 0x80;
          return noise;
        case 0x07:    // CXPPMM
          if (myCollision & 0x2000) noise |= 0x80;
          if (myCollision & 0x4000) noise |= 0x40;
          return noise;
        }
    }

    // ----------------------------------------------------------------
    // Otherwise it's an input read... don't need to update frame yet.
    // ----------------------------------------------------------------
    switch (addr)
    {
    case 0x08:    // INPT0
    {
      Int32 r = myConsole.controller(Controller::Left).read(Controller::Nine);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        uInt32 needed = (r*10) / 525; //52.52 actually
        if((uInt32)gSystemCycles > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x09:    // INPT1
    {
      Int32 r = myConsole.controller(Controller::Left).read(Controller::Five);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        uInt32 needed = (r*10) / 525; //52.52 actually
        if((uInt32)gSystemCycles > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x0A:    // INPT2
    {
      Int32 r = myConsole.controller(Controller::Right).read(Controller::Nine);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        uInt32 needed = (r*10) / 525; //52.52 actually
        if((uInt32)gSystemCycles > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x0B:    // INPT3
    {
      Int32 r = myConsole.controller(Controller::Right).read(Controller::Five);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        uInt32 needed = (r*10) / 525; //52.52 actually
        if((uInt32)gSystemCycles > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }          

    case 0x0C:    // INPT4
      return myConsole.controller(Controller::Left).read(Controller::Six) ? (0x80 | noise) : noise;

    case 0x0D:    // INPT5
      return myConsole.controller(Controller::Right).read(Controller::Six) ? (0x80 | noise) : noise;

    default:
      return noise;
    }
}

uInt8 poke_needs_update_display[] __attribute__((section(".dtcm"))) =
{
    1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1,
    1,1,1,1,1,0,0,0,   0,0,0,1,1,1,1,1,
    1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1
};

uInt8 player_reset_pos[] =
{
  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   3,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
 17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
 37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,
 57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,
 77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
 97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156,
157, 158, 159,   0,   1,   2,   3,   4 
};

ITCM_CODE void TIA::poke(uInt16 addr, uInt8 value)
{
  addr = addr & 0x003f;

  Int32 clock = (3*gSystemCycles); 
  Int32 delta_clock = (clock - myClockWhenFrameStarted);

  // Update frame to current CPU cycle before we make any changes!
  if (poke_needs_update_display[addr])
  {
      Int8 delay = ourPokeDelayTable[addr];
      // See if this is a poke to a PF register
      if(delay == -1)
      {
        delay = delay_tab[delta_clock % 228];
      }
      updateFrame(clock + delay);
  }
    
  switch(addr)
  {
    case 0x00:    // Vertical sync set-clear
    {
      myVSYNC = value;

      if(myVSYNC & 0x02)
      {
        // Indicate when VSYNC should be finished.  This should really 
        // be 3 * 228 according to Atari's documentation, however, some 
        // games don't supply the full 3 scanlines of VSYNC.
        myVSYNCFinishClock = clock + 228;
      }
      else if(clock >= myVSYNCFinishClock)
      {
        // We're no longer interested in myVSYNCFinishClock
        myVSYNCFinishClock = 0x7FFFFFFF;

        // Since we're finished with the frame tell the processor to halt
        mySystem->m6502().stop();
      }
      break;
    }

    case 0x01:    // Vertical blank set-clear
    {
      // Is the dump to ground path being set for I0, I1, I2, and I3?
      if(!(myVBLANK & 0x80) && (value & 0x80))
      {
        myDumpEnabled = true;
      }
      else
      // Is the dump to ground path being removed from I0, I1, I2, and I3?
      if((myVBLANK & 0x80) && !(value & 0x80))
      {
        myDumpEnabled = false;
        myDumpDisabledCycle = gSystemCycles;
      }

      myVBLANK = value;
      break;
    }

    case 0x02:    // Wait for leading edge of HBLANK
    {
        // Tell the cpu to waste the necessary amount of time
        uInt32 cyclesToEndOfLine = 76 - ((gSystemCycles - myCyclesWhenFrameStarted) % 76);

        if (cyclesToEndOfLine < 76)
        {
            gSystemCycles += cyclesToEndOfLine;
        }
        break;
    }

    case 0x03:    // Reset horizontal sync counter
    {
      break;
    }

    case 0x04:    // Number-size of player-missle 0
    {
      myNUSIZ0 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];

      break;
    }

    case 0x05:    // Number-size of player-missle 1
    {
      myNUSIZ1 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      break;
    }

    case 0x06:    // Color-Luminance Player 0
    {
      myColor[MYCOLUP0] = color_repeat_table[value];
      break;
    }

    case 0x07:    // Color-Luminance Player 1
    {
      myColor[MYCOLUP1] = color_repeat_table[value];
      break;
    }

    case 0x08:    // Color-Luminance Playfield
    {
      myColor[MYCOLUPF] = color_repeat_table[value];
      break;
    }

    case 0x09:    // Color-Luminance Background
    {
      myColor[MYCOLUBK] = color_repeat_table[value];
      break;
    }

    case 0x0A:    // Control Playfield, Ball size, Collisions
    {
      myCTRLPF = value;

      // The playfield priority and score bits from the control register
      // are accessed when the frame is being drawn.  We precompute the 
      // necessary value here so we can save time while drawing.
      myPlayfieldPriorityAndScore = ((myCTRLPF & 0x06) << 5);

      // Update the playfield mask based on reflection state if 
      // we're still on the left hand side of the playfield
      if(((delta_clock) % 228) < (68 + 79))
      {
        myCurrentPFMask = ourPlayfieldTable[myCTRLPF & 0x01];
      }

      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      break;
    }

    case 0x0B:    // Reflect Player 0
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP0) || (!(value & 0x08) && myREFP0))
      {
        myREFP0 = (value & 0x08);
        myCurrentGRP0 = ourPlayerReflectTable[myCurrentGRP0];
      }
      break;
    }

    case 0x0C:    // Reflect Player 1
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP1) || (!(value & 0x08) && myREFP1))
      {
        myREFP1 = (value & 0x08);
        myCurrentGRP1 = ourPlayerReflectTable[myCurrentGRP1];
      }
      break;
    }

    case 0x0D:    // Playfield register byte 0
    {
      myPF = (myPF & 0x000FFFF0) | ((value >> 4) & 0x0F);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x0E:    // Playfield register byte 1
    {
      myPF = (myPF & 0x000FF00F) | ((uInt32)value << 4);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x0F:    // Playfield register byte 2
    {
      myPF = (myPF & 0x00000FFF) | ((uInt32)value << 12);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x10:    // Reset Player 0
    {
      uInt8 hpos = (delta_clock) % 228;
      uInt8 newx = player_reset_pos[hpos];  //hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = ourPlayerPositionResetWhenTable[myNUSIZ0 & 7][myPOSP0][newx];

      // Player is being reset in neither the delay nor display section
      if (when == 0)
      {
        myPOSP0 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset during the display of one of its copies
      else if (when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 11 (01/21/99).
        updateFrame(clock + 11);

        myPOSP0 = newx;

        // Setup the mask to skip the first copy of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else
      {
        myPOSP0 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      break;
    }

    case 0x11:    // Reset Player 1
    {
      uInt8 hpos = (delta_clock) % 228;
      uInt8 newx = player_reset_pos[hpos];  //hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);
        
      // TODO: Remove the following special hack for Rabbit Transit
      // and Dragon Stomper (Excalibur) by StarPath/ARcadia
      if ((clock - myLastHMOVEClock) == (20 * 3) && (hpos==69))
      {
          newx = 11;
      }

      // Find out under what condition the player is being reset
      Int8 when = ourPlayerPositionResetWhenTable[myNUSIZ1 & 7][myPOSP1][newx];

      // Player is being reset in neither the delay nor display section
      if (when == 0)
      {
        myPOSP1 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset during the display of one of its copies
      else if (when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 11 (01/21/99).
        updateFrame(clock + 11);

        myPOSP1 = newx;

        // Setup the mask to skip the first copy of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else
      {
        myPOSP1 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      break;
    }

    case 0x12:    // Reset Missle 0
    {
      uInt8 hpos = (delta_clock) % 228;
      myPOSM0 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Dolphin by
      // figuring out what really happens when Reset Missle 
      // occurs 20 cycles after an HMOVE (04/13/02).
      if(((clock - myLastHMOVEClock) == (20 * 3)) && (hpos == 69))
      {
        myPOSM0 = 8;
      }
      // TODO: Remove the following special hack for Solaris by
      // figuring out what really happens when Reset Missle 
      // occurs 9 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (9 * 3)) && (hpos == 36))
      {
        myPOSM0 = 8;
      }
 
      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      break;
    }

    case 0x13:    // Reset Missle 1
    {
      uInt8 hpos = (delta_clock) % 228;
      myPOSM1 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Pitfall II by
      // figuring out what really happens when Reset Missle 
      // occurs 3 cycles after an HMOVE (04/13/02).
      if(((clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSM1 = 3;
      }
 
      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      break;
    }

    case 0x14:    // Reset Ball
    {
      uInt8 hpos = (delta_clock) % 228 ;
      myPOSBL = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);
        
      // TODO: Remove the following special hack by figuring out what
      // really happens when Reset Ball occurs 18 cycles after an HMOVE.
      if((clock - myLastHMOVEClock) == (18 * 3))
      {
        // Escape from the Mindmaster (01/09/99)
        if((hpos == 60) || (hpos == 69))
          myPOSBL = 10;
        // Mission Survive (04/11/08)
        else if(hpos == 63)
          myPOSBL = 7;
      }
      // TODO: Remove the following special hack for Escape from the
      // Mindmaster by figuring out what really happens when Reset Ball 
      // occurs 15 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (15 * 3)) && (hpos == 60))
      {
        myPOSBL = 10;
      } 
      // TODO: Remove the following special hack for Decathlon by
      // figuring out what really happens when Reset Ball 
      // occurs 3 cycles after an HMOVE (04/13/02).
      else if(((clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSBL = 3;
      } 
      // TODO: Remove the following special hack for Robot Tank by
      // figuring out what really happens when Reset Ball 
      // occurs 7 cycles after an HMOVE (04/13/02).
      else if(((clock - myLastHMOVEClock) == (7 * 3)) && (hpos == 30))
      {
        myPOSBL = 6;
      } 
      // TODO: Remove the following special hack for Hole Hunter by
      // figuring out what really happens when Reset Ball 
      // occurs 6 cycles after an HMOVE (04/13/02).
      else if(((clock - myLastHMOVEClock) == (6 * 3)) && (hpos == 27))
      {
        myPOSBL = 5;
      }
      // TODO: Remove the following special hack for Swoops! by
      // figuring out what really happens when Reset Ball 
      // occurs 9 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (9 * 3)) && (hpos == 36))
      {
        myPOSBL = 7;
      }
      // TODO: Remove the following special hack for Solaris by
      // figuring out what really happens when Reset Ball 
      // occurs 12 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (12 * 3)) && (hpos == 45))
      {
        myPOSBL = 8;
      }
       
      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];
      break;
    }

    case 0x15:    // Audio control 0
          AUDC[0] = value & 0x0f;
          Update_tia_sound(0);
          break;
    case 0x16:    // Audio control 1
          AUDC[1] = value & 0x0f;
          Update_tia_sound(1);
          break;
          
    case 0x17:    // Audio frequency 0
          AUDF[0] = value & 0x1f;
          Update_tia_sound(0);
          break;          
    case 0x18:    // Audio frequency 1
          AUDF[1] = value & 0x1f;
          Update_tia_sound(1);
          break;
          
    case 0x19:    // Audio volume 0
          AUDV[0] = (value & 0x0f) << 3;
          Update_tia_sound(0);
          break;

    case 0x1A:    // Audio volume 1
          AUDV[1] = (value & 0x0f) << 3;
          Update_tia_sound(1);
          break;
  
    case 0x1B: // Graphics Player 0
    {
      // Set player 0 graphics
      myGRP0 = value;

      // Copy player 1 graphics into its delayed register
      myDGRP1 = myGRP1;

      // Get the "current" data for GRP0 base on delay register and reflect
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register and reflect
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;
      break;
    }

    case 0x1C: // Graphics Player 1
    {
      // Set player 1 graphics
      myGRP1 = value;

      // Copy player 0 graphics into its delayed register
      myDGRP0 = myGRP0;

      // Copy ball graphics into its delayed register
      myDENABL = myENABL;

      // Get the "current" data for GRP0 base on delay register
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;
      break;
    }      

    case 0x1D:    // Enable Missle 0 graphics
    {
      myENAM0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;
      break;
    }

    case 0x1E:    // Enable Missle 1 graphics
    {
      myENAM1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x1F:    // Enable Ball graphics
    {
      myENABL = value & 0x02;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;

      break;
    }

    case 0x20:    // Horizontal Motion Player 0
    {
      myHMP0 = value >> 4;
      break;
    }

    case 0x21:    // Horizontal Motion Player 1
    {
      myHMP1 = value >> 4;
      break;
    }

    case 0x22:    // Horizontal Motion Missle 0
    {
      Int8 tmp = value >> 4;

      // Should we enabled TIA M0 "bug" used for stars in Cosmic Ark?
      if((clock == (myLastHMOVEClock + 21 * 3)) && (myHMM0 == 7) && (tmp == 6))
      {
        myM0CosmicArkMotionEnabled = true;
        myM0CosmicArkCounter = 0;
      }

      myHMM0 = tmp;
      break;
    }

    case 0x23:    // Horizontal Motion Missle 1
    {
      myHMM1 = value >> 4;
      break;
    }

    case 0x24:    // Horizontal Motion Ball
    {
      myHMBL = value >> 4;
      break;
    }

    case 0x25:    // Vertial Delay Player 0
    {
      myVDELP0 = value & 0x01;

      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;
      break;
    }

    case 0x26:    // Vertial Delay Player 1
    {
      myVDELP1 = value & 0x01;

      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;
      break;
    }

    case 0x27:    // Vertial Delay Ball
    {
      myVDELBL = value & 0x01;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;
      break;
    }

    case 0x28:    // Reset missle 0 to player 0
    {
      if(myRESMP0 && !(value & 0x02))
      {
        uInt16 middle;

        if((myNUSIZ0 & 0x07) == 0x05)
          middle = 8;
        else if((myNUSIZ0 & 0x07) == 0x07)
          middle = 16;
        else
          middle = 4;

        myPOSM0 = (myPOSP0 + middle) % 160;
        myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
            [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      }

      myRESMP0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;

      break;
    }

    case 0x29:    // Reset missle 1 to player 1
    {
      if(myRESMP1 && !(value & 0x02))
      {
        uInt16 middle;

        if((myNUSIZ1 & 0x07) == 0x05)
          middle = 8;
        else if((myNUSIZ1 & 0x07) == 0x07)
          middle = 16;
        else
          middle = 4;

        myPOSM1 = (myPOSP1 + middle) % 160;
        myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
            [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      }

      myRESMP1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x2A:    // Apply horizontal motion
    {
      // Figure out what cycle we're at
      Int32 x = ((clock - myClockWhenFrameStarted) % 228) / 3;

      // See if we need to enable the HMOVE blank bug
      if(myCartInfo.hBlankZero)
      {
          myHMOVEBlankEnabled = ourHMOVEBlankEnableCycles[x];
      }

      myPOSP0 += ourCompleteMotionTable[x][myHMP0];
      myPOSP1 += ourCompleteMotionTable[x][myHMP1];
      myPOSM0 += ourCompleteMotionTable[x][myHMM0];
      myPOSM1 += ourCompleteMotionTable[x][myHMM1];
      myPOSBL += ourCompleteMotionTable[x][myHMBL];

      if(myPOSP0 >= 160)
        myPOSP0 -= 160;
      else if(myPOSP0 < 0)
        myPOSP0 += 160;

      if(myPOSP1 >= 160)
        myPOSP1 -= 160;
      else if(myPOSP1 < 0)
        myPOSP1 += 160;

      if(myPOSM0 >= 160)
        myPOSM0 -= 160;
      else if(myPOSM0 < 0)
        myPOSM0 += 160;

      if(myPOSM1 >= 160)
        myPOSM1 -= 160;
      else if(myPOSM1 < 0)
        myPOSM1 += 160;

      if(myPOSBL >= 160)
        myPOSBL -= 160;
      else if(myPOSBL < 0)
        myPOSBL += 160;

      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      // Remember what clock HMOVE occured at
      myLastHMOVEClock = clock;

      // Disable TIA M0 "bug" used for stars in Cosmic ark
      myM0CosmicArkMotionEnabled = false;
      break;
    }

    case 0x2b:    // Clear horizontal motion registers
    {
      myHMP0 = 0;
      myHMP1 = 0;
      myHMM0 = 0;
      myHMM1 = 0;
      myHMBL = 0;
      break;
    }

    case 0x2c:    // Clear collision latches
    {
      myCollision = 0;
      break;
    }

    default:
    {
      break;
    }
  }
}

void TIA::togglePalette()
{
  // Only NTSC available...
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourBallMaskTable[4][4][320];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourDisabledMaskTable[640];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourMissleMaskTable[4][8][4][320];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourPlayerMaskTable[4][2][8][320];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int8 TIA::ourPlayerPositionResetWhenTable[8][160][160];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourNTSCPalette[256] = 
{
  0x000000, 0x1c1c1c, 0x393939, 0x595959,           // 1
  0x797979, 0x929292, 0xababab, 0xbcbcbc, 
  0xcdcdcd, 0xd9d9d9, 0xe6e6e6, 0xececec, 
  0xf2f2f2, 0xf8f8f8, 0xffffff, 0xffffff, 
    
  0x391701, 0x5e2304, 0x833008, 0xa54716,           // 2
  0xc85f24, 0xe37820, 0xff911d, 0xffab1d, 
  0xffc51d, 0xffce34, 0xffd84c, 0xffe651, 
  0xfff456, 0xfff977, 0xffff98, 0xffff98, 
    
  0x451904, 0x721e11, 0x9f241e, 0xb33a20,           // 3
  0xc85122, 0xe36920, 0xff811e, 0xff8c25, 
  0xff982c, 0xffae38, 0xffc545, 0xffc559, 
  0xffc66d, 0xffd587, 0xffe4a1, 0xffe4a1, 
    
  0x4a1704, 0x7e1a0d, 0xb21d17, 0xc82119,           // 4
  0xdf251c, 0xec3b38, 0xfa5255, 0xfc6161, 
  0xff706e, 0xff7f7e, 0xff8f8f, 0xff9d9e, 
  0xffabad, 0xffb9bd, 0xffc7ce, 0xffc7ce, 

  0x050568, 0x3b136d, 0x912640, 0x912640,           // 5
  0xa532a6, 0xb938ba, 0xcd3ecf, 0xdb47dd, 
  0xea51eb, 0xf45ff5, 0xfe6dff, 0xfe7afd, 
  0xff87fb, 0xff95fd, 0xffa4ff, 0xffa4ff, 

  0x280479, 0x400984, 0x590f90, 0x70249d,           // 6
  0x8839aa, 0xa441c3, 0xc04adc, 0xd054ed, 
  0xe05eff, 0xe96dff, 0xf27cff, 0xf88aff, 
  0xff98ff, 0xfea1ff, 0xfeabff, 0xfeabff, 
    
  0x35088a, 0x420aad, 0x500cd0, 0x6428d0,           // 7
  0x7945d0, 0x8d4bd4, 0xa251d9, 0xb058ec, 
  0xbe60ff, 0xc56bff, 0xcc77ff, 0xd183ff, 
  0xd790ff, 0xdb9dff, 0xdfaaff, 0xdfaaff, 
    
  0x051e81, 0x0626a5, 0x082fca, 0x263dd4,           // 8
  0x444cde, 0x4f5aee, 0x5a68ff, 0x6575ff, 
  0x7183ff, 0x8091ff, 0x90a0ff, 0x97a9ff, 
  0x9fb2ff, 0xafbeff, 0xc0cbff, 0xc0cbff, 
    
  0x0c048b, 0x2218a0, 0x382db5, 0x483ec7,           // 9
  0x584fda, 0x6159ec, 0x6b64ff, 0x7a74ff, 
  0x8a84ff, 0x918eff, 0x9998ff, 0xa5a3ff, 
  0xb1aeff, 0xb8b8ff, 0xc0c2ff, 0xc0c2ff, 
    
  0x1d295a, 0x1d3876, 0x1d4892, 0x1c5cac,           // 10
  0x1c71c6, 0x3286cf, 0x489bd9, 0x4ea8ec, 
  0x55b6ff, 0x70c7ff, 0x8cd8ff, 0x93dbff, 
  0x9bdfff, 0xafe4ff, 0xc3e9ff, 0xc3e9ff, 
    
  0x2f4302, 0x395202, 0x446103, 0x417a12,           // 11
  0x3e9421, 0x4a9f2e, 0x57ab3b, 0x5cbd55, 
  0x61d070, 0x69e27a, 0x72f584, 0x7cfa8d, 
  0x87ff97, 0x9affa6, 0xadffb6, 0xadffb6, 
 
  0x0a4108, 0x0d540a, 0x10680d, 0x137d0f,           // 12
  0x169212, 0x19a514, 0x1cb917, 0x1ec919, 
  0x21d91b, 0x47e42d, 0x6ef040, 0x78f74d, 
  0x83ff5b, 0x9aff7a, 0xb2ff9a, 0xb2ff9a, 
    
  0x04410b, 0x05530e, 0x066611, 0x077714,           // 13
  0x088817, 0x099b1a, 0x0baf1d, 0x48c41f, 
  0x86d922, 0x8fe924, 0x99f927, 0xa8fc41, 
  0xb7ff5b, 0xc9ff6e, 0xdcff81, 0xdcff81, 
    
  0x02350f, 0x073f15, 0x0c4a1c, 0x2d5f1e,           // 14
  0x4f7420, 0x598324, 0x649228, 0x82a12e, 
  0xa1b034, 0xa9c13a, 0xb2d241, 0xc4d945, 
  0xd6e149, 0xe4f04e, 0xf2ff53, 0xf2ff53, 
    
  0x263001, 0x243803, 0x234005, 0x51541b,           // 15
  0x806931, 0x978135, 0xaf993a, 0xc2a73e, 
  0xd5b543, 0xdbc03d, 0xe1cb38, 0xe2d836, 
  0xe3e534, 0xeff258, 0xfbff7d, 0xfbff7d, 
    
  0x401a02, 0x581f05, 0x702408, 0x8d3a13,           // 16
  0xab511f, 0xb56427, 0xbf7730, 0xd0853a, 
  0xe19344, 0xeda04e, 0xf9ad58, 0xfcb75c, 
  0xffc160, 0xffc671, 0xffcb83, 0xffcb83
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourPALPalette[256] = 
{
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xffffff, 
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xffffff, 
  0x4a3700, 0x4a3700, 0x705813, 0x705813, 
  0x8c732a, 0x8c732a, 0xa68d46, 0xa68d46, 
  0xbea767, 0xbea767, 0xd4c18b, 0xd4c18b, 
  0xeadcb3, 0xeadcb3, 0xfff6de, 0xfff6de, 
  0x284a00, 0x284a00, 0x44700f, 0x44700f, 
  0x5c8c21, 0x5c8c21, 0x74a638, 0x74a638, 
  0x8cbe51, 0x8cbe51, 0xa6d46e, 0xa6d46e, 
  0xc0ea8e, 0xc0ea8e, 0xdbffb0, 0xdbffb0, 
  0x4a1300, 0x4a1300, 0x70280f, 0x70280f, 
  0x8c3d21, 0x8c3d21, 0xa65438, 0xa65438, 
  0xbe6d51, 0xbe6d51, 0xd4886e, 0xd4886e, 
  0xeaa58e, 0xeaa58e, 0xffc4b0, 0xffc4b0, 
  0x004a22, 0x004a22, 0x0f703b, 0x0f703b, 
  0x218c52, 0x218c52, 0x38a66a, 0x38a66a, 
  0x51be83, 0x51be83, 0x6ed49d, 0x6ed49d, 
  0x8eeab8, 0x8eeab8, 0xb0ffd4, 0xb0ffd4, 
  0x4a0028, 0x4a0028, 0x700f44, 0x700f44, 
  0x8c215c, 0x8c215c, 0xa63874, 0xa63874, 
  0xbe518c, 0xbe518c, 0xd46ea6, 0xd46ea6, 
  0xea8ec0, 0xea8ec0, 0xffb0db, 0xffb0db, 
  0x00404a, 0x00404a, 0x0f6370, 0x0f6370, 
  0x217e8c, 0x217e8c, 0x3897a6, 0x3897a6, 
  0x51afbe, 0x51afbe, 0x6ec7d4, 0x6ec7d4, 
  0x8edeea, 0x8edeea, 0xb0f4ff, 0xb0f4ff, 
  0x43002c, 0x43002c, 0x650f4b, 0x650f4b, 
  0x7e2165, 0x7e2165, 0x953880, 0x953880, 
  0xa6519a, 0xa6519a, 0xbf6eb7, 0xbf6eb7, 
  0xd38ed3, 0xd38ed3, 0xe5b0f1, 0xe5b0f1, 
  0x001d4a, 0x001d4a, 0x0f3870, 0x0f3870, 
  0x21538c, 0x21538c, 0x386ea6, 0x386ea6, 
  0x518dbe, 0x518dbe, 0x6ea8d4, 0x6ea8d4, 
  0x8ec8ea, 0x8ec8ea, 0xb0e9ff, 0xb0e9ff, 
  0x37004a, 0x37004a, 0x570f70, 0x570f70, 
  0x70218c, 0x70218c, 0x8938a6, 0x8938a6, 
  0xa151be, 0xa151be, 0xba6ed4, 0xba6ed4, 
  0xd28eea, 0xd28eea, 0xeab0ff, 0xeab0ff, 
  0x00184a, 0x00184a, 0x0f2e70, 0x0f2e70, 
  0x21448c, 0x21448c, 0x385ba6, 0x385ba6, 
  0x5174be, 0x5174be, 0x6e8fd4, 0x6e8fd4, 
  0x8eabea, 0x8eabea, 0xb0c9ff, 0xb0c9ff, 
  0x13004a, 0x13004a, 0x280f70, 0x280f70, 
  0x3d218c, 0x3d218c, 0x5438a6, 0x5438a6, 
  0x6d51be, 0x6d51be, 0x886ed4, 0x886ed4, 
  0xa58eea, 0xa58eea, 0xc4b0ff, 0xc4b0ff, 
  0x00014a, 0x00014a, 0x0f1170, 0x0f1170, 
  0x21248c, 0x21248c, 0x383aa6, 0x383aa6, 
  0x5153be, 0x5153be, 0x6e70d4, 0x6e70d4, 
  0x8e8fea, 0x8e8fea, 0xb0b2ff, 0xb0b2ff, 
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xffffff, 
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xff4ffff
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const TIA& c)
    : myConsole(c.myConsole)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA& TIA::operator = (const TIA&)
{
  assert(false);

  return *this;
}
