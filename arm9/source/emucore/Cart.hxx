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
// Copyright (c) 1995-2003 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Cart.hxx,v 1.2 2003/02/17 04:59:54 bwmott Exp $
//============================================================================

#ifndef CARTRIDGE_HXX
#define CARTRIDGE_HXX

class Cartridge;
class System;

#include "bspf.hxx"
#include "Device.hxx"
#include "System.hxx"

extern uInt8 fast_cart_buffer[];

extern PageAccess page_access;
extern uInt16 myCurrentOffset;

#define NTSC    0
#define PAL     1


// The following is a simple table mapping games to type's using MD5 values
struct CartInfo
{
  string md5;
  string gameID;
  string type;
  Int8  controllerType;
  Int8  special;
  Int8  mode;
  Int8  vblankZero;
  Int8  hBlankZero;
  Int8  analogSensitivity;      // 10=1.0
  uInt8 tv_type;                // NTSC or PAL
  uInt8 displayStartScanline;   
  uInt8 displayStopScanline;  
  uInt8 screenScale;            // 100 = 100% (smaller numbers squish screen to fit)
  Int8  xOffset;
  Int8  yOffset;
};

extern CartInfo myCartInfo;

#define CTR_LJOY         0     // Left Joystick is used for player 1 (default)
#define CTR_RJOY         1     // Right Joystick is used for player 1
#define CTR_RAIDERS      2     // Special 2 joystick setup for Raiders of the Lost Ark
#define CTR_PADDLE0      3     // For Paddle Games like Breakout and Kaboom
#define CTR_PADDLE1      4     // A few odd games use the OTHER paddle...sigh...
#define CTR_DRIVING      5     // For Driving Controller games like Indy500
#define CTR_KEYBOARD0    6     // For keyboard games like Codebreaker
#define CTR_STARRAID     7     // Star raiders has Left Joystick and Right Keypad
#define CTR_BOOSTER      8     // Omega Race and Thrust+
#define CTR_PADDLE2      9     // Paddle in right port
#define CTR_PADDLE3     10     // Tac-Scan uses the second set of paddles and the other paddle on that side. Double sigh.
#define CTR_STARGATE    11     // Defender II, Stargate and Defender Arcade (Hack) need both joysticks
#define CTR_KEYBOARD1   12     // For keyboard games - second port
#define CTR_SOLARIS     13     // For Solaris - needs button on 2nd controller
#define CTR_GENESIS     14     // For Genesis 2-button Controller support
#define CTR_MCA         15     // For missile command arcade - 3 button support
#define CTR_BUMPBASH    16     // For bumper bash (both paddle buttons used)

#define SPEC_NONE        0     // Nothing special to do with this game...
#define SPEC_HAUNTED     1     // Haunted House - fix bug by patching offset 1103's E5 to E9
#define SPEC_CONMARS     2     // Conquest of Mars - fix bug for collision detections
#define SPEC_QUADRUN     3     // Quadrun has some audio artifacts we can't generate so we disable them
#define SPEC_AR          4     // AR Carts we must track distinct memory access
#define SPEC_MELTDOWN    5     // Meltdown requires NUSIZ0/1 changes
#define SPEC_BUMPBASH    6     // Bumper Bash requires NUSIZ0/1 changes
#define SPEC_KOOLAID     7     // Patch to avoid collisions (2002 original patch)
#define SPEC_BIGBIRD     8     // Patched for joystick use.
#define SPEC_ALPHABM     9     // Patched for joystick use.
#define SPEC_COOKIEM    10     // Patched for joystick use.

#define MODE_NO          0     // Normal Mode
#define MODE_FF          1     // Flicker Free Mode (blend last 2 frames equally)
#define MODE_BACKG       2     // Flicker Reduce (try using background color grab - helps with Missile Command, Astroblast etc. with shifting backgrounds)
#define MODE_BLACK       3     // Ficker Reduce (using Black background improvement only)


// Analog Sensitivity... 10 = 1.0 and normal... 1.1 is faster and 0.9 is slower
#define ANA0_7        7
#define ANA0_8        8
#define ANA0_9        9
#define ANA1_0       10
#define ANA1_1       11
#define ANA1_2       12
#define ANA1_3       13
#define ANA1_4       14
#define ANA1_5       15
#define ANA1_6       16
#define ANA1_7       17
#define ANA1_8       18
#define ANA1_9       19
#define ANA2_0       20
#define ANA2_5       25

/**
  A cartridge is a device which contains the machine code for a 
  game and handles any bankswitching performed by the cartridge.
 
  @author  Bradford W. Mott
  @version $Id: Cart.hxx,v 1.2 2003/02/17 04:59:54 bwmott Exp $
*/
class Cartridge : public Device
{
  public:
    /**
      Create a new cartridge object allocated on the heap.  The
      type of cartridge created depends on the properties object.

      @param image A pointer to the ROM image
      @param size The size of the ROM image 
      @param properties The properties associated with the game
      @return Pointer to the new cartridge object allocated on the heap
    */
    static Cartridge* create(const uInt8* image, uInt32 size);

  public:
    /**
      Create a new cartridge
    */
    Cartridge();
 
    /**
      Destructor
    */
    virtual ~Cartridge();
    
  private:
    /**
      Try to auto-detect the bankswitching type of the cartridge

      @param image A pointer to the ROM image
      @param size The size of the ROM image 
      @return The "best guess" for the cartridge type
    */
    static string autodetectType(const uInt8* image, uInt32 size);

    /**
      Utility method used by isProbably3F and isProbably3E
    */
    static int searchForBytes(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2);
    static int searchForBytes3(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2, uInt8 byte3);    
    static int searchForBytes4(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2, uInt8 byte3, uInt8 byte4);
    static int searchForBytes5(const uInt8* image, uInt32 size, uInt8 byte1, uInt8 byte2, uInt8 byte3, uInt8 byte4, uInt8 byte5);
    
    /**
      Returns true if the image is probably a SuperChip (256 bytes RAM)
    */
    static bool isProbablySC(const uInt8* image, uInt32 size);
    
    // Returns true if probalby FE cart
    static bool isProbablyFE(const uInt8* image, uInt32 size);

    // Returns true if probably UA cart
    static bool isProbablyUA(const uInt8* image, uInt32 size);    

    /**
      Returns true if the image is probably a 3F bankswitching cartridge
    */
    static bool isProbably3F(const uInt8* image, uInt32 size);

    /**
      Returns true if the image is probably a 3E bankswitching cartridge
    */
    static bool isProbably3E(const uInt8* image, uInt32 size);
    
    /**
      Returns true if the image is probably a DPC+ bankswitching cartridge
    */
    static bool isProbablyDPCplus(const uInt8* image, uInt32 size);

    /**
      Returns true if the image is probably a E0 bankswitching cartridge
    */
    static bool isProbablyE0(const uInt8* image, uInt32 size);

    /**
      Returns true if the image is probably a E7 bankswitching cartridge
    */
    static bool isProbablyE7(const uInt8* image, uInt32 size);

  private:
    // Copy constructor isn't supported by cartridges so make it private
    Cartridge(const Cartridge&);

    // Assignment operator isn't supported by cartridges so make it private
    Cartridge& operator = (const Cartridge&);
};
#endif

