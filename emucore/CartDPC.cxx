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
#include <cstring>

#include "System.hxx"
#include "CartDPC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPC::CartridgeDPC(const uInt8 *image, uInt32 size,
									const Settings &settings)
	 : Cartridge(settings),
		mySize(size),
		mySystemCycles(0),
		myFractionalClocks(0)
{
	// Make a copy of the entire image
	memcpy(myImage, image, BSPF_min<uInt32>(size, 8192u + 2048u + 256u));
	createCodeAccessBase(8192);

	// Pointer to the program ROM (8K @ 0 byte offset)
	myProgramImage = myImage;

	// Pointer to the display ROM (2K @ 8K offset)
	myDisplayImage = myProgramImage + 8192;

	// Initialize the DPC data fetcher registers
	for (int i = 0; i < 8; ++i)
		myTops[i] = myBottoms[i] = myCounters[i] = myFlags[i] = 0;

	// None of the data fetchers are in music mode
	myMusicMode[0] = myMusicMode[1] = myMusicMode[2] = false;

	// Initialize the DPC's random number generator register (must be non-zero)
	myRandomNumber = 1;

	// Remember startup bank
	myStartBank = 1;

	for (int i = 0; i < 5; ++i)
		myDecrementEnable[i] = true;
	for (int i = 5; i < 8; ++i)
		myDecrementEnable[i] = !myMusicMode[i - 5];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPC::~CartridgeDPC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::reset()
{
	// Update cycles to the current system cycles
	mySystemCycles = mySystem->cycles();
	myFractionalClocks = 0;

	// Upon reset we switch to the startup bank
	bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::systemCyclesReset()
{
	// Get the current system cycle
	uInt32 cycles = mySystem->cycles();

	// Adjust the cycle counter so that it reflects the new value
	mySystemCycles -= cycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPC::install(System &system)
{
	mySystem = &system;
	uInt16 shift = mySystem->pageShift();
	uInt16 mask = mySystem->pageMask();

	// Make sure the system we're being installed in has a page size that'll work
	assert(((0x1080 & mask) == 0) && ((0x1100 & mask) == 0));

	System::PageAccess access(0, 0, 0, this, System::PA_READWRITE);

	// Set the page accessing method for the DPC reading & writing pages
	for (uInt32 j = 0x1000; j < 0x1080; j += (1 << shift))
		mySystem->setPageAccess(j >> shift, access);

	// Install pages for the startup bank
	bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void CartridgeDPC::updateMusicModeDataFetchers()
{
	uInt32 currentCycles = mySystem->cycles();
	Int32 cycles = currentCycles - mySystemCycles;
	mySystemCycles = currentCycles;

	uInt32 totalClocks = (2400 * cycles) + myFractionalClocks;

	if (totalClocks < 143183)
	{
		myFractionalClocks = totalClocks;
		return;
	}

	Int32 wholeClocks = totalClocks / 143183;
	myFractionalClocks = totalClocks % 143183;

	if (wholeClocks <= 0)
	{
		return;
	}

	for (int x = 5; x <= 7; ++x)
	{
		if (myMusicMode[x - 5])
		{
			Int32 top = myTops[x] + 1;
			Int32 newLow = (Int32)(myCounters[x] & 0x00ff);

			if (myTops[x] != 0)
			{
				newLow -= (wholeClocks % top);
				if (newLow < 0)
				{
					newLow += top;
				}
			}
			else
			{
				newLow = 0;
			}

			if (newLow <= myBottoms[x])
			{
				myFlags[x] = 0x00;
			}
			else if (newLow <= myTops[x])
			{
				myFlags[x] = 0xff;
			}

			myCounters[x] = (myCounters[x] & 0x0700) | (uInt16)newLow;
		}
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDPC::peek(uInt16 address)
{
	address &= 0x0FFF;

	if (bankLocked())
		return myProgramImage[(myCurrentBank << 12) + address];

	clockRandomNumberGenerator();

	if (__builtin_expect((address >= 0x0040), 1))
	{
		if (__builtin_expect((address >= 0x0FF8), 0))
		{
			if (address == 0x0FF8)
				bank(0);
			else if (address == 0x0FF9)
				bank(1);
		}
		return myProgramImage[(myCurrentBank << 12) + address];
	}

	uInt8 result = 0;
	uInt32 index = address & 0x07;
	uInt32 function = (address >> 3) & 0x07;

	if ((myCounters[index] & 0x00ff) == myTops[index])
	{
		myFlags[index] = 0xff;
	}
	else if ((myCounters[index] & 0x00ff) == myBottoms[index])
	{
		myFlags[index] = 0x00;
	}

	switch (function)
	{
	case 0x00:
	{
		if (index < 4)
		{
			result = myRandomNumber;
		}
		else
		{
			static const uInt8 musicAmplitudes[8] = {
				 0x00, 0x04, 0x05, 0x09, 0x06, 0x0a, 0x0b, 0x0f};

			updateMusicModeDataFetchers();

			uInt8 i = 0;
			if (myMusicMode[0] && myFlags[5])
				i |= 0x01;
			if (myMusicMode[1] && myFlags[6])
				i |= 0x02;
			if (myMusicMode[2] && myFlags[7])
				i |= 0x04;

			result = musicAmplitudes[i];
		}
		break;
	}
	case 0x01:
	{
		result = myDisplayImage[myCounters[index] ^ 0x07FF];
		break;
	}
	case 0x02:
	{
		result = myDisplayImage[myCounters[index] ^ 0x07FF] & myFlags[index];
		break;
	}
	case 0x07:
	{
		result = myFlags[index];
		break;
	}
	default:
	{
		result = 0;
	}
	}

	if (myDecrementEnable[index])
	{
		myCounters[index] = (myCounters[index] - 1) & 0x07ff;
	}

	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPC::poke(uInt16 address, uInt8 value)
{
	address &= 0x0FFF;

	clockRandomNumberGenerator();

	if (__builtin_expect(((address >= 0x0040) && (address < 0x0080)), 0))
	{
		uInt32 index = address & 0x07;
		uInt32 function = (address >> 3) & 0x07;

		switch (function)
		{
		case 0x00:
		{
			myTops[index] = value;
			myFlags[index] = 0x00;
			break;
		}
		case 0x01:
		{
			myBottoms[index] = value;
			break;
		}
		case 0x02:
		{
			if ((index >= 5) && myMusicMode[index - 5])
			{
				myCounters[index] = (myCounters[index] & 0x0700) | (uInt16)myTops[index];
			}
			else
			{
				myCounters[index] = (myCounters[index] & 0x0700) | (uInt16)value;
			}
			break;
		}
		case 0x03:
		{
			myCounters[index] = (((uInt16)value & 0x07) << 8) | (myCounters[index] & 0x00ff);

			if (index >= 5)
			{
				bool isMusicMode = (value & 0x10);
				myMusicMode[index - 5] = isMusicMode;
				myDecrementEnable[index] = !isMusicMode;
			}
			break;
		}
		case 0x06:
		{
			myRandomNumber = 1;
			break;
		}
		default:
		{
			break;
		}
		}
		return false;
	}

	if (__builtin_expect((address >= 0x0FF8), 0))
	{
		if (address == 0x0FF8)
			bank(0);
		else if (address == 0x0FF9)
			bank(1);
	}

	return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPC::bank(uInt16 bank)
{
	if (bankLocked())
		return false;

	// Remember what bank we're in
	myCurrentBank = bank;
	uInt16 offset = myCurrentBank << 12;
	uInt16 shift = mySystem->pageShift();
	uInt16 mask = mySystem->pageMask();

	System::PageAccess access(0, 0, 0, this, System::PA_READ);

	// Set the page accessing methods for the hot spots
	for (uInt32 i = (0x1FF8 & ~mask); i < 0x2000; i += (1 << shift))
	{
		access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
		mySystem->setPageAccess(i >> shift, access);
	}

	// Setup the page access methods for the current bank
	for (uInt32 address = 0x1080; address < (0x1FF8U & ~mask);
		  address += (1 << shift))
	{
		access.directPeekBase = &myProgramImage[offset + (address & 0x0FFF)];
		access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
		mySystem->setPageAccess(address >> shift, access);
	}
	return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDPC::bank() const
{
	return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeDPC::bankCount() const
{
	return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPC::patch(uInt16 address, uInt8 value)
{
	address &= 0x0FFF;

	// For now, we ignore attempts to patch the DPC address space
	if (address >= 0x0080)
	{
		myProgramImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
		return myBankChanged = true;
	}
	else
		return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8 *CartridgeDPC::getImage(int &size) const
{
	size = mySize;
	return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPC::save(Serializer &out) const
{
	out.putString(name());

	// Indicates which bank is currently active
	out.putShort(myCurrentBank);

	// The top registers for the data fetchers
	out.putByteArray(myTops, 8);

	// The bottom registers for the data fetchers
	out.putByteArray(myBottoms, 8);

	// The counter registers for the data fetchers
	out.putShortArray(myCounters, 8);

	// The flag registers for the data fetchers
	out.putByteArray(myFlags, 8);

	// The music mode flags for the data fetchers
	for (int i = 0; i < 3; ++i)
		out.putBool(myMusicMode[i]);

	// The random number generator register
	out.putByte(myRandomNumber);

	out.putInt(mySystemCycles);
	out.putInt(myFractionalClocks);

	return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeDPC::load(Serializer &in)
{
	if (in.getString() != name())
		return false;

	// Indicates which bank is currently active
	myCurrentBank = in.getShort();

	// The top registers for the data fetchers
	in.getByteArray(myTops, 8);

	// The bottom registers for the data fetchers
	in.getByteArray(myBottoms, 8);

	// The counter registers for the data fetchers
	in.getShortArray(myCounters, 8);

	// The flag registers for the data fetchers
	in.getByteArray(myFlags, 8);

	// The music mode flags for the data fetchers
	for (int i = 0; i < 3; ++i)
		myMusicMode[i] = in.getBool();

	// The random number generator register
	myRandomNumber = in.getByte();

	// Get system cycles and fractional clocks
	mySystemCycles = (Int32)in.getInt();
	myFractionalClocks = in.getInt();

	// Now, go to the current bank
	bank(myCurrentBank);

	return true;
}
