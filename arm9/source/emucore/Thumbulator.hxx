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

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================
#define THUMB_SUPPORT
#ifdef THUMB_SUPPORT

#include "bspf.hxx"

#define ROMADDMASK 0x7FFF
#define RAMADDMASK 0x1FFF

#define ROMSIZE (ROMADDMASK+1)
#define RAMSIZE (RAMADDMASK+1)

//0b10000 User       PC, R14 to R0, CPSR
//0b10001 FIQ        PC, R14_fiq to R8_fiq, R7 to R0, CPSR, SPSR_fiq
//0b10010 IRQ        PC, R14_irq, R13_irq, R12 to R0, CPSR, SPSR_irq
//0b10011 Supervisor PC, R14_svc, R13_svc, R12 to R0, CPSR, SPSR_svc
//0b10111 Abort      PC, R14_abt, R13_abt, R12 to R0, CPSR, SPSR_abt
//0b11011 Undefined  PC, R14_und, R13_und, R12 to R0, CPSR, SPSR_und
//0b11111 System

#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1B
#define MODE_SYS 0x1F

#define CPSR_T (1<<5)
#define CPSR_F (1<<6)
#define CPSR_I (1<<7)
#define CPSR_N (1<<31)
#define CPSR_Z (1<<30)
#define CPSR_C (1<<29)
#define CPSR_V (1<<28)
#define CPSR_Q (1<<27)

class Thumbulator
{
  public:
    Thumbulator(uInt16* rom);
    ~Thumbulator();

    /**
      Run the ARM code, and return when finished.  A string exception is
      thrown in case of any fatal errors/aborts (if enabled), containing the
      actual error, and the contents of the registers at that point in time.

      @return  The results of any debugging output (if enabled),
               otherwise an empty string
    */
    void run();

  private:
    
    enum class Op : uInt8 {
      invalid,
      adc,
      add1, 
      add2_0, add2_1, add2_2, add2_3, add2_4, add2_5, add2_6, add2_7,
      add3, add4, add5, add6, add7,
      and_,
      asr1, asr2,
      b1_000, b1_100, b1_200, b1_300, b1_400, b1_500, b1_600, b1_700,
      b1_800, b1_900, b1_a00, b1_b00, b1_c00, b1_d00, b1_e00, b1_f00,
      b2_0, b2_1,
      bic,
      bkpt,
      blx1, blx2,
      bx,
      cmn,
      cmp1_0, cmp1_1, cmp1_2, cmp1_3, cmp1_4, cmp1_5, cmp1_6, cmp1_7,
      cmp2, cmp3,
      cps,
      cpy,
      eor,
      ldmia,
      ldr1, ldr2, ldr3, 
      ldr4_0, ldr4_1, ldr4_2, ldr4_3, ldr4_4, ldr4_5, ldr4_6, ldr4_7,
      ldrb1, ldrb2,
      ldrh1, ldrh2,
      ldrsb,
      ldrsh,
      lsl1, lsl2,
      lsr1, lsr2,
      mov1, mov2, mov3,
      mul,
      mvn,
      neg,
      orr,
      pop,
      push,
      rev,
      rev16,
      revsh,
      ror,
      sbc,
      setend,
      stmia,
      str1, str2, str3,
      strb1, strb2,
      strh1, strh2,
      sub1, sub2, sub3, sub4,
      swi,
      sxtb,
      sxth,
      tst,
      uxtb,
      uxth
    };
    
    inline uInt16 read16 ( uInt32 addr );
    inline uInt32 read32 ( uInt32 );
    inline uInt32 readRAM32 ( uInt32 );
    inline uInt32 readROM32 ( uInt32 );
    inline void write16 ( uInt32 addr, uInt32 data );
    inline void write32 ( uInt32 addr, uInt32 data );

    inline void do_cflag ( uInt32 a, uInt32 b, uInt32 c );
    inline void do_cflag_fast ( uInt32 a, uInt32 b );    
    inline void do_sub_vflag ( uInt32 a, uInt32 b, uInt32 c );
    inline void do_add_vflag ( uInt32 a, uInt32 b, uInt32 c );
    inline void do_cflag_bit ( uInt32 x );
    inline void do_vflag_bit ( uInt32 x );

    void execute ( void );
    int reset ( void );
    
    static Op decodeInstructionWord(uint16_t inst);

  private:
    uInt32 halfadd;
};

#endif
