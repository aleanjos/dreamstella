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


#include "Serializer.hxx"
#include <stdlib.h>
#include <string.h>

Serializer::Serializer(const string& filename, bool readonly)
   : myFile(NULL), myMemoryBuf(NULL), myUseFilestream(true) {
   myFile = fopen(filename.c_str(), readonly ? "rb" : "wb");
}

Serializer::Serializer()
   : myFile(NULL), myUseFilestream(false), myPos(0) {
   myBufSize = 1024 * 64;
   myMemoryBuf = (uInt8*)malloc(myBufSize);
}

Serializer::~Serializer() {
   if (myFile) fclose(myFile);
   if (myMemoryBuf) free(myMemoryBuf);
}

bool Serializer::isValid() const {
   return (myUseFilestream ? (myFile != NULL) : (myMemoryBuf != NULL));
}

void Serializer::reset() {
   if (myUseFilestream && myFile) rewind(myFile);
   else myPos = 0;
}

uInt8 Serializer::getByte() {
   uInt8 val = 0;
   if (myUseFilestream) fread(&val, 1, 1, myFile);
   else if (myPos < myBufSize) val = myMemoryBuf[myPos++];
   return val;
}

void Serializer::getByteArray(uInt8* array, uInt32 size) {
   if (myUseFilestream) fread(array, 1, size, myFile);
   else {
      memcpy(array, &myMemoryBuf[myPos], size);
      myPos += size;
   }
}

uInt16 Serializer::getShort() {
   uInt16 val = 0;
   getByteArray((uInt8*)&val, sizeof(uInt16));
   return val;
}

uInt32 Serializer::getInt() {
   uInt32 val = 0;
   getByteArray((uInt8*)&val, sizeof(uInt32));
   return val;
}

bool Serializer::getBool() {
   return getByte() == TruePattern;
}


void Serializer::putByte(uInt8 value) {
   if (myUseFilestream) fwrite(&value, 1, 1, myFile);
   else if (myPos < myBufSize) myMemoryBuf[myPos++] = value;
}

void Serializer::putByteArray(const uInt8* array, uInt32 size) {
   if (myUseFilestream) {
      if (myFile) fwrite(array, 1, size, myFile);
   }
   else {
      if (myPos + size < myBufSize) {
         memcpy(&myMemoryBuf[myPos], array, size);
         myPos += size;
      }
      else {
         printf("SERIOUS ERROR: Serializer buffer overflow! (Size: %u)\n", (unsigned int)size);
      }
   }
}

void Serializer::putShort(uInt16 value) {
   putByteArray((uInt8*)&value, sizeof(uInt16));
}

void Serializer::putInt(uInt32 value) {
   putByteArray((uInt8*)&value, sizeof(uInt32));
}

void Serializer::putBool(bool b) {
   putByte(b ? TruePattern : FalsePattern);
}


void Serializer::putString(const string& str) {
   uInt32 len = str.length();
   putInt(len);
   putByteArray((const uInt8*)str.data(), len);
}

string Serializer::getString() {
   uInt32 len = getInt();
   if (len == 0) return "";
   char* buf = (char*)malloc(len + 1);
   getByteArray((uInt8*)buf, len);
   buf[len] = '\0';
   string res(buf);
   free(buf);
   return res;
}


void Serializer::getShortArray(uInt16* array, uInt32 size) {
   for (uInt32 i = 0; i < size; ++i) {
      array[i] = getShort();
   }
}

void Serializer::putShortArray(const uInt16* array, uInt32 size) {
   for (uInt32 i = 0; i < size; ++i) {
      putShort(array[i]);
   }
}

void Serializer::getIntArray(uInt32* array, uInt32 size) {
   for (uInt32 i = 0; i < size; ++i) {
      array[i] = getInt();
   }
}

void Serializer::putIntArray(const uInt32* array, uInt32 size) {
   for (uInt32 i = 0; i < size; ++i) {
      putInt(array[i]);
   }
}