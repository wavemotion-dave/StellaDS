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

#ifndef MEDIASOURCE_HXX
#define MEDIASOURCE_HXX

#include <string>

class MediaSource;

#include "bspf.hxx"

/**
  This class provides an interface for accessing graphics and audio data.

  @author  Bradford W. Mott
  @version $Id: MediaSrc.hxx,v 1.8 2004/07/14 16:15:06 stephena Exp $
*/
class MediaSource
{
  public:
    /**
      Create a new media source
    */
    MediaSource();
 
    /**
      Destructor
    */
    virtual ~MediaSource();

  public:
    /**
      This method should be called at an interval corresponding to the 
      desired frame rate to update the media source.  Invoking this method
      will update the graphics buffer and generate the corresponding audio
      samples.
    */
    virtual void update() = 0;

    /**
      Toggle between the available palettes.  The frontends will need to
      reload their palette.
    */
    virtual void togglePalette() = 0;
    
  public:
    /**
      Get the palette which maps frame data to RGB values.

      @return Array of integers which represent the palette (RGB)
    */
    virtual const uInt32* palette() const = 0;

    /**
      Answers the height of the frame buffer

      @return The frame's height
    */
    virtual uInt32 height() const = 0;

    /**
      Answers the width of the frame buffer

      @return The frame's width
    */
    virtual uInt32 width() const = 0;

  public:


  private:
    // Copy constructor isn't supported by this class so make it private
    MediaSource(const MediaSource&);

    // Assignment operator isn't supported by this class so make it private
    MediaSource& operator = (const MediaSource&);
};
#endif

