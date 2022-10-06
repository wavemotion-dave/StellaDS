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

#ifndef M6507LOW_HXX
#define M6507LOW_HXX

class M6507Low;

#include "bspf.hxx"
#include "M6502.hxx"

/**
  This class provides a low compatibility 6502 microprocessor emulator.  
  The memory accesses and cycle updates of this emulator are not 100% 
  accurate as shown below:

    1. Only memory accesses which are actually needed are done 
       (i.e. no "false" reads and writes are performed)

    2. Cycle counts are updated at the beginning of the instruction
       execution and not valid at the sub-instruction level

  If speed is the most important issue then use this class, however, if 
  better compatibility is neccessary use one of the other 6502 classes.
  
  @author  Bradford W. Mott
  @version $Id: M6502Low.hxx,v 1.2 2002/05/13 19:10:25 stephena Exp $
*/
class M6502Low : public M6502
{
  public:
    /**
      Create a new low compatibility 6502 microprocessor with the specified 
      cycle multiplier.

      @param systemCyclesPerProcessorCycle The cycle multiplier
    */
    M6502Low(uInt32 systemCyclesPerProcessorCycle);

    /**
      Destructor
    */
    virtual ~M6502Low();

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

    /**
      Get a null terminated string which is the processors's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const;
    
  protected:
    /*
      Get the byte at the specified address 

      @return The byte at the specified address
    */
    
    inline uInt8 peek(uInt16 address);
    inline uInt8 peek_PC(uInt16 address);
    
    inline void poke_AR(uInt16 address, uInt8 value);
    inline uInt8 peek_AR(uInt16 address);
    
    inline uInt8 peek_NB(uInt16 address);
    inline uInt8 peek_PCNB(uInt16 address);
    inline void poke_NB(uInt16 address, uInt8 value);

    inline uInt8 peek_F8(uInt16 address);
    inline uInt8 peek_PCF8(uInt16 address);
    inline void poke_F8(uInt16 address, uInt8 value);

    inline uInt8 peek_F6(uInt16 address);
    inline uInt8 peek_PCF6(uInt16 address);
    inline void poke_F6(uInt16 address, uInt8 value);

    inline uInt8 peek_F4(uInt16 address);
    inline uInt8 peek_PCF4(uInt16 address);
    inline void poke_F4(uInt16 address, uInt8 value);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    void poke(uInt16 address, uInt8 value);

};
#endif

