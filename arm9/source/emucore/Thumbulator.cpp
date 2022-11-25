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

uInt16 rom[ROMSIZE] ALIGN(32);
extern uInt8 fast_cart_buffer[];

bool  bSafeThumb  __attribute__((section(".dtcm"))) = 1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Thumbulator(uInt16* rom_ptr)
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
ITCM_CODE void Thumbulator::run( void )
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
  register uInt16 *thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
  while (1)
  {
      register uInt16 inst = *thumb_ptr++;
      
      if (inst & 0x8000)  // High bit set in instruction (0x8000 IS set)
      {
          if (inst & 0x4000)
          {
              //B(1) conditional branch
              if((inst&0xF000)==0xD000)
              {
                rb=(inst>>0)&0xFF;
                if(rb&0x80) rb|=0xFFFFFF00; // Sign extend
                switch(inst & 0xF00)
                {
                  case 0x000: //b eq  z set
                    if (!ZNflags)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x100: //b ne  z clear
                    if (ZNflags)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x200: //b cs c set
                    if(cFlag)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x300: //b cc c clear
                    if(!(cFlag))
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x400: //b mi n set
                    if((ZNflags&0x80000000))
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x500: //b pl n clear
                    if(!((ZNflags&0x80000000)))
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x600: //b vs v set
                    if(vFlag)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x700: //b vc v clear
                    if(!(vFlag))
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x800: //b hi c set z clear
                    if((cFlag)&&(ZNflags))
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0x900: //b ls c clear or z set
                    if((!ZNflags)||(!(cFlag)))
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0xA00: //b ge N == V
                    ra=0;
                    if(  ((ZNflags&0x80000000)) &&  (vFlag) ) ra++;
                    else if((!((ZNflags&0x80000000)))&&(!(vFlag))) ra++;
                    if(ra)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0xB00: //b lt N != V
                    ra=0;
                    if((!((ZNflags&0x80000000)))&&(vFlag)) ra++;
                    else if((!(vFlag))&&((ZNflags&0x80000000))) ra++;
                    if(ra)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0xC00: //b gt Z==0 and N == V
                    ra=0;
                    if(  ((ZNflags&0x80000000)) &&  (vFlag) ) ra++;
                    else if((!((ZNflags&0x80000000)))&&(!(vFlag))) ra++;
                    if(!ZNflags) ra=0;
                    if(ra)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0xD00: //b le Z==1 or N != V
                    ra=0;
                    if((!((ZNflags&0x80000000)))&&(vFlag)) ra++;
                    else if((!(vFlag))&&((ZNflags&0x80000000))) ra++;
                    else if(!ZNflags) ra++;
                    if(ra)
                    {
                        thumb_ptr += (int)rb+1;
                    }
                    continue;

                  case 0xE00:
                    //undefined instruction
                    break;

                  case 0xF00:
                    //swi
                    break;
                }
              }

              //B(2) unconditional branch
              if((inst&0xF800)==0xE000)
              {
                rb=(inst>>0)&0x7FF;
                if(rb&(1<<10))  rb|=(~0)<<11;   // Sign extended
                thumb_ptr += (int)rb+1;
                continue;
              }

              //BL/BLX(1)
              if((inst&0xE000)==0xE000) //BL,BLX
              {
                if((inst&0x1800)==0x1000) //H=b10
                {
                  halfadd=inst;
                  continue;
                }
                else if((inst&0x1800)==0x1800) //H=b11
                {
                  FIX_R15_PC  
                  //branch to thumb
                  rb=halfadd&((1<<11)-1);
                  if(rb&1<<10) rb|=(~((1<<11)-1)); //sign extend
                  rb<<=11;
                  rb|=inst&((1<<11)-1);
                  rb<<=1;
                  rb+=reg_sys[15];
                  write_register(14,reg_sys[15]-2);
                  write_register(15,rb);
                  thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                  continue;
                }
                else if((inst&0x1800)==0x0800) //H=b01
                {
                  // fxq: this should exit the code without having to detect it
                  break;
                }
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

              //STMIA
              if((inst&0xF800)==0xC000)
              {
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
                continue;
              }

              //SWI
              if((inst&0xFF00)==0xDF00)
              {
                rb=inst&0xFF;
                break;
              }          
              
          }
          else
          {              
              //POP
              if((inst&0xFE00)==0xBC00)
              {
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
                  }
                  continue;
              }

              //PUSH
              if((inst&0xFE00)==0xB400)
              {
                  if (inst & 0x100) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[14]);}
                  if (inst & 0x080) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[7]);}
                  if (inst & 0x040) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[6]);}
                  if (inst & 0x020) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[5]);}
                  if (inst & 0x010) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[4]);}
                  if (inst & 0x008) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[3]);}
                  if (inst & 0x004) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[2]);}
                  if (inst & 0x002) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[1]);}
                  if (inst & 0x001) {reg_sys[13] -= 4; write32(reg_sys[13], reg_sys[0]);}
                  continue;
              }
              
              //LDR(4)
              if((inst&0xF800)==0x9800)
              {
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x07;
                rb<<=2;
                rb+=read_register(13);
                write_register(rd,read32(rb));
                continue;
              }

              //STR(3)
              if((inst&0xF800)==0x9000)
              {
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x07;
                rb<<=2;
                rb=read_register(13)+rb;
                write32(rb,read_register(rd));
                continue;
              }              
              
              //STRH(1)
              if((inst&0xF800)==0x8000)
              {
                rd=(inst>>0)&0x07;
                rn=(inst>>3)&0x07;
                rb=(inst>>6)&0x1F;
                rb<<=1;
                rb=read_register(rn)+rb;
                rc=read_register(rd);
                write16(rb,rc&0xFFFF);
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
              
              //SUB(4)
              if((inst&0xFF80)==0xB080)
              {
                rb=inst&0x7F;
                rb<<=2;
                ra=read_register(13);
                ra-=rb;
                write_register(13,ra);
                continue;
              }

              //BKPT
              if((inst&0xFF00)==0xBE00)
              {
                rb=(inst>>0)&0xFF;
                break;
              }

              //CPS
              if((inst&0xFFE8)==0xB660)
              {
                break;
              }

              //REV
              if((inst&0xFFC0)==0xBA00)
              {
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
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
                ra=read_register(rn);
                rc =((ra>> 0)&0xFF)<< 8;
                rc|=((ra>> 8)&0xFF)<< 0;
                if(rc&0x8000) rc|=0xFFFF0000;
                else          rc&=0x0000FFFF;
                write_register(rd,rc);
                continue;
              }

              //SETEND
              if((inst&0xFFF7)==0xB650)
              {
                break;
              }

              //SXTB
              if((inst&0xFFC0)==0xB240)
              {
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
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
                ra=read_register(rm);
                rc=ra&0xFFFF;
                if(rc&0x8000) rc|=(~0)<<16;
                write_register(rd,rc);
                continue;
              }
              
              //ADD(5) rd = pc plus immediate
              if((inst&0xF800)==0xA000)
              {
                FIX_R15_PC
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x7;
                rb<<=2;
                ra=read_register(15);
                rc=(ra&(~3))+rb;
                write_register(rd,rc);
                continue;
              }

              //UXTB
              if((inst&0xFFC0)==0xB2C0)
              {
                rd=(inst>>0)&0x7;
                rm=(inst>>3)&0x7;
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
                ra=read_register(rm);
                rc=ra&0xFFFF;
                write_register(rd,rc);
                continue;
              }
          }
      }
      else // High bit not set in instruction (0x8000 is NOT set)
      {   
          if (inst & 0x4000)
          {
              if (inst & 0x2000)
              {
                  //LDR(1) two register immediate
                  if((inst&0xF800)==0x6800)
                  {
                    reg_sys[(inst>>0)&0x07] = read32(reg_sys[(inst>>3)&0x07] + ((inst>>4)&0x7C));
                    continue;
                  }
                  
                  //STRB(1)
                  if((inst&0xF800)==0x7000)
                  {
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
                    write_register(rd,rc&0xFF);
                    continue;
                  }
                  
                  //STR(1)
                  if((inst&0xF800)==0x6000)
                  {
                    rd=(inst>>0)&0x07;
                    rn=(inst>>3)&0x07;
                    rb=(inst>>6)&0x1F;
                    rb<<=2;
                    rb=read_register(rn)+rb;
                    rc=read_register(rd);
                    write32(rb,rc);
                    continue;
                  }
              }
              else // instrucion ! & 0x2000
              {
                  if (inst & 0x1000)
                  {
                      //STRB(2)
                      if((inst&0xFE00)==0x5400)
                      {
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
                        write_register(rd,rc&0xFF);
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
                        rc&=0xFF;
                        if(rc&0x80) rc|=((~0)<<8);
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

                      //STR(2)
                      if((inst&0xFE00)==0x5000)
                      {
                        rd=(inst>>0)&0x7;
                        rn=(inst>>3)&0x7;
                        rm=(inst>>6)&0x7;
                        rb=read_register(rn)+read_register(rm);
                        rc=read_register(rd);
                        write32(rb,rc);
                        continue;
                      }

                      //STRH(2)
                      if((inst&0xFE00)==0x5200)
                      {
                        rd=(inst>>0)&0x7;
                        rn=(inst>>3)&0x7;
                        rm=(inst>>6)&0x7;
                        rb=read_register(rn)+read_register(rm);
                        rc=read_register(rd);
                        write16(rb,rc&0xFFFF);
                        continue;
                      }
                  }
                  else
                  {
                      //CMP(2) compare register
                      if((inst&0xFFC0)==0x4280)
                      {
                        rn=(inst>>0)&0x7;
                        rm=(inst>>3)&0x7;
                        ZNflags=reg_sys[rn]-reg_sys[rm];
                        do_cflag(reg_sys[rn],~reg_sys[rm],1);
                        if (bSafeThumb) do_sub_vflag(reg_sys[rn],reg_sys[rm],ZNflags);
                        continue;
                      }
                      
                      //LDR(3)
                      if((inst&0xF800)==0x4800)
                      {
                        FIX_R15_PC
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
                      

                      //AND
                      if((inst&0xFFC0)==0x4000)
                      {
                        rd=(inst>>0)&0x7;
                        reg_sys[rd] &= reg_sys[(inst>>3)&0x7];
                        do_znflags(reg_sys[rd]);
                        continue;
                      }
                      
                      //ADC
                      if((inst&0xFFC0)==0x4140)
                      {
                        rd=(inst>>0)&0x07;
                        ra=read_register(rd);
                        rb=read_register((inst>>3)&0x07);
                        reg_sys[rd]=ra+rb+cFlag;
                        do_znflags(reg_sys[rd]);
                        do_cflag(ra,rb,cFlag);
                        if (bSafeThumb) do_add_vflag(ra,rb,reg_sys[rd]);
                        continue;
                      }

                      //ADD(4) two registers one or both high no flags
                      if((inst&0xFF00)==0x4400)
                      {
                        rd=(inst>>0)&0x7;
                        rd|=(inst>>4)&0x8;
                        rm=(inst>>3)&0xF;
                        ra=read_register(rd);
                        rb=read_register(rm);
                        rc=ra+rb;
                        write_register(rd,rc);
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
                        if (rd==15) thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
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
                        do_znflags(rc);
                        continue;
                      }

                      //BX
                      if((inst&0xFF87)==0x4700)
                      {
                        FIX_R15_PC  
                        rm=(inst>>3)&0xF;
                        rc=read_register(rm);
                        rc+=2;
                        if(rc&1)
                        {
                          write_register(15,rc);
                          thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                          continue;
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
                                  continue;
                                }
                            break;
                        }
                      }
                      
                      //SBC
                      if((inst&0xFFC0)==0x4180)
                      {
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
                        continue;
                      }

                      //TST
                      if((inst&0xFFC0)==0x4200)
                      {
                        rn=(inst>>0)&0x7;
                        rm=(inst>>3)&0x7;
                        ra=read_register(rn);
                        rb=read_register(rm);
                        ZNflags=ra&rb;
                        continue;
                      }
                      

                      //ASR(2) two register
                      if((inst&0xFFC0)==0x4100)
                      {
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
                        do_znflags(rc);
                        do_cflag(ra,~rb,1);
                        if (bSafeThumb) do_sub_vflag(ra,rb,rc);
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
                        do_znflags(rc);
                        continue;
                      }

                      //MVN
                      if((inst&0xFFC0)==0x43C0)
                      {
                        rd=(inst>>0)&0x7;
                        rm=(inst>>3)&0x7;
                        ra=read_register(rm);
                        rc=(~ra);
                        write_register(rd,rc);
                        do_znflags(rc);
                        continue;
                      }

                      //NEG
                      if((inst&0xFFC0)==0x4240)
                      {
                        rd=(inst>>0)&0x7;
                        rm=(inst>>3)&0x7;
                        ra=read_register(rm);
                        rc=0-ra;
                        write_register(rd,rc);
                        do_znflags(rc);
                        do_cflag(0,~ra,1);
                        if (bSafeThumb) do_sub_vflag(0,ra,rc);
                        continue;
                      }

                      //ORR
                      if((inst&0xFFC0)==0x4300)
                      {
                        rd=(inst>>0)&0x7;
                        rm=(inst>>3)&0x7;
                        ra=read_register(rd);
                        rb=read_register(rm);
                        rc=ra|rb;
                        write_register(rd,rc);
                        do_znflags(rc);
                        continue;
                      }
                      
                      //CMN
                      if((inst&0xFFC0)==0x42C0)
                      {
                        rn=(inst>>0)&0x7;
                        rm=(inst>>3)&0x7;
                        ra=read_register(rn);
                        rb=read_register(rm);
                        rc=ra+rb;
                        do_znflags(rc);
                        do_cflag(ra,rb,0);
                        if (bSafeThumb) do_add_vflag(ra,rb,rc);
                        continue;
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
                        do_znflags(rc);
                        continue;
                      }


                      //LSL(2) two register
                      if((inst&0xFFC0)==0x4080)
                      {
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
                        continue;
                      }

                      //LSR(2) two register
                      if((inst&0xFFC0)==0x40C0)
                      {
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
                        continue;
                      }


                      //ROR
                      if((inst&0xFFC0)==0x41C0)
                      {
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
                        continue;
                      }

                      //BLX(2)
                      if((inst&0xFF87)==0x4780)
                      {
                        FIX_R15_PC  
                        rm=(inst>>3)&0xF;
                        rc=read_register(rm);
                        rc+=2;
                        if(rc&1)
                        {
                          write_register(14,reg_sys[15]-2);
                          write_register(15,rc);
                          thumb_ptr = &rom[(reg_sys[15]-2) >> 1];
                          continue;
                        }
                        else
                        {
                          // fxq: this could serve as exit code
                          break;
                        }
                      }
                      
                  }
              }
          }
          else // Bit 14=0
          {
              //ADD(2) big immediate one register
              if((inst&0xF800)==0x3000)
              {
                rd=(inst>>8)&0x7;
                reg_sys[rd] += (inst>>0)&0xFF;
                do_znflags(reg_sys[rd]);
                // ------------------------------------------------------------------------------------------------------
                // TBD: This is incorrect (but faster) emulation on an instruction that is very common... we're adding 
                // a small number to a 32-bit register and we're going to assume that there is no vflag nor cFlag needed.
                // ------------------------------------------------------------------------------------------------------
                //if (bSafeThumb) do_add_vflag(ra,-rb,rc);
                //if (bSafeThumb) do_cflag(ra,rb,0);
                continue;
              }

              //CMP(1) compare immediate
              if((inst&0xF800)==0x2800)
              {
                rb=(inst>>0)&0xFF;
                ra=read_register((inst>>8)&0x07);
                ZNflags=ra-rb;
                do_cflag_fast(ra,~rb);
                if (bSafeThumb) do_sub_vflag(ra,rb,ZNflags);
                continue;
              }              
              
              //ADD(3) three registers
              if((inst&0xFE00)==0x1800)
              {
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
                continue;
              }

              //MOV(1) immediate
              if((inst&0xF800)==0x2000)
              {
                ZNflags=(inst>>0)&0xFF;
                write_register((inst>>8)&0x07,ZNflags);
                continue;
              }
              
              //SUB(3)
              if((inst&0xFE00)==0x1A00)
              {
                rd=(inst>>0)&0x7;
                rn=(inst>>3)&0x7;
                rm=(inst>>6)&0x7;
                reg_sys[rd]=reg_sys[rn]-reg_sys[rm];
                do_znflags(reg_sys[rd]);
                do_cflag(reg_sys[rn],~reg_sys[rm],1);
                if (bSafeThumb) do_sub_vflag(reg_sys[rn],reg_sys[rm],reg_sys[rd]);
                continue;
              }


              //ASR(1) two register immediate
              if((inst&0xF800)==0x1000)
              {
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
                do_znflags(rc);
                continue;
              }

              //LSR(1) two register immediate
              if((inst&0xF800)==0x0800)
              {
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
                continue;
              }

              //SUB(2)
              if((inst&0xF800)==0x3800)
              {
                rb=(inst>>0)&0xFF;
                rd=(inst>>8)&0x07;
                ra=read_register(rd);
                rc=ra-rb;
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag_fast(ra,~rb);
                if (bSafeThumb) do_sub_vflag(ra,rb,rc);
                continue;
              }
              
              //MOV(2) two low registers
              if((inst&0xFFC0)==0x1C00)
              {
                rd=(inst>>0)&7;
                rn=(inst>>3)&7;
                rc=read_register(rn);
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag_bit(0);
                do_vflag_bit(0);
                continue;
              }

              //SUB(1)
              if((inst&0xFE00)==0x1E00)
              {
                rd=(inst>>0)&7;
                rn=(inst>>3)&7;
                rb=(inst>>6)&7;
                ra=read_register(rn);
                rc=ra-rb;
                write_register(rd,rc);
                do_znflags(rc);
                do_cflag_fast(ra,~rb);
                if (bSafeThumb) do_sub_vflag(ra,rb,rc);
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
                  do_znflags(rc);
                  if (bSafeThumb)
                  {
                      do_cflag(ra,rb,0);
                      do_add_vflag(ra,rb,rc);
                  }
                  continue;
                }
                else
                {
                  //this is a mov
                }
              }              
          }
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE int Thumbulator::reset ( void )
{
    if (myCartInfo.banking == BANK_CDFJ)
    {
      reg_sys[13]=0x40001fb4; //sp
      reg_sys[14]=0x00000800; //lr (duz this use odd addrs)
      reg_sys[15]=0x0000080b; //pc entry point of 0x809+2
    }
    else
    {
      reg_sys[13]=0x40001fb4; //sp
      reg_sys[14]=0x00000c00; //lr (duz this use odd addrs)
      reg_sys[15]=0x00000c0b; //pc entry point of 0xc09+2
    }
    return 0;
}
