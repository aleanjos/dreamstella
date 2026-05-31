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

#include <cctype>
#include <algorithm>
#include <cstdio>

#include "bspf.hxx"
#include "Props.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties()
{
  setDefaults();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties(const Properties& properties)
{
  copy(properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::~Properties()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::set(PropertyType key, const string& value)
{
   if (key >= 0 && key < LastPropType)
   {
      myProperties[key] = value;
   }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Properties::get(PropertyType key) const
{
  if(key >= 0 && key < LastPropType)
    return myProperties[key];
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::set(const string& key, const string& value)
{
   PropertyType type = getPropertyType(key);
   if (type >= 0 && type < LastPropType)
   {
      set(key, value);
   }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties& Properties::operator = (const Properties& properties)
{
  // Do the assignment only if this isn't a self assignment
  if(this != &properties)
  {
    // Now, make myself a copy of the given object
    copy(properties);
  }

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::copy(const Properties& properties)
{
  // Now, copy each property from properties
  for(int i = 0; i < LastPropType; ++i)
    myProperties[i] = properties.myProperties[i];
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::setDefaults()
{
  for(int i = 0; i < LastPropType; ++i)
    myProperties[i] = ourDefaultProperties[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertyType Properties::getPropertyType(const string& name)
{
  for(int i = 0; i < LastPropType; ++i)
    if(ourPropertyNames[i] == name)
      return (PropertyType)i;

  // Otherwise, indicate that the item wasn't found
  return LastPropType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::printHeader()
{
   printf("Cartridge_MD5 | Cartridge_Manufacturer | Cartridge_ModelNo | Cartridge_Name | "
      "Cartridge_Note | Cartridge_Rarity | Cartridge_Sound | Cartridge_Type | "
      "Console_LeftDifficulty | Console_RightDifficulty | Console_TelevisionType | "
      "Console_SwapPorts | Controller_Left | Controller_Right | Controller_SwapPaddles | "
      "Controller_MouseAxis | Display_Format | Display_YStart | Display_Height | "
      "Display_Phosphor | Display_PPBlend\n");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* Properties::ourDefaultProperties[LastPropType] = {
  "",          // Cartridge.MD5
  "",          // Cartridge.Manufacturer
  "",          // Cartridge.ModelNo
  "Untitled",  // Cartridge.Name
  "",          // Cartridge.Note
  "",          // Cartridge.Rarity
  "MONO",      // Cartridge.Sound
  "AUTO",      // Cartridge.Type
  "B",         // Console.LeftDifficulty
  "B",         // Console.RightDifficulty
  "COLOR",     // Console.TelevisionType  
  "NO",        // Console.SwapPorts
  "JOYSTICK",  // Controller.Left
  "JOYSTICK",  // Controller.Right
  "NO",        // Controller.SwapPaddles
  "AUTO",      // Controller.MouseAxis
  "AUTO",      // Display.Format
  "34",        // Display.YStart
  "210",       // Display.Height
  "NO",        // Display.Phosphor
  "77"         // Display.PPBlend
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* Properties::ourPropertyNames[LastPropType] = {
  "Cartridge.MD5",
  "Cartridge.Manufacturer",
  "Cartridge.ModelNo",
  "Cartridge.Name",
  "Cartridge.Note",
  "Cartridge.Rarity",
  "Cartridge.Sound",
  "Cartridge.Type",
  "Console.LeftDifficulty",
  "Console.RightDifficulty",
  "Console.TelevisionType",
  "Console.SwapPorts",
  "Controller.Left",
  "Controller.Right",
  "Controller.SwapPaddles",
  "Controller.MouseAxis",
  "Display.Format",
  "Display.YStart",
  "Display.Height",
  "Display.Phosphor",
  "Display.PPBlend"
};
