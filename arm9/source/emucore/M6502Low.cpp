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
#include "Cart.hxx"
#include "CartAR.hxx"
#include "System.hxx"
#include "M6502Low.hxx"
#include "TIA.hxx"
#include "M6532.hxx"
#include "CartDPCPlus.hxx"

uInt16 PC __attribute__ ((aligned (4))) __attribute__((section(".dtcm")));   // Program Counter
uInt8 A                                 __attribute__((section(".dtcm")));   // Accumulator
uInt8 X                                 __attribute__((section(".dtcm")));   // X index register
uInt8 Y                                 __attribute__((section(".dtcm")));   // Y index register
uInt8 SP                                __attribute__((section(".dtcm")));   // Stack Pointer
                            
uInt8 N                                 __attribute__((section(".dtcm")));   // N flag for processor status register
uInt8 V                                 __attribute__((section(".dtcm")));   // V flag for processor status register
uInt8 B                                 __attribute__((section(".dtcm")));   // B flag for processor status register
uInt8 D                                 __attribute__((section(".dtcm")));   // D flag for processor status register
uInt8 I                                 __attribute__((section(".dtcm")));   // I flag for processor status register
uInt8 notZ                              __attribute__((section(".dtcm")));   // Z flag complement for processor status register
uInt8 C                                 __attribute__((section(".dtcm")));   // C flag for processor status register
        
uInt32 NumberOfDistinctAccesses         __attribute__((section(".dtcm")));     // For AR cart use only - track the # of distinct PC accesses
uInt8  cartDriver                       __attribute__((section(".dtcm"))) = 0; // Set to 1 for carts that are non-banking to invoke faster peek/poke handling
uInt16 f8_bankbit                       __attribute__((section(".dtcm"))) = 0x1FFF;

extern CartridgeAR  *myAR;
extern TIA          *theTIA;
extern M6532        *theM6532;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502Low::M6502Low(uInt32 systemCyclesPerProcessorCycle)
    : M6502(systemCyclesPerProcessorCycle)
{
    NumberOfDistinctAccesses = 0;
    cartDriver = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502Low::~M6502Low()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define fake_peek()  gSystemCycles++;

// -------------------------------------------------------------------------------
// This is the normal driver - optmized but will drive unused TIA pins to 0x00
// and is not compatible with buggy games that rely on undriven bits being
// at the last known bus state. We have peekBUS() and pokeBUS() for those games.
// -------------------------------------------------------------------------------
inline uInt8 M6502Low::peek(uInt16 address)
{
  gSystemCycles++;
    
  PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
  if(access.directPeekBase != 0) return *(access.directPeekBase + (address & MY_PAGE_MASK));
  else return access.device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek_PC(uInt16 address)
{
  gSystemCycles++;
    
  PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
  if(access.directPeekBase != 0) return *(access.directPeekBase + (address & MY_PAGE_MASK));
  else return access.device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502Low::poke(uInt16 address, uInt8 value)
{
  gSystemCycles++;
    
  PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
  if(access.directPokeBase != 0) *(access.directPokeBase + (address & MY_PAGE_MASK)) = value;
  else access.device->poke(address, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502Low::execute(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      ++gSystemCycles;
      PageAccess& access = myPageAccessTable[(PC & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
      if (access.directPeekBase != 0) operand = *(access.directPeekBase + (PC & MY_PAGE_MASK));
      else operand = access.device->peek(PC);
      PC++;

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        #include "M6502Low.ins"        
      }
    }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* M6502Low::name() const
{
  return "M6502Low";
}


// ==============================================================================
// Special Non-Banked (2k and 4k carts) handling. This is highly optimized 
// for a cart whose memory can be entirely mapped into the 6502 address space
// and doesn't require us to check for hot spots or other things that slow us down.
// This gets us about 15% speed boost on these games and makes them playable 
// on the older/slower DS-LITE/PHAT hardware.
// ==============================================================================
inline uInt8 M6502Low::peek_PCNB(uInt16 address)
{
  gSystemCycles++;
  return fast_cart_buffer[address & 0xFFF];
}


inline uInt8 M6502Low::peek_NB(uInt16 address)
{
  gSystemCycles++;

  if (address & 0x1000)
  {
      return fast_cart_buffer[address & 0xFFF];    
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }
}


inline void M6502Low::poke_NB(uInt16 address, uInt8 value)
{
  gSystemCycles++;
  
  if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
  else if (address & 0x200) theM6532->poke(address, value);
  else theTIA->poke(address, value);
}


void M6502Low::execute_NB(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      ++gSystemCycles;
      operand = fast_cart_buffer[PC++ & 0xFFF];

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        // A trick of the light... here we map peek/poke to the "NB" cart versions. This improves speed for non-bank-switched carts.
        #define peek    peek_NB
        #define peek_PC peek_PCNB              
        #define poke    poke_NB
        #include "M6502Low.ins"        
        #undef peek   
        #undef peek_PC
        #undef poke 
      }
    }
}

// -------------------------------------------------------------------------------
// Special F8 driver for much faster speeds but comes at a cost of 
// compatibility - so in cart.cpp we enable this only for the 
// well-behaved carts that can support it...
// -------------------------------------------------------------------------------

inline uInt8 M6502Low::peek_PCF8(uInt16 address)
{
  gSystemCycles++;
  return fast_cart_buffer[address & f8_bankbit];
}


inline uInt8 M6502Low::peek_F8(uInt16 address)
{
  gSystemCycles++;

  if (address & 0x1000)
  {
      if ((address & 0x0FFF) == 0x0FF8) f8_bankbit=0x0FFF;
      else if ((address & 0x0FFF) == 0x0FF9) f8_bankbit=0x1FFF;
      
      return fast_cart_buffer[address & f8_bankbit];
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }
}


inline void M6502Low::poke_F8(uInt16 address, uInt8 value)
{
  gSystemCycles++;
  
  if (address & 0x1000)
  {
      if ((address & 0x0FFF) == 0x0FF8) f8_bankbit=0x0FFF;
      else if ((address & 0x0FFF)) f8_bankbit=0x1FFF;
  }
  else
  {
      if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
      else if (address & 0x200) theM6532->poke(address, value);
      else theTIA->poke(address, value);
  }
}


void M6502Low::execute_F8(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      ++gSystemCycles;
      operand = fast_cart_buffer[PC & f8_bankbit];
      PC++;

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        // A trick of the light... here we map peek/poke to the "F8" cart versions. This improves speed for non-bank-switched carts.
        #define peek    peek_F8
        #define peek_PC peek_PCF8              
        #define poke    poke_F8
        #include "M6502Low.ins"        
        #undef peek   
        #undef peek_PC
        #undef poke
      }
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek_PCF8SC(uInt16 address)
{
  gSystemCycles++;
  return fast_cart_buffer[address & f8_bankbit];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502Low::execute_F8SC(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      ++gSystemCycles;
      operand = fast_cart_buffer[PC & f8_bankbit];
      PC++;

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        #define peek_PC    peek_PCF8SC
        #include "M6502Low.ins"
        #undef peek_PC
      }
    }
}

// -------------------------------------------------------------------------------
// Special F6 driver for much faster speeds but comes at a cost of 
// compatibility - so in cart.cpp we enable this only for the 
// well-behaved carts that can support it...
// -------------------------------------------------------------------------------
inline uInt8 M6502Low::peek_PCF6(uInt16 address)
{
  gSystemCycles++;
  return cart_buffer[myCurrentOffset | (address & 0xFFF)];
}


inline uInt8 M6502Low::peek_F6(uInt16 address)
{
  gSystemCycles++;

  if (address & 0x1000)
  {
      address &= 0xFFF;
      if (address >= 0xFF6)
      {
          if      (address == 0x0FF6) myCurrentOffset = 0x0000;
          else if (address == 0x0FF7) myCurrentOffset = 0x1000;
          else if (address == 0x0FF8) myCurrentOffset = 0x2000;
          else if (address == 0x0FF9) myCurrentOffset = 0x3000;
      }
      return cart_buffer[myCurrentOffset | address];
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }  
}


inline void M6502Low::poke_F6(uInt16 address, uInt8 value)
{
  gSystemCycles++;
  
  if (address & 0x1000)
  {
      address &= 0xFFF;
      if      (address == 0x0FF6) myCurrentOffset = 0x0000;
      else if (address == 0x0FF7) myCurrentOffset = 0x1000;
      else if (address == 0x0FF8) myCurrentOffset = 0x2000;
      else if (address == 0x0FF9) myCurrentOffset = 0x3000;
  }
  else
  {
      if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
      else if (address & 0x200) theM6532->poke(address, value);
      else theTIA->poke(address, value);
  }
}


void M6502Low::execute_F6(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      if (PC >= 0xFFF6) 
      {
          operand = peek_F6(PC++);
      }
      else 
      {
          gSystemCycles++;
          operand = cart_buffer[myCurrentOffset | (PC++ & 0xFFF)];
      }

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        // A trick of the light... here we map peek/poke to the "F6" cart versions. This improves speed for non-bank-switched carts.
        #define peek    peek_F6
        #define peek_PC peek_PCF6
        #define poke    poke_F6
        #include "M6502Low.ins"        
        #undef peek   
        #undef peek_PC
        #undef poke 
      }
    }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek_PCF6SC(uInt16 address)
{
  gSystemCycles++;
  return cart_buffer[myCurrentOffset | (address & 0xFFF)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502Low::execute_F6SC(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
       gSystemCycles++;
       operand = cart_buffer[myCurrentOffset | (PC++ & 0xFFF)];
        
      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        #define peek_PC    peek_PCF6SC
        #include "M6502Low.ins"
        #undef peek_PC
      }
    }
}

// -------------------------------------------------------------------------------
// Special F4 driver for much faster speeds but comes at a cost of 
// compatibility - so in cart.cpp we enable this only for the 
// well-behaved carts that can support it...
// -------------------------------------------------------------------------------
inline uInt8 M6502Low::peek_PCF4(uInt16 address)
{
  gSystemCycles++;
  return cart_buffer[myCurrentOffset | (address & 0xFFF)];
}


inline uInt8 M6502Low::peek_F4(uInt16 address)
{
  gSystemCycles++;

  if (address & 0x1000)
  {
      address &= 0xFFF;
      if (address >= 0xFF4)
      {
          if      (address == 0x0FF4) myCurrentOffset = 0x0000;
          else if (address == 0x0FF5) myCurrentOffset = 0x1000;
          else if (address == 0x0FF6) myCurrentOffset = 0x2000;
          else if (address == 0x0FF7) myCurrentOffset = 0x3000;
          else if (address == 0x0FF8) myCurrentOffset = 0x4000;
          else if (address == 0x0FF9) myCurrentOffset = 0x5000;
          else if (address == 0x0FFA) myCurrentOffset = 0x6000;
          else if (address == 0x0FFB) myCurrentOffset = 0x7000;
      }
      return cart_buffer[myCurrentOffset | address];
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }  
}


inline void M6502Low::poke_F4(uInt16 address, uInt8 value)
{
  gSystemCycles++;
  
  if (address & 0x1000)
  {
      address &= 0xFFF;
      if (address >= 0xFF4)
      {
          if      (address == 0x0FF4) myCurrentOffset = 0x0000;
          else if (address == 0x0FF5) myCurrentOffset = 0x1000;
          else if (address == 0x0FF6) myCurrentOffset = 0x2000;
          else if (address == 0x0FF7) myCurrentOffset = 0x3000;
          else if (address == 0x0FF8) myCurrentOffset = 0x4000;
          else if (address == 0x0FF9) myCurrentOffset = 0x5000;
          else if (address == 0x0FFA) myCurrentOffset = 0x6000;
          else if (address == 0x0FFB) myCurrentOffset = 0x7000;
      }
  }
  else
  {
    if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
    else if (address & 0x200) theM6532->poke(address, value);
    else theTIA->poke(address, value);
  }
}


void M6502Low::execute_F4(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      gSystemCycles++;
      operand = cart_buffer[myCurrentOffset | (PC++ & 0xFFF)];

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        // A trick of the light... here we map peek/poke to the "F6" cart versions. This improves speed for non-bank-switched carts.
        #define peek    peek_F4
        #define peek_PC peek_PCF4
        #define poke    poke_F4
        #include "M6502Low.ins"        
        #undef peek   
        #undef peek_PC
        #undef poke 
      }
    }
}

// ==============================================================================
// Special AR Cart Handling below... this requries us to track distinct
// memory fetches (so we can emulate the 5-fetch write cycle of the Starpath
// Supercharger) as well as more complicated bank switching / RAM loading...
// This uses significant DS memory space but we have plenty of RAM and the 
// Starpath Supercharger "AR" games are some of the better games on the system 
// so we'll go the extra mile here...
// ==============================================================================

#define DISTINCT_THRESHOLD  5
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek_AR(uInt16 address)
{
  NumberOfDistinctAccesses++;
  gSystemCycles++;
  
  if (address & 0x1000)
  {
      uInt16 addr = address & 0x0FFF;     // Map down to 4k...
      if (addr & 0x0800)  // If we are in the upper bank...
      {
          // Is the "dummy" SC BIOS hotspot for reading a load being accessed?
          if(addr == 0x0850 && bPossibleLoad)
          {
            // Get load that's being accessed (BIOS places load number at 0x80)
            uInt8 load = mySystem->peek(0x0080);

            // Read the specified load into RAM
            myAR->loadIntoRAM(load);
          }
          // Is the bank configuration hotspot being accessed?
          else if(addr == 0x0FF8)
          {
            // Yes, so handle bank configuration
            myWritePending = false;
            myAR->bankConfiguration(myDataHoldRegister);
          }
          // Handle poke if writing enabled
          else if (myWritePending)
          {
              if (NumberOfDistinctAccesses >= DISTINCT_THRESHOLD)
              {
                  //if(!bPossibleLoad)    // Can't poke to ROM :-) -- but we're looking for speed so...
                  myImage1[addr] = myDataHoldRegister;
                  myWritePending = false;
              }
          }

          return myImage1[addr];
      }
      else // WE are in the lower bank
      {
          // Handle poke if writing enabled
          if (myWritePending)
          {
              if (NumberOfDistinctAccesses >= DISTINCT_THRESHOLD)
              {
                  myImage0[addr] = myDataHoldRegister;
                  myWritePending = false;
              }
          }
          // Is the data hold register being set?
          else if (addr < 0x100)
          {
            myDataHoldRegister = addr;
            NumberOfDistinctAccesses = 0;
            if (myWriteEnabled) myWritePending = true;
          }

          return myImage0[addr];
      }
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502Low::poke_AR(uInt16 address, uInt8 value)
{
  NumberOfDistinctAccesses++;
  gSystemCycles++;  
    
  // TIA access is common... filter that one out first...
  if (address < 0x80)
  {
      extern TIA *theTIA;
      theTIA->poke(address, value);
      return;
  }
    
  // --------------------------------------------------------------------
  // If not TIA access, we check if we are in the code area...
  // --------------------------------------------------------------------
  if (address & 0x1000)
  {
      uInt16 addr = address & 0x0FFF; // Map down to 4k...

      // Is the data hold register being set?
      if(!(addr & 0x0F00) && (!myWritePending))
      {
        myDataHoldRegister = addr;
        NumberOfDistinctAccesses = 0;
        if (myWriteEnabled) myWritePending = true;
      }
      // Is the bank configuration hotspot being accessed?
      else if (addr == 0x0FF8)
      {
        // Yes, so handle bank configuration
        myWritePending = false;
        myAR->bankConfiguration(myDataHoldRegister);
      }
      // Handle poke if writing enabled
      else if (myWritePending)
      {
          if (NumberOfDistinctAccesses >= DISTINCT_THRESHOLD)
          {
            if((addr & 0x0800) == 0)
              myImage0[addr] = myDataHoldRegister;
            else if(!bPossibleLoad)    // Can't poke to ROM :-)
              myImage1[addr] = myDataHoldRegister;
            myWritePending = false;
          }
      }
  }
  else  // Otherwise let the system handle it...
  {
      if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
      else if (address & 0x200) theM6532->poke(address, value);
      else theTIA->poke(address, value);
  }
}


void M6502Low::execute_AR(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction
      operand = peek_AR(PC++);

      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        // A trick of the light... here we map peek/poke to the "AR cart versions.
        #define peek    peek_AR
        #define peek_PC peek_AR              
        #define poke    poke_AR
        #include "M6502Low.ins"        
        #undef peek
        #undef peek_PC
        #undef poke
      }
    }
}

extern uInt8 myDPC[];
extern CartridgeDPCPlus *myCartDPC;
extern uInt32 myCounters[];
extern uInt8 *myDisplayImageDPCP;
extern uInt32 myFractionalCounters[];
extern uInt32 myFractionalIncrements[];
extern uInt32 myTops[];
extern uInt32 myTopsMinusBottoms[];
extern uInt8 *myDPCptr;
extern uInt32 myMusicCounters[3];
extern uInt32 myMusicFrequencies[3];
extern uInt32 myMusicWaveforms[3];
extern uInt32 myMusicCountersShifted[3];
extern Int32 myDPCPCycles;

ITCM_CODE uInt8 M6502Low::peek_Fetch(uInt8 address)
{
  uInt8 result;

  switch(address)
  {
    case 0x05: // AMPLITUDE
    {
      // -----------------------------------------------------------------------
      // Update the music data fetchers (counter & flag)
      // This is a rough approximation of timing - we can't really
      // keep up with the fast fetching music anyway so this is good enough.
      // -----------------------------------------------------------------------
      if ((gSystemCycles - myDPCPCycles) >= 60)
      {
        // Let's update counters and flags of the music mode data fetchers
        for (int i=0; i<3;i++) 
        {
            myMusicCounters[i] += myMusicFrequencies[i];
            myMusicCountersShifted[i] = myMusicCounters[i] >> 27;
        }
        myDPCPCycles = gSystemCycles;
      }

      // using myDisplayImageDPCP[] instead of myDPC[] because waveforms
      // can be modified during runtime.
      return (uInt8) (myDisplayImageDPCP[(myMusicWaveforms[0]) + (myMusicCountersShifted[0])] +
                 myDisplayImageDPCP[(myMusicWaveforms[1]) + (myMusicCountersShifted[1])] +
                 myDisplayImageDPCP[(myMusicWaveforms[2]) + (myMusicCountersShifted[2])]);
      break;
    }
          
    case 0x0008:
        return myDisplayImageDPCP[myCounters[0]++];
        break;
    case 0x0009:
        return myDisplayImageDPCP[myCounters[1]++];
        break;
    case 0x000A:
        return myDisplayImageDPCP[myCounters[2]++];
        break;
    case 0x000B:
        return myDisplayImageDPCP[myCounters[3]++];
        break;
    case 0x000C:
        return myDisplayImageDPCP[myCounters[4]++];
        break;
    case 0x000D:
        return myDisplayImageDPCP[myCounters[5]++];
        break;
    case 0x000E:
        return myDisplayImageDPCP[myCounters[6]++];
        break;
    case 0x000F:
        return myDisplayImageDPCP[myCounters[7]++];
        break;
           
    case 0x0010:
        if (((myTops[0]-(myCounters[0] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[0]) return myDisplayImageDPCP[myCounters[0]++ & 0xFFF]; else {myCounters[0]++;return 0;}
        break;
    case 0x0011:
        if (((myTops[1]-(myCounters[1] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[1]) return myDisplayImageDPCP[myCounters[1]++ & 0xFFF]; else {myCounters[1]++;return 0;}
        break;
    case 0x0012:
        if (((myTops[2]-(myCounters[2] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[2]) return myDisplayImageDPCP[myCounters[2]++ & 0xFFF]; else {myCounters[2]++;return 0;}
        break;
    case 0x0013:
        if (((myTops[3]-(myCounters[3] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[3]) return myDisplayImageDPCP[myCounters[3]++ & 0xFFF]; else {myCounters[3]++;return 0;}
        break;
    case 0x0014:
        if (((myTops[4]-(myCounters[4] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[4]) return myDisplayImageDPCP[myCounters[4]++ & 0xFFF]; else {myCounters[4]++;return 0;}
        break;
    case 0x0015:
        if (((myTops[5]-(myCounters[5] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[5]) return myDisplayImageDPCP[myCounters[5]++ & 0xFFF]; else {myCounters[5]++;return 0;}
        break;
    case 0x0016:
        if (((myTops[6]-(myCounters[6] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[6]) return myDisplayImageDPCP[myCounters[6]++ & 0xFFF]; else {myCounters[6]++;return 0;}
        break;
    case 0x0017:
        if (((myTops[7]-(myCounters[7] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[7]) return myDisplayImageDPCP[myCounters[7]++ & 0xFFF]; else {myCounters[7]++;return 0;}
        break;
          
    case 0x0018:
        result = myDisplayImageDPCP[myFractionalCounters[0] >> 8];
        myFractionalCounters[0] = (myFractionalCounters[0] + myFractionalIncrements[0]);// & 0x0fffff;
        return result;
        break;
    case 0x0019:
        result = myDisplayImageDPCP[myFractionalCounters[1] >> 8];
        myFractionalCounters[1] = (myFractionalCounters[1] + myFractionalIncrements[1]);// & 0x0fffff;
        return result;
        break;
    case 0x001A:
        result = myDisplayImageDPCP[myFractionalCounters[2] >> 8];
        myFractionalCounters[2] = (myFractionalCounters[2] + myFractionalIncrements[2]);// & 0x0fffff;
        return result;
        break;
    case 0x001B:
        result = myDisplayImageDPCP[myFractionalCounters[3] >> 8];
        myFractionalCounters[3] = (myFractionalCounters[3] + myFractionalIncrements[3]);// & 0x0fffff;
        return result;
        break;
    case 0x001C:
        result = myDisplayImageDPCP[myFractionalCounters[4] >> 8];
        myFractionalCounters[4] = (myFractionalCounters[4] + myFractionalIncrements[4]);// & 0x0fffff;
        return result;
        break;
    case 0x001D:
        result = myDisplayImageDPCP[myFractionalCounters[5] >> 8];
        myFractionalCounters[5] = (myFractionalCounters[5] + myFractionalIncrements[5]);// & 0x0fffff;
        return result;
        break;
    case 0x001E:
        result = myDisplayImageDPCP[myFractionalCounters[6] >> 8];
        myFractionalCounters[6] = (myFractionalCounters[6] + myFractionalIncrements[6]);// & 0x0fffff;
        return result;
        break;
    case 0x001F:
        result = myDisplayImageDPCP[myFractionalCounters[7] >> 8];
        myFractionalCounters[7] = (myFractionalCounters[7] + myFractionalIncrements[7]);// & 0x0fffff;
        return result;
        break;
          
    case 0x0020:
        return (((myTops[0]-(myCounters[0] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[0]) ? 0xFF : 0;
        break;
    case 0x0021:
        return (((myTops[1]-(myCounters[1] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[1]) ? 0xFF : 0;
        break;
    case 0x0022:
        return (((myTops[2]-(myCounters[2] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[2]) ? 0xFF : 0;
        break;
    case 0x0023:
        return (((myTops[3]-(myCounters[3] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[3]) ? 0xFF : 0;
        break;
    case 0x0024:
        return (((myTops[4]-(myCounters[4] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[4]) ? 0xFF : 0;
        break;
    case 0x0025:
        return (((myTops[5]-(myCounters[5] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[5]) ? 0xFF : 0;
        break;
    case 0x0026:
        return (((myTops[6]-(myCounters[6] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[6]) ? 0xFF : 0;
        break;
    case 0x0027:
        return (((myTops[7]-(myCounters[7] & 0x00ff)) & 0xFF) > myTopsMinusBottoms[7]) ? 0xFF : 0;
        break;
          
    default:
        return myCartDPC->peekFetch(address);
        break;
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE uInt8 M6502Low::peek_DPCP(uInt16 address)
{
  ++gSystemCycles;
  
  if (address & 0x1000)
  {
      address &= 0xFFF;
      if (address < 0x28)
      {
          return peek_Fetch(address);
      }
      else if (address >= 0xFF4) 
      {
        switch (address)
        {
            case 0x0FF6:  myDPCptr = &myDPC[0x0000];return myDPCptr[address];
            case 0x0FF7:  myDPCptr = &myDPC[0x1000];return myDPCptr[address];
            case 0x0FF8:  myDPCptr = &myDPC[0x2000];return myDPCptr[address];
            case 0x0FF9:  myDPCptr = &myDPC[0x3000];return myDPCptr[address];
            case 0x0FFA:  myDPCptr = &myDPC[0x4000];return myDPCptr[address];
            case 0x0FFB:  myDPCptr = &myDPC[0x5000];return myDPCptr[address];
        }
      }
      return myDPCptr[(address)];
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek_DPCPPC(uInt16 address)
{
  ++gSystemCycles;
  return myDPCptr[(address & 0xFFF)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502Low::poke_DPCP(uInt16 address, uInt8 value)
{
  ++gSystemCycles;
  
  if (address & 0x1000)
  {
      return myCartDPC->poke(address, value);
  }
  else
  {
      if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
      else if (address & 0x200) theM6532->poke(address, value);
      else theTIA->poke(address, value);
  }    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502Low::execute_DPCP(uInt16 number)
{
    uInt32 fast_loop = number;
    uInt16 operandAddress;
    uInt8 operand;
    
    // Clear all of the execution status bits except for the fatal error bit
    myExecutionStatus = 0;

    while (fast_loop-- && !myExecutionStatus)
    {
      // Get the next 6502 instruction - do this the fast way!
      ++gSystemCycles;
      operand = myDPCptr[(PC++ & 0xFFF)];
        
      // 6502 instruction emulation is generated by an M4 macro file
      switch (operand)
      {
        #define DPC_PLUS_FAST_FETCH              
        #define peek    peek_DPCP
        #define peek_PC peek_DPCPPC
        #define poke    poke_DPCP
        #include "M6502Low.ins"    
        #undef peek
        #undef peek_PC
        #undef poke
        #undef DPC_PLUS_FAST_FETCH
      }
    }
}