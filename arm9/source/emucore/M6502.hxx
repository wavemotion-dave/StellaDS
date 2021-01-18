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
// $Id: M6502.hxx,v 1.2 2002/05/13 19:10:25 stephena Exp $
//============================================================================

#ifndef M6502_HXX
#define M6502_HXX

class D6502;
class M6502;

#include "bspf.hxx"
#include "System.hxx"

extern uInt8 A; 
extern uInt8 X; 
extern uInt8 Y; 
extern uInt8 SP;
extern uInt16 PC;
extern uInt8 N; 
extern uInt8 V; 
extern uInt8 B; 
extern uInt8 D; 
extern uInt8 I; 
extern uInt8 notZ;
extern uInt8 C;


/**
  This is an abstract base class for classes that emulate the
  6502 microprocessor.  The 6502 is an 8-bit microprocessor that
  has a 64K addressing space.

  @author  Bradford W. Mott
  @version $Id: M6502.hxx,v 1.2 2002/05/13 19:10:25 stephena Exp $ 
*/
class M6502
{
  public:
    /**
      The 6502 debugger class is a friend who needs special access
    */
    friend class D6502;

  public:
    /**
      Enumeration of the 6502 addressing modes
    */
    enum AddressingMode 
    {
      Absolute, AbsoluteX, AbsoluteY, Immediate, Implied,
      Indirect, IndirectX, IndirectY, Invalid, Relative,
      Zero, ZeroX, ZeroY
    };

  public:
    /**
      Create a new 6502 microprocessor with the specified cycle 
      multiplier.  The cycle multiplier is the number of system cycles 
      per processor cycle.

      @param systemCyclesPerProcessorCycle The cycle multiplier
    */
    M6502(uInt32 systemCyclesPerProcessorCycle);

    /**
      Destructor
    */
    virtual ~M6502();

  public:
    /**
      Install the processor in the specified system.  Invoked by the
      system when the processor is attached to it.

      @param system The system the processor should install itself in
    */
    virtual void install(System& system);

  public:
    /**
      Reset the processor to its power-on state.  This method should not 
      be invoked until the entire 6502 system is constructed and installed
      since it involves reading the reset vector from memory.
    */
    virtual void reset();

    /**
      Request a maskable interrupt
    */
    virtual void irq();

    /**
      Request a non-maskable interrupt
    */
    virtual void nmi();

    /**
      Get a null terminated string which is the processor's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const = 0;

  public:
    /**
      Execute instructions until the specified number of instructions
      is executed, someone stops execution, or an error occurs.  Answers
      true iff execution stops normally.

      @param number Indicates the number of instructions to execute
      @return true iff execution stops normally
    */
    virtual bool execute(uInt16 number) = 0;

    /**
      Tell the processor to stop executing instructions.  Invoking this 
      method while the processor is executing instructions will stop 
      execution as soon as possible.
    */
    void stop();

    /**
      Answer true iff a fatal error has occured from which the processor
      cannot recover (i.e. illegal instruction, etc.)

      @return true iff a fatal error has occured
    */
    bool fatalError() const
    {
      return myExecutionStatus & FatalErrorBit;
    }
  
  public:
    /**
      Overload the ostream output operator for addressing modes.

      @param out The stream to output the addressing mode to
      @param mode The addressing mode to output
    */
    friend ostream& operator<<(ostream& out, const AddressingMode& mode);

  protected:
    /**
      Get the 8-bit value of the Processor Status register.

      @return The processor status register
    */
    uInt8 PS() const;

    /**
      Change the Processor Status register to correspond to the given value.

      @param ps The value to set the processor status register to
    */
    void PS(uInt8 ps);

  protected:

    /** 
      Bit fields used to indicate that certain conditions need to be 
      handled such as stopping execution, fatal errors, maskable interrupts 
      and non-maskable interrupts
    */
    uInt8 myExecutionStatus;

    /**
      Constants used for setting bits in myExecutionStatus
    */
    enum 
    {
      StopExecutionBit = 0x01,
      FatalErrorBit = 0x02,
      MaskableInterruptBit = 0x04,
      NonmaskableInterruptBit = 0x08
    };
  
    /// Pointer to the system the processor is installed in or the null pointer
    System* mySystem;

    /// Indicates the number of system cycles per processor cycle 
    const uInt32 mySystemCyclesPerProcessorCycle;

  protected:
    /// Lookup table used for binary-code-decimal math
    static uInt8 ourBCDTable[2][256];
};
#endif

