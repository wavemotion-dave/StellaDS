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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Cart.cxx,v 1.8 2005/02/13 19:17:02 stephena Exp $
//============================================================================
#include <nds.h>

#include <assert.h>
#include <string.h>
#include "Cart.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3F.hxx"
#include "Cart4K.hxx"
#include "CartAR.hxx"
#include "CartDPC.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFASC.hxx"
#include "CartFE.hxx"
#include "CartMC.hxx"
#include "CartMB.hxx"
#include "CartCV.hxx"
#include "CartUA.hxx"
#include "MD5.hxx"

bool bUseRightJoy = false;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge* Cartridge::create(const uInt8* image, uInt32 size)
{
  Cartridge* cartridge = 0;

   string type = autodetectType(image, size);
   
  // We should know the cart's type by now so let's create it
  if(type == "2K")
    cartridge = new Cartridge2K(image);
  else if(type == "3E") // ALEK
    cartridge = new Cartridge3E(image, size);
  else if(type == "3F")
    cartridge = new Cartridge3F(image, size);
  else if(type == "4K")
    cartridge = new Cartridge4K(image);
  else if(type == "AR")
    cartridge = new CartridgeAR(image, size);
  else if(type == "DPC")
    cartridge = new CartridgeDPC(image, size);
  else if(type == "E0")
    cartridge = new CartridgeE0(image);
  else if(type == "E7")
    cartridge = new CartridgeE7(image);
  else if(type == "F4")
    cartridge = new CartridgeF4(image);
  else if(type == "F4SC")
    cartridge = new CartridgeF4SC(image);
  else if(type == "F6")
    cartridge = new CartridgeF6(image);
  else if(type == "F6SC")
    cartridge = new CartridgeF6SC(image);
  else if(type == "F8")
    cartridge = new CartridgeF8(image);
  else if(type == "F8SC")
    cartridge = new CartridgeF8SC(image);
  else if(type == "FASC")
    cartridge = new CartridgeFASC(image);
  else if(type == "FE")
    cartridge = new CartridgeFE(image);
  else if(type == "MC")
    cartridge = new CartridgeMC(image, size);
  else if(type == "MB")
    cartridge = new CartridgeMB(image);
  else if(type == "CV")
    cartridge = new CartridgeCV(image, size);
  else if(type == "UA")
    cartridge = new CartridgeUA(image);
  else
  {
    // TODO: At some point this should be handled in a better way...
    assert(false);
  }

  return cartridge;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::~Cartridge()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::autodetectType(const uInt8* image, uInt32 size)
{
  extern void OutputCartInfo(string type, string md5);
  // The following is a simple table mapping games to type's using MD5 values
  struct MD5ToType
  {
    const char* md5;
    const char* type;
    bool  useRightJoy;
  };

  static MD5ToType table[] = {
    {"5336f86f6b982cc925532f2e80aa1e17", "E0",    false},    // Death Star
    {"b311ab95e85bc0162308390728a7361d", "E0",    false},    // Gyruss
    {"c29f8db680990cb45ef7fef6ab57a2c2", "E0",    false},    // Super Cobra
    {"085322bae40d904f53bdcc56df0593fc", "E0",    false},    // Tutankamn
    {"c7f13ef38f61ee2367ada94fdcc6d206", "E0",    false},    // Popeye
    {"6339d28c9a7f92054e70029eb0375837", "E0",    false},    // Star Wars, Arcade
    {"27c6a2ca16ad7d814626ceea62fa8fb4", "E0",    false},    // Frogger II
    {"3347a6dd59049b15a38394aa2dafa585", "E0",    false},    // Montezuma's Revenge
    {"6dda84fb8e442ecf34241ac0d1d91d69", "F6SC",  false},    // Dig Dug
    {"57fa2d09c9e361de7bd2aa3a9575a760", "F8SC",  false},    // Stargate
    {"3a771876e4b61d42e3a3892ad885d889", "F8SC",  false},    // Defender ][
    {"efefc02bbc5258815457f7a5b8d8750a", "FASC",  false},    // Tunnel runner
    {"7e51a58de2c0db7d33715f518893b0db", "FASC",  false},    // Mountain King
    {"9947f1ebabb56fd075a96c6d37351efa", "FASC",  false},    // Omega Race
    {"0443cfa9872cdb49069186413275fa21", "E7",    false},    // Burger Timer
    {"76f53abbbf39a0063f24036d6ee0968a", "E7",    false},    // Bump-N-Jump
    {"3b76242691730b2dd22ec0ceab351bc6", "E7",    false},    // He-Man
    {"ac7c2260378975614192ca2bc3d20e0b", "FE",    false},    // Decathlon
    {"4f618c2429138e0280969193ed6c107e", "FE",    false},    // Robot Tank
    {"6d842c96d5a01967be9680080dd5be54", "DPC",   false},    // Pitfall II
    {"d3bb42228a6cd452c111c1932503cc03", "UA",    false},    // Funky Fish
    {"8bbfd951c89cc09c148bfabdefa08bec", "UA",    false},    // Pleiades
    {"ab4ac994865fb16ebb85738316309457", "2K",    true},     // Basketball (1978).bin
    {"7e8aa18bc9502eb57daaf5e7c1e94da7", "4K",    true},     // Wizard of Wor
    {"663ef22eb399504d5204c543b8a86bcd", "4K",    true},     // Wizard of Wor
    {"0c7926d660f903a2d6910c254660c32c", "2K",    true},     // Air-Sea Battle (pal)
    {"1d1d2603ec139867c1d1f5ddf83093f1", "2K",    true},     // Air-Sea Battle (sears)
    {"16cb43492987d2f32b423817cdaaf7c4", "2K",    true},     // Air-Sea Battle
    {"4d7517ae69f95cfbc053be01312b7dba", "2K",    true},     // Surround
    {"31d08cb465965f80d3541a57ec82c625", "4K",    true},     // Surround (4k)    
    {"c370c3268ad95b3266d6e36ff23d1f0c", "2K",    true},     // Surround (pal)
    {"7b938c7ddf18e8362949b62c7eaa660a", "4K",    true},     // Starship (4k)
    {"e363e467f605537f3777ad33e74e113a", "2K",    true},     // Starship (2k)
    {"0bfabf1e98bdb180643f35f2165995d0", "2K",    true},     // HomeRun
    {"9f901509f0474bf9760e6ebd80e629cd", "4K",    true},     // HomeRun(sears 4k)
    {"705fe719179e65b0af328644f3a04900", "4K",    true},     // Slot Machine (4k)
    {"81254ebce88fa46c4ff5a2f4d2bad538", "4K",    true},     // Slot Machine (4k)
    {"f90b5da189f24d7e1a2117d8c8abc952", "4K",    true},     // Slot Machine (2k)
    {"fc6052438f339aea373bbc999433388a", "4K",    true},     // Slot Machine (pal)    
    {(char*)0,                           (char*)0}
  };
    
  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Take a closer look at the ROM image and try to figure out its type
  const char* type = 0;

  bUseRightJoy = false;
  // First we'll see if it's type is listed in the table above
  for(MD5ToType* entry = table; (entry->md5 != 0); ++entry)
  {
    if(entry->md5 == md5)
    {
      type = entry->type;
      bUseRightJoy = entry->useRightJoy;
      break;
    }
  }

  // If we didn't find the type in the table then guess it based on size
  if(type == 0)
  {
    if((size % 8448) == 0)
    {
      type = "AR";
    }
    //ALEK else if((size == 2048) || (memcmp(image, image + 2048, 2048) == 0))
    else if((size == 2048) ||
          (size == 4096 && memcmp(image, image + 2048, 2048) == 0))
    {
      type = "2K";
    }
    //ALEK else if((size == 4096) || (memcmp(image, image + 4096, 4096) == 0))
    else if(size == 4096)
    {
      type = "4K";
    }
    //ALEK else if((size == 8192) || (memcmp(image, image + 8192, 8192) == 0))
    // ALEK {
    // ALEK  type = isProbably3F(image, size) ? "3F" : "F8";
    //ALEK }
    else if(size == 8192)  // 8K
    {
      // TODO - autodetect FE and UA, probably not possible
      if(isProbablySC(image, size))
        type = "F8SC";
      else if(memcmp(image, image + 4096, 4096) == 0)
        type = "4K";
      else if(isProbablyE0(image, size))
        type = "E0";
      else if(isProbably3F(image, size))
        type = isProbably3E(image, size) ? "3E" : "3F";
      else
        type = "F8";
    }
    //ALEK else if((size == 10495) || (size == 10240))
    else if((size == 10495) || (size == 10496) || (size == 10240))  // 10K - Pitfall2
    {
      type = "DPC";
    }
    else if(size == 12288)
    {
      // TODO - this should really be in a method that checks the first
      // 512 bytes of ROM and finds if either the lower 256 bytes or
      // higher 256 bytes are all the same.  For now, we assume that
      // all carts of 12K are CBS RAM Plus/FASC.
      type = "FASC";
    }
    else if(size == 16384)  // 16K ALEK
    {    
      if(isProbablySC(image, size))
        type = "F6SC";
      else if(isProbablyE7(image, size))
        type = "E7";
      else if(isProbably3F(image, size))
        type = isProbably3E(image, size) ? "3E" : "3F";
      else
        type = "F6";
    }
    else if(size == 32768)
    {
      // Assume this is a 32K super-cart then check to see if it is
#if 0 
// ALEK
      type = "F4SC";

      uInt8 first = image[0];
      for(uInt32 i = 0; i < 256; ++i)
      {
        if(image[i] != first)
        {
          // It's not a super cart (probably)
          type = isProbably3F(image, size) ? "3F" : "F4";
          break;
        }
      }
#endif
      if(isProbablySC(image, size))
        type = "F4SC";
      else if(isProbably3F(image, size))
        type = isProbably3E(image, size) ? "3E" : "3F";
      else
        type = "F4";
    }
    else if(size == 65536)
    {
      //ALEK type = isProbably3F(image, size) ? "3F" : "MB";
      // TODO - autodetect 4A50
      if(isProbably3F(image, size))
        type = isProbably3E(image, size) ? "3E" : "3F";
      else
        type = "MB";
    }
    else if(size == 131072)
    {
      //ALEK type = isProbably3F(image, size) ? "3F" : "MC";
      // TODO - autodetect 4A50
      if(isProbably3F(image, size))
        type = isProbably3E(image, size) ? "3E" : "3F";
      else
        type = "MC";
    }
    else  // what else can we do?
    {
      if(isProbably3F(image, size))
        type = isProbably3E(image, size) ? "3E" : "3F";
      else
        type = "4K";  // Most common bankswitching type
    }  
#if 0 
//ALEK
    else
    {
      // Assume this is a 16K super-cart then check to see if it is
      type = "F6SC";

      uInt8 first = image[0];
      for(uInt32 i = 0; i < 256; ++i)
      {
        if(image[i] != first)
        {
          // It's not a super cart (probably)
          type = isProbably3F(image, size) ? "3F" : "F6";
          break;
        }
      }
    }
#endif    
  }
  
  
  OutputCartInfo(type, md5);

  return type;
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge::searchForBytes(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 1; ++i)
  {
    if((image[i] == byte1) && (image[i + 1] == byte2))
    {
      ++count;
    }
  }

  return count;
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablySC(const uInt8* image, uInt32 size)
{
  // We assume a Superchip cart contains the same bytes for its entire
  // RAM area; obviously this test will fail if it doesn't
  // The RAM area will be the first 256 bytes of each 4K bank
  uInt32 banks = size / 4096;
  for(uInt32 i = 0; i < banks; ++i)
  {
    uInt8 first = image[i*4096];
    for(uInt32 j = 0; j < 256; ++j)
    {
      if(image[i*4096+j] != first)
        return false;
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbably3F(const uInt8* image, uInt32 size)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 1; ++i)
  {
    if((image[i] == 0x85) && (image[i + 1] == 0x3F))
    {
      ++count;
    }
  }

  return (count > 2);
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbably3E(const uInt8* image, uInt32 size)
{
  return (searchForBytes(image, size, 0x85, 0x3E) > 2);
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyE0(const uInt8* image, uInt32 size)
{
  // E0 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FF7 using absolute non-indexed addressing
  // So we search for the pattern 'LDA Fxxx' or 'STA Fxxx' in hex
  // using the regex (AD|8D, E0-F7, xF)
  // This must be present at least three times, since there are
  // three segments to be initialized (and a few more so that more
  // of the ROM is used)
  // Thanks to "stella@casperkitty.com" for this advice
  uInt32 count = 0;
  for(uInt32 i = 0; i < size - 2; ++i)
  {
    uInt8 b1 = image[i], b2 = image[i+1], b3 = image[i+2];
    if((b1 == 0xAD || b1 == 0x8D) &&
       (b2 >= 0xE0 && b2 <= 0xF7) &&
       ((b3 & 0xF) == 0xF))
    {
      if(++count > 4)  return true;
    }
  }
  return false;
}

//ALEK - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::isProbablyE7(const uInt8* image, uInt32 size)
{
  // E7 carts map their second 1K block of RAM at addresses
  // $800 to $8FF.  However, since this occurs in the upper 2K address
  // space, and the last 2K in the cart always points to the last 2K of the
  // ROM image, the RAM area should fall in addresses $3800 to $38FF
  // Similar to the Superchip cart, we assume this RAM block contains
  // the same bytes for its entire area
  // Also, we want to distinguish between ROMs that have large blocks
  // of the same amount of (probably unused) data by making sure that
  // something differs in the previous 32 or next 32 bytes
  uInt8 first = image[0x3800];
  for(uInt32 i = 0x3800; i < 0x3A00; ++i)
  {
    if(first != image[i])
      return false;
  }

  // OK, now scan the surrounding 32 byte blocks
  uInt32 count1 = 0, count2 = 0;
  for(uInt32 i = 0x3800 - 32; i < 0x3800; ++i)
  {
    if(first != image[i])
      ++count1;
  }
  for(uInt32 i = 0x3A00; i < 0x3A00 + 32; ++i)
  {
    if(first != image[i])
      ++count2;
  }

  return (count1 > 0 || count2 > 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Cartridge&)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge& Cartridge::operator = (const Cartridge&)
{
  assert(false);
  return *this;
}

