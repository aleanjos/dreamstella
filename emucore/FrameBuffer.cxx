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

#include <dc/video.h>

#include <algorithm>
#include <cstdio>

#include "bspf.hxx"

#include "Console.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "TIA.hxx"

#include "FrameBuffer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::FrameBuffer(OSystem *osystem)
	 : myOSystem(osystem),
		mySDLFlags(0),
		myRedrawEntireFrame(true),
		myUsePhosphor(false),
		myPhosphorBlend(77),
		myInitializedCount(0),
		myPausedCount(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::~FrameBuffer(void)
{
	// Free all allocated surfaces
	while (!mySurfaceList.empty())
	{
		delete (*mySurfaceList.begin()).second;
		mySurfaceList.erase(mySurfaceList.begin());
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus FrameBuffer::initialize(const string &title, uInt32 width, uInt32 height)
{
	myInitializedCount++;

	// Set the available video modes for this framebuffer
	setAvailableVidModes(width, height);

	// Initialize video subsystem (make sure we get a valid mode)
	VideoMode mode = getSavedVidMode();

	if (width <= mode.screen_w && height <= mode.screen_h)
	{
		if (initSubsystem(mode))
		{
			myImageRect.setWidth(mode.image_w);
			myImageRect.setHeight(mode.image_h);
			myImageRect.moveTo(mode.image_x, mode.image_y);

			myScreenRect.setWidth(mode.screen_w);
			myScreenRect.setHeight(mode.screen_h);
		}
		else
		{
			printf("ERROR: Couldn't initialize video subsystem");
			return kFailNotSupported;
		}
	}
	else
		return kFailTooLarge;

	return kSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::update()
{
	// Run the console for one frame
	// Note that the debugger can cause a breakpoint to occur, which changes
	// the EventHandler state 'behind our back' - we need to check for that
	myOSystem->console().tia().update();

	if (myOSystem->eventHandler().frying())
		myOSystem->console().fry();

	// And update the screen
	drawTIA(myRedrawEntireFrame);

	// The frame doesn't need to be completely redrawn anymore
	myRedrawEntireFrame = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::refresh()
{
	drawTIA(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBSurface *FrameBuffer::surface(uInt32 id) const
{
	map<uInt32, FBSurface *>::const_iterator iter = mySurfaceList.find(id);
	return iter != mySurfaceList.end() ? iter->second : NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::tiaPixel(uInt32 idx, uInt8 shift) const
{
	uInt8 c = *(myOSystem->console().tia().currentFrameBuffer() + idx) | shift;
	uInt8 p = *(myOSystem->console().tia().previousFrameBuffer() + idx) | shift;

	return (!myUsePhosphor ? myDefPalette[c] : myAvgPalette[c][p]);
}

void FrameBuffer::setTIAPalette(const uInt32 *palette)
{
	for (int i = 0; i < 256; ++i)
	{
		uInt8 r = (palette[i] >> 16) & 0xff;
		uInt8 g = (palette[i] >> 8) & 0xff;
		uInt8 b = palette[i] & 0xff;

		uInt16 color565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

		myDefPalette[i] = (uInt32)color565;
	}

	myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::stateChanged(EventHandler::State state)
{
	// Make sure any onscreen messages are removed
	myMsg.enabled = false;
	myMsg.counter = 0;

	myRedrawEntireFrame = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::changeVidMode(int direction)
{
	EventHandler::State state = myOSystem->eventHandler().state();
	bool inUIMode = (state == EventHandler::S_DEBUGGER ||
						  state == EventHandler::S_LAUNCHER);

	// Ignore any attempts to change video size while in UI mode
	if (inUIMode && direction != 0)
		return false;

	// Only save mode changes in TIA mode with a valid selector
	bool saveModeChange = !inUIMode && (direction == -1 || direction == +1);

	if (direction == +1)
		myCurrentModeList->next();
	else if (direction == -1)
		myCurrentModeList->previous();

	VideoMode vidmode = myCurrentModeList->current(myOSystem->settings(), fullScreen());
	if (setVidMode(vidmode))
	{
		centerAppWindow(vidmode);

		myImageRect.setWidth(vidmode.image_w);
		myImageRect.setHeight(vidmode.image_h);
		myImageRect.moveTo(vidmode.image_x, vidmode.image_y);

		myScreenRect.setWidth(vidmode.screen_w);
		myScreenRect.setHeight(vidmode.screen_h);

		refresh();
	}
	else
		return false;

	return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2) const
{
	if (c2 > c1)
		BSPF_swap(c1, c2);

	return ((c1 - c2) * myPhosphorBlend) / 100 + c2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VariantList &FrameBuffer::supportedTIAFilters(const string &type)
{
	uInt32 max_zoom = maxWindowSizeForScreen(320, 210,
														  myOSystem->desktopWidth(), myOSystem->desktopHeight());
	uInt8 mask = (type == "soft" ? 0x1 : 0x2);

	uInt32 firstmode = 0;

	myTIAFilters.clear();

	return myTIAFilters;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
														 uInt32 screenWidth, uInt32 screenHeight)
{
	uInt32 multiplier = 1;
	for (;;)
	{
		// Figure out the zoomed size of the window
		uInt32 width = baseWidth * multiplier;
		uInt32 height = baseHeight * multiplier;

		if ((width > screenWidth) || (height > screenHeight))
			break;

		++multiplier;
	}
	return multiplier > 1 ? multiplier - 1 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::setAvailableVidModes(uInt32 baseWidth, uInt32 baseHeight)
{
#ifndef __DREAMCAST__
	// Modelists are different depending on what state we're in
	EventHandler::State state = myOSystem->eventHandler().state();

	myWindowedModeList.clear();
	myFullscreenModeList.clear();

	// Scan list of filters, adding only those which are appropriate
	// for the given dimensions
	uInt32 max_zoom = maxWindowSizeForScreen(baseWidth, baseHeight,
														  myOSystem->desktopWidth(), myOSystem->desktopHeight());

	uInt32 firstmode = 0;

	for (uInt32 i = firstmode; i < GFX_NumModes; ++i)
	{
		uInt32 zoom = ourGraphicsModes[i].zoom;
		if (zoom <= max_zoom)
		{
			VideoMode m;
			m.image_x = m.image_y = 0;
			m.image_w = m.screen_w = baseWidth * zoom;
			m.image_h = m.screen_h = baseHeight * zoom;
			m.gfxmode = ourGraphicsModes[i];

			addVidMode(m);
		}
	}
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::addVidMode(VideoMode &mode)
{
#ifndef __DREAMCAST__

	// The are minimum size requirements on a screen, no matter is in fullscreen
	// or windowed mode
	// Various part of the UI system depend on having at least 320x240 pixels
	// available, so we must enforce it here

	// Windowed modes can be sized exactly as required, since there's normally
	// no restriction on window size (between the minimum and maximum size)
	mode.screen_w = BSPF_max<uInt32>(mode.screen_w, 320u);
	mode.screen_h = BSPF_max<uInt32>(mode.screen_h, 240u);
	mode.image_x = (mode.screen_w - mode.image_w) >> 1;
	mode.image_y = (mode.screen_h - mode.image_h) >> 1;
	myWindowedModeList.add(mode);

	// There are often stricter requirements on fullscreen modes, and they're
	// normally different depending on the OSystem in use
	// As well, we usually can't get fullscreen modes in the exact size
	// we want, so we need to calculate image offsets
	const ResolutionList &res = myOSystem->supportedResolutions();
	for (uInt32 i = 0; i < res.size(); ++i)
	{
		if (mode.screen_w <= res[i].width && mode.screen_h <= res[i].height)
		{
			// Auto-calculate 'smart' centering; platform-specific framebuffers are
			// free to ignore or augment it
			mode.screen_w = BSPF_max<uInt32>(res[i].width, 320u);
			mode.screen_h = BSPF_max<uInt32>(res[i].height, 240u);
			mode.image_x = (mode.screen_w - mode.image_w) >> 1;
			mode.image_y = (mode.screen_h - mode.image_h) >> 1;
			break;
		}
	}
	myFullscreenModeList.add(mode);

#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoMode FrameBuffer::getSavedVidMode()
{
	VideoMode mode;

	mode.image_x = 0;
	mode.image_y = 0;

	mode.screen_w = 320;
	mode.screen_h = 240;

	mode.image_w = 320;
	mode.image_h = 240;

	// mode.gfxmode.type = GFX_Zoom1x;
	// mode.gfxmode.name = "zoom1x";
	// mode.gfxmode.description = "";
	// mode.gfxmode.zoom = 1;
	// mode.gfxmode.avail = '\x1';

	return mode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::VideoModeList()
	 : myIdx(-1)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::VideoModeList::~VideoModeList()
{
	clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::add(VideoMode mode)
{
	myModeList.push_back(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::clear()
{
	myModeList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameBuffer::VideoModeList::isEmpty() const
{
	return myModeList.isEmpty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameBuffer::VideoModeList::size() const
{
	return myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::previous()
{
	--myIdx;
	if (myIdx < 0)
		myIdx = myModeList.size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FrameBuffer::VideoMode FrameBuffer::
	 VideoModeList::current(const Settings &settings, bool isFullscreen) const
{
	// Fullscreen modes are related to the 'fullres' setting
	//   If it's 'auto', we just use the mode as already previously defined
	//   If it's not 'auto', attempt to fit the mode into the resolution
	//   specified by 'fullres' (if possible)
	if (isFullscreen && !BSPF_equalsIgnoreCase(settings.getString("fullres"), "auto"))
	{
		// Only use 'fullres' if it's *bigger* than the requested mode
		// const GUI::Size& s = settings.getSize("fullres");

		// if (s.w != -1 && s.h != -1 && (uInt32)s.w >= myModeList[myIdx].screen_w &&
		//	(uInt32)s.h >= myModeList[myIdx].screen_h)
		//{
		VideoMode mode = myModeList[myIdx];
		mode.screen_w = 320; // s.w;
		mode.screen_h = 240; // s.h;
		mode.image_x = 320;	// (mode.screen_w - mode.image_w) >> 1;
		mode.image_y = 240;	// (mode.screen_h - mode.image_h) >> 1;

		return mode;
	}
	//}

	// Otherwise, we just use the mode has it was defined in ::addVidMode()
	return myModeList[myIdx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::next()
{
	myIdx = (myIdx + 1) % myModeList.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByGfxMode(GfxID id)
{
	// First we determine which graphics mode is being requested
	bool found = false;
	GraphicsMode gfxmode;
	for (uInt32 i = 0; i < GFX_NumModes; ++i)
	{
		if (ourGraphicsModes[i].type == id)
		{
			gfxmode = ourGraphicsModes[i];
			found = true;
			break;
		}
	}
	if (!found)
		gfxmode = ourGraphicsModes[0];

	// Now we scan the list for the applicable video mode
	set(gfxmode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::setByGfxMode(const string &name)
{
	// First we determine which graphics mode is being requested
	bool found = false;
	GraphicsMode gfxmode;
	for (uInt32 i = 0; i < GFX_NumModes; ++i)
	{
		if (BSPF_equalsIgnoreCase(ourGraphicsModes[i].name, name) ||
			 BSPF_equalsIgnoreCase(ourGraphicsModes[i].description, name))
		{
			gfxmode = ourGraphicsModes[i];
			found = true;
			break;
		}
	}
	if (!found)
		gfxmode = ourGraphicsModes[0];

	// Now we scan the list for the applicable video mode
	set(gfxmode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameBuffer::VideoModeList::set(const GraphicsMode &gfxmode)
{
	// Attempt to point the current mode to the one given
	myIdx = -1;

	// First search for the given gfx id
	for (unsigned int i = 0; i < myModeList.size(); ++i)
	{
		if (myModeList[i].gfxmode.type == gfxmode.type)
		{
			myIdx = i;
			return;
		}
	}

	// If we get here, then the gfx type couldn't be found, so we search
	// for the first mode with the same zoomlevel (making sure that the
	// requested mode can fit inside the current screen)
	if (gfxmode.zoom > myModeList[myModeList.size() - 1].gfxmode.zoom)
	{
		myIdx = myModeList.size() - 1;
		return;
	}
	else
	{
		for (unsigned int i = 0; i < myModeList.size(); ++i)
		{
			if (myModeList[i].gfxmode.zoom == gfxmode.zoom)
			{
				myIdx = i;
				return;
			}
		}
	}

	// Finally, just pick the lowest video mode
	myIdx = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
						  uInt32 colorA, uInt32 colorB)
{
	hLine(x + 1, y, x + w - 2, colorA);
	hLine(x, y + 1, x + w - 1, colorA);
	vLine(x, y + 1, y + h - 2, colorA);
	vLine(x + 1, y, y + h - 1, colorA);

	hLine(x + 1, y + h - 2, x + w - 1, colorB);
	hLine(x + 1, y + h - 1, x + w - 2, colorB);
	vLine(x + w - 1, y + 1, y + h - 2, colorB);
	vLine(x + w - 2, y + 1, y + h - 1, colorB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
								  uInt32 color, FrameStyle style)
{
	switch (style)
	{
	case kSolidLine:
		hLine(x, y, x + w - 1, color);
		hLine(x, y + h - 1, x + w - 1, color);
		vLine(x, y, y + h - 1, color);
		vLine(x + w - 1, y, y + h - 1, color);
		break;

	case kDashLine:
		unsigned int i, skip, lwidth = 1;

		for (i = x, skip = 1; i < x + w - 1; i = i + lwidth + 1, ++skip)
		{
			if (skip % 2)
			{
				hLine(i, y, i + lwidth, color);
				hLine(i, y + h - 1, i + lwidth, color);
			}
		}
		for (i = y, skip = 1; i < y + h - 1; i = i + lwidth + 1, ++skip)
		{
			if (skip % 2)
			{
				vLine(x, i, i + lwidth, color);
				vLine(x + w - 1, i, i + lwidth, color);
			}
		}
		break;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer::GraphicsMode FrameBuffer::ourGraphicsModes[GFX_NumModes] = {
	 {GFX_Zoom1x, "zoom1x", "Zoom 1x", 1, 0x3},
	 {GFX_Zoom2x, "zoom2x", "Zoom 2x", 2, 0x3},
	 {GFX_Zoom3x, "zoom3x", "Zoom 3x", 3, 0x3},
	 {GFX_Zoom4x, "zoom4x", "Zoom 4x", 4, 0x3},
	 {GFX_Zoom5x, "zoom5x", "Zoom 5x", 5, 0x3},
	 {GFX_Zoom6x, "zoom6x", "Zoom 6x", 6, 0x3},
	 {GFX_Zoom7x, "zoom7x", "Zoom 7x", 7, 0x3},
	 {GFX_Zoom8x, "zoom8x", "Zoom 8x", 8, 0x3},
	 {GFX_Zoom9x, "zoom9x", "Zoom 9x", 9, 0x3},
	 {GFX_Zoom10x, "zoom10x", "Zoom 10x", 10, 0x3}};
