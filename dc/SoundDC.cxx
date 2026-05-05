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

#include "OSystem.hxx"
#include "SoundDC.hxx"

SoundDC *SoundDC::myStaticInstance = nullptr;

SoundDC::SoundDC(OSystem *osystem)
    : Sound(osystem), myStreamHandle(SND_STREAM_INVALID), myTIASound(nullptr)
{
    myStaticInstance = this;
    isMuted = false;
    myVolume = 100;
    myLastRegisterSetCycle = 0;

    myQueueSize = 0;
    myQueueHead = 0;
    myQueueTail = 0;
}

SoundDC::~SoundDC()
{
    close();
    myStaticInstance = nullptr;
}

void *SoundDC::callback(snd_stream_hnd_t hnd, int len, int *out_len)
{
    int framesNeeded = len / 4;
    static Int16 tempBuffer[4096] __attribute__((aligned(32)));

    memset(tempBuffer, 0, len);

    if (myStaticInstance && !myStaticInstance->isMuted)
    {
        myStaticInstance->processFragment(tempBuffer, framesNeeded);
    }

    *out_len = len;
    return tempBuffer;
}

void SoundDC::open()
{
    myFreq = 31400;
    mySamples = 1024;
    myFrameRate = (float)myOSystem->myFramerate;

    myTIASound = new TIASound(myFreq);
    myTIASound->channels(2, false);

    myFragmentSizeLogDiv1 = 1.5f / myFrameRate;
    myFragmentSizeLogDiv2 = 1.0f / myFrameRate;

    snd_init();
    snd_stream_init();

    myStreamHandle = snd_stream_alloc(callback, mySamples * 4);
    snd_stream_start(myStreamHandle, myFreq, 1);
}

void SoundDC::close()
{
    if (myStreamHandle != SND_STREAM_INVALID)
    {
        snd_stream_stop(myStreamHandle);
        snd_stream_destroy(myStreamHandle);
        myStreamHandle = SND_STREAM_INVALID;
    }
    if (myTIASound)
    {
        delete myTIASound;
        myTIASound = nullptr;
    }
}

void SoundDC::enqueue(uInt16 addr, uInt8 value, float delta)
{
    if (myQueueSize >= QUEUE_CAPACITY)
    {
        if (myTIASound)
            myTIASound->set(myQueue[myQueueHead].addr, myQueue[myQueueHead].value);
        myQueueHead = (myQueueHead + 1) % QUEUE_CAPACITY;
        myQueueSize--;
    }

    myQueue[myQueueTail].addr = addr;
    myQueue[myQueueTail].value = value;
    myQueue[myQueueTail].delta = delta;
    myQueueTail = (myQueueTail + 1) % QUEUE_CAPACITY;
    myQueueSize++;
}

void SoundDC::set(uInt16 addr, uInt8 value, Int32 cycle)
{
    float delta = ((float)(cycle - myLastRegisterSetCycle)) / 1193191.666f;
    enqueue(addr, value, delta);
    myLastRegisterSetCycle = cycle;
}

void SoundDC::processFragment(Int16 *stream, uInt32 length)
{
    float queueDuration = 0.0f;
    for (uInt32 i = 0; i < myQueueSize; ++i)
    {
        queueDuration += myQueue[(myQueueHead + i) % QUEUE_CAPACITY].delta;
    }

    if (queueDuration > myFragmentSizeLogDiv1)
    {
        float removed = 0.0f;
        while (removed < myFragmentSizeLogDiv2 && myQueueSize > 0)
        {
            RegWrite &info = myQueue[myQueueHead];
            removed += info.delta;
            myTIASound->set(info.addr, info.value);
            myQueueHead = (myQueueHead + 1) % QUEUE_CAPACITY;
            --myQueueSize;
        }
    }

    uInt32 position = 0;
    float remaining = (float)length;

    while (remaining > 0.0f)
    {
        if (myQueueSize == 0)
        {
            myTIASound->process(stream + (position * 2), (uInt32)remaining);
            break;
        }

        RegWrite &info = myQueue[myQueueHead];
        float duration = remaining / (float)myFreq;

        if (info.delta <= duration)
        {
            if (info.delta > 0.0f)
            {
                uInt32 samplesToProcess = (uInt32)((float)myFreq * info.delta);
                myTIASound->process(stream + (position * 2), samplesToProcess);
                position += samplesToProcess;
                remaining -= (float)samplesToProcess;
            }
            myTIASound->set(info.addr, info.value);
            myQueueHead = (myQueueHead + 1) % QUEUE_CAPACITY;
            --myQueueSize;
        }
        else
        {
            myTIASound->process(stream + (position * 2), (uInt32)remaining);
            info.delta -= duration;
            remaining = 0;
        }
    }
}

void SoundDC::poll()
{
    if (myStaticInstance && myStaticInstance->myStreamHandle != SND_STREAM_INVALID)
    {
        snd_stream_poll(myStaticInstance->myStreamHandle);
    }
}

void SoundDC::adjustCycleCounter(Int32 amount) { myLastRegisterSetCycle += amount; }

void SoundDC::setEnabled(bool enable) {}
void SoundDC::setChannels(uInt32 channels) {}
void SoundDC::setFrameRate(float framerate) {}

void SoundDC::mute(bool state)
{
    isMuted = state;
    
    if (state)
        snd_stream_volume(myStaticInstance->myStreamHandle, 0);
    else
        snd_stream_volume(myStaticInstance->myStreamHandle, 255);

}

void SoundDC::reset()
{
    myQueueSize = myQueueHead = myQueueTail = 0;
    myLastRegisterSetCycle = 0;
    if (myTIASound)
        myTIASound->reset();
}

void SoundDC::setVolume(Int32 percent)
{
    myVolume = percent;
    if (myTIASound)
        myTIASound->volume(percent);
}

void SoundDC::adjustVolume(Int8 direction) {}
bool SoundDC::save(Serializer &out) const { return true; }
bool SoundDC::load(Serializer &in) { return true; }
string SoundDC::name() const { return "Sound Dreamcast KOS Implementation"; }
