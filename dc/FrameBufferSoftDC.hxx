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


#ifndef FRAMEBUFFER_SOFTDC_HXX
#define FRAMEBUFFER_SOFTDC_HXX

class OSystem;
class RectList;

#include "bspf.hxx"
#include "FrameBuffer.hxx"


/**
  This class implements a Sega Dreamcast software framebuffer, based on Stella's FrameBufferSoft. Adapted by Ale.

  @author  Stephen Anthony (Original author)
  @version $Id$
*/
class FrameBufferSoftDC : public FrameBuffer
{
  friend class FBSurfaceSoft;

  public:
    /**
      Creates a new software framebuffer
    */
    FrameBufferSoftDC(OSystem* osystem);


    /**
      Destructor
    */
    virtual ~FrameBufferSoftDC();

    uInt32 myDrawHeight; // Used for PAL video compatibility.
    alignas(32) uInt32 dcPalette[256];

    void setTIAPalette(const uInt32* palette) override;
    virtual FBInitStatus initialize(const string& title, uInt32 width, uInt32 height) override;

    void drawTIA(bool full);

    //////////////////////////////////////////////////////////////////////
    // The following are derived from public methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

    /**
      This method is called to query the type of the FrameBuffer.
    */
    BufferType type() const { return kSoftBuffer; }

    /**
      This method is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    void scanline(uInt32 row, uInt8* data) const;

  protected:
    //////////////////////////////////////////////////////////////////////
    // The following are derived from protected methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize the video subsystem
      with the given video mode.  Normally, it will also call setVidMode().

      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    bool initSubsystem(VideoMode& mode);


    /**
      This method is called to change to the given video mode.  If the mode
      is successfully changed, 'mode' holds the actual dimensions used.

      @param mode  The video mode to use

      @return  False on any errors (in which case 'mode' is invalid), else true
    */
    bool setVidMode(VideoMode& mode);

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    void invalidate();

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one
    */
    FBSurface* createSurface(int w, int h, bool useBase = false) const;

    /**
      This method should be called anytime the TIA needs to be redrawn
      to the screen (full indicating that a full redraw is required).
    */

    /**
      This method is called after any drawing is done (per-frame).
    */
    void postFrameUpdate();

    /**
      This method is called to provide information about the FrameBuffer.
    */
    string about() const;

    void drawDebugFPS(int fps);

  private:
    int myZoomLevel;
    int myBytesPerPixel;
    int myBaseOffset;
    int myPitch;
    uint16_t myBackBuffer[320 * 240] __attribute__((aligned(32)));

    enum RenderType {
      kSoftZoom_16,
      kSoftZoom_24,
      kSoftZoom_32,
      kPhosphor_16,
      kPhosphor_24,
      kPhosphor_32
    };
    RenderType myRenderType;

    // Indicates if the TIA image has been modified
    bool myTiaDirty;
	 	 
    // Indicates if we're in a purely UI mode
    bool myInUIMode;

    public:
      uint16_t* getBackBuffer() { return myBackBuffer; }
};

#endif
