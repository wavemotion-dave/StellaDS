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

#ifndef CARTRIDGE_CDF_HXX
#define CARTRIDGE_CDF_HXX

class System;
#ifdef THUMB_SUPPORT
class Thumbulator;
#endif

#include "bspf.hxx"
#include "Cart.hxx"

#define COMMSTREAM        0x20
#define JUMPSTREAM_BASE   0x21

#define DSRAM   0x800


/**
  Cartridge class used for CDF/CDFJ.
*/
class CartridgeCDF : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
    */
    CartridgeCDF(const uInt8* image, uInt32 size);
 
    /**
      Destructor
    */
    virtual ~CartridgeCDF();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset();

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system);

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    void bank(uInt16 bank);


    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uInt8* getImage(int& size) const;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    virtual const char* name() const;

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
    
    uInt32 getDatastreamPointer(uInt8 index) const;
    void setDatastreamPointer(uInt8 index, uInt32 value);
    uInt32 getDatastreamIncrement(uInt8 index) const;
    uInt8 readFromDatastream(uInt8 index);
    

  private:
    /** 
      Call Special Functions
    */
    void callFunction(uInt8 value);

  private:
    // The ROM image and size
    uInt32 mySize;

    // Pointer to the 1K frequency table
    uInt8* myFrequencyImage;
    
    //void updateMusicModeDataFetchers(void);
    //uInt32 getWaveform(uInt8 index) const;
    //uInt32 getSample(void);  
};

extern uInt16 myDatastreamBase;
extern uInt16 myDatastreamIncrementBase;
extern uInt16 myAmplitudeStream;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 CartridgeCDF::getDatastreamPointer(uInt8 index) const
{
  uInt32 *ptr = (uInt32*) &fast_cart_buffer[myDatastreamBase + index * 4];
  return *ptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeCDF::setDatastreamPointer(uInt8 index, uInt32 value)
{
  uInt32 *ptr = (uInt32*) &fast_cart_buffer[myDatastreamBase + index * 4];
  *ptr = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt32 CartridgeCDF::getDatastreamIncrement(uInt8 index) const
{
  uInt32 *ptr = (uInt32*) &fast_cart_buffer[myDatastreamIncrementBase + index * 4];
  return *ptr;
}


#endif
