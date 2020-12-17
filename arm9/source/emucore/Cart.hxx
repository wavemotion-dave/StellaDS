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

// The following is a simple table mapping games to type's using MD5 values
struct CartInfo
{
  const char* md5;
  const char* type;
  Int8  controllerType;
  Int8  special;
  Int8  mode;
  Int8  analogSensitivity;      // 10=1.0
  Int8  yOffset;
};

extern CartInfo myCartInfo;

#define CTR_LJOY      0     // Left Joystick is used for player 1 (default)
#define CTR_RJOY      1     // Right Joystick is used for player 1
#define CTR_RAIDERS   2     // Special 2 joystick setup for Raiders of the Lost Ark
#define CTR_PADDLE0   3     // For Paddle Games like Breakout and Kaboom
#define CTR_PADDLE1   4     // A few odd games use the OTHER paddle...sigh...
#define CTR_DRIVING   5     // For Driving Controller games like Indy500
#define CTR_KEYBOARD  6     // For keyboard games like Codebreaker
#define CTR_STARRAID  7     // Star raiders has Left Joystick and Right Keypad

#define SPEC_NONE     0     // Nothing special to do with this game...
#define SPEC_HAUNTED  1     // Haunted House - fix bug by patching offset 1103's E5 to E9
#define SPEC_CONMARS  2     // Conquest of Mars - fix bug for collision detections
#define SPEC_PITFALL2 3     // For Pitfall 2 we are employing a few other tricks to get speed...

#define MODE_NO       0     // Normal Mode
#define MODE_FF       1     // Flicker Free Mode

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

    /**
      Returns true if the image is probably a SuperChip (256 bytes RAM)
    */
    static bool isProbablySC(const uInt8* image, uInt32 size);

    /**
      Returns true if the image is probably a 3F bankswitching cartridge
    */
    static bool isProbably3F(const uInt8* image, uInt32 size);

    /**
      Returns true if the image is probably a 3E bankswitching cartridge
    */
    static bool isProbably3E(const uInt8* image, uInt32 size);

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

