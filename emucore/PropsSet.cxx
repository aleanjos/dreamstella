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
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>

#include "bspf.hxx"
#include "FSNode.hxx"

#include "DefProps.hxx"
#include "OSystem.hxx"
#include "Props.hxx"
#include "Settings.hxx"

#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet(OSystem *osystem)
	 : myOSystem(osystem)
{
	load("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
	myExternalProps.clear();
	myTempProps.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void stripQuotes(char *str)
{
	if (!str)
		return;
	size_t len = strlen(str);
	if (len > 1 && str[0] == '"' && str[len - 1] == '"')
	{
		str[len - 1] = '\0';
		memmove(str, str + 1, len - 1);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::load(const string &filename)
{
	FILE *in = fopen(filename.c_str(), "r");

	if (!in)
	{
		return;
	}

	char buffer[512];

	while (fgets(buffer, sizeof(buffer), in))
	{
		char *newline = strchr(buffer, '\n');
		if (newline)
			*newline = '\0';
		newline = strchr(buffer, '\r');
		if (newline)
			*newline = '\0';

		if (strlen(buffer) == 0 || buffer[0] == ';')
			continue;

		char *token = strtok(buffer, " \t");

		if (token)
		{
			stripQuotes(token);
			string md5 = token;

			if (md5.length() > 0)
			{
				Properties properties;

				while (true)
				{
					char *keyStr = strtok(NULL, " \t");
					if (!keyStr)
						break;

					char *valStr = strtok(NULL, " \t");
					if (!valStr)
						break;

					stripQuotes(keyStr);
					stripQuotes(valStr);

					properties.set(keyStr, valStr);
				}

				myExternalProps[md5] = properties;
			}
		}
	}

	fclose(in);
}

bool PropertiesSet::getMD5(const string &md5, Properties &properties,
									bool useDefaults) const
{
	properties.setDefaults();
	bool found = false;

	char path[128];
	sprintf(path, "/cd/properties/%s.pro", md5.c_str());

	FILE *f = fopen(path, "r");
	if (f)
	{
		found = true;

		char line[256];
		while (fgets(line, sizeof(line), f))
		{
			if (strlen(line) < 5)
				continue;

			char *keyStart = strchr(line, '\"');
			if (!keyStart)
				continue;

			char *keyEnd = strchr(keyStart + 1, '\"');
			if (!keyEnd)
				continue;

			char *valStart = strchr(keyEnd + 1, '\"');
			if (!valStart)
				continue;

			char *valEnd = strchr(valStart + 1, '\"');
			if (!valEnd)
				continue;

			*keyEnd = '\0';
			*valEnd = '\0';

			const char *key = keyStart + 1;
			const char *val = valStart + 1;

			properties.set(key, val);
		}
		fclose(f);
	}

	// There are three lists to search when looking for a properties entry,
	// which must be done in the following order
	// If 'useDefaults' is specified, only use the built-in list
	//
	//  'save': entries previously inserted that are saved on program exit
	//  'temp': entries previously inserted that are discarded
	//  'builtin': the defaults compiled into the program

	// First check properties from external file
	if (!useDefaults)
	{
		// Check external list
		PropsList::const_iterator iter = myExternalProps.find(md5);
		if (iter != myExternalProps.end())
		{
			properties = iter->second;
			found = true;
		}
		else // Search temp list
		{
			iter = myTempProps.find(md5);
			if (iter != myTempProps.end())
			{
				properties = iter->second;
				found = true;
			}
		}
	}

	// Otherwise, search the internal database using binary search
	if (!found)
	{
		int low = 0, high = DEF_PROPS_SIZE - 1;
		while (low <= high)
		{
			int i = (low + high) / 2;
			int cmp = BSPF_compareIgnoreCase(md5, DefProps[i][Cartridge_MD5]);

			if (cmp == 0) // found it
			{
				for (int p = 0; p < LastPropType; ++p)
					if (DefProps[i][p][0] != 0)
						properties.set((PropertyType)p, DefProps[i][p]);

				found = true;
				break;
			}
			else if (cmp < 0)
				high = i - 1; // look at lower range
			else
				low = i + 1; // look at upper range
		}
	}

	return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::getMD5WithInsert(const FilesystemNode &rom,
												 const string &md5, Properties &properties)
{
	if (!getMD5(md5, properties))
	{
		properties.set(Cartridge_MD5, md5);
		// Create a name suitable for using in properties
		properties.set(Cartridge_Name, rom.getNameWithExt(""));

		insert(properties, false);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties &properties, bool save)
{

	// Note that the following code is optimized for insertion when an item
	// doesn't already exist, and when the external properties file is
	// relatively small (which is the case with current versions of Stella,
	// as the properties are built-in)
	// If an item does exist, it will be removed and insertion done again
	// This shouldn't be a speed issue, as insertions will only fail with
	// duplicates when you're changing the current ROM properties, which
	// most people tend not to do

	// Since the PropSet is keyed by md5, we can't insert without a valid one
	const string &md5 = properties.get(Cartridge_MD5);
	if (md5 == "")
		return;

	// The status of 'save' determines which list to save to
	PropsList &list = save ? myExternalProps : myTempProps;

	pair<PropsList::iterator, bool> ret;
	ret = list.insert(make_pair(md5, properties));
	if (ret.second == false)
	{
		// Remove old item and insert again
		list.erase(ret.first);
		list.insert(make_pair(md5, properties));
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::removeMD5(const string &md5)
{
	// We only remove from the external list
	myExternalProps.erase(md5);
}
