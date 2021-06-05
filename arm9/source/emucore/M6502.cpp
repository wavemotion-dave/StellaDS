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
// $Id: M6502.cxx,v 1.2 2001/12/29 17:55:59 bwmott Exp $
//============================================================================

#include "M6502.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::M6502(uInt32 systemCyclesPerProcessorCycle)
    : myExecutionStatus(0),
      mySystem(0)
{
  uInt16 t;

  // Compute the BCD lookup table
  for(t = 0; t < 256; ++t)
  {
    ourBCDTable[0][t] = ((t >> 4) * 10) + (t & 0x0f);
    ourBCDTable[1][t] = (((t % 100) / 10) << 4) | (t % 10);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::~M6502()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::install(System& system)
{
  // Remember which system I'm installed in
  mySystem = &system;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::reset()
{
  // Clear the execution status flags
  myExecutionStatus = 0;

  // Set registers to default values
  A = X = Y = 0;
  SP = 0xff;
  PS(0x20);

  // Load PC from the reset vector
  PC = (uInt16)mySystem->peek(0xfffc) | ((uInt16)mySystem->peek(0xfffd) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::irq()
{
  myExecutionStatus |= MaskableInterruptBit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::nmi()
{
  myExecutionStatus |= NonmaskableInterruptBit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::stop()
{
  myExecutionStatus |= StopExecutionBit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6502::PS() const
{
  uInt8 ps = 0x20;

  if(N) 
    ps |= 0x80;
  if(V) 
    ps |= 0x40;
  if(B) 
    ps |= 0x10;
  if(D) 
    ps |= 0x08;
  if(I) 
    ps |= 0x04;
  if(!notZ) 
    ps |= 0x02;
  if(C) 
    ps |= 0x01;

  return ps;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::PS(uInt8 ps)
{
  N = ps & 0x80;
  V = ps & 0x40;
  B = ps & 0x10;
  D = ps & 0x08;
  I = ps & 0x04;
  notZ = !(ps & 0x02);
  C = ps & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6502::ourBCDTable[2][256];

