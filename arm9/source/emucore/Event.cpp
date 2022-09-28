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
#include "Event.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Event()
    : myNumberOfTypes(Event::LastType)
{
  // Set all of the events to 0 / false to start with
  for(int i = 0; i < myNumberOfTypes; ++i)
  {
    myValues[i] = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::~Event()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE Int32 Event::get(Type type) const
{
  return myValues[type];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ITCM_CODE void Event::set(Type type, Int32 value)
{
  myValues[type] = value;
}

