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
// $Id: CartF4.hxx,v 1.2 2005/02/13 19:17:02 stephena Exp $
//============================================================================

#ifndef CartridgeTV_HXX
#define CartridgeTV_HXX

class CartridgeTV;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for 256K banked BF games.
*/
class CartridgeTV : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image Pointer to the ROM image
    */
    CartridgeTV(const uInt8* image);
 
    /**
      Destructor
    */
    virtual ~CartridgeTV();

  public:
    /**
      Get a null terminated string which is the device's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const;

    /**
      Reset device to its power-on state
    */
    virtual void reset();

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    virtual void install(System& system);

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    virtual void poke(uInt16 address, uInt8 value);

  private:
    /**
      Install pages for the specified bank in the system

      @param bank The bank that should be installed in the system
    */
    void bank(uInt16 bank);

  private:
    uInt8 *myImage;
    
    uInt32 myCurrentOffset32;
};
#endif

