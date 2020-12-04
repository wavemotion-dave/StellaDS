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
// $Id: SoundSDL.cxx,v 1.8 2005/02/13 19:17:02 stephena Exp $
//============================================================================

#include <nds.h>
#include <assert.h>

#include <stdio.h>

#include "TIASound.hxx"
#include "Console.hxx"
#include "System.hxx"
#include "SoundSDL.hxx"
#include <math.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::SoundSDL(uInt32 fragsize)
    : myIsInitializedFlag(false),
      myIsMuted(false),
      myVolume(100)
{
    myIsMuted = false;
    Tia_sound_init(31400, 22050);
    myIsInitializedFlag = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::callback(uInt8* stream, int len)
{
	processFragment(stream, (Int32)len);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::~SoundSDL()
{
  myIsInitializedFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundSDL::isSuccessfullyInitialized() const
{
  return myIsInitializedFlag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    if(myIsMuted == state)
    {
      return;
    }

    myIsMuted = state;

    myRegWriteQueue.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::reset()
{
	if(myIsInitializedFlag)
	{
		myIsMuted = false;
		myLastRegisterSetCycle = 0;
		myRegWriteQueue.clear();
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::setVolume(Int32 percent)
{
	;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::adjustVolume(Int8 direction)
{
	;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::set(uInt16 addr, uInt8 value, Int32 cycle)
{
	Update_tia_sound(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::processFragment(uInt8* stream, Int32 length)
{
	Tia_process(stream, length);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::RegWriteQueue::RegWriteQueue(uInt32 capacity)
    : myCapacity(capacity),
      myBuffer(0),
      mySize(0),
      myHead(0),
      myTail(0)
{
  myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
  {
    grow();
  }

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL::RegWrite& SoundSDL::RegWriteQueue::front()
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL::RegWriteQueue::grow()
{
  RegWrite* buffer = new RegWrite[myCapacity * 2];
  for(uInt32 i = 0; i < mySize; ++i)
  {
    buffer[i] = myBuffer[(myHead + i) % myCapacity];
  }
  myHead = 0;
  myTail = mySize;
  myCapacity = myCapacity * 2;
  delete[] myBuffer;
  myBuffer = buffer;
}
