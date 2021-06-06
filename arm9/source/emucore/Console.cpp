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
// $Id: Console.cxx,v 1.38 2004/08/12 23:54:36 stephena Exp $
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
#include "Joystick.hxx"
#include "Keyboard.hxx"
#include "M6502Low.hxx"
#include "M6502Hi.hxx"
#include "M6532.hxx"
#include "MD5.hxx"
#include "MediaSrc.hxx"
#include "Paddles.hxx"
#include "Sound.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "TIASound.hxx"

TIA *theTIA = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const uInt8* image, uInt32 size, const char* filename, Sound& sound)
    : mySound(sound)
{
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
  M6532* m6532 = new M6532(*this);
  TIA* tia = new TIA(*this, mySound);
  myCartridge = Cartridge::create(image, size);
  theTIA = tia;
  Tia_sound_init(31400, 22050);

  // -------------------------------------------------------------------------------------------
  // Depending on the game we will "install" either Joysticks, Paddles or Driving Controllers
  // -------------------------------------------------------------------------------------------
  if ((myCartInfo.controllerType == CTR_PADDLE0) || (myCartInfo.controllerType == CTR_PADDLE1))
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
  else if (myCartInfo.controllerType == CTR_KEYBOARD)
  {
      myControllers[0] = new Keyboard(Controller::Left, *myEvent);
      myControllers[1] = new Keyboard(Controller::Right, *myEvent);
  }
  else if (myCartInfo.controllerType == CTR_STARRAID)
  {
      myControllers[0] = new Joystick(Controller::Left, *myEvent);
      myControllers[1] = new Keyboard(Controller::Right, *myEvent);
  }
  else  // Most games fall into this category... the venerable Joystick with one red button!
  {
      myControllers[0] = new Joystick(Controller::Left, *myEvent);
      myControllers[1] = new Joystick(Controller::Right, *myEvent);
  }
        
  mySystem->attach(m6502);
  mySystem->attach(m6532);
  mySystem->attach(tia);
  mySystem->attach(myCartridge);

  myMediaSource = tia;

  mySystem->reset();

  fakePaddleResistance = 500000;

  mySound.init(this, myMediaSource, mySystem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const Console& console)
    : mySound(console.mySound)
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
Sound& Console::sound() const
{
  return mySound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console& Console::operator = (const Console&)
{
  // TODO: Write this method
  assert(false);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleFormat()
{
;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::togglePalette()
{
	myMediaSource->togglePalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::saveProperties(string filename, bool merge)
{
;
}
