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

#include <cstdio>

#include <dc/video.h>
#include <dc/sq.h>
#include <arch/timer.h>

#include "Console.hxx"
#include "OSystem.hxx"
#include "TIA.hxx"

#include "FrameBufferSoftDC.hxx"

FrameBufferSoftDC::FrameBufferSoftDC(OSystem *osystem)
    : FrameBuffer(osystem),
      myZoomLevel(1),
      myRenderType(kSoftZoom_16),
      myTiaDirty(false),
      myInUIMode(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBufferSoftDC::~FrameBufferSoftDC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoftDC::initSubsystem(VideoMode &mode)
{
    return setVidMode(mode);
}

FBInitStatus FrameBufferSoftDC::initialize(const string &title, uInt32 width, uInt32 height)
{
    myDrawHeight = height;
    return FrameBuffer::initialize(title, width, height);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBufferSoftDC::setVidMode(VideoMode &mode)
{
    vid_clear(0, 0, 0);
    vid_set_mode(DM_320x240, PM_RGB565);

    uInt32 vram_addr = (uInt32)vram_s;
    uInt32 val = (vram_addr >> 24) & 0x1c;

    *((volatile uInt32 *)0xff000038) = val;
    *((volatile uInt32 *)0xff00003c) = val;

    myBytesPerPixel = 2;
    myPitch = 320;

    myRenderType = kSoftZoom_16;
    myUsePhosphor = false;

    myBaseOffset = 0;

    mode.screen_w = 320;
    mode.screen_h = 240;
    myZoomLevel = 1;

    return true;
}

void FrameBufferSoftDC::drawTIA(bool fullRedraw)
{
    const TIA &tia = myOSystem->console().tia();

    const uInt32 *palette = dcPalette;
    uInt32 drawHeight = tia.height();

    uInt8 *currentFB = tia.currentFrameBuffer();
    if (!currentFB)
        return;

    for (uInt32 y = 0; y < 240; ++y)
    {
        uInt32 currentFBY = 0;
        bool fillBlack = false;

        if (drawHeight > 240)
        {
            uInt32 excess = (drawHeight - 240) / 2;
            currentFBY = y + excess;
        }
        else if (drawHeight < 240)
        {
            uInt32 offset = (240 - drawHeight) / 2;
            if (y < offset || y >= (offset + drawHeight))
                fillBlack = true;
            else
                currentFBY = y - offset;
        }
        else
        {
            currentFBY = y;
        }

        volatile uint32_t *sq = (volatile uint32_t *)(0xe0000000 | (((uint32_t)vram_s + (y * 640)) & 0x03ffffe0));

        if (fillBlack)
        {
            for (int x = 0; x < 20; ++x)
            {
                sq[0] = 0;
                sq[1] = 0;
                sq[2] = 0;
                sq[3] = 0;
                sq[4] = 0;
                sq[5] = 0;
                sq[6] = 0;
                sq[7] = 0;
                __asm__ volatile("pref @%0" : : "r"(sq));
                sq += 8;
            }

            continue;
        }

        uInt8 *pixelAtari = currentFB + (currentFBY * 160);

        for (int x = 0; x < 20; ++x)
        {
            sq[0] = palette[pixelAtari[0]];
            sq[1] = palette[pixelAtari[1]];
            sq[2] = palette[pixelAtari[2]];
            sq[3] = palette[pixelAtari[3]];
            sq[4] = palette[pixelAtari[4]];
            sq[5] = palette[pixelAtari[5]];
            sq[6] = palette[pixelAtari[6]];
            sq[7] = palette[pixelAtari[7]];

            __asm__ volatile("pref @%0" : : "r"(sq));

            pixelAtari += 8;
            sq += 8;
        }
    }

    sq_wait();

    // Adaptive VSYNC and framerate control
    static uint64_t lastFrameTime = 0;
    uint64_t currentTime = timer_us_gettime64();

    if (myOSystem->myFramerate > 55)
    {
        if ((currentTime - lastFrameTime) < 16000)
            vid_waitvbl();
    }
    else
    {
        uint64_t targetDuration = 1000000 / myOSystem->myFramerate;

        if (lastFrameTime > 0)
        {
            while ((timer_us_gettime64() - lastFrameTime) < targetDuration)
            {
            }
        }
    }

    lastFrameTime = timer_us_gettime64();
}

void FrameBufferSoftDC::setTIAPalette(const uInt32 *palette)
{
    for (int i = 0; i < 256; i++)
    {
        uInt32 c = palette[i];
        uInt8 r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        uInt16 c16 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        dcPalette[i] = (c16 << 16) | c16;
    }
}
