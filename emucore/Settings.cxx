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
#include <algorithm>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Version.hxx"

#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(OSystem *osystem)
	 : myOSystem(osystem)
{
	// Add this settings object to the OSystem
	myOSystem->attach(this);

	loadDefaultSettings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
	myInternalSettings.clear();
	myExternalSettings.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Settings::loadCommandLine(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		// strip off the '-' character
		string key = argv[i];
		if (key[0] == '-')
		{
			key = key.substr(1, key.length());

			// Take care of the arguments which are meant to be executed immediately
			// (and then Stella should exit)
			if (key == "help" || key == "listrominfo")
			{
				setExternal(key, "true");
				return "";
			}

			// Take care of arguments without an option or ones that shouldn't
			// be saved to the config file
			if (key == "rominfo" || key == "debug" || key == "holdreset" ||
				 key == "holdselect" || key == "takesnapshot")
			{
				setExternal(key, "true");
				continue;
			}

			char buf[64];
			if (++i >= argc)
			{
				snprintf(buf, sizeof(buf), "Missing argument for %s", key);
				printf(buf);
				fflush(stdout);
				;
				return "";
			}

			string value = argv[i];

			snprintf(buf, sizeof(buf), "key = %s , value = %s", key, value);

			// Settings read from the commandline must not be saved to
			// the rc-file, unless they were previously set
			if (int idx = getInternalPos(key) != -1)
			{
				setInternal(key, value, idx); // don't set initialValue here
				snprintf(buf, sizeof(buf), "(I)\n");
			}
			else
			{
				setExternal(key, value);
				snprintf(buf, sizeof(buf), "(E)\n");
			}

			printf(buf);
			fflush(stdout);
		}
		else
			return key;
	}

	return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::validate()
{
	string s;
	int i;

	s = getString("video");
	if (s != "soft" && s != "gl")
		setInternal("video", "soft");

	s = getString("timing");
	if (s != "sleep" && s != "busy")
		setInternal("timing", "sleep");

#ifdef SOUND_SUPPORT
	i = getInt("volume");
	if (i < 0 || i > 100)
		setInternal("volume", "100");
	i = getInt("freq");
	if (!(i == 11025 || i == 22050 || i == 31400 || i == 44100 || i == 48000))
		setInternal("freq", "31400");
#endif

	i = getInt("joydeadzone");
	if (i < 0)
		setInternal("joydeadzone", "0");
	else if (i > 29)
		setInternal("joydeadzone", "29");

	if (i < 1)
		setInternal("dsense", "1");
	else if (i > 10)
		setInternal("dsense", "10");

	i = getInt("dsense");
	if (i < 1)
		setInternal("dsense", "1");
	else if (i > 10)
		setInternal("dsense", "10");

	i = getInt("msense");
	if (i < 1)
		setInternal("msense", "1");
	else if (i > 15)
		setInternal("msense", "15");

	i = getInt("ssinterval");
	if (i < 1)
		setInternal("ssinterval", "2");
	else if (i > 10)
		setInternal("ssinterval", "10");

	s = getString("palette");
	if (s != "standard" && s != "z26" && s != "user")
		setInternal("palette", "standard");

	s = getString("launcherfont");
	if (s != "small" && s != "medium" && s != "large")
		setInternal("launcherfont", "medium");

	i = getInt("romviewer");
	if (i < 0)
		setInternal("romviewer", "0");
	else if (i > 2)
		setInternal("romviewer", "2");

	i = getInt("loglevel");
	if (i < 0 || i > 2)
		setInternal("loglevel", "1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Variant &Settings::value(const string &key) const
{
	// Try to find the named setting and answer its value
	int idx = -1;
	if ((idx = getInternalPos(key)) != -1)
		return myInternalSettings[idx].value;
	else if ((idx = getExternalPos(key)) != -1)
		return myExternalSettings[idx].value;
	else
		return EmptyVariant;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setValue(const string &key, const Variant &value)
{
	if (int idx = getInternalPos(key) != -1)
		setInternal(key, value, idx);
	else
		setExternal(key, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInternalPos(const string &key) const
{
	for (unsigned int i = 0; i < myInternalSettings.size(); ++i)
		if (myInternalSettings[i].key == key)
			return i;

	return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getExternalPos(const string &key) const
{
	for (unsigned int i = 0; i < myExternalSettings.size(); ++i)
		if (myExternalSettings[i].key == key)
			return i;

	return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setInternal(const string &key, const Variant &value,
								  int pos, bool useAsInitial)
{
	int idx = -1;

	if (pos != -1 && pos >= 0 && pos < (int)myInternalSettings.size() &&
		 myInternalSettings[pos].key == key)
	{
		idx = pos;
	}
	else
	{
		for (unsigned int i = 0; i < myInternalSettings.size(); ++i)
		{
			if (myInternalSettings[i].key == key)
			{
				idx = i;
				break;
			}
		}
	}

	if (idx != -1)
	{
		myInternalSettings[idx].key = key;
		myInternalSettings[idx].value = value;
		if (useAsInitial)
			myInternalSettings[idx].initialValue = value;
	}
	else
	{
		Setting setting;
		setting.key = key;
		setting.value = value;
		if (useAsInitial)
			setting.initialValue = value;

		myInternalSettings.push_back(setting);
		idx = myInternalSettings.size() - 1;
	}

	return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setExternal(const string &key, const Variant &value,
								  int pos, bool useAsInitial)
{
	int idx = -1;

	if (pos != -1 && pos >= 0 && pos < (int)myExternalSettings.size() &&
		 myExternalSettings[pos].key == key)
	{
		idx = pos;
	}
	else
	{
		for (unsigned int i = 0; i < myExternalSettings.size(); ++i)
		{
			if (myExternalSettings[i].key == key)
			{
				idx = i;
				break;
			}
		}
	}

	if (idx != -1)
	{
		myExternalSettings[idx].key = key;
		myExternalSettings[idx].value = value;
		if (useAsInitial)
			myExternalSettings[idx].initialValue = value;
	}
	else
	{
		Setting setting;
		setting.key = key;
		setting.value = value;
		if (useAsInitial)
			setting.initialValue = value;

		myExternalSettings.push_back(setting);
		idx = myExternalSettings.size() - 1;
	}

	return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(const Settings &)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings &Settings::operator=(const Settings &)
{
	assert(false);
	return *this;
}

void Settings::loadDefaultSettings()
{
	setInternal("video", "soft");
	setInternal("fullscreen", "1");
	setInternal("tia_filter", "zoom1x");
	setInternal("center", "true");
	setInternal("grabmouse", "true");

	setInternal("palette", "standard");
	setInternal("colorloss", "false");

	setInternal("framerate", "0");

	setInternal("timing", "sleep");

	setInternal("sound", "true");
	setInternal("fragsize", "512");
	setInternal("freq", "31400");
	setInternal("volume", "100");

	setInternal("joydeadzone", "13");
	setInternal("usemouse", "analog");
	setInternal("dsense", "5");
	setInternal("msense", "7");
	setInternal("saport", "lr");
	setInternal("ctrlcombo", "true");

	setInternal("cpurandom", "true");
	setInternal("ramrandom", "true");

	setInternal("tiadriven", "false");
	setInternal("fastscbios", "false");
	setInternal("uimessages", "false");
}
