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

#ifndef BASE_HXX
#define BASE_HXX

#include <iosfwd>

#include "bspf.hxx"

namespace Common {

   class Base
   {
   public:
      enum Format {
         F_16, F_16_1, F_16_2, F_16_4, F_16_8, F_2, F_10, F_DEFAULT
      };

      static void setFormat(Base::Format base) { myDefaultBase = base; }
      static Base::Format format() { return myDefaultBase; }

      static void setHexUppercase(bool enable);
      static bool hexUppercase();

      /** Convert integer to a string in the given base format */
      static string toString(int value, Common::Base::Format outputBase = Common::Base::F_DEFAULT);

      static inline std::ostream& HEX2(std::ostream& os) { return os; }
      static inline std::ostream& HEX4(std::ostream& os) { return os; }
      static inline std::ostream& HEX8(std::ostream& os) { return os; }

   private:
      Base() {}

   private:
      static Format myDefaultBase;

      static bool myIsUppercase;

      static const char** myFmt;
      static const char* myUpperFmt[4];
      static const char* myLowerFmt[4];
   };

} 
#endif
