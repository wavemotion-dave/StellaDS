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
#include <nds.h>
#include "bspf.hxx"
#include "Thumbulator.hxx"
#include "Cart.hxx"

uInt32 reg_sys[16]   __attribute__((section(".dtcm"))) = {0};
uInt32  cFlag        __attribute__((section(".dtcm"))) = 0;
uInt32  vFlag        __attribute__((section(".dtcm"))) = 0;

extern bool  isCDFJPlus;
extern uInt8 fast_cart_buffer[];

uInt16 rom[ROMSIZE/2] ALIGN(32);
uInt8  decodedRom[ROMSIZE/2] ALIGN(32);

bool  bSafeThumb  __attribute__((section(".dtcm"))) = 1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Thumbulator(uInt16* rom_ptr)
{
  for (uInt16 i=0; i<ROMSIZE / 2; i++)
  {
    rom[i] = rom_ptr[i];
  }
    
  for(uInt16 i = 0; i < ROMSIZE / 2; ++i)
  {
    decodedRom[i] = (uInt8)decodeInstructionWord(rom[i]);
  }    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::~Thumbulator()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::run( void )
{
  reset();
  execute();
  return;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::write16 ( uInt32 addr, uInt32 data )
{
  uInt16 *ptr = (uInt16*)&fast_cart_buffer[addr & 0xFFFF];
  *ptr = data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::write32 ( uInt32 addr, uInt32 data )
{
  uInt32 *ptr = (uInt32*) &fast_cart_buffer[addr & 0xFFFF];
  *ptr = data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt16 Thumbulator::read16 ( uInt32 addr )
{
  if (addr & 0x40000000)
  {
      uInt16 *ptr = (uInt16*)&fast_cart_buffer[addr & 0xFFFF];
      return *ptr;
  }
  return rom[(addr) >> 1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 Thumbulator::read32 ( uInt32 addr )
{
  uInt32 *ptr;

  if (addr & 0x40000000)
  {
      ptr = (uInt32*) &fast_cart_buffer[addr & 0xFFFF];
  }
  else
  {
      ptr = (uInt32*) &rom[(addr) >> 1];
  }
  return *ptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 Thumbulator::readROM32 ( uInt32 addr )
{
  uInt32 *ptr;
  ptr = (uInt32*) &rom[(addr) >> 1];
  return *ptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 Thumbulator::readRAM32 ( uInt32 addr )
{
  uInt32 *ptr;
  ptr = (uInt32*) &fast_cart_buffer[addr & 0xFFFF];  
  return *ptr;    
}

#define read_register(reg)       reg_sys[reg]
#define write_register(reg,data) reg_sys[reg]=(data)

#define do_znflags(x) ZNflags=(x)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_cflag ( uInt32 a, uInt32 b, uInt32 c )
{
  if (((a|b) & 0x80000000) == 0) cFlag=0;
  else if ((a&b) & 0x80000000) cFlag=1;
  else
  {
      uInt32 rc=(a&0x7FFFFFFF)+(b&0x7FFFFFFF)+c; //carry in
      cFlag = (rc & 0x80000000) ? 1:0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_cflag_fast ( uInt32 a, uInt32 b )  // For when you know the high bit of b is set (i.e. on CMP). Assumes carry in.
{
  if (a & 0x80000000) cFlag=1;
  else
  {
      uInt32 rc=a+(b&0x7FFFFFFF)+1;
      cFlag = (rc & 0x80000000) ? 1:0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_sub_vflag ( uInt32 a, uInt32 b, uInt32 c )
{
  //if the sign bits are different and result matches b
  vFlag = ((((a^b)&0x80000000)) & ((b&c&0x80000000)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_add_vflag ( uInt32 a, uInt32 b, uInt32 c )
{
  //if sign bits are the same and the result is different
  vFlag = (((a&b&0x80000000)) & (((b^c)&0x80000000)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_cflag_bit ( uInt32 x )
{
  cFlag = x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_vflag_bit ( uInt32 x )
{
  vFlag = x;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Op Thumbulator::decodeInstructionWord(uint16_t inst) 
{
  //ADC
  if((inst & 0xFFC0) == 0x4140) return Op::adc;

  //ADD(1) small immediate two registers
  if((inst & 0xFE00) == 0x1C00 && (inst >> 6) & 0x7) return Op::add1;

  //ADD(2) big immediate one register
  if((inst & 0xF800) == 0x3000) 
  {
      switch (inst & 0x0700)
      {
          case 0x0000:  return Op::add2_0;
          case 0x0100:  return Op::add2_1;
          case 0x0200:  return Op::add2_2;
          case 0x0300:  return Op::add2_3;
          case 0x0400:  return Op::add2_4;
          case 0x0500:  return Op::add2_5;
          case 0x0600:  return Op::add2_6;
          case 0x0700:  return Op::add2_7;
      }
      return Op::add2_0;
  }

  //ADD(3) three registers
  if((inst & 0xFE00) == 0x1800) return Op::add3;

  //ADD(4) two registers one or both high no flags
  if((inst & 0xFF00) == 0x4400) return Op::add4;

  //ADD(5) rd = pc plus immediate
  if((inst & 0xF800) == 0xA000) return Op::add5;

  //ADD(6) rd = sp plus immediate
  if((inst & 0xF800) == 0xA800) return Op::add6;

  //ADD(7) sp plus immediate
  if((inst & 0xFF80) == 0xB000) return Op::add7;

  //AND
  if((inst & 0xFFC0) == 0x4000) return Op::and_;

  //ASR(1) two register immediate
  if((inst & 0xF800) == 0x1000) return Op::asr1;

  //ASR(2) two register
  if((inst & 0xFFC0) == 0x4100) return Op::asr2;

  //B(1) conditional branch
  if((inst & 0xF000) == 0xD000) 
  {
      switch(inst & 0x0F00)
      {
          case 0x000: 
              {
                 if (inst & 0x80) return Op::b1_000_neg;
                 return Op::b1_000_pos;
              }
          case 0x100:
              {
                 if (inst & 0x80) return Op::b1_100_neg;
                 return Op::b1_100_pos;
              }
          case 0x200: return Op::b1_200;
          case 0x300: return Op::b1_300;
          case 0x400: return Op::b1_400;
          case 0x500: return Op::b1_500;
          case 0x600: return Op::b1_600;
          case 0x700: return Op::b1_700;
          case 0x800: return Op::b1_800;
          case 0x900: return Op::b1_900;
          case 0xA00: return Op::b1_a00;
          case 0xB00: return Op::b1_b00;
          case 0xC00: return Op::b1_c00;
          case 0xD00: return Op::b1_d00;
          case 0xE00: return Op::b1_e00;
          case 0xF00: return Op::b1_f00;
      }
      return Op::b1_f00;
  }

  //B(2) unconditional branch
  if((inst & 0xF800) == 0xE000) 
  {
      if (inst&(1<<10)) return Op::b2_1;
      return Op::b2_0;
  }

  //BIC
  if((inst & 0xFFC0) == 0x4380) return Op::bic;

  //BKPT
  if((inst & 0xFF00) == 0xBE00) return Op::bkpt;

  //BL/BLX(1)
  if((inst & 0xE000) == 0xE000) return Op::blx1;

  //BLX(2)
  if((inst & 0xFF87) == 0x4780) return Op::blx2;

  //BX
  if((inst & 0xFF87) == 0x4700) return Op::bx;

  //CMN
  if((inst & 0xFFC0) == 0x42C0) return Op::cmn;

  //CMP(1) compare immediate
  if((inst & 0xF800) == 0x2800) 
  {
      switch (inst & 0x0700)
      {
          case 0x0000:  return Op::cmp1_0;
          case 0x0100:  return Op::cmp1_1;
          case 0x0200:  return Op::cmp1_2;
          case 0x0300:  return Op::cmp1_3;
          case 0x0400:  return Op::cmp1_4;
          case 0x0500:  return Op::cmp1_5;
          case 0x0600:  return Op::cmp1_6;
          case 0x0700:  return Op::cmp1_7;
      }
      return Op::cmp1_0;
  }

  //CMP(2) compare register
  if((inst & 0xFFC0) == 0x4280) return Op::cmp2;

  //CMP(3) compare high register
  if((inst & 0xFF00) == 0x4500) return Op::cmp3;

  //CPS
  if((inst & 0xFFE8) == 0xB660) return Op::cps;

  //CPY copy high register
  if((inst & 0xFFC0) == 0x4600) return Op::cpy;

  //EOR
  if((inst & 0xFFC0) == 0x4040) return Op::eor;

  //LDMIA
  if((inst & 0xF800) == 0xC800) return Op::ldmia;

  //LDR(1) two register immediate
  if((inst & 0xF800) == 0x6800) return Op::ldr1;

  //LDR(2) three register
  if((inst & 0xFE00) == 0x5800) return Op::ldr2;

  //LDR(3)
  if((inst & 0xF800) == 0x4800) return Op::ldr3;

  //LDR(4)
  if((inst & 0xF800) == 0x9800) 
  {
      switch (inst & 0x0700)
      {
          case 0x0000:  return Op::ldr4_0;
          case 0x0100:  return Op::ldr4_1;
          case 0x0200:  return Op::ldr4_2;
          case 0x0300:  return Op::ldr4_3;
          case 0x0400:  return Op::ldr4_4;
          case 0x0500:  return Op::ldr4_5;
          case 0x0600:  return Op::ldr4_6;
          case 0x0700:  return Op::ldr4_7;
      }
      return Op::ldr4_0;
  }

  //LDRB(1)
  if((inst & 0xF800) == 0x7800) return Op::ldrb1;

  //LDRB(2)
  if((inst & 0xFE00) == 0x5C00) return Op::ldrb2;

  //LDRH(1)
  if((inst & 0xF800) == 0x8800) return Op::ldrh1;

  //LDRH(2)
  if((inst & 0xFE00) == 0x5A00) return Op::ldrh2;

  //LDRSB
  if((inst & 0xFE00) == 0x5600) return Op::ldrsb;

  //LDRSH
  if((inst & 0xFE00) == 0x5E00) return Op::ldrsh;

  //LSL(1)
  if((inst & 0xF800) == 0x0000) return Op::lsl1;

  //LSL(2) two register
  if((inst & 0xFFC0) == 0x4080) return Op::lsl2;

  //LSR(1) two register immediate
  if((inst & 0xF800) == 0x0800) return Op::lsr1;

  //LSR(2) two register
  if((inst & 0xFFC0) == 0x40C0) return Op::lsr2;

  //MOV(1) immediate
  if((inst & 0xF800) == 0x2000) return Op::mov1;

  //MOV(2) two low registers
  if((inst & 0xFFC0) == 0x1C00) return Op::mov2;

  //MOV(3)
  if((inst & 0xFF00) == 0x4600) return Op::mov3;

  //MUL
  if((inst & 0xFFC0) == 0x4340) return Op::mul;

  //MVN
  if((inst & 0xFFC0) == 0x43C0) return Op::mvn;

  //NEG
  if((inst & 0xFFC0) == 0x4240) return Op::neg;

  //ORR
  if((inst & 0xFFC0) == 0x4300) return Op::orr;

  //POP
  if((inst & 0xFE00) == 0xBC00) return Op::pop;

  //PUSH
  if((inst & 0xFE00) == 0xB400) return Op::push;

  //REV
  if((inst & 0xFFC0) == 0xBA00) return Op::rev;

  //REV16
  if((inst & 0xFFC0) == 0xBA40) return Op::rev16;

  //REVSH
  if((inst & 0xFFC0) == 0xBAC0) return Op::revsh;

  //ROR
  if((inst & 0xFFC0) == 0x41C0) return Op::ror;

  //SBC
  if((inst & 0xFFC0) == 0x4180) return Op::sbc;

  //SETEND
  if((inst & 0xFFF7) == 0xB650) return Op::setend;

  //STMIA
  if((inst & 0xF800) == 0xC000) return Op::stmia;

  //STR(1)
  if((inst & 0xF800) == 0x6000) return Op::str1;

  //STR(2)
  if((inst & 0xFE00) == 0x5000) return Op::str2;

  //STR(3)
  if((inst & 0xF800) == 0x9000) return Op::str3;

  //STRB(1)
  if((inst & 0xF800) == 0x7000) return Op::strb1;

  //STRB(2)
  if((inst & 0xFE00) == 0x5400) return Op::strb2;

  //STRH(1)
  if((inst & 0xF800) == 0x8000) return Op::strh1;

  //STRH(2)
  if((inst & 0xFE00) == 0x5200) return Op::strh2;

  //SUB(1)
  if((inst & 0xFE00) == 0x1E00) return Op::sub1;

  //SUB(2)
  if((inst & 0xF800) == 0x3800) return Op::sub2;

  //SUB(3)
  if((inst & 0xFE00) == 0x1A00) return Op::sub3;

  //SUB(4)
  if((inst & 0xFF80) == 0xB080) return Op::sub4;

  //SWI
  if((inst & 0xFF00) == 0xDF00) return Op::swi;

  //SXTB
  if((inst & 0xFFC0) == 0xB240) return Op::sxtb;

  //SXTH
  if((inst & 0xFFC0) == 0xB200) return Op::sxth;

  //TST
  if((inst & 0xFFC0) == 0x4200) return Op::tst;

  //UXTB
  if((inst & 0xFFC0) == 0xB2C0) return Op::uxtb;

  //UXTH
  if((inst & 0xFFC0) == 0xB280) return Op::uxth;

  return Op::invalid;
}



// ------------------------------------------------------------------------
// Somewhere between 10-20% of instructions modify the R15 PC register 
// so we default to not updating it unless we know we're using it.
// This produces a small but meaningful speed-up of Thumb processing...
// ------------------------------------------------------------------------
#define FIX_R15_PC reg_sys[15] = ((u32) (thumb_ptr - rom) << 1) + 3;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void Thumbulator::execute ( void )
{
  uInt32 sp,ra,rb,rc,rd, rm, rn;
  uInt32 ZNflags = 0x00000000;
  uInt16 *thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
  uInt8  *thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
  
  while (1)
  {
      uInt16 inst = *thumb_ptr++;
      Thumbulator::Op decoded = (Thumbulator::Op)*thumb_decode_ptr++;
      
      switch (decoded)
      {
          case Op::b1_000_pos:  //B(1) conditional branch
                if (!ZNflags)
                {
                    rb=(inst>>0)&0xFF;
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_000_neg:  //B(1) conditional branch
                if (!ZNflags)
                {
                    rb=(inst | 0xFFFFFF00);
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_100_pos:  //B(1) conditional branch
                if (ZNflags)
                {
                    rb=(inst>>0)&0xFF;
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;

          case Op::b1_100_neg:  //B(1) conditional branch
                if (ZNflags)
                {
                    rb=(inst | 0xFFFFFF00);
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_200:  //B(1) conditional branch
                if (cFlag)
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_300:  //B(1) conditional branch
                if(!(cFlag))
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_400:  //B(1) conditional branch
                if((ZNflags&0x80000000))
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_500:  //B(1) conditional branch
                if(!((ZNflags&0x80000000)))
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_600:  //B(1) conditional branch
                if(vFlag)
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_700:  //B(1) conditional branch
                if(!(vFlag))
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;

          case Op::b1_800:  //B(1) conditional branch
                if((cFlag)&&(ZNflags))
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_900:  //B(1) conditional branch
                if((!ZNflags)||(!(cFlag)))
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;

          case Op::b1_a00:  //B(1) conditional branch
                ra=0;
                if(  ((ZNflags&0x80000000)) &&  (vFlag) ) ra++;
                else if((!((ZNflags&0x80000000)))&&(!(vFlag))) ra++;
                if(ra)
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_b00:  //B(1) conditional branch
                ra=0;
                if((!((ZNflags&0x80000000)))&&(vFlag)) ra++;
                else if((!(vFlag))&&((ZNflags&0x80000000))) ra++;
                if(ra)
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_c00:  //B(1) conditional branch
                ra=0;
                if(  ((ZNflags&0x80000000)) &&  (vFlag) ) ra++;
                else if((!((ZNflags&0x80000000)))&&(!(vFlag))) ra++;
                if(!ZNflags) ra=0;
                if(ra)
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_d00:  //B(1) conditional branch
                ra=0;
                if((!((ZNflags&0x80000000)))&&(vFlag)) ra++;
                else if((!(vFlag))&&((ZNflags&0x80000000))) ra++;
                else if(!ZNflags) ra++;
                if(ra)
                {
                    rb=(inst>>0)&0xFF;
                    if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                    thumb_ptr += (int)rb+1;
                    thumb_decode_ptr += (int)rb+1;
                }
              break;
              
          case Op::b1_e00:  //B(1) conditional branch
          case Op::b1_f00:  //B(1) conditional branch
              return;
              
          case Op::b2_0:  //B(2) unconditional branch no sign extend
                rb=(inst>>0)&0x7FF;
                rb++;
                thumb_ptr += rb;
                thumb_decode_ptr += rb;
              break;
              
          case Op::b2_1:  //B(2) unconditional branch - sign extend
                rb=(inst | 0xFFFFF800);
                rb++;
                thumb_ptr += (int)rb;
                thumb_decode_ptr += (int)rb;
              break;
              
          case Op::blx1: //BL/BLX(1)
                FIX_R15_PC  
                if((inst&0x1800)==0x1000) //H=b10
                {
                    rb = inst & ((1 << 11) - 1);
                    if(rb & 1<<10) rb |= (~((1 << 11) - 1)); //sign extend
                    rb <<= 12;
                    rb += reg_sys[15];
                    write_register(14, rb);
                }
                else if((inst&0x1800)==0x1800) //H=b11
                {
                  //branch to thumb
                  rb = read_register(14);
                  rb += (inst & ((1 << 11) - 1)) << 1;
                  rb += 2;
                  write_register(14, (reg_sys[15]-2) | 1);
                  write_register(15, rb);
                  thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                  thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                }
                else if((inst&0x1800)==0x0800) //H=b01
                {
                  // fxq: this should exit the code without having to detect it
                  rb = read_register(14);
                  rb += (inst & ((1 << 11) - 1)) << 1;
                  rb &= 0xFFFFFFFC;
                  rb += 2;
                  write_register(14, (reg_sys[15]-2) | 1);
                  write_register(15, rb);
                  thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                  thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                }
              break;

          case Op::ldmia:
                rn=(inst>>8)&0x7;
                sp=read_register(rn);
                for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
                {
                  if(inst&rb)
                  {
                    write_register(ra,readRAM32(sp));
                    sp+=4;
                  }
                }
                write_register(rn,sp);
              break;
              
          case Op::stmia:
                rn=(inst>>8)&0x7;
                sp=read_register(rn);
                for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
                {
                  if(inst&rb)
                  {
                    write32(sp,read_register(ra));
                    sp+=4;
                  }
                }
                write_register(rn,sp);
              break;
              
          case Op::swi:
                rb=inst&0xFF;
                return;
              
          case Op::push:
                  if (inst & 0x100) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[14]);}
                  if (inst & 0x080) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[7]);}
                  if (inst & 0x040) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[6]);}
                  if (inst & 0x020) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[5]);}
                  if (inst & 0x010) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[4]);}
                  if (inst & 0x008) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[3]);}
                  if (inst & 0x004) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[2]);}
                  if (inst & 0x002) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[1]);}
                  if (inst & 0x001) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[0]);}
              break;

          case Op::pop:
                  if (inst & 0x001) {reg_sys[0] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x002) {reg_sys[1] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x004) {reg_sys[2] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x008) {reg_sys[3] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x010) {reg_sys[4] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x020) {reg_sys[5] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x040) {reg_sys[6] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x080) {reg_sys[7] =  readRAM32(reg_sys[13]); reg_sys[13] += 4;}
                  if (inst & 0x100) 
                  {
                      reg_sys[15] = readRAM32(reg_sys[13]) + 2; 
                      reg_sys[13] += 4;
                      thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                      thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                  }
              break;
              
          case Op::ldr4_0:
                rb=(inst<<2)&0x3FF;
                //rb<<=2;
                rb+=read_register(13);
                write_register(0,read32(rb));
              break;

          case Op::ldr4_1:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(1,read32(rb));
              break;
              
          case Op::ldr4_2:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(2,read32(rb));
              break;

          case Op::ldr4_3:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(3,read32(rb));
              break;

          case Op::ldr4_4:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(4,read32(rb));
              break;

          case Op::ldr4_5:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(5,read32(rb));
              break;

          case Op::ldr4_6:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(6,read32(rb));
              break;
              
          case Op::ldr4_7:
                rb=(inst>>0)&0xFF;
                rb<<=2;
                rb+=read_register(13);
                write_register(7,read32(rb));
              break;

          case Op::str3:
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x07;
                rb<<=2;
                rb=read_register(13)+rb;
                write32(rb,read_register(rd));
              break;
              
          case Op::strh1:
                rd=(inst>>0)&0x07;
                rn=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rb<<=1;
                rb=read_register(rn)+rb;
                rc=read_register(rd);
                write16(rb,rc&0xFFFF);
              break;              
              
          case Op::add6:
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x7;
                rb<<=2;
                ra=read_register(13);
                rc=ra+rb;
                write_register(rd,rc);
              break;
              
          case Op::ldrh1:
                rd=(inst>>0)&0x07;
                rn=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rb<<=1;
                rb=read_register(rn)+rb;
                rc=read16(rb);
                write_register(rd,rc&0xFFFF);
              break;

          case Op::add7:
                rb=(inst>>0)&0x7F;
                rb<<=2;
                ra=read_register(13);
                rc=ra+rb;
                write_register(13,rc);
              break;
              
          case Op::sub4:
                rb=inst&0x7F;
                rb<<=2;
                ra=read_register(13);
                ra-=rb;
                write_register(13,ra);
              break;
              
          case Op::bkpt:
                rb=(inst>>0)&0xFF;
                return;
              break;
              
          case Op::cps:
                return;
              break;
              
          case Op::rev:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                ra=read_register(rn);
                rc =((ra>> 0)&0xFF)<<24;
                rc|=((ra>> 8)&0xFF)<<16;
                rc|=((ra>>16)&0xFF)<< 8;
                rc|=((ra>>24)&0xFF)<< 0;
                write_register(rd,rc);
              break;
              
          case Op::rev16:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                ra=read_register(rn);
                rc =((ra>> 0)&0xFF)<< 8;
                rc|=((ra>> 8)&0xFF)<< 0;
                rc|=((ra>>16)&0xFF)<<24;
                rc|=((ra>>24)&0xFF)<<16;
                write_register(rd,rc);
              break;

          case Op::revsh:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                ra=read_register(rn);
                rc =((ra>> 0)&0xFF)<< 8;
                rc|=((ra>> 8)&0xFF)<< 0;
                if(rc&0x8000) rc|=0xFFFF0000;
                else          rc&=0x0000FFFF;
                write_register(rd,rc);
              break;
              
          case Op::setend:
                return;
              break;
              
          case Op::sxtb:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rm);
                rc=ra&0xFF;
                if(rc&0x80) rc|=(~0)<<8;
                write_register(rd,rc);
              break;
              
          case Op::sxth:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rm);
                rc=ra&0xFFFF;
                if(rc&0x8000) rc|=(~0)<<16;
                write_register(rd,rc);
              break;
              
          case Op::add5:
                FIX_R15_PC
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x7;
                rb<<=2;
                ra=read_register(15);
                rc=(ra&(~3))+rb;
                write_register(rd,rc);
              break;
              
          case Op::uxtb:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rm);
                rc=ra&0xFF;
                write_register(rd,rc);
              break;
              
          case Op::uxth:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rm);
                rc=ra&0xFFFF;
                write_register(rd,rc);
              break;
              
          case Op::ldr1:
                reg_sys[(inst>>0)&0x07] = read32(reg_sys[(inst>>3)&0x07] + ((inst>>4)&0x7C));
              break;
              
          case Op::strb1:
                rd=(inst>>0)&0x07;
                rn=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rb=read_register(rn)+rb;
                rc=read_register(rd);
                ra=read16(rb);
                if(rb&1)
                {
                  ra&=0x00FF;
                  ra|=rc<<8;
                }
                else
                {
                  ra&=0xFF00;
                  ra|=rc&0x00FF;
                }
                write16(rb,ra);
              break;
              
          case Op::ldrb1:
                rd=(inst>>0)&0x07;
                rn=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rb=read_register(rn)+rb;
                rc=read16(rb);
                if(rb&1)
                {
                  rc>>=8;
                }
                write_register(rd,rc&0xFF);
              break;
              
          case Op::str1:
                rd=(inst>>0)&0x07;
                rn=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rb<<=2;
                rb=read_register(rn)+rb;
                rc=read_register(rd);
                write32(rb,rc);
              break;
              
          case Op::strb2:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read_register(rd);
                ra=read16(rb);
                if(rb&1)
                {
                  ra&=0x00FF;
                  ra|=rc<<8;
                }
                else
                {
                  ra&=0xFF00;
                  ra|=rc&0x00FF;
                }
                write16(rb,ra);
              break;
              
          case Op::ldrb2:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read16(rb);
                if(rb&1)
                {
                  rc>>=8;
                }
                write_register(rd,rc&0xFF);
              break;
              
          case Op::ldrsh:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read16(rb);
                rc&=0xFFFF;
                if(rc&0x8000) rc|=((~0)<<16);
                write_register(rd,rc);
              break;
              
          case Op::ldrh2:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read16(rb);
                write_register(rd,rc&0xFFFF);
              break;
              
          case Op::ldrsb:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read16(rb);
                if(rb&1)
                {
                  rc>>=8;
                }
                rc&=0xFF;
                if(rc&0x80) rc|=((~0)<<8);
                write_register(rd,rc);
              break;
              
          case Op::ldr2:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read32(rb);
                write_register(rd,rc);
              break;
              
          case Op::str2:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read_register(rd);
                write32(rb,rc);
              break;
              
          case Op::strh2:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                rb=read_register(rn)+read_register(rm);
                rc=read_register(rd);
                write16(rb,rc&0xFFFF);
              break;
              
          case Op::ldr3:
                FIX_R15_PC
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x07;
                rb<<=2;
                ra=read_register(15);
                ra&=~3;
                rb+=ra;
                rc=read32(rb);
                write_register(rd,rc);
              break;
              
          case Op::and_:
                rd=(inst>>0)&0x7;
                reg_sys[rd] &= reg_sys[(inst>>3)&0x7];
                do_znflags(reg_sys[rd]);
              break;
              
              
          case Op::adc:
                rd=(inst>>0)&0x07;
                ra=read_register(rd);
                rb=read_register((inst>>3)&0x07);
                reg_sys[rd]=ra+rb+cFlag;
                do_znflags(reg_sys[rd]);
                do_cflag(ra,rb,cFlag);
                if (bSafeThumb) do_add_vflag(ra,rb,reg_sys[rd]);
              break;
              
          case Op::add4:
                rd=(inst>>0)&0x7;
                rd|=(inst>>4)&0x8;
                rm=(inst>>3)&0xF;
                ra=read_register(rd);
                rb=read_register(rm);
                rc=ra+rb;
                write_register(rd,rc);
              break;
              
          case Op::mov3:
                FIX_R15_PC
                rd=(inst>>0)&0x7;
                rd|=(inst>>4)&0x8;
                rm=(inst>>3)&0xF;
                rc=read_register(rm);
                if (rd==15) rc+=2; // fxq fix for MOV R15
                write_register(rd,rc);
                if (rd==15)
                {
                    thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                    thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                }
              break;
              
              
          case Op::bic:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rd);
                rb=read_register(rm);
                rc=ra&(~rb);
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::bx:
                FIX_R15_PC  
                rm=(inst>>3)&0xF;
                rc=read_register(rm);
                rc+=2;
                if(rc&1)
                {
                  write_register(15,rc);
                  thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                  thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                }
                else
                {
                        uInt32 pc = reg_sys[15] & 0xFFFFFFFE;
                        // branch to even address denotes 32 bit ARM code, which the Thumbulator
                        // class does not support. So capture relavent information and hand it
                        // off to the Cartridge class for it to handle.

                        bool handled = false;
                        // address to test for is + 4 due to pipelining
                      #define CDF1_SetNote     (0x00000752 + 4)
                      #define CDF1_ResetWave   (0x00000756 + 4)
                      #define CDF1_GetWavePtr  (0x0000075a + 4)
                      #define CDF1_SetWaveSize (0x0000075e + 4)

                      extern uInt32 CDFCallback(uInt8 function, uInt32 value1, uInt32 value2);

                        if      (pc == CDF1_SetNote)
                        {
                          CDFCallback(0, reg_sys[2], reg_sys[3]);
                          handled = true;
                        }
                        else if (pc == CDF1_ResetWave)
                        {
                          CDFCallback(1, read_register(2), 0);
                          handled = true;
                        }
                        else if (pc == CDF1_GetWavePtr)
                        {
                          reg_sys[2] = CDFCallback(2, reg_sys[2], 0);
                          handled = true;
                        }
                        else if (pc == CDF1_SetWaveSize)
                        {
                          CDFCallback(3, reg_sys[2], reg_sys[3]);
                          handled = true;
                        }
                        else if (pc == 0x0000083a)
                        {
                            // exiting Custom ARM code, returning to BUS Driver control
                        }
                        else
                        {
                            //myCartridge->thumbCallback(255, 0, 0);
                        }

                        if (handled)
                        {
                          rc = read_register(14); // lr
                          rc += 2;
                          write_register(15, rc);
                          thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                          thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                        } 
                        else return;
                }
              break;
              
          case Op::sbc:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rd);
                rb=read_register(rm);
                rc=ra-rb;
                if(!(cFlag)) rc--;
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag(ra,rb,0);
                if (bSafeThumb) do_sub_vflag(ra,rb,rc);
              break;
              
          case Op::tst:
                rn=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rn);
                rb=read_register(rm);
                ZNflags=ra&rb;
              break;
              
          case Op::asr2:
                rd=(inst>>0)&0x07;
                ra=(inst>>3)&0x07;
                rc=read_register(rd);
                rb=read_register(ra);
                rb&=0xFF;
                if(rb==0)
                {
                }
                else if(rb<32)
                {
                  do_cflag_bit(rc&(1<<(rb-1)));
                  ra=rc&0x80000000;
                  rc>>=rb;
                  if(ra) //asr, sign is shifted in
                  {
                    rc|=(~0)<<(32-rb);
                  }
                }
                else
                {
                  if(rc&0x80000000)
                  {
                    do_cflag_bit(1);
                    rc=(~0);
                  }
                  else
                  {
                    do_cflag_bit(0);
                    rc=0;
                  }
                }
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
              
          case Op::cmp3:
                rn=(inst>>0)&0x7;
                rn|=(inst>>4)&0x8;
                rm=(inst>>3)&0xF;
                ra=read_register(rn);
                rb=read_register(rm);
                rc=ra-rb;
                do_znflags(rc);
                do_cflag(ra,~rb,1);
                if (bSafeThumb) do_sub_vflag(ra,rb,rc);
              break;
              
          case Op::mul:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rd);
                rb=read_register(rm);
                rc=ra*rb;
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::mvn:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rm);
                rc=(~ra);
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::neg:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rm);
                rc=0-ra;
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag(0,~ra,1);
                if (bSafeThumb) do_sub_vflag(0,ra,rc);
              break;
              
          case Op::orr:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rd);
                rb=read_register(rm);
                rc=ra|rb;
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::cmn:
                rn=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rn);
                rb=read_register(rm);
                rc=ra+rb;
                do_znflags(rc);
                do_cflag(ra,rb,0);
                if (bSafeThumb) do_add_vflag(ra,rb,rc);
              break;
              
          case Op::cpy:
                //same as mov except you can use both low registers
                //going to let mov handle high registers
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                rc=read_register(rm);
                write_register(rd,rc);
              break;
              
          case Op::eor:
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ra=read_register(rd);
                rb=read_register(rm);
                rc=ra^rb;
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::lsl2:
                rd=(inst>>0)&0x07;
                ra=(inst>>3)&0x07;
                rc=read_register(rd);
                rb=read_register(ra);
                rb&=0xFF;
                if(rb==0)
                {
                }
                else if(rb<32)
                {
                  do_cflag_bit(rc&(1<<(32-rb)));
                  rc<<=rb;
                }
                else if(rb==32)
                {
                  do_cflag_bit(rc&1);
                  rc=0;
                }
                else
                {
                  do_cflag_bit(0);
                  rc=0;
                }
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::lsr2:
                rd=(inst>>0)&0x07;
                ra=(inst>>3)&0x07;
                rc=read_register(rd);
                rb=read_register(ra);
                rb&=0xFF;
                if(rb==0)
                {
                }
                else if(rb<32)
                {
                  do_cflag_bit(rc&(1<<(32-rb)));
                  rc>>=rb;
                }
                else if(rb==32)
                {
                  do_cflag_bit(rc&0x80000000);
                  rc=0;
                }
                else
                {
                  do_cflag_bit(0);
                  rc=0;
                }
                write_register(rd,rc);
                do_znflags(rc);              
              break;
              
          case Op::ror:
                rd=(inst>>0)&0x7;
                ra=(inst>>3)&0x7;
                rc=read_register(rd);
                ra=read_register(ra);
                ra&=0xFF;
                if(!ra)
                {
                  ra&=0x1F;
                  if(ra==0)
                  {
                    do_cflag_bit(rc&0x80000000);
                  }
                  else
                  {
                    do_cflag_bit(rc&(1<<(ra-1)));
                    rb=rc<<(32-ra);
                    rc>>=ra;
                    rc|=rb;
                  }
                }
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::blx2:
                FIX_R15_PC  
                rm=(inst>>3)&0xF;
                rc=read_register(rm);
                rc+=2;
                if(rc&1)
                {
                  write_register(14,reg_sys[15]-2);
                  write_register(15,rc);
                  thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                  thumb_decode_ptr = &decodedRom[(reg_sys[15]-2) >> 1];
                }
                else
                {
                  // fxq: this could serve as exit code
                  return;
                }
              break;
              
          case Op::add2_0:
                reg_sys[0] += (inst>>0)&0xFF;
                do_znflags(reg_sys[0]);
                // ------------------------------------------------------------------------------------------------------
                // TBD: This is incorrect (but faster) emulation on an instruction that is very common... we're adding 
                // a small number to a 32-bit register and we're going to assume that there is no vflag nor cFlag needed.
                // ------------------------------------------------------------------------------------------------------
                //if (bSafeThumb) do_add_vflag(ra,-rb,rc);
                //if (bSafeThumb) do_cflag(ra,rb,0);
              break;
          case Op::add2_1:
                reg_sys[1] += (inst>>0)&0xFF;
                do_znflags(reg_sys[1]);
              break;
          case Op::add2_2:
                reg_sys[2] += (inst>>0)&0xFF;
                do_znflags(reg_sys[2]);
              break;
          case Op::add2_3:
                reg_sys[3] += (inst>>0)&0xFF;
                do_znflags(reg_sys[3]);
              break;
          case Op::add2_4:
                reg_sys[4] += (inst>>0)&0xFF;
                do_znflags(reg_sys[4]);
              break;
          case Op::add2_5:
                reg_sys[5] += (inst>>0)&0xFF;
                do_znflags(reg_sys[5]);
              break;
          case Op::add2_6:
                reg_sys[6] += (inst>>0)&0xFF;
                do_znflags(reg_sys[6]);
              break;
          case Op::add2_7:
                reg_sys[7] += (inst>>0)&0xFF;
                do_znflags(reg_sys[7]);
              break;
              
          case Op::cmp1_0:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[0]-rb;
                do_cflag_fast(reg_sys[0],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[0],rb,ZNflags);
              break;

          case Op::cmp1_1:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[1]-rb;
                do_cflag_fast(reg_sys[1],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[1],rb,ZNflags);
              break;
              
          case Op::cmp1_2:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[2]-rb;
                do_cflag_fast(reg_sys[2],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[2],rb,ZNflags);
              break;

          case Op::cmp1_3:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[3]-rb;
                do_cflag_fast(reg_sys[3],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[3],rb,ZNflags);
              break;

          case Op::cmp1_4:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[4]-rb;
                do_cflag_fast(reg_sys[4],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[4],rb,ZNflags);
              break;
              
          case Op::cmp1_5:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[5]-rb;
                do_cflag_fast(reg_sys[5],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[5],rb,ZNflags);
              break;
              
          case Op::cmp1_6:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[6]-rb;
                do_cflag_fast(reg_sys[6],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[6],rb,ZNflags);
              break;

          case Op::cmp1_7:
                rb=(inst>>0)&0xFF;
                ZNflags=reg_sys[7]-rb;
                do_cflag_fast(reg_sys[7],~rb);
                if (bSafeThumb) do_sub_vflag(reg_sys[7],rb,ZNflags);
              break;
              
          case Op::cmp2:
                rn=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
                ZNflags=reg_sys[rn]-reg_sys[rm];
                do_cflag(reg_sys[rn],~reg_sys[rm],1);
                if (bSafeThumb) do_sub_vflag(reg_sys[rn],reg_sys[rm],ZNflags);
              break;
              
          case Op::add3:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                reg_sys[rd] = reg_sys[rn] + reg_sys[rm];
                do_znflags(reg_sys[rd]);
                if (bSafeThumb)
                {
                    do_cflag(reg_sys[rn],reg_sys[rm],0);
                    do_add_vflag(reg_sys[rn],reg_sys[rm],reg_sys[rd]);
                }
              break;
              
          case Op::mov1:
                ZNflags=(inst>>0)&0xFF;
                write_register((inst>>8)&0x07,ZNflags);
              break;
              
          case Op::sub3:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                reg_sys[rd]=reg_sys[rn]-reg_sys[rm];
                do_znflags(reg_sys[rd]);
                do_cflag(reg_sys[rn],~reg_sys[rm],1);
                if (bSafeThumb) do_sub_vflag(reg_sys[rn],reg_sys[rm],reg_sys[rd]);
              break;

          case Op::asr1:
                rd=(inst>>0)&0x07;
                rm=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rc=read_register(rm);
                if(rb==0)
                {
                  if(rc&0x80000000)
                  {
                    do_cflag_bit(1);
                    rc=~0;
                  }
                  else
                  {
                    do_cflag_bit(0);
                    rc=0;
                  }
                }
                else
                {
                  do_cflag_bit(rc&(1<<(rb-1)));
                  ra=rc&0x80000000;
                  rc>>=rb;
                  if(ra) //asr, sign is shifted in
                  {
                    rc|=(~0)<<(32-rb);
                  }
                }
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::lsl1:
                rd=(inst>>0)&0x07;
                rm=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rc=read_register(rm);
                if(rb!=0)
                {
                  do_cflag_bit(rc&(1<<(32-rb)));
                  rc<<=rb;
                }
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::lsr1:
                rd=(inst>>0)&0x07;
                rm=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rc=read_register(rm);
                if(rb==0)
                {
                  do_cflag_bit(rc&0x80000000);
                  rc=0;
                }
                else
                {
                  do_cflag_bit(rc&(1<<(rb-1)));
                  rc>>=rb;
                }
                write_register(rd,rc);
                do_znflags(rc);
              break;
              
          case Op::sub2:
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x07;
                ra=read_register(rd);
                rc=ra-rb;
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag_fast(ra,~rb);
                if (bSafeThumb) do_sub_vflag(ra,rb,rc);
              break;
              
          case Op::mov2:
                rd=(inst>>0)&7;
                rn=(inst>>3)&7;
                rc=read_register(rn);
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag_bit(0);
                do_vflag_bit(0);
              break;
              
          case Op::sub1:
                rd=(inst>>0)&7;
                rn=(inst>>3)&7;
                rb=(inst>>6)&7;
                ra=read_register(rn);
                rc=ra-rb;
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag_fast(ra,~rb);
                if (bSafeThumb) do_sub_vflag(ra,rb,rc);
              break;
              
          case Op::add1:
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rb=(inst>>6)&0x7;
                if(rb)
                {
                  ra=read_register(rn);
                  rc=ra+rb;
                  write_register(rd,rc);
                  do_znflags(rc);
                  if (bSafeThumb)
                  {
                      do_cflag(ra,rb,0);
                      do_add_vflag(ra,rb,rc);
                  }
                }
                else
                {
                  //this is a mov
                }
              break;
              
          case Op::invalid:
                return;
              break;              
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE int Thumbulator::reset ( void )
{
    if (myCartInfo.banking == BANK_CDFJ)
    {
      if (isCDFJPlus)
      {
          extern uInt32 cBase, cStack, cStart;
          reg_sys[13]=cStack;   //sp
          reg_sys[14]=cBase;    //lr (duz this use odd addrs)
          reg_sys[15]=cStart+3; //pc entry point of 0x809+2
      }
      else
      {
          reg_sys[13]=0x40001fb4; //sp
          reg_sys[14]=0x00000800; //lr (duz this use odd addrs)
          reg_sys[15]=0x0000080b; //pc entry point of 0x809+2
      }
    }
    else
    {
      reg_sys[13]=0x40001fb4; //sp
      reg_sys[14]=0x00000c00; //lr (duz this use odd addrs)
      reg_sys[15]=0x00000c0b; //pc entry point of 0xc09+2
    }
    return 0;
}
