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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// This file has been modified by Dave Bernazzani (wavemotion-dave)
// for optimized execution on the DS/DSi platform. Please seek the
// official Stella source distribution which is far cleaner, newer,
// and better maintained.
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "Booster.hxx"
#include "Cart.hxx"
#include "Console.hxx"
#include "Control.hxx"
#include "Driving.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "Genesis.hxx"
#include "Joystick.hxx"
#include "Keyboard.hxx"
#include "SaveKey.hxx"
#include "M6502Low.hxx"
#include "M6532.hxx"
#include "MD5.hxx"
#include "MediaSrc.hxx"
#include "Paddles.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "TIASound.hxx"

TIA *theTIA     __attribute__((section(".dtcm"))) = 0;
M6532 *theM6532 __attribute__((section(".dtcm"))) = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const uInt8* image, uInt32 size, const char* filename)
{
  extern uInt16 mySoundFreq;
  myControllers[0] = 0;
  myControllers[1] = 0;
  myMediaSource = 0;
  mySwitches = 0;
  mySystem = 0;
  myEvent = 0;

  // Create an event handler which will collect and dispatch events
  myEventHandler = new EventHandler(this);
  myEvent = myEventHandler->event();

  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  mySwitches = new Switches(*myEvent);
  mySystem = new System(MY_ADDR_SHIFT, MY_PAGE_SHIFT);

  M6502* m6502 = new M6502Low(1);
  theM6532 = new M6532(*this);
  myCartridge = Cartridge::create(image, size); // Do this before creating the TIA because we use some of the cart properties there...

  // ------------------------------------------------------------------------------------------------------
  // If we are DPC+ we reduce the sound quality so that we can squeeze ARM-assisted Thumbulator code in...
  // ------------------------------------------------------------------------------------------------------
  extern uInt16 origSoundFreq;
  if (myCartInfo.banking == BANK_DPCP) mySoundFreq = 11025; else mySoundFreq = origSoundFreq;
    
  TIA* tia = new TIA(*this);
  theTIA = tia;
  Tia_sound_init(31400, mySoundFreq);

  // -------------------------------------------------------------------------------------------
  // Depending on the game we will "install" either Joysticks, Paddles or Driving Controllers
  // -------------------------------------------------------------------------------------------
  if ((myCartInfo.controllerType == CTR_PADDLE0) || (myCartInfo.controllerType == CTR_PADDLE1) || 
      (myCartInfo.controllerType == CTR_PADDLE2) || (myCartInfo.controllerType == CTR_PADDLE3) ||
      (myCartInfo.controllerType == CTR_BUMPBASH))
  {
      myControllers[0] = new Paddles(Controller::Left, *myEvent);
      myControllers[1] = new Paddles(Controller::Right, *myEvent);
  }
  else if (myCartInfo.controllerType == CTR_DRIVING)
  {
      myControllers[0] = new Driving(Controller::Left, *myEvent);
      myControllers[1] = new Driving(Controller::Right, *myEvent);
  }
  else if (myCartInfo.controllerType == CTR_BOOSTER)
  {
      myControllers[0] = new BoosterGrip(Controller::Left, *myEvent);
      myControllers[1] = new BoosterGrip(Controller::Right, *myEvent);
  }
  else if (myCartInfo.controllerType == CTR_GENESIS)
  {
      myControllers[0] = new Genesis(Controller::Left, *myEvent);
      myControllers[1] = new SaveKey(Controller::Right, *myEvent);  // For the Genesis pad, we will allow a Save Key
  }
  else if ((myCartInfo.controllerType == CTR_KEYBOARD0) || (myCartInfo.controllerType == CTR_KEYBOARD1))
  {
      myControllers[0] = new Keyboard(Controller::Left, *myEvent);
      myControllers[1] = new Keyboard(Controller::Right, *myEvent);
  }
  else if (myCartInfo.controllerType == CTR_STARRAID)
  {
      myControllers[0] = new Joystick(Controller::Left, *myEvent);
      myControllers[1] = new Keyboard(Controller::Right, *myEvent);
  }
  else  // Most games fall into this category... the venerable Joystick with one red button! For this type, we place a virtual SaveKey in the right jack.
  {
      myControllers[0] = new Joystick(Controller::Left, *myEvent);
      if (myCartInfo.controllerType == CTR_LJOY)
        myControllers[1] = new SaveKey(Controller::Right, *myEvent);    // If we are a Left Joystick, we can support a Save Key in the right port
      else
        myControllers[1] = new Joystick(Controller::Right, *myEvent);
  }
        
  mySystem->attach(m6502);
  mySystem->attach(theM6532);
  mySystem->attach(tia);
  mySystem->attach(myCartridge);

  myMediaSource = tia;

  mySystem->reset();

  fakePaddleResistance = 500000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const Console& console)
{
  // TODO: Write this method
  assert(false);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::~Console()
{
  delete mySystem;
  delete mySwitches;
  delete myControllers[0];
  delete myControllers[1];
  delete myEventHandler;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::update()
{
// myFrameBuffer.update();
	myMediaSource->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console& Console::operator = (const Console&)
{
  // TODO: Write this method
  assert(false);

  return *this;
}

