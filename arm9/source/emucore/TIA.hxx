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
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIA.hxx,v 1.14 2004/06/13 04:53:04 bwmott Exp $
//============================================================================

#ifndef TIA_HXX
#define TIA_HXX

class Console;
class Sound;
class System;

#include <string>

#include "bspf.hxx"
#include "Device.hxx"
#include "MediaSrc.hxx"

#define      myP0Bit       0x01         // Bit for Player 0
#define      myM0Bit       0x02         // Bit for Missle 0
#define      myP1Bit       0x04         // Bit for Player 1
#define      myM1Bit       0x08         // Bit for Missle 1
#define      myBLBit       0x10         // Bit for Ball
#define      myPFBit       0x20         // Bit for Playfield
#define      ScoreBit      0x40         // Bit for Playfield score mode
#define      PriorityBit   0x080        // Bit for Playfield priority

// Used to set the collision register to the correct value
extern uInt16 ourCollisionTable[64];
extern uInt8 myPriorityEncoder[2][256];

extern    uInt16 myCollision;    // Collision register

    // Note that these position registers contain the color clock 
    // on which the object's serial output should begin (0 to 159)
extern    Int16 myPOSP0;         // Player 0 position register
extern    Int16 myPOSP1;         // Player 1 position register
extern    Int16 myPOSM0;         // Missle 0 position register
extern    Int16 myPOSM1;         // Missle 1 position register
extern    Int16 myPOSBL;         // Ball position register

extern uInt8 myPlayfieldPriorityAndScore;
extern uInt32 myColor[4];

extern    uInt8 myCTRLPF;       // Playfield control register
extern    uInt8 myREFP0;         // Indicates if player 0 is being reflected
extern    uInt8 myREFP1;         // Indicates if player 1 is being reflected
extern    uInt32 myPF;          // Playfield graphics (19-12:PF2 11-4:PF1 3-0:PF0)
extern    uInt8 myGRP0;         // Player 0 graphics register
extern    uInt8 myGRP1;         // Player 1 graphics register
extern    uInt8 myDGRP0;        // Player 0 delayed graphics register
extern    uInt8 myDGRP1;        // Player 1 delayed graphics register
extern    uInt8 myENAM0;         // Indicates if missle 0 is enabled
extern    uInt8 myENAM1;         // Indicates if missle 0 is enabled
extern    uInt8 myENABL;         // Indicates if the ball is enabled
extern    uInt8 myDENABL;        // Indicates if the virtically delayed ball is enabled
extern    Int8 myHMP0;          // Player 0 horizontal motion register
extern    Int8 myHMP1;          // Player 1 horizontal motion register
extern    Int8 myHMM0;          // Missle 0 horizontal motion register
extern    Int8 myHMM1;          // Missle 1 horizontal motion register
extern    Int8 myHMBL;          // Ball horizontal motion register
extern    uInt8 myVDELP0;        // Indicates if player 0 is being virtically delayed
extern    uInt8 myVDELP1;        // Indicates if player 1 is being virtically delayed
extern    uInt8 myVDELBL;        // Indicates if the ball is being virtically delayed
extern    uInt8 myRESMP0;        // Indicates if missle 0 is reset to player 0
extern    uInt8 myRESMP1;        // Indicates if missle 1 is reset to player 1
extern    uInt8* myCurrentFrameBuffer[2]; // Pointer to the current frame buffer
extern    uInt8* myFramePointer;          // Pointer to the next pixel that will be drawn in the current frame buffer
extern    uInt16* myDSFramePointer;       // Pointer to start of the DS video frame
extern    uInt32 myFrameXStart;           // Where do we start drawing X Pos?
extern    uInt32 myFrameYStart;           // Where do we start drawing Y Pos?
extern    uInt32 myFrameWidth;            // The width of the frame
extern    uInt32 myFrameHeight;           // The height of the frame
extern    uInt32 myStartDisplayOffset;
extern    uInt32 myStopDisplayOffset;
extern    Int32 myVSYNCFinishClock; 
extern    uInt8 myEnabledObjects;
extern    Int32 myClockWhenFrameStarted;
extern    Int32 myCyclesWhenFrameStarted;   
extern    Int32 myClockStartDisplay;
extern    Int32 myClockStopDisplay;
extern    Int32 myClockAtLastUpdate;
extern    Int32 myClocksToEndOfScanLine;
extern    Int32 myMaximumNumberOfScanlines;
extern    uInt8 myVSYNC;                        // Holds the VSYNC register value
extern    uInt8 myVBLANK;                       // Holds the VBLANK register value
extern    Int32 myLastHMOVEClock;
extern    uInt8 myHMOVEBlankEnabled;
extern    uInt8 myAllowHMOVEBlanks;
extern    uInt8 myM0CosmicArkMotionEnabled;
extern    uInt32 myM0CosmicArkCounter;
extern    uInt8 myCurrentGRP0;                  // Graphics for Player 0 that should be displayed.  This will be reflected if the player is being reflected.
extern    uInt8 myCurrentGRP1;                  // Graphics for Player 1 that should be displayed.  This will be reflected if the player is being reflected.

// It's VERY important that the BL, M0, M1, P0 and P1 current mask pointers are always on a uInt32 boundary.  Otherwise, the TIA code will fail on a good number of CPUs.
extern    uInt8* myCurrentBLMask;               // Pointer to the currently active mask array for the ball
extern    uInt8* myCurrentM0Mask;               // Pointer to the currently active mask array for missle 0
extern    uInt8* myCurrentM1Mask;               // Pointer to the currently active mask array for missle 1
extern    uInt8* myCurrentP0Mask;               // Pointer to the currently active mask array for player 0
extern    uInt8* myCurrentP1Mask;               // Pointer to the currently active mask array for player 1
extern    uInt32* myCurrentPFMask;                // Pointer to the currently active mask array for the playfield
extern    uInt8 ourPlayerReflectTable[256];        // Used to reflect a players graphics
extern    uInt32 ourPlayfieldTable[2][160];        // Playfield mask table for reflected and non-reflected playfields
extern    uInt8 myNUSIZ0;       // Number and size of player 0 and missle 0
extern    uInt8 myNUSIZ1;       // Number and size of player 1 and missle 1


/**
  This class is a device that emulates the Television Interface Adapator 
  found in the Atari 2600 and 7800 consoles.  The Television Interface 
  Adapator is an integrated circuit designed to interface between an 
  eight bit microprocessor and a television video modulator. It converts 
  eight bit parallel data into serial outputs for the color, luminosity, 
  and composite sync required by a video modulator.  

  This class outputs the serial data into a frame buffer which can then
  be displayed on screen.

  @author  Bradford W. Mott
  @version $Id: TIA.hxx,v 1.14 2004/06/13 04:53:04 bwmott Exp $
*/
class TIA : public Device , public MediaSource
{
  public:
    /**
      Create a new TIA for the specified console

      @param console The console the TIA is associated with
      @param sound   The sound object the TIA is associated with
    */
    TIA(const Console& console, Sound& sound);
 
    /**
      Destructor
    */
    virtual ~TIA();

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
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    virtual void systemCyclesReset();

    /**
      Install TIA in the specified system.  Invoked by the system
      when the TIA is attached to it.

      @param system The system the device should install itself in
    */
    virtual void install(System& system);

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    virtual void poke(uInt16 address, uInt8 value);

  public:
    /**
      This method should be called at an interval corresponding to
      the desired frame rate to update the media source.
    */
    virtual void update();

    /**
      Get the palette which maps frame data to RGB values.

      @return Array of integers which represent the palette (RGB)
    */
    virtual const uInt32* palette() const;

    /**
      Answers the height of the frame buffer

      @return The frame's height
    */
    uInt32 height() const;

    /**
      Answers the width of the frame buffer

      @return The frame's width
    */
    uInt32 width() const;


    /**
      Toggle between the available palettes.  The frontends will need to
      reload their palette.
    */
    virtual void togglePalette();
    
  private:
    // Actual palette = NTSC
    uInt32 ourActualPalette ;

    // Compute the ball mask table
    void computeBallMaskTable();

    // Compute the collision decode table
    void computeCollisionTable();

    // Compute the missle mask table
    void computeMissleMaskTable();

    // Compute the player mask table
    void computePlayerMaskTable();

    // Compute the player position reset when table
    void computePlayerPositionResetWhenTable();

    // Compute the player reflect table
    void computePlayerReflectTable();

    // Compute playfield mask table
    void computePlayfieldMaskTable();

    // Update the current frame buffer up to one scanline
    void updateFrameScanline(uInt32 clocksToUpdate, uInt32 hpos);

    // Update the current frame buffer to the specified color clock
    void updateFrame(Int32 clock);

    // Waste cycles until the current scanline is finished
    void waitHorizontalSync();

  private:
    // Console the TIA is associated with
    const Console& myConsole;

    // Sound object the TIA is associated with
    Sound& mySound;

  private:
    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL (and maybe SECAM) systems when the previous frame
    // contains an odd number of scanlines.
    bool myColorLossEnabled;

  private:
    // Indicates when the dump for paddles was last set
    Int32 myDumpDisabledCycle;

    // Indicates if the dump is current enabled for the paddles
    bool myDumpEnabled;

  private:
    // Ball mask table (entries are true or false)
    static uInt8 ourBallMaskTable[4][4][320];

    // A mask table which can be used when an object is disabled
    static uInt8 ourDisabledMaskTable[640];

    // Missle mask table (entries are true or false)
    static uInt8 ourMissleMaskTable[4][8][4][320];

    // Used to convert value written in a motion register into 
    // its internal representation
    static const Int32 ourCompleteMotionTable[76][16];

    // Indicates if HMOVE blanks should occur for the corresponding cycle
    static const bool ourHMOVEBlankEnableCycles[76];

    // Player mask table
    static uInt8 ourPlayerMaskTable[4][2][8][320];

    // Indicates if player is being reset during delay, display or other times
    static Int8 ourPlayerPositionResetWhenTable[8][160][160];

    // Table of RGB values for NTSC
    static const uInt32 ourNTSCPalette[256];

    // Table of RGB values for PAL.  NOTE: The odd numbered entries in
    // this array are always shades of grey.  This is used to implement
    // the PAL color loss effect.
    static const uInt32 ourPALPalette[256];

  private:
    // Copy constructor isn't supported by this class so make it private
    TIA(const TIA&);

    // Assignment operator isn't supported by this class so make it private
    TIA& operator = (const TIA&);
};

#endif
