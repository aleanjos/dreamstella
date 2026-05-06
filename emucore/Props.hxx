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
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef PROPERTIES_HXX
#define PROPERTIES_HXX

#include "bspf.hxx"

enum PropertyType {
  Cartridge_MD5,
  Cartridge_Manufacturer,
  Cartridge_ModelNo,
  Cartridge_Name,
  Cartridge_Note,
  Cartridge_Rarity,
  Cartridge_Sound,
  Cartridge_Type,
  Console_LeftDifficulty,
  Console_RightDifficulty,
  Console_TelevisionType,
  Console_SwapPorts,
  Controller_Left,
  Controller_Right,
  Controller_SwapPaddles,
  Controller_MouseAxis,
  Display_Format,
  Display_YStart,
  Display_Height,
  Display_Phosphor,
  Display_PPBlend,
  LastPropType
};

/**
  This class represents objects which maintain a collection of 
  properties.  A property is a key and its corresponding value.

  A properties object can contain a reference to another properties
  object as its "defaults"; this second properties object is searched 
  if the property key is not found in the original property list.

  @author  Bradford W. Mott
  @version $Id$
*/

class Properties
{
public:
   Properties();
   Properties(const Properties& properties);
   virtual ~Properties();
   Properties& operator = (const Properties& properties);

   const string& get(PropertyType key) const;
   void set(PropertyType key, const string& value);
   void set(const string& key, const string& value);

   void setDefaults();
   void copy(const Properties& properties);

   static PropertyType getPropertyType(const string& name);
   static void printHeader();

private:
   string myProperties[LastPropType];
   static const char* ourDefaultProperties[LastPropType];
   static const char* ourPropertyNames[LastPropType];
};

#endif
