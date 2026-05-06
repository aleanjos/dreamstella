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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// ---------------------------------------------------------------------------
// Sega Dreamcast Port (2025-2026):
// Adapted, optimized, and maintained by Alessandro dos Anjos (Ale-DC)
// ---------------------------------------------------------------------------
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef MEDIA_FACTORY_HXX
#define MEDIA_FACTORY_HXX

#include "OSystem.hxx"
#include "Settings.hxx"

#include "FrameBuffer.hxx"
#include "FrameBufferSoftDC.hxx"

#include "Sound.hxx"
#include "SoundDC.hxx"

/**
  This class deals with the different framebuffer/sound implementations
  for the various ports of Stella, and always returns a valid media object
  based on the specific port and restrictions on that port.

  I think you can see why this mess was put into a factory class :)

  @author  Stephen Anthony
  @version $Id$
*/
class MediaFactory
{
  public:
    static FrameBuffer* createVideo(OSystem* osystem)
    {
      FrameBuffer* fb = (FrameBuffer*) NULL;

      // OpenGL mode *may* fail, so we check for it first


      // If OpenGL failed, or if it wasn't requested, create the appropriate
      // software framebuffer
      if(!fb)
        fb = new FrameBufferSoftDC(osystem);

      // This should never happen
      assert(fb != NULL);

      return fb;
    }

    static Sound* createAudio(OSystem* osystem)
    {
      Sound* sound = (Sound*) NULL;
      sound = new SoundDC(osystem);
      return sound;
    }
};

#endif
