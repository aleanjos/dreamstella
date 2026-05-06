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

#ifndef SERIALIZER_HXX
#define SERIALIZER_HXX

#include <stdio.h>
#include "bspf.hxx"

class Serializer {
public:
   Serializer(const string& filename, bool readonly = false);
   Serializer();
   virtual ~Serializer();

   bool isValid() const;
   void reset();

   uInt8 getByte();
   void getByteArray(uInt8* array, uInt32 size);
   uInt16 getShort();
   void getShortArray(uInt16* array, uInt32 size);
   uInt32 getInt();
   void getIntArray(uInt32* array, uInt32 size);
   string getString();
   bool getBool();

   void putByte(uInt8 value);
   void putByteArray(const uInt8* array, uInt32 size);
   void putShort(uInt16 value);
   void putShortArray(const uInt16* array, uInt32 size);
   void putInt(uInt32 value);
   void putIntArray(const uInt32* array, uInt32 size);
   void putString(const string& str);
   void putBool(bool b);

private:
   FILE* myFile;
   uInt8* myMemoryBuf;
   uInt32 myBufSize;
   uInt32 myPos;
   bool myUseFilestream;

   static const uInt8 TruePattern = 0xfe;
   static const uInt8 FalsePattern = 0x01;
};

#endif