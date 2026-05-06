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

#include <cassert>
#include <cstdio>

#include <ctime>
#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#include <arch/timer.h>
#include "M6532.hxx"
#include "SoundDC.hxx"

#include "bspf.hxx"

#include "MediaFactory.hxx"
#include "Sound.hxx"
#include "FSNode.hxx"
#include "MD5.hxx"
#include "Cart.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "Random.hxx"
#include "StateManager.hxx"
#include "Version.hxx"
#include "OptionsMenu.hxx"

#include "OSystem.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem()
	 : myEventHandler(NULL),
		myFrameBuffer(NULL),
		mySound(NULL),
		mySettings(NULL),
		myPropSet(NULL),
		myConsole(NULL),
		myStateManager(NULL),
		myQuitLoop(false),
		myPauseLoop(false),
		myFrameRefresh(false),
		myRomFile(""),
		myRomMD5(""),
		myFeatures(""),
		myBuildInfo(""),
		myOptionsMenu(NULL)
{
	// Calculate startup time
	myMillisAtStart = (uInt32)(time(NULL) * 1000);

	initGuiColors();

	myFeatures += "Sound ";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
	// Remove any game console that is currently attached
	deleteConsole();

	// OSystem takes responsibility for framebuffer and sound,
	// since it created them
	delete myFrameBuffer;
	delete mySound;

	// These must be deleted after all the others
	// This is a bit hacky, since it depends on ordering
	// of d'tor calls

	delete myStateManager;
	delete myPropSet;
	delete myOptionsMenu;
	delete myEventHandler;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::create()
{
	myDesktopWidth = 320;
	myDesktopHeight = 240;

	// Create the event handler for the system
	myEventHandler = new EventHandler(this);
	myEventHandler->initialize();

	// Options menu for Dreamcast
	myOptionsMenu = new OptionsMenu(myEventHandler);

	// Create a properties set for us to use and set it up
	myPropSet = new PropertiesSet(this);

	// Create a StateManager
	myStateManager = new StateManager(this);

	// Create the sound object; the sound subsystem isn't actually
	// opened until needed, so this is non-blocking (on those systems
	// that only have a single sound device (no hardware mixing)
	createSound();

	// Let the random class know about us; it needs access to getTicks()
	Random::setSystem(this);

	return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setFramerate(float framerate)
{
	if (framerate > 0.0)
	{
		myDisplayFrameRate = framerate;
		myTimePerFrame = (uInt32)(1000000.0 / myDisplayFrameRate);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus OSystem::createFrameBuffer()
{
	// There is only ever one FrameBuffer created per run of Stella
	// Due to the multi-surface nature of the FrameBuffer, repeatedly
	// creating and destroying framebuffer objects causes crashes which
	// are far too invasive to fix right now
	// Besides, how often does one really switch between software and
	// OpenGL rendering modes, and even when they do, does it really
	// need to be dynamic?

	bool firstTime = (myFrameBuffer == NULL);
	if (firstTime)
		myFrameBuffer = MediaFactory::createVideo(this);

	myFrameDc = (FrameBufferSoftDC *)myFrameBuffer;

	// Re-initialize the framebuffer to current settings
	FBInitStatus fbstatus = kFailComplete;

	if (myEventHandler->state() || myEventHandler->state() != NULL)
		fbstatus = myConsole->initializeVideo();

	return fbstatus;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createSound()
{
	if (!mySound)
		mySound = MediaFactory::createAudio(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::createConsole(const FilesystemNode &rom, const string &md5sum, bool newrom)
{
	// Do a little error checking; it shouldn't be necessary
	if (myConsole)
		deleteConsole();

	bool showmessage = false;

	// If same ROM has been given, we reload the current one (assuming one exists)
	if (!newrom && rom == myRomFile)
	{
		showmessage = true; // we show a message if a ROM is being reloaded
	}
	else
	{
		myRomFile = rom;
		myRomMD5 = md5sum;

		// Each time a new console is loaded, we simulate a cart removal
		// Some carts need knowledge of this, as they behave differently
		// based on how many power-cycles they've been through since plugged in
		// mySettings->setValue("romloadcount", 0);
	}

	// Create an instance of the 2600 game console
	string type, id;

	myConsole = openConsole(myRomFile, myRomMD5, type, id);

	if (myConsole)
	{
		myConsole->initializeAudio();

		myEventHandler->reset(EventHandler::S_EMULATE);

		if (createFrameBuffer() != kSuccess)
		{
			return "ERROR: Couldn't create framebuffer for console";
		}

		// Update the timing info for a new console run
		resetLoopTiming();
	}
	else
	{
		myConsole = 0;
		return "ERROR: Console open failed";
	}

	return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::deleteConsole()
{
	if (myConsole)
	{
		mySound->close();

		delete myConsole;
		myConsole = NULL;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::reloadConsole()
{
	deleteConsole();
	return createConsole(myRomFile, myRomMD5, false) == EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const FilesystemNode &romfile)
{
	string md5, type, id, result = "";
	Console *console = 0;

	console = openConsole(romfile, md5, type, id);

	if (!console || console == NULL)
	{
		return "";
	}

	result = getROMInfo(console);
	delete console;
	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console *OSystem::openConsole(const FilesystemNode &romfile, string &md5,
										string &type, string &id)
{
#define CMDLINE_PROPS_UPDATE(cl_name, prop_name) \
	s = mySettings->getString(cl_name);           \
	if (s != "")                                  \
		props.set(prop_name, s);

	Console *console = nullptr;

	// Open the cartridge image and read it in
	uInt8 *image = 0;
	uInt32 size = 0;
	if ((image = openROM(romfile, md5, size)) != 0)
	{
		// Get a valid set of properties, including any entered on the commandline
		// For initial creation of the Cart, we're only concerned with the BS type
		Properties props;
		myPropSet->getMD5(md5, props);
		string s = "";
		CMDLINE_PROPS_UPDATE("bs", Cartridge_Type);
		CMDLINE_PROPS_UPDATE("type", Cartridge_Type);

		// Now create the cartridge
		string cartmd5 = md5;
		type = props.get(Cartridge_Type);
		Cartridge *cart =
			 Cartridge::create(image, size, cartmd5, type, id, *this, *mySettings);

		// It's possible that the cart created was from a piece of the image,
		// and that the md5 (and hence the cart) has changed
		if (props.get(Cartridge_MD5) != cartmd5)
		{
			if (!myPropSet->getMD5(cartmd5, props))
			{
				// Cart md5 wasn't found, so we create a new props for it
				props.set(Cartridge_MD5, cartmd5);
				props.set(Cartridge_Name, props.get(Cartridge_Name) + id);
				myPropSet->insert(props, false);
			}
		}

		CMDLINE_PROPS_UPDATE("channels", Cartridge_Sound);
		CMDLINE_PROPS_UPDATE("ld", Console_LeftDifficulty);
		CMDLINE_PROPS_UPDATE("rd", Console_RightDifficulty);
		CMDLINE_PROPS_UPDATE("tv", Console_TelevisionType);
		CMDLINE_PROPS_UPDATE("sp", Console_SwapPorts);
		CMDLINE_PROPS_UPDATE("lc", Controller_Left);
		CMDLINE_PROPS_UPDATE("rc", Controller_Right);
		s = mySettings->getString("bc");
		if (s != "")
		{
			props.set(Controller_Left, s);
			props.set(Controller_Right, s);
		}
		CMDLINE_PROPS_UPDATE("cp", Controller_SwapPaddles);
		CMDLINE_PROPS_UPDATE("format", Display_Format);
		CMDLINE_PROPS_UPDATE("ystart", Display_YStart);
		CMDLINE_PROPS_UPDATE("height", Display_Height);

		// Finally, create the cart with the correct properties
		if (cart)
			console = new Console(this, cart, props);
	}

	// Free the image since we don't need it any longer
	if (image != 0 && size > 0)
		delete[] image;

	return console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 *OSystem::openROM(const FilesystemNode &rom, string &md5, uInt32 &size)
{
	// This method has a documented side-effect:
	// It not only loads a ROM and creates an array with its contents,
	// but also adds a properties entry if the one for the ROM doesn't
	// contain a valid name

	uInt8 *image = 0;
	if ((size = rom.read(image)) == 0)
	{
		delete[] image;
		return (uInt8 *)0;
	}

	// If we get to this point, we know we have a valid file to open
	// Now we make sure that the file has a valid properties entry
	// To save time, only generate an MD5 if we really need one
	if (md5 == "")
		md5 = MD5(image, size);

	// Some games may not have a name, since there may not
	// be an entry in stella.pro.  In that case, we use the rom name
	// and reinsert the properties object
	Properties props;
	myPropSet->getMD5WithInsert(rom, md5, props);

	return image;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const Console *console)
{
	const ConsoleInfo &info = console->about();
	char outBuf[512];

	snprintf(outBuf, sizeof(outBuf),
				"  Cart Name:       %s\n"
				"  Cart MD5:        %s\n"
				"  Controller 0:    %s\n"
				"  Controller 1:    %s\n"
				"  Display Format:  %s\n"
				"  Bankswitch Type: %s",
				info.CartName.c_str(), info.CartMD5.c_str(),
				info.Control0.c_str(), info.Control1.c_str(),
				info.DisplayFormat.c_str(), info.BankSwitch.c_str());

	return string(outBuf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::resetLoopTiming()
{
	myTimingInfo.start = myTimingInfo.virt = getTicks();
	myTimingInfo.current = 0;
	myTimingInfo.totalTime = 0;
	myTimingInfo.totalFrames = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::validatePath(string &path, const string &setting,
									const string &defaultpath)
{
	const string &s = mySettings->getString(setting) == "" ? defaultpath : mySettings->getString(setting);
	FilesystemNode node(s);
	if (!node.isDirectory())
		node.makeDir();

	path = node.getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoymap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_AXIS(sda_event, sda_mode, sda_stick, sda_axis, sda_val, sda_cmp_event) \
	if (eraseAll || sda_cmp_event == sda_event)                                             \
		myEventHandler->addJoyAxisMapping(sda_event, sda_mode, sda_stick, sda_axis, sda_val, false);

#define SET_DEFAULT_BTN(sdb_event, sdb_mode, sdb_stick, sdb_button, sdb_cmp_event) \
	if (eraseAll || sdb_cmp_event == sdb_event)                                     \
		myEventHandler->addJoyButtonMapping(sdb_event, sdb_mode, sdb_stick, sdb_button, false);

#define SET_DEFAULT_HAT(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, sdh_cmp_event) \
	if (eraseAll || sdh_cmp_event == sdh_event)                                           \
		myEventHandler->addJoyHatMapping(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, false);

	bool eraseAll = (event == Event::NoType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 OSystem::getTicks() const
{
	return timer_us_gettime64();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::mainLoop()
{
	myPauseLoop = false;
	myQuitLoop = false;

	resetLoopTiming();

	for (;;)
	{
		if (myPauseLoop)
		{
			mySound->mute(true);
			bool resume = myOptionsMenu->showOptionsMenu();

			if (resume)
			{
				thd_sleep(100);
				myPauseLoop = false;
				mySound->mute(false);
				resetLoopTiming();
			}
			else 
			{
				myPauseLoop = true;
			}
		}

		if (myQuitLoop)
			break;

		if (!myPauseLoop)
			runFrame();
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::runFrame()
{
	myConsole->tia().update();
	myConsole->riot().update();
	myTimingInfo.start = getTicks();
	myEventHandler->poll(myTimingInfo.start);
	SoundDC::poll();
	myFrameDc->drawTIA(false);
}

uInt32 *OSystem::ourGUIColors = NULL;

void OSystem::initGuiColors()
{
	if (ourGUIColors != NULL)
		return;

	int numItems = 256;

	ourGUIColors = (uInt32 *)calloc(kNumUIPalettes * numItems, sizeof(uInt32));

	auto setColor = [&](int palette, int index, uInt32 color)
	{
		ourGUIColors[palette * numItems + index] = color;
	};

	uInt32 std[] = {0x686868, 0x000000, 0x404040, 0x000000, 0x62a108, 0x9f0000,
						 0xc9af7c, 0xf0f0cf, 0xc80000, 0xac3410, 0xd55941, 0xffffff,
						 0xffd652, 0xac3410, 0xac3410, 0xd55941, 0xac3410, 0xd55941,
						 0xc80000, 0x00ff00, 0xc8c8ff};

	for (int i = 0; i < 21; i++)
		setColor(0, i, std[i]);

	uInt32 cls[] = {0x686868, 0x000000, 0x404040, 0x20a020, 0x00ff00, 0xc80000,
						 0x000000, 0x000000, 0xc80000, 0x000000, 0x000000, 0x20a020,
						 0x00ff00, 0x20a020, 0x20a020, 0x00ff00, 0x20a020, 0x00ff00,
						 0xc80000, 0x00ff00, 0xc8c8ff};

	for (int i = 0; i < 21; i++)
		setColor(1, i, cls[i]);
}

OSystem::OSystem(const OSystem &osystem)
{
}

OSystem &OSystem::operator=(const OSystem &)
{
	assert(false);
	return *this;
}
