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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <fat.h>
#include <dirent.h>
#include "MT24LC256.hxx"
#include "System.hxx"
#include "SaveKey.hxx"

unsigned int myDigitalPinState[9];
unsigned int myAnalogPinValue[9];
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SaveKey::SaveKey(Jack jack, const Event& event) : Controller(jack, event),
    myEEPROM(NULL)
{
    DIR* dir = opendir("/data");
    if (dir)
    {
        closedir(dir);  /* Directory exists. */
    }
    else
    {
        mkdir("/data", 0777);
    }        
  myEEPROM = new MT24LC256("/data/StellaDS.EE");

  myDigitalPinState[One] = myDigitalPinState[Two] = true;
  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SaveKey::~SaveKey()
{
  delete myEEPROM;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SaveKey::read(DigitalPin pin)
{
  // We need to override the Controller::read() method, since the timing
  // of the actual read is important for the EEPROM (we can't just read
  // 60 times per second in the ::update() method)
  switch(pin)
  {
    // Pin 3: EEPROM SDA
    //        input data from the 24LC256 EEPROM using the I2C protocol
    case Three:
      return (myDigitalPinState[Three] = myEEPROM->readSDA());

    default:
      return 1;
  }
}

Int32 SaveKey::read(AnalogPin pin)
{
    return maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SaveKey::write(DigitalPin pin, bool value)
{
  // Change the pin state based on value
  switch(pin)
  {
    // Pin 3: EEPROM SDA
    //        output data to the 24LC256 EEPROM using the I2C protocol
    case Three:
      myDigitalPinState[Three] = value;
      myEEPROM->writeSDA(value);
      break;

    // Pin 4: EEPROM SCL
    //        output clock data to the 24LC256 EEPROM using the I2C protocol
    case Four:
      myDigitalPinState[Four] = value;
      myEEPROM->writeSCL(value);
      break;

    default:
      break;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SaveKey::systemCyclesReset()
{
  // The EEPROM keeps track of cycle counts, and needs to know when the
  // cycles are reset
  myEEPROM->systemCyclesReset();
}
