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

#ifndef M6502HIGH_HXX
#define M6502HIGH_HXX

class M6502High;

#include "bspf.hxx"
#include "M6502.hxx"

/**
  This class provides a high compatibility 6502 microprocessor emulator.  
  The memory accesses and cycle counts it generates are valid at the
  sub-instruction level and "false" reads are generated (such as the ones 
  produced by the Indirect,X addressing when it crosses a page boundary).
  This provides provides better compatibility for hardware that has side
  effects and for games which are very time sensitive.

  @author  Bradford W. Mott
  @version $Id: M6502Hi.hxx,v 1.2 2002/05/13 19:10:25 stephena Exp $
*/
class M6502High : public M6502
{
  public:
    /**
      Create a new high compatibility 6502 microprocessor with the 
      specified cycle multiplier.

      @param systemCyclesPerProcessorCycle The cycle multiplier
    */
    M6502High(uInt32 systemCyclesPerProcessorCycle);

    /**
      Destructor
    */
    virtual ~M6502High();

  public:
    /**
      Execute instructions until the specified number of instructions
      is executed, someone stops execution, or an error occurs.  Answers
      true iff execution stops normally.

      @param number Indicates the number of instructions to execute
      @return true iff execution stops normally
    */
    virtual void execute(uInt16 number);
    virtual void execute_NB(uInt16 number);
    virtual void execute_F8(uInt16 number);
    virtual void execute_F6(uInt16 number);
    virtual void execute_F4(uInt16 number);
    virtual void execute_AR(uInt16 number);
    virtual void execute_F8SC(uInt16 number);
    virtual void execute_F6SC(uInt16 number);
    virtual void execute_DPCP(uInt16 number);

    /**
      Get a null terminated string which is the processors's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const;

  public:
    /**
      Get the number of memory accesses to distinct memory locations

      @return The number of memory accesses to distinct memory locations
    */
    uInt32 distinctAccesses() const
    {
      return myNumberOfDistinctAccesses;
    }

  protected:
    /**
      Called after an interrupt has be requested using irq() or nmi()
    */
    void interruptHandler();

  protected:
    /*
      Get the byte at the specified address and update the cycle
      count

      @return The byte at the specified address
    */
    inline uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value and
      update the cycle count

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    inline void poke(uInt16 address, uInt8 value);

  private:
    // Indicates the numer of distinct memory accesses
    uInt32 myNumberOfDistinctAccesses;

    // Indicates the last address which was accessed
    uInt16 myLastAddress;
};
#endif

