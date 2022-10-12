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

#define HEX8 ""
#define HEX4 ""
#define HEX2 ""

#ifdef THUMB_SUPPORT

#include "bspf.hxx"
#include "Thumbulator.hxx"

uInt32 reg_sys[16] __attribute__((section(".dtcm"))) = {0};
uInt32 cpsr __attribute__((section(".dtcm"))) = 0;
uInt8  cFlag __attribute__((section(".dtcm"))) = 0;

uInt16 rom[ROMSIZE];
extern uInt16 fast_cart_buffer[];
extern uInt32 debug[];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Thumbulator(uInt16* rom_ptr, uInt16* ram_ptr)
  : ram(ram_ptr)
{
  for (uInt16 i=0; i<ROMSIZE; i++)
  {
    rom[i] = rom_ptr[i];
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
  fast_cart_buffer[(addr&RAMADDMASK) >> 1] = data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::write32 ( uInt32 addr, uInt32 data )
{
    uInt32 *ptr = (uInt32*) &fast_cart_buffer[(addr&RAMADDMASK) >> 1];
    *ptr = data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 Thumbulator::read16 ( uInt32 addr )
{
  if (addr & 0xF0000000)
  {
      return fast_cart_buffer[(addr&RAMADDMASK) >> 1];
  }
  return rom[(addr) >> 1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 Thumbulator::read32 ( uInt32 addr )
{
  uInt32 *ptr;

  if (addr & 0xF0000000)
  {
      ptr = (uInt32*) &fast_cart_buffer[(addr&RAMADDMASK) >> 1];
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
  ptr = (uInt32*) &fast_cart_buffer[(addr&RAMADDMASK) >> 1];  
  return *ptr;    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 Thumbulator::read_register ( uInt32 reg )
{
  return reg_sys[reg];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::write_register ( uInt32 reg, uInt32 data )
{
  reg_sys[reg]=data;
}

#define do_zflag(x) notZflag=x
#define do_nflag(x) nFlag=(x&0x80000000)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_cflag ( uInt32 a, uInt32 b, uInt32 c )
{
  cFlag = 0;
  if (((a|b) & 0x80000000) == 0) return;
  uInt32 rc=(a&0x7FFFFFFF)+(b&0x7FFFFFFF)+c; //carry in
  if (((rc>>31)+(a>>31)+(b>>31)) & 2) cFlag=1;  //carry out
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_sub_vflag ( uInt32 a, uInt32 b, uInt32 c )
{
  //if the sign bits are different
  if((a&0x80000000)^(b&0x80000000))
  {
    //and result matches b
    if((b&0x80000000)==(c&0x80000000))
      cpsr|=CPSR_V;
    else 
      cpsr&=~CPSR_V;
  } else cpsr&=~CPSR_V;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_add_vflag ( uInt32 a, uInt32 b, uInt32 c )
{
  //if sign bits are the same
  if((a&0x80000000)==(b&0x80000000))
  {
    //and the result is different
    if((b&0x80000000)!=(c&0x80000000))
      cpsr|=CPSR_V;
    else 
      cpsr&=~CPSR_V;
  } else cpsr&=~CPSR_V;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_cflag_bit ( uInt32 x )
{
  cFlag = (x ? 1:0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void Thumbulator::do_vflag_bit ( uInt32 x )
{
  if(x) cpsr|=CPSR_V;
  else  cpsr&=~CPSR_V;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::execute ( void )
{
  uInt32 sp, 
         ra,rb,rc,
         rd,rs,op;
  uInt8 rm, rn;
  uInt32 notZflag = 1;
  uInt32 nFlag = 0;
    
  while (1)
  {
      uInt16 inst = rom[(reg_sys[15]-2) >> 1];
      reg_sys[15] += 2;

      //ADC
      if((inst&0xFFC0)==0x4140)
      {
        rd=(inst>>0)&0x07;
        rm=(inst>>3)&0x07;
        ra=read_register(rd);
        rb=read_register(rm);
        reg_sys[rd]=ra+rb+cFlag;
        do_nflag(reg_sys[rd]);
        do_zflag(reg_sys[rd]);
        do_cflag(ra,rb,cFlag);
        do_add_vflag(ra,rb,reg_sys[rd]);
        continue;
      }

      //ADD(1) small immediate two registers
      if((inst&0xFE00)==0x1C00)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rb=(inst>>6)&0x7;
        if(rb)
        {
          ra=read_register(rn);
          rc=ra+rb;
          write_register(rd,rc);
          do_nflag(rc);
          do_zflag(rc);
          do_cflag(ra,rb,0);
          do_add_vflag(ra,rb,rc);
          continue;
        }
        else
        {
          //this is a mov
        }
      }

      //ADD(2) big immediate one register
      if((inst&0xF800)==0x3000)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x7;
        // ----------------------------------------------------------------------------------------------------
        // TBD: This is incorrect (but fast) emulation on an instruction that is very common... we're adding 
        // a small number to a 32-bit register and we're going to assume that there is no carry or negative.
        // ----------------------------------------------------------------------------------------------------
        //ra=read_register(rd);
        //rc=ra+rb;
        //write_register(rd,rc);
        reg_sys[rd] += rb;
        //do_nflag(reg_sys[rd]);
        do_zflag(reg_sys[rd]);
        //do_cflag(ra,rb,0);
        //do_add_vflag(ra,-rb,rc);
        continue;
      }

      //ADD(3) three registers
      if((inst&0xFE00)==0x1800)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        reg_sys[rd] = reg_sys[rn] + reg_sys[rm];
        do_nflag(reg_sys[rd]);
        do_zflag(reg_sys[rd]);
        do_cflag(reg_sys[rn],reg_sys[rm],0);
        do_add_vflag(reg_sys[rn],reg_sys[rm],reg_sys[rd]);
        continue;
      }

      //ADD(4) two registers one or both high no flags
      if((inst&0xFF00)==0x4400)
      {
        rd=(inst>>0)&0x7;
        rd|=(inst>>4)&0x8;
        rm=(inst>>3)&0xF;
        if(0)
          statusMsg << "add r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rd);
        rb=read_register(rm);
        rc=ra+rb;
        write_register(rd,rc);
        continue;
      }

      //ADD(5) rd = pc plus immediate
      if((inst&0xF800)==0xA000)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x7;
        rb<<=2;
        ra=read_register(15);
        rc=(ra&(~3))+rb;
        write_register(rd,rc);
        continue;
      }

      //ADD(6) rd = sp plus immediate
      if((inst&0xF800)==0xA800)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x7;
        rb<<=2;
        ra=read_register(13);
        rc=ra+rb;
        write_register(rd,rc);
        continue;
      }

      //ADD(7) sp plus immediate
      if((inst&0xFF80)==0xB000)
      {
        rb=(inst>>0)&0x7F;
        rb<<=2;
        ra=read_register(13);
        rc=ra+rb;
        write_register(13,rc);
        continue;
      }

      //AND
      if((inst&0xFFC0)==0x4000)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        reg_sys[rd] &= reg_sys[rm];
        do_nflag(reg_sys[rd]);
        do_zflag(reg_sys[rd]);
        continue;
      }

      //ASR(1) two register immediate
      if((inst&0xF800)==0x1000)
      {
        rd=(inst>>0)&0x07;
        rm=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        if(0)
          statusMsg << "asrs r" << dec << rd << ",r" << dec << rm
                    << ",#0x" << HEX2 << rb << endl;
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //ASR(2) two register
      if((inst&0xFFC0)==0x4100)
      {
        rd=(inst>>0)&0x07;
        rs=(inst>>3)&0x07;
        if(0)
          statusMsg << "asrs r" << dec << rd << ",r" << dec << rs << endl;
        rc=read_register(rd);
        rb=read_register(rs);
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //B(1) conditional branch
      if((inst&0xF000)==0xD000)
      {
        rb=(inst>>0)&0xFF;
        if(rb&0x80)
          rb|=(~0)<<8;
        op=(inst>>8)&0xF;
        rb<<=1;
        rb+=reg_sys[15];
        rb+=2;
        switch(op)
        {
          case 0x0: //b eq  z set
            //if(cpsr&CPSR_Z)
            if (!notZflag)
            {
              write_register(15,rb);
            }
            continue;

          case 0x1: //b ne  z clear
            //if(!(cpsr&CPSR_Z))
            if (notZflag)
            {
              write_register(15,rb);
            }
            continue;

          case 0x2: //b cs c set
            if(cFlag)
            {
              write_register(15,rb);
            }
            continue;

          case 0x3: //b cc c clear
            if(!(cFlag))
            {
              write_register(15,rb);
            }
            continue;

          case 0x4: //b mi n set
            if(nFlag)
            {
              write_register(15,rb);
            }
            continue;

          case 0x5: //b pl n clear
            if(!(nFlag))
            {
              write_register(15,rb);
            }
            continue;

          case 0x6: //b vs v set
            if(cpsr&CPSR_V)
            {
              write_register(15,rb);
            }
            continue;

          case 0x7: //b vc v clear
            if(!(cpsr&CPSR_V))
            {
              write_register(15,rb);
            }
            continue;

          case 0x8: //b hi c set z clear
            if((cFlag)&&(notZflag))
            {
              write_register(15,rb);
            }
            continue;

          case 0x9: //b ls c clear or z set
            if((!notZflag)||(!(cFlag)))
            {
              write_register(15,rb);
            }
            continue;

          case 0xA: //b ge N == V
            ra=0;
            if(  (nFlag) &&  (cpsr&CPSR_V) ) ra++;
            if((!(nFlag))&&(!(cpsr&CPSR_V))) ra++;
            if(ra)
            {
              write_register(15,rb);
            }
            continue;

          case 0xB: //b lt N != V
            ra=0;
            if((!(nFlag))&&(cpsr&CPSR_V)) ra++;
            if((!(cpsr&CPSR_V))&&(nFlag)) ra++;
            if(ra)
            {
              write_register(15,rb);
            }
            continue;

          case 0xC: //b gt Z==0 and N == V
            ra=0;
            if(  (nFlag) &&  (cpsr&CPSR_V) ) ra++;
            if((!(nFlag))&&(!(cpsr&CPSR_V))) ra++;
            if(!notZflag) ra=0;
            if(ra)
            {
              write_register(15,rb);
            }
            continue;

          case 0xD: //b le Z==1 or N != V
            ra=0;
            if((!(nFlag))&&(cpsr&CPSR_V)) ra++;
            if((!(cpsr&CPSR_V))&&(nFlag)) ra++;
            if(!notZflag) ra++;
            if(ra)
            {
              write_register(15,rb);
            }
            continue;

          case 0xE:
            //undefined instruction
            break;

          case 0xF:
            //swi
            break;
        }
      }

      //B(2) unconditional branch
      if((inst&0xF800)==0xE000)
      {
        rb=(inst>>0)&0x7FF;
        if(rb&(1<<10))
          rb|=(~0)<<11;
        rb<<=1;
        reg_sys[15] += (rb+2);
        continue;
      }

      //BIC
      if((inst&0xFFC0)==0x4380)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        ra=read_register(rd);
        rb=read_register(rm);
        rc=ra&(~rb);
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //BKPT
      if((inst&0xFF00)==0xBE00)
      {
        rb=(inst>>0)&0xFF;
        break;
      }

      //BL/BLX(1)
      if((inst&0xE000)==0xE000) //BL,BLX
      {
        if((inst&0x1800)==0x1000) //H=b10
        {
          if(0)
            statusMsg << endl;
          halfadd=inst;
          continue;
        }
        else if((inst&0x1800)==0x1800) //H=b11
        {
          //branch to thumb
          rb=halfadd&((1<<11)-1);
          if(rb&1<<10)
            rb|=(~((1<<11)-1)); //sign extend
          rb<<=11;
          rb|=inst&((1<<11)-1);
          rb<<=1;
          rb+=reg_sys[15];
          write_register(14,reg_sys[15]-2);
          write_register(15,rb);
          continue;
        }
        else if((inst&0x1800)==0x0800) //H=b01
        {
          // fxq: this should exit the code without having to detect it
          break;
        }
      }

      //BLX(2)
      if((inst&0xFF87)==0x4780)
      {
        rm=(inst>>3)&0xF;
        if(0)
          statusMsg << "blx r" << dec << rm << endl;
        rc=read_register(rm);
        rc+=2;
        if(rc&1)
        {
          write_register(14,reg_sys[15]-2);
          write_register(15,rc);
          continue;
        }
        else
        {
          // fxq: this could serve as exit code
          break;
        }
      }

      //BX
      if((inst&0xFF87)==0x4700)
      {
        rm=(inst>>3)&0xF;
        if(0)
          statusMsg << "bx r" << dec << rm << endl;
        rc=read_register(rm);
        rc+=2;
        if(rc&1)
        {
          write_register(15,rc);
          continue;
        }
        else
        {
          //fprintf(stderr,"cannot branch to arm 0x%08X 0x%04X\n",pc,inst);
          // fxq: or maybe this one??
          break;
        }
      }

      //CMN
      if((inst&0xFFC0)==0x42C0)
      {
        rn=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "cmns r" << dec << rn << ",r" << dec << rm << endl;
        ra=read_register(rn);
        rb=read_register(rm);
        rc=ra+rb;
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,rb,0);
        do_add_vflag(ra,rb,rc);
        continue;
      }

      //CMP(1) compare immediate
      if((inst&0xF800)==0x2800)
      {
        rb=(inst>>0)&0xFF;
        rn=(inst>>8)&0x07;
        ra=read_register(rn);
        rc=ra-rb;
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,~rb,1);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //CMP(2) compare register
      if((inst&0xFFC0)==0x4280)
      {
        rn=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        ra=read_register(rn);
        rb=read_register(rm);
        rc=ra-rb;
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,~rb,1);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //CMP(3) compare high register
      if((inst&0xFF00)==0x4500)
      {
        rn=(inst>>0)&0x7;
        rn|=(inst>>4)&0x8;
        rm=(inst>>3)&0xF;
        ra=read_register(rn);
        rb=read_register(rm);
        rc=ra-rb;
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,~rb,1);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //CPS
      if((inst&0xFFE8)==0xB660)
      {
        break;
      }

      //CPY copy high register
      if((inst&0xFFC0)==0x4600)
      {
        //same as mov except you can use both low registers
        //going to let mov handle high registers
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        rc=read_register(rm);
        write_register(rd,rc);
        continue;
      }

      //EOR
      if((inst&0xFFC0)==0x4040)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        ra=read_register(rd);
        rb=read_register(rm);
        rc=ra^rb;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //LDMIA
      if((inst&0xF800)==0xC800)
      {
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
        continue;
      }

      //LDR(1) two register immediate
      if((inst&0xF800)==0x6800)
      {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        rb<<=2;
        rb=read_register(rn)+rb;
        rc=read32(rb);
        write_register(rd,rc);
        continue;
      }

      //LDR(2) three register
      if((inst&0xFE00)==0x5800)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        rb=read_register(rn)+read_register(rm);
        rc=read32(rb);
        write_register(rd,rc);
        continue;
      }

      //LDR(3)
      if((inst&0xF800)==0x4800)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        rb<<=2;
        ra=read_register(15);
        ra&=~3;
        rb+=ra;
        rc=read32(rb);
        write_register(rd,rc);
        continue;
      }

      //LDR(4)
      if((inst&0xF800)==0x9800)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        rb<<=2;
        ra=read_register(13);
        //ra&=~3;
        rb+=ra;
        rc=read32(rb);
        write_register(rd,rc);
        continue;
      }

      //LDRB(1)
      if((inst&0xF800)==0x7800)
      {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        rb=read_register(rn)+rb;
        rc=read16(rb);
        if(rb&1)
        {
          rc>>=8;
        }
        else
        {
        }
        write_register(rd,rc&0xFF);
        continue;
      }

      //LDRB(2)
      if((inst&0xFE00)==0x5C00)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        rb=read_register(rn)+read_register(rm);
        rc=read16(rb);
        if(rb&1)
        {
          rc>>=8;
        }
        else
        {
        }
        write_register(rd,rc&0xFF);
        continue;
      }

      //LDRH(1)
      if((inst&0xF800)==0x8800)
      {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        rb<<=1;
        rb=read_register(rn)+rb;
        rc=read16(rb);
        write_register(rd,rc&0xFFFF);
        continue;
      }

      //LDRH(2)
      if((inst&0xFE00)==0x5A00)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        rb=read_register(rn)+read_register(rm);
        rc=read16(rb);
        write_register(rd,rc&0xFFFF);
        continue;
      }

      //LDRSB
      if((inst&0xFE00)==0x5600)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        rb=read_register(rn)+read_register(rm);
        rc=read16(rb);
        if(rb&1)
        {
          rc>>=8;
        }
        else
        {
        }
        rc&=0xFF;
        if(rc&0x80) rc|=((~0)<<8);
        write_register(rd,rc);
        continue;
      }

      //LDRSH
      if((inst&0xFE00)==0x5E00)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        rb=read_register(rn)+read_register(rm);
        rc=read16(rb);
        rc&=0xFFFF;
        if(rc&0x8000) rc|=((~0)<<16);
        write_register(rd,rc);
        continue;
      }

      //LSL(1)
      if((inst&0xF800)==0x0000)
      {
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //LSL(2) two register
      if((inst&0xFFC0)==0x4080)
      {
        rd=(inst>>0)&0x07;
        rs=(inst>>3)&0x07;
        rc=read_register(rd);
        rb=read_register(rs);
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //LSR(1) two register immediate
      if((inst&0xF800)==0x0800)
      {
        rd=(inst>>0)&0x07;
        rm=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        if(0)
          statusMsg << "lsrs r" << dec << rd << ",r" << dec << rm
                    << ",#0x" << HEX2 << rb << endl;
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //LSR(2) two register
      if((inst&0xFFC0)==0x40C0)
      {
        rd=(inst>>0)&0x07;
        rs=(inst>>3)&0x07;
        if(0)
          statusMsg << "lsrs r" << dec << rd << ",r" << dec << rs << endl;
        rc=read_register(rd);
        rb=read_register(rs);
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //MOV(1) immediate
      if((inst&0xF800)==0x2000)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        write_register(rd,rb);
        do_nflag(rb);
        do_zflag(rb);
        continue;
      }

      //MOV(2) two low registers
      if((inst&0xFFC0)==0x1C00)
      {
        rd=(inst>>0)&7;
        rn=(inst>>3)&7;
        rc=read_register(rn);
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag_bit(0);
        do_vflag_bit(0);
        continue;
      }

      //MOV(3)
      if((inst&0xFF00)==0x4600)
      {
        rd=(inst>>0)&0x7;
        rd|=(inst>>4)&0x8;
        rm=(inst>>3)&0xF;
        rc=read_register(rm);
        if (rd==15) rc+=2; // fxq fix for MOV R15
        write_register(rd,rc);
        continue;
      }

      //MUL
      if((inst&0xFFC0)==0x4340)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        ra=read_register(rd);
        rb=read_register(rm);
        rc=ra*rb;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //MVN
      if((inst&0xFFC0)==0x43C0)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "mvns r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rm);
        rc=(~ra);
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //NEG
      if((inst&0xFFC0)==0x4240)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "negs r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rm);
        rc=0-ra;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(0,~ra,1);
        do_sub_vflag(0,ra,rc);
        continue;
      }

      //ORR
      if((inst&0xFFC0)==0x4300)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "orrs r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rd);
        rb=read_register(rm);
        rc=ra|rb;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //POP
      if((inst&0xFE00)==0xBC00)
      {
        sp=read_register(13);
        for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
        {
          if(inst&rb)
          {
            write_register(ra,readRAM32(sp));
            sp+=4;
          }
        }
        if(inst&0x100)
        {
          rc=readRAM32(sp);
          rc+=2;
          write_register(15,rc);
          sp+=4;
        }
        write_register(13,sp);
        continue;
      }

      //PUSH
      if((inst&0xFE00)==0xB400)
      {
        if(0)
        {
          statusMsg << "push {";
          for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
          {
            if(inst&rb)
            {
              if(rc) statusMsg << ",";
              statusMsg << "r" << dec << ra;
              rc++;
            }
          }
          if(inst&0x100)
          {
            if(rc) statusMsg << ",";
            statusMsg << "lr";
          }
          statusMsg << "}" << endl;
        }

        sp=read_register(13);
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
        {
          if(inst&rb)
          {
            rc++;
          }
        }
        if(inst&0x100) rc++;
        rc<<=2;
        sp-=rc;
        rd=sp;
        for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
        {
          if(inst&rb)
          {
            write32(rd,read_register(ra));
            rd+=4;
          }
        }
        if(inst&0x100)
        {
          write32(rd,read_register(14));
        }
        write_register(13,sp);
        continue;
      }

      //REV
      if((inst&0xFFC0)==0xBA00)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        if(0)
          statusMsg << "rev r" << dec << rd << ",r" << dec << rn << endl;
        ra=read_register(rn);
        rc =((ra>> 0)&0xFF)<<24;
        rc|=((ra>> 8)&0xFF)<<16;
        rc|=((ra>>16)&0xFF)<< 8;
        rc|=((ra>>24)&0xFF)<< 0;
        write_register(rd,rc);
        continue;
      }

      //REV16
      if((inst&0xFFC0)==0xBA40)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        if(0)
          statusMsg << "rev16 r" << dec << rd << ",r" << dec << rn << endl;
        ra=read_register(rn);
        rc =((ra>> 0)&0xFF)<< 8;
        rc|=((ra>> 8)&0xFF)<< 0;
        rc|=((ra>>16)&0xFF)<<24;
        rc|=((ra>>24)&0xFF)<<16;
        write_register(rd,rc);
        continue;
      }

      //REVSH
      if((inst&0xFFC0)==0xBAC0)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        if(0)
          statusMsg << "revsh r" << dec << rd << ",r" << dec << rn << endl;
        ra=read_register(rn);
        rc =((ra>> 0)&0xFF)<< 8;
        rc|=((ra>> 8)&0xFF)<< 0;
        if(rc&0x8000) rc|=0xFFFF0000;
        else          rc&=0x0000FFFF;
        write_register(rd,rc);
        continue;
      }

      //ROR
      if((inst&0xFFC0)==0x41C0)
      {
        rd=(inst>>0)&0x7;
        rs=(inst>>3)&0x7;
        if(0)
          statusMsg << "rors r" << dec << rd << ",r" << dec << rs << endl;
        rc=read_register(rd);
        ra=read_register(rs);
        ra&=0xFF;
        if(ra==0)
        {
        }
        else
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
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //SBC
      if((inst&0xFFC0)==0x4180)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "sbc r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rd);
        rb=read_register(rm);
        rc=ra-rb;
        if(!(cFlag)) rc--;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,rb,0);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //SETEND
      if((inst&0xFFF7)==0xB650)
      {
        break;
      }

      //STMIA
      if((inst&0xF800)==0xC000)
      {
        rn=(inst>>8)&0x7;
        if(0)
        {
          statusMsg << "stmia r" << dec << rn << "!,{";
          for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
          {
            if(inst&rb)
            {
              if(rc) statusMsg << ",";
              statusMsg << "r" << dec << ra;
              rc++;
            }
          }
          statusMsg << "}" << endl;
        }
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
        continue;
      }

      //STR(1)
      if((inst&0xF800)==0x6000)
      {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        rb<<=2;
        if(0)
          statusMsg << "str r" << dec << rd << ",[r" << dec << rn
                    << ",#0x" << HEX2 << rb << "]" << endl;
        rb=read_register(rn)+rb;
        rc=read_register(rd);
        write32(rb,rc);
        continue;
      }

      //STR(2)
      if((inst&0xFE00)==0x5000)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        if(0)
          statusMsg << "str r" << dec << rd << ",[r" << dec << rn
                    << ",r" << dec << rm << "]" << endl;
        rb=read_register(rn)+read_register(rm);
        rc=read_register(rd);
        write32(rb,rc);
        continue;
      }

      //STR(3)
      if((inst&0xF800)==0x9000)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        rb<<=2;
        if(0)
          statusMsg << "str r" << dec << rd << ",[SP,#0x" << HEX2 << rb << "]" << endl;
        rb=read_register(13)+rb;
        rc=read_register(rd);
        write32(rb,rc);
        continue;
      }

      //STRB(1)
      if((inst&0xF800)==0x7000)
      {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        if(0)
          statusMsg << "strb r" << dec << rd << ",[r" << dec << rn
                    << ",#0x" << HEX8 << rb << "]" << endl;
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
        write16(rb,ra&0xFFFF);
        continue;
      }

      //STRB(2)
      if((inst&0xFE00)==0x5400)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        if(0)
          statusMsg << "strb r" << dec << rd << ",[r" << dec << rn
                    << ",r" << rm << "]" << endl;
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
        write16(rb,ra&0xFFFF);
        continue;
      }

      //STRH(1)
      if((inst&0xF800)==0x8000)
      {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        rb<<=1;
        if(0)
          statusMsg << "strh r" << dec << rd << ",[r" << dec << rn
                    << ",#0x" << HEX2 << rb << "]" << endl;
        rb=read_register(rn)+rb;
        rc=read_register(rd);
        write16(rb,rc&0xFFFF);
        continue;
      }

      //STRH(2)
      if((inst&0xFE00)==0x5200)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        if(0)
          statusMsg << "strh r" << dec << rd << ",[r" << dec << rn
                    << ",r" << dec << rm << "]" << endl;
        rb=read_register(rn)+read_register(rm);
        rc=read_register(rd);
        write16(rb,rc&0xFFFF);
        continue;
      }

      //SUB(1)
      if((inst&0xFE00)==0x1E00)
      {
        rd=(inst>>0)&7;
        rn=(inst>>3)&7;
        rb=(inst>>6)&7;
        if(0)
          statusMsg << "subs r" << dec << rd << ",r" << dec << rn
                    << ",#0x" << HEX2 << rb << endl;
        ra=read_register(rn);
        rc=ra-rb;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,~rb,1);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //SUB(2)
      if((inst&0xF800)==0x3800)
      {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        if(0)
          statusMsg << "subs r" << dec << rd << ",#0x" << HEX2 << rb << endl;
        ra=read_register(rd);
        rc=ra-rb;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,~rb,1);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //SUB(3)
      if((inst&0xFE00)==0x1A00)
      {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rm=(inst>>6)&0x7;
        if(0)
          statusMsg << "subs r" << dec << rd << ",r" << dec << rn
                    << ",r" << dec << rm << endl;
        ra=read_register(rn);
        rb=read_register(rm);
        rc=ra-rb;
        write_register(rd,rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra,~rb,1);
        do_sub_vflag(ra,rb,rc);
        continue;
      }

      //SUB(4)
      if((inst&0xFF80)==0xB080)
      {
        rb=inst&0x7F;
        rb<<=2;
        if(0)
          statusMsg << "sub SP,#0x" << HEX2 << rb << endl;
        ra=read_register(13);
        ra-=rb;
        write_register(13,ra);
        continue;
      }

      //SWI
      if((inst&0xFF00)==0xDF00)
      {
        rb=inst&0xFF;
        break;
      }

      //SXTB
      if((inst&0xFFC0)==0xB240)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "sxtb r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rm);
        rc=ra&0xFF;
        if(rc&0x80) rc|=(~0)<<8;
        write_register(rd,rc);
        continue;
      }

      //SXTH
      if((inst&0xFFC0)==0xB200)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "sxth r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rm);
        rc=ra&0xFFFF;
        if(rc&0x8000) rc|=(~0)<<16;
        write_register(rd,rc);
        continue;
      }

      //TST
      if((inst&0xFFC0)==0x4200)
      {
        rn=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "tst r" << dec << rn << ",r" << dec << rm << endl;
        ra=read_register(rn);
        rb=read_register(rm);
        rc=ra&rb;
        do_nflag(rc);
        do_zflag(rc);
        continue;
      }

      //UXTB
      if((inst&0xFFC0)==0xB2C0)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "uxtb r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rm);
        rc=ra&0xFF;
        write_register(rd,rc);
        continue;
      }

      //UXTH
      if((inst&0xFFC0)==0xB280)
      {
        rd=(inst>>0)&0x7;
        rm=(inst>>3)&0x7;
        if(0)
          statusMsg << "uxth r" << dec << rd << ",r" << dec << rm << endl;
        ra=read_register(rm);
        rc=ra&0xFFFF;
        write_register(rd,rc);
        continue;
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Thumbulator::reset ( void )
{
  //memset(ram,0xFF,sizeof(ram));
  cpsr=CPSR_T|CPSR_I|CPSR_F|MODE_SVC;
    
//  notZflag = 1;
//  nFlag = 0;    

  reg_sys[13]=0x40001fb4; //sp
  reg_sys[14]=0x00000c00; //lr (duz this use odd addrs)
  reg_sys[15]=0x00000c0b; // entry point of 0xc09+2
  //  reg_sys[15]+=2;
  mamcr = 0;

  statusMsg.str("");

  return 0;
}

#endif
