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

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================

#ifdef THUMB_SUPPORT

#ifndef THUMB__HXX
#define THUMB__HXX

#ifdef THUMB_SUPPORT

#include "bspf.hxx"
#include <string>

class DummyStream {
public:
   template<typename T> DummyStream& operator<<(const T&) { return *this; }
   std::string str() const { return ""; }
};

#define ROMADDMASK 0x7FFF
#define RAMADDMASK 0x1FFF
#define ROMSIZE (ROMADDMASK+1)
#define RAMSIZE (RAMADDMASK+1)

// Constantes de modo ARM
#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1B
#define MODE_SYS 0x1F

#define CPSR_T (1<<5)
#define CPSR_F (1<<6)
#define CPSR_I (1<<7)
#define CPSR_N (1<<31)
#define CPSR_Z (1<<30)
#define CPSR_C (1<<29)
#define CPSR_V (1<<28)
#define CPSR_Q (1<<27)

class Thumbulator
{
public:
   Thumbulator(const uInt16* rom, uInt16* ram, bool traponfatal = true);
   ~Thumbulator();

   void dump_regs(void);
   void dump_counters(void);
   int execute(void);
   int reset(void);
   std::string run(void);
   void trapFatalErrors(bool enable) { trapOnFatal = enable; }

private:
   uInt32 fetch16(uInt32 addr);
   uInt32 fetch32(uInt32 addr);
   uInt32 read16(uInt32 addr);
   uInt32 read32(uInt32 addr);
   void write16(uInt32 addr, uInt32 data);
   void write32(uInt32 addr, uInt32 data);

   inline uInt32 read_register(uInt32 reg) __attribute__((always_inline))
   {
      reg &= 0xF;
      return (reg == 13 || reg == 14) ? reg_svc[reg] : reg_sys[reg];
   }

   inline uInt32 write_register(uInt32 reg, uInt32 data) __attribute__((always_inline))
   {
      reg &= 0xF;
      if (reg == 13 || reg == 14) 
         reg_svc[reg] = data;
      else 
         reg_sys[reg] = data;
      return data;
   }

   // Flags
   void do_zflag(uInt32 x);
   void do_nflag(uInt32 x);
   void do_cflag(uInt32 a, uInt32 b, uInt32 c);
   void do_sub_vflag(uInt32 a, uInt32 b, uInt32 c);
   void do_add_vflag(uInt32 a, uInt32 b, uInt32 c);
   void do_cflag_bit(uInt32 x);
   void do_vflag_bit(uInt32 x);

   int fatalError(const char* opcode, uInt32 v1, const char* msg);
   int fatalError(const char* opcode, uInt32 v1, uInt32 v2, const char* msg);

private:
   const uInt16* rom;
   uInt16* ram;

   uInt32 reg[16];
   uInt32 cpsr;
   uInt32 reg_svc[16], reg_sys[16];

   uInt32 instructions, fetches, reads, writes, mamcr, halfadd;

   bool trapOnFatal;
   DummyStream statusMsg;
};

#endif
#endif#endif

#endif
