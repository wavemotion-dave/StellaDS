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
// $Id: Console.hxx,v 1.22 2004/07/10 13:20:35 stephena Exp $
//============================================================================

#ifndef CONSOLE_HXX
#define CONSOLE_HXX

class Console;
class Controller;
class Event;
class EventHandler;
class MediaSource;
class Sound;
class Switches;
class System;

#include "bspf.hxx"
#include "Control.hxx"
#include "Cart.hxx"

#define A26_VID_WIDTH  160      // We are showing 160 pixels wide
#define A26_VID_HEIGHT 200      // Technically only need 192 but many games use the overscan area... 
#define A26_VID_XOFS   0
#define A26_VID_YOFS   0

/**
  This class represents the entire game console.

  @author  Bradford W. Mott
  @version $Id: Console.hxx,v 1.22 2004/07/10 13:20:35 stephena Exp $
*/
class Console
{
  public:
    /**
      Create a new console for emulating the specified game using the
      given event object and game profiles.

      @param image       The ROM image of the game to emulate
      @param size        The size of the ROM image  
      @param filename    The name of the file that contained the ROM image
      @param settings    The settings object to use
      @param profiles    The game profiles object to use
      @param framebuffer The framebuffer object to use
      @param sound       The sound object to use
    */
    Console(const uInt8* image, uInt32 size, const char* filename,  Sound& sound);

    /**
      Create a new console object by copying another one

      @param console The object to copy
    */
    Console(const Console& console);
 
    /**
      Destructor
    */
    virtual ~Console();

  public:
    /**
      Updates the console by one frame.  Each frontend should
      call this method 'framerate' times per second.
    */
    void update();

    /**
      Get the controller plugged into the specified jack

      @return The specified controller
    */
    Controller& controller(Controller::Jack jack) const
    {
      return (jack == Controller::Left) ? *myControllers[0] : *myControllers[1];
    }

    /**
      Get the frame rate for the emulation
    */
    uInt32 frameRate() const;

    /**
      Get the sound object of the console

      @return The sound object for this console
    */
    Sound& sound() const;

    /**
      Get the console switches

      @return The console switches
    */
    Switches& switches() const
    {
      return *mySwitches;
    }

    /**
      Get the 6502 based system used by the console to emulate the game

      @return The 6502 based system
    */
    System& system() const
    {
      return *mySystem;
    }

    /**
      Get the event handler of the console

      @return The event handler
    */
    EventHandler& eventHandler() const
    {
      return *myEventHandler;
    }

  public:
    /**
      Overloaded assignment operator

      @param console The console object to set myself equal to
      @return Myself after assignment has taken place
    */
    Console& operator = (const Console& console);
    
    uInt32 fakePaddleResistance;

  public:

    /**
      Toggle between NTSC and PAL mode.  The frontends will need to
      reload their palette.
    */
    void toggleFormat();

    /**
      Toggle between the available palettes.  The frontends will need to
      reload their palette.
    */
    void togglePalette();

    /**
      Save a copy of the current properties after any changes.

      @param filename Filename to save the properties into.
      @param merge Whether or not to merge the changes into the
                   main properties file.
    */
    void saveProperties(string filename, bool merge = false);

    // Pointer to the media source object 
   MediaSource* myMediaSource;
  
   Cartridge* myCartridge;

  private:
    // Pointers to the left and right controllers
    Controller* myControllers[2];

    // Pointer to the event object to use
    Event* myEvent;

    // Pointer to the switches on the front of the console
    Switches* mySwitches;
 
    // Pointer to the 6502 based system being emulated 
    System* mySystem;

    // Reference to the Sound object
    Sound& mySound;

    // Frame rate being used by the emulator
    uInt32 myFrameRate;

    // Pointer to the EventHandler object
    EventHandler* myEventHandler;
};
#endif
