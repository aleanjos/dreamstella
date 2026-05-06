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

#ifndef SOUND_DC_HXX
#define SOUND_DC_HXX


#include <kos.h>
#include <dc/sound/stream.h>
#include <dc/sound/sound.h>

class OSystem;

#include "bspf.hxx"
#include "TIASnd.hxx"
#include "Sound.hxx"

#define QUEUE_CAPACITY 1024

class SoundDC : public Sound
{
  public:
    SoundDC(OSystem* osystem);
    virtual ~SoundDC();

  public:
    void open();
    void close();
    void setEnabled(bool enable);
    void setChannels(uInt32 channels);
    void setFrameRate(float framerate);
    void mute(bool state);
    void reset();
    void setVolume(Int32 percent);
    void adjustVolume(Int8 direction);
    bool save(Serializer& out) const;
    bool load(Serializer& in);
    string name() const;

  public:
    void set(uInt16 addr, uInt8 value, Int32 cycle);
    static void poll();
    void adjustCycleCounter(Int32 amount);

  protected:
    struct RegWrite {
        uInt16 addr;
        uInt8 value;
        float delta; 
    };

    void enqueue(uInt16 addr, uInt8 value, float delta);
    void processFragment(Int16* stream, uInt32 length);

    static void* callback(snd_stream_hnd_t hnd, int len, int *out_len);

  private:
    static SoundDC* myStaticInstance;

    snd_stream_hnd_t myStreamHandle;
    TIASound* myTIASound;

    bool isMuted;
    uInt32 myVolume;
    Int32 myLastRegisterSetCycle;

    RegWrite myQueue[QUEUE_CAPACITY];
    int myQueueSize;
    int myQueueHead;
    int myQueueTail;

    float myFragmentSizeLogBase2;
    float myFragmentSizeLogDiv1;
    float myFragmentSizeLogDiv2;
    float myFrameRate;
    uInt32 mySamples;
    uInt32 myFreq;
};

#endif
