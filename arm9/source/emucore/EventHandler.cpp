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

#include <nds.h>

#include <algorithm>
#include <sstream>

#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "StellaEvent.hxx"
#include "System.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::EventHandler(Console* console)
    : myConsole(console)
{
  // Create the event object which will be used for this handler
  myEvent = new Event();

  // Erase the KeyEvent array 
  for(Int32 i = 0; i < StellaEvent::LastKCODE; ++i)
    myKeyTable[i] = Event::NoType;

  setKeymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::~EventHandler()
{
  if(myEvent)
    delete myEvent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event* EventHandler::event()
{
  return myEvent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setKeymap()
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  string list = "";
  replace(list.begin(), list.end(), ':', ' ');

  if(isValidList(list, StellaEvent::LastKCODE))
  {
    istringstream buf(list);
    string key;

    // Fill the keymap table with events
    for(Int32 i = 0; i < StellaEvent::LastKCODE; ++i)
    {
      buf >> key;
      myKeyTable[i] = (Event::Type) atoi(key.c_str());
    }
  }
  else
    setDefaultKeymap();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::getKeymapArray(Event::Type** array, uInt32* size)
{
  *array = myKeyTable;
  *size  = StellaEvent::LastKCODE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::setDefaultKeymap()
{
  myKeyTable[StellaEvent::KCODE_1]         = Event::KeyboardZero1;
  myKeyTable[StellaEvent::KCODE_2]         = Event::KeyboardZero2;
  myKeyTable[StellaEvent::KCODE_3]         = Event::KeyboardZero3;
  myKeyTable[StellaEvent::KCODE_q]         = Event::KeyboardZero4;
  myKeyTable[StellaEvent::KCODE_w]         = Event::KeyboardZero5;
  myKeyTable[StellaEvent::KCODE_e]         = Event::KeyboardZero6;
  myKeyTable[StellaEvent::KCODE_a]         = Event::KeyboardZero7;
  myKeyTable[StellaEvent::KCODE_s]         = Event::KeyboardZero8;
  myKeyTable[StellaEvent::KCODE_d]         = Event::KeyboardZero9;
  myKeyTable[StellaEvent::KCODE_z]         = Event::KeyboardZeroStar;
  myKeyTable[StellaEvent::KCODE_x]         = Event::KeyboardZero0;
  myKeyTable[StellaEvent::KCODE_c]         = Event::KeyboardZeroPound;

  myKeyTable[StellaEvent::KCODE_8]         = Event::KeyboardOne1;
  myKeyTable[StellaEvent::KCODE_9]         = Event::KeyboardOne2;
  myKeyTable[StellaEvent::KCODE_0]         = Event::KeyboardOne3;
  myKeyTable[StellaEvent::KCODE_i]         = Event::KeyboardOne4;
  myKeyTable[StellaEvent::KCODE_o]         = Event::KeyboardOne5;
  myKeyTable[StellaEvent::KCODE_p]         = Event::KeyboardOne6;
  myKeyTable[StellaEvent::KCODE_k]         = Event::KeyboardOne7;
  myKeyTable[StellaEvent::KCODE_l]         = Event::KeyboardOne8;
  myKeyTable[StellaEvent::KCODE_SEMICOLON] = Event::KeyboardOne9;
  myKeyTable[StellaEvent::KCODE_COMMA]     = Event::KeyboardOneStar;
  myKeyTable[StellaEvent::KCODE_PERIOD]    = Event::KeyboardOne0;
  myKeyTable[StellaEvent::KCODE_SLASH]     = Event::KeyboardOnePound;

  myKeyTable[StellaEvent::KCODE_UP]        = Event::JoystickZeroUp;
  myKeyTable[StellaEvent::KCODE_DOWN]      = Event::JoystickZeroDown;
  myKeyTable[StellaEvent::KCODE_LEFT]      = Event::JoystickZeroLeft;
  myKeyTable[StellaEvent::KCODE_RIGHT]     = Event::JoystickZeroRight;
  myKeyTable[StellaEvent::KCODE_SPACE]     = Event::JoystickZeroFire;
  myKeyTable[StellaEvent::KCODE_4]         = Event::BoosterGripZeroTrigger;
  myKeyTable[StellaEvent::KCODE_5]         = Event::BoosterGripZeroBooster;

  myKeyTable[StellaEvent::KCODE_y]         = Event::JoystickOneUp;
  myKeyTable[StellaEvent::KCODE_h]         = Event::JoystickOneDown;
  myKeyTable[StellaEvent::KCODE_g]         = Event::JoystickOneLeft;
  myKeyTable[StellaEvent::KCODE_j]         = Event::JoystickOneRight;
  myKeyTable[StellaEvent::KCODE_f]         = Event::JoystickOneFire;
  myKeyTable[StellaEvent::KCODE_6]         = Event::BoosterGripOneTrigger;
  myKeyTable[StellaEvent::KCODE_7]         = Event::BoosterGripOneBooster;

  myKeyTable[StellaEvent::KCODE_INSERT]    = Event::DrivingZeroCounterClockwise;
  myKeyTable[StellaEvent::KCODE_PAGEUP]    = Event::DrivingZeroClockwise;
  myKeyTable[StellaEvent::KCODE_HOME]      = Event::DrivingZeroFire;

  myKeyTable[StellaEvent::KCODE_DELETE]    = Event::PaddleZeroResistance;
  myKeyTable[StellaEvent::KCODE_END]       = Event::PaddleZeroFire;

    
  myKeyTable[StellaEvent::KCODE_F1]        = Event::ConsoleSelect;
  myKeyTable[StellaEvent::KCODE_F2]        = Event::ConsoleReset;
  myKeyTable[StellaEvent::KCODE_F3]        = Event::ConsoleColor;
  myKeyTable[StellaEvent::KCODE_F4]        = Event::ConsoleBlackWhite;
  myKeyTable[StellaEvent::KCODE_F5]        = Event::ConsoleLeftDifficultyA;
  myKeyTable[StellaEvent::KCODE_F6]        = Event::ConsoleLeftDifficultyB;
  myKeyTable[StellaEvent::KCODE_F7]        = Event::ConsoleRightDifficultyA;
  myKeyTable[StellaEvent::KCODE_F8]        = Event::ConsoleRightDifficultyB;
  myKeyTable[StellaEvent::KCODE_F9]        = Event::PaddleTwoResistance;
  myKeyTable[StellaEvent::KCODE_F10]       = Event::PaddleTwoFire;
  myKeyTable[StellaEvent::KCODE_F11]       = Event::PaddleOneResistance;
  myKeyTable[StellaEvent::KCODE_F12]       = Event::PaddleOneFire;
  myKeyTable[StellaEvent::KCODE_F13]       = Event::PaddleThreeResistance;
  myKeyTable[StellaEvent::KCODE_F14]       = Event::PaddleThreeFire;
  myKeyTable[StellaEvent::KCODE_PAUSE]     = Event::Pause;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::isValidList(string list, uInt32 length)
{
  // Rudimentary check to see if the list contains 'length' keys
  istringstream buf(list);
  string key;
  uInt32 i = 0;

  while(buf >> key)
    i++;

  return (i == length);
}

