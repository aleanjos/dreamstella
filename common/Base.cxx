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

#include "Base.hxx"
#include <cstdio>

namespace Common {

   Base::Format Base::myDefaultBase = Base::F_DEFAULT;

#ifndef __DREAMCAST__
   std::ios_base::fmtflags Base::myHexflags = std::ios_base::fmtflags(0);
#else
   bool Base::myIsUppercase = true;
#endif

   const char* Base::myUpperFmt[4] = { "%X", "%02X", "%04X", "%08X" };
   const char* Base::myLowerFmt[4] = { "%x", "%02x", "%04x", "%08x" };
   const char** Base::myFmt = Base::myUpperFmt;

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   void Base::setHexUppercase(bool enable)
   {
#ifndef __DREAMCAST__
      if (enable) {
         myHexflags |= std::ios_base::uppercase;
         myFmt = Base::myUpperFmt;
      }
      else {
         myHexflags &= ~std::ios_base::uppercase;
         myFmt = Base::myLowerFmt;
      }
#else
      myIsUppercase = enable;
      myFmt = enable ? Base::myUpperFmt : Base::myLowerFmt;
#endif
   }

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   bool Base::hexUppercase()
   {
#ifndef __DREAMCAST__
      return myHexflags & std::ios_base::uppercase;
#else
      return myIsUppercase;
#endif
   }

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   string Base::toString(int value, Common::Base::Format outputBase)
   {
      char vToS_buf[16];
      if (outputBase == Base::F_DEFAULT)
         outputBase = myDefaultBase;

      switch (outputBase)
      {
      case Base::F_2:     // base 2
      {
         string s = "";
         for (int i = 0; i < 8; ++i) {
            s = (value & (1 << i) ? "1" : "0") + s;
         }
         return s;
      }

      case Base::F_10:    // base 10
         if (value < 0x100)
            BSPF_snprintf(vToS_buf, 4, "%3d", value);
         else
            BSPF_snprintf(vToS_buf, 6, "%5d", value);
         break;

      case Base::F_16_1:  // base 16: 1 byte
         BSPF_snprintf(vToS_buf, 3, myFmt[0], value);
         break;
      case Base::F_16_2:  // base 16: 2 bytes
         BSPF_snprintf(vToS_buf, 3, myFmt[1], value);
         break;
      case Base::F_16_4:  // base 16: 4 bytes
         BSPF_snprintf(vToS_buf, 5, myFmt[2], value);
         break;
      case Base::F_16_8:  // base 16: 8 bytes
         BSPF_snprintf(vToS_buf, 9, myFmt[3], value);
         break;

      case Base::F_16:
      default:
         if (value < 0x100)
            BSPF_snprintf(vToS_buf, 3, myFmt[1], value);
         else if (value < 0x10000)
            BSPF_snprintf(vToS_buf, 5, myFmt[2], value);
         else
            BSPF_snprintf(vToS_buf, 9, myFmt[3], value);
         break;
      }

      return string(vToS_buf);
   }
}
