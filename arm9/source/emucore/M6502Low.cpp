//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: M6502Low.cxx,v 1.2 2002/05/13 19:10:25 stephena Exp $
//============================================================================
#include <nds.h>
#include "Cart.hxx"
#include "CartAR.hxx"
#include "System.hxx"
#include "M6502Low.hxx"
#include "TIA.hxx"
#include "M6532.hxx"

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
        
uInt32 NumberOfDistinctAccesses         __attribute__((section(".dtcm")));   // For AR cart use only - track the # of distince PC accesses
uInt8 noBanking                         __attribute__((section(".dtcm")))=0; // Set to 1 for carts that are non-banking to invoke faster peek/poke handling
uInt16 f8_bankbit                       __attribute__((section(".dtcm"))) = 0x1FFF;

extern CartridgeAR *myAR;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502Low::M6502Low(uInt32 systemCyclesPerProcessorCycle)
    : M6502(systemCyclesPerProcessorCycle)
{
    NumberOfDistinctAccesses = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502Low::~M6502Low()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define fake_peek()  gSystemCycles++;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek(uInt16 address)
{
  gSystemCycles++;

  PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
  if(access.directPeekBase != 0) myDataBusState = *(access.directPeekBase + (address & MY_PAGE_MASK));
  else myDataBusState = access.device->peek(address);

  return myDataBusState;    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502Low::peek_PC(uInt16 address)
{
  gSystemCycles++;

  PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
  if(access.directPeekBase != 0) myDataBusState = *(access.directPeekBase + (address & MY_PAGE_MASK));
  else myDataBusState = access.device->peek(address);

  return myDataBusState;    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502Low::poke(uInt16 address, uInt8 value)
{
  gSystemCycles++;
    
  PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
  if(access.directPokeBase != 0) *(access.directPokeBase + (address & MY_PAGE_MASK)) = value;
  else access.device->poke(address, value);
    
  myDataBusState = value;    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502Low::execute(uInt16 number)
{
  // --------------------------------------------------------------------
  // For games that can be specially executed for speed... do so here.
  // --------------------------------------------------------------------
  if (noBanking)
  {
      if (noBanking == 2)
          return execute_F8(number);    // If we are F8
      else
          return execute_NB(number);    // If we are 2K or 4K (non-banked), we can run faster here...
  }
  // ----------------------------------------------------------------
  // For Starpath Supercharger games, we must track distinct memory
  // access. This takes time so we don't do it for other game types...
  // ----------------------------------------------------------------
  if (myCartInfo.special == SPEC_AR)
  {
    return execute_AR(number);      // To optimize the complicated AR memory handling
  }
      
  uInt16 fast_loop = number;
  // Clear all of the execution status bits except for the fatal error bit
  myExecutionStatus &= FatalErrorBit;

  // Loop until execution is stopped or a fatal error occurs
  for(;;)
  {
    uInt16 operandAddress=0;
    uInt8 operand=0;
    for(; !myExecutionStatus && (fast_loop != 0); --fast_loop)
    {
      // Get the next 6502 instruction - do this the fast way!
      gSystemCycles++;

      PageAccess& access = myPageAccessTable[(PC & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
      if (access.directPeekBase != 0) myDataBusState = *(access.directPeekBase + (PC & MY_PAGE_MASK));
      else myDataBusState = access.device->peek(PC);        
      PC++;

      // 6502 instruction emulation is generated by an M4 macro file
      switch (myDataBusState)
      {
        #include "M6502Low.ins"         // All 6502 instructions are handled in this include file   
      }
    }
      
    if (myExecutionStatus)
    {
        // See if we need to handle an interrupt
        if(myExecutionStatus & (MaskableInterruptBit | NonmaskableInterruptBit))
        {
          // Yes, so handle the interrupt
          interruptHandler();
        }

        // See if execution has been stopped
        if(myExecutionStatus & StopExecutionBit)
        {
          // Yes, so answer that everything finished fine
          return true;
        }
        else
        // See if a fatal error has occured
        if(myExecutionStatus & FatalErrorBit)
        {
          // Yes, so answer that something when wrong
          return false;
        }
    }
    else return true;  // we've executed the specified number of instructions
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void M6502Low::interruptHandler()
{
  // Handle the interrupt
  if((myExecutionStatus & MaskableInterruptBit) && !I)
  {
    gSystemCycles += 7; // 7 cycle operation
    mySystem->poke(0x0100 + SP--, (PC - 1) >> 8);		// The high byte of the return address
    mySystem->poke(0x0100 + SP--, (PC - 1) & 0x00ff);	// The low byte of the return address
    mySystem->poke(0x0100 + SP--, PS() & (~0x10));	// The status byte from the processor status register
    D = false;	// Set our flags
    I = true;
    PC = (uInt16)mySystem->peek(0xFFFE) | ((uInt16)mySystem->peek(0xFFFF) << 8);	// Grab the address from the interrupt vector
  }
  else if(myExecutionStatus & NonmaskableInterruptBit)
  {
    gSystemCycles += 7; // 7 cycle operation
    mySystem->poke(0x0100 + SP--, (PC - 1) >> 8);
    mySystem->poke(0x0100 + SP--, (PC - 1) & 0x00ff);
    mySystem->poke(0x0100 + SP--, PS() & (~0x10));
    D = false;
    PC = (uInt16)mySystem->peek(0xFFFA) | ((uInt16)mySystem->peek(0xFFFB) << 8);
  }

  // Clear the interrupt bits in myExecutionStatus
  myExecutionStatus &= ~(MaskableInterruptBit | NonmaskableInterruptBit);
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
extern TIA   *theTIA;
extern M6532 *theM6532;

inline uInt8 M6502Low::peek_PCNB(uInt16 address)
{
  gSystemCycles++;
  myDataBusState = fast_cart_buffer[address & 0xFFF];
  return myDataBusState;    
}


inline uInt8 M6502Low::peek_NB(uInt16 address)
{
  gSystemCycles++;

  if (address & 0xF000)
  {
      myDataBusState = fast_cart_buffer[address & 0xFFF];    
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }

  return myDataBusState;    
}


inline void M6502Low::poke_NB(uInt16 address, uInt8 value)
{
  gSystemCycles++;
  
  if ((address & 0x280) == 0x80) myRAM[address & 0x7F] = value;
  else if (address & 0x200) theM6532->poke(address, value);
  else theTIA->poke(address, value);
}

ITCM_CODE void M6502Low::interruptHandlerNB()
{
  // Handle the interrupt
  if((myExecutionStatus & MaskableInterruptBit) && !I)
  {
    gSystemCycles += 7; // 7 cycle operation
    poke_NB(0x0100 + SP--, (PC - 1) >> 8);		// The high byte of the return address
    poke_NB(0x0100 + SP--, (PC - 1) & 0x00ff);	// The low byte of the return address
    poke_NB(0x0100 + SP--, PS() & (~0x10));	// The status byte from the processor status register
    D = false;	// Set our flags
    I = true;
    PC = (uInt16)peek_PCNB(0xFFFE) | ((uInt16)peek_PCNB(0xFFFF) << 8);	// Grab the address from the interrupt vector
  }
  else if(myExecutionStatus & NonmaskableInterruptBit)
  {
    gSystemCycles += 7; // 7 cycle operation
    poke_NB(0x0100 + SP--, (PC - 1) >> 8);
    poke_NB(0x0100 + SP--, (PC - 1) & 0x00ff);
    poke_NB(0x0100 + SP--, PS() & (~0x10));
    D = false;
    PC = (uInt16)peek_PCNB(0xFFFA) | ((uInt16)peek_PCNB(0xFFFB) << 8);
  }

  // Clear the interrupt bits in myExecutionStatus
  myExecutionStatus &= ~(MaskableInterruptBit | NonmaskableInterruptBit);
}

bool M6502Low::execute_NB(uInt16 number)
{
  uInt16 fast_loop = number;
  // Clear all of the execution status bits except for the fatal error bit
  myExecutionStatus &= FatalErrorBit;

  // Loop until execution is stopped or a fatal error occurs
  for(;;)
  {
    uInt16 operandAddress=0;
    uInt8 operand=0;
    for(; !myExecutionStatus && (fast_loop != 0); --fast_loop)
    {
      // Get the next 6502 instruction - do this the fast way!
      gSystemCycles++;
      myDataBusState = fast_cart_buffer[PC & 0xFFF];
      PC++;

      // 6502 instruction emulation is generated by an M4 macro file
      switch (myDataBusState)
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
      
    if (myExecutionStatus)
    {
        // See if we need to handle an interrupt
        if(myExecutionStatus & (MaskableInterruptBit | NonmaskableInterruptBit))
        {
          // Yes, so handle the interrupt
          interruptHandlerNB();
        }

        // See if execution has been stopped
        if(myExecutionStatus & StopExecutionBit)
        {
          // Yes, so answer that everything finished fine
          return true;
        }
        else
        // See if a fatal error has occured
        if(myExecutionStatus & FatalErrorBit)
        {
          // Yes, so answer that something when wrong
          return false;
        }
    }
    else return true;  // we've executed the specified number of instructions
  }
}


inline uInt8 M6502Low::peek_PCF8(uInt16 address)
{
  gSystemCycles++;
  myDataBusState = fast_cart_buffer[address & f8_bankbit];
  return myDataBusState;    
}


inline uInt8 M6502Low::peek_F8(uInt16 address)
{
  gSystemCycles++;

  if (address & 0x1000)
  {
      if ((address & 0x0FFF) == 0x0FF8) f8_bankbit=0x0FFF;
      else if ((address & 0x0FFF) == 0x0FF9) f8_bankbit=0x1FFF;
      myDataBusState = fast_cart_buffer[address & f8_bankbit];
  }
  else
  {
      if ((address & 0x280) == 0x80) return myRAM[address & 0x7F];
      else if (address & 0x200) return theM6532->peek(address);
      else return theTIA->peek(address);
  }

  return myDataBusState;    
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void M6502Low::interruptHandlerF8()
{
  // Handle the interrupt
  if((myExecutionStatus & MaskableInterruptBit) && !I)
  {
    gSystemCycles += 7; // 7 cycle operation
    poke_F8(0x0100 + SP--, (PC - 1) >> 8);		// The high byte of the return address
    poke_F8(0x0100 + SP--, (PC - 1) & 0x00ff);	// The low byte of the return address
    poke_F8(0x0100 + SP--, PS() & (~0x10));	// The status byte from the processor status register
    D = false;	// Set our flags
    I = true;
    PC = (uInt16)peek_PCF8(0xFFFE) | ((uInt16)peek_PCF8(0xFFFF) << 8);	// Grab the address from the interrupt vector
  }
  else if(myExecutionStatus & NonmaskableInterruptBit)
  {
    gSystemCycles += 7; // 7 cycle operation
    poke_F8(0x0100 + SP--, (PC - 1) >> 8);
    poke_F8(0x0100 + SP--, (PC - 1) & 0x00ff);
    poke_F8(0x0100 + SP--, PS() & (~0x10));
    D = false;
    PC = (uInt16)peek_PCF8(0xFFFA) | ((uInt16)peek_PCF8(0xFFFB) << 8);
  }

  // Clear the interrupt bits in myExecutionStatus
  myExecutionStatus &= ~(MaskableInterruptBit | NonmaskableInterruptBit);
}

bool M6502Low::execute_F8(uInt16 number)
{
  uInt16 fast_loop = number;
  // Clear all of the execution status bits except for the fatal error bit
  myExecutionStatus &= FatalErrorBit;

  // Loop until execution is stopped or a fatal error occurs
  for(;;)
  {
    uInt16 operandAddress=0;
    uInt8 operand=0;
    for(; !myExecutionStatus && (fast_loop != 0); --fast_loop)
    {
      // Get the next 6502 instruction - do this the fast way!
      gSystemCycles++;
      myDataBusState = fast_cart_buffer[PC & f8_bankbit];
      PC++;

      // 6502 instruction emulation is generated by an M4 macro file
      switch (myDataBusState)
      {
        // A trick of the light... here we map peek/poke to the "NB" cart versions. This improves speed for non-bank-switched carts.
        #define peek    peek_F8
        #define peek_PC peek_PCF8              
        #define poke    poke_F8
        #include "M6502Low.ins"        
        #undef peek   
        #undef peek_PC
        #undef poke 
      }
    }
      
    if (myExecutionStatus)
    {
        // See if we need to handle an interrupt
        if(myExecutionStatus & (MaskableInterruptBit | NonmaskableInterruptBit))
        {
          // Yes, so handle the interrupt
          interruptHandlerF8();
        }

        // See if execution has been stopped
        if(myExecutionStatus & StopExecutionBit)
        {
          // Yes, so answer that everything finished fine
          return true;
        }
        else
        // See if a fatal error has occured
        if(myExecutionStatus & FatalErrorBit)
        {
          // Yes, so answer that something when wrong
          return false;
        }
    }
    else return true;  // we've executed the specified number of instructions
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
ITCM_CODE uInt8 M6502Low::peek_AR(uInt16 address)
{
  NumberOfDistinctAccesses++;
  gSystemCycles++;
    
  if (address & 0xF000)
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

            return myImage1[addr];
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
                  if(!bPossibleLoad)    // Can't poke to ROM :-)
                     myImage1[addr] = myDataHoldRegister;
                  myWritePending = false;
              }
          }

          return myImage1[addr];
      }
      else // WE are in the lower bank
      {
          // Is the data hold register being set?
          if((!(addr & 0x0F00)) && (!myWritePending))
          {
            myDataHoldRegister = addr;
            NumberOfDistinctAccesses = 0;
            if (myWriteEnabled) myWritePending = true;
          }
          // Handle poke if writing enabled
          else if (myWritePending)
          {
              if (NumberOfDistinctAccesses >= DISTINCT_THRESHOLD)
              {
                  myImage0[addr] = myDataHoldRegister;
                  myWritePending = false;
              }
          }

         return myImage0[addr];
      }
  }
  else
  {
      PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
      if(access.directPeekBase != 0) myDataBusState = *(access.directPeekBase + (address & MY_PAGE_MASK));
      else myDataBusState = access.device->peek(address);        
      return myDataBusState; 
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void M6502Low::poke_AR(uInt16 address, uInt8 value)
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
  if (address & 0xF000)
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
      PageAccess& access = myPageAccessTable[(address & MY_ADDR_MASK) >> MY_PAGE_SHIFT];
      if(access.directPokeBase != 0) *(access.directPokeBase + (address & MY_PAGE_MASK)) = value;
      else access.device->poke(address, value);
      myDataBusState = value;
  }
}


bool M6502Low::execute_AR(uInt16 number)
{
  uInt16 fast_loop = number;
    
  // Clear all of the execution status bits except for the fatal error bit
  myExecutionStatus &= FatalErrorBit;

  // Loop until execution is stopped or a fatal error occurs
  for(;;)
  {
    uInt16 operandAddress;
    uInt8 operand;
      
    for(; !myExecutionStatus && (fast_loop != 0); --fast_loop)
    {
      // Get the next 6502 instruction
      myDataBusState = peek_AR(PC++);
        
      // 6502 instruction emulation is generated by an M4 macro file
      switch (myDataBusState)
      {
        // A trick of the light... here we map peek/poke to the "AR" cart versions. Slower but needed for some games...
        #define peek    peek_AR
        #define peek_PC peek_AR              
        #define poke    poke_AR
        #include "M6502Low.ins"        
        #undef peek
        #undef peek_PC
        #undef poke
      }
    }

    if (myExecutionStatus)
    {
        // See if we need to handle an interrupt
        if(myExecutionStatus & (MaskableInterruptBit | NonmaskableInterruptBit))
        {
          // Yes, so handle the interrupt
          interruptHandler();
        }

        // See if execution has been stopped
        if(myExecutionStatus & StopExecutionBit)
        {
          // Yes, so answer that everything finished fine
          return true;
        }
        else
        // See if a fatal error has occured
        if(myExecutionStatus & FatalErrorBit)
        {
          // Yes, so answer that something when wrong
          return false;
        }
    } 
    else return true;  // we've executed the specified number of instructions
  }
}

// End of file
