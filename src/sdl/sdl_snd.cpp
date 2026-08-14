/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2015-2019 GrangerHub

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, see <https://www.gnu.org/licenses/>

===========================================================================
*/

// SDL3 audio backend using the stream-based API.
// Replaces the SDL2 callback-based SDL_OpenAudio/SDL_AudioSpec approach
// with SDL_OpenAudioDeviceStream + SDL_AudioStream.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <SDL3/SDL.h>

#include "client/snd_local.h"
#include "qcommon/cvar.h"
#include "qcommon/q_shared.h"

bool snd_inited = false;

cvar_t *s_sdlBits;
cvar_t *s_sdlSpeed;
cvar_t *s_sdlChannels;
cvar_t *s_sdlDevSamps;
cvar_t *s_sdlMixSamps;

static int dmapos = 0;
static int dmasize = 0;
static SDL_AudioStream *audioStream = NULL;

/*
===============
SNDDMA_AudioStreamCallback

SDL3 audio stream callback. Fires when the audio device needs more data.
We feed audio from the Quake-style DMA buffer into the SDL3 audio stream.
The DMA buffer is a circular buffer written by the main thread's sound
mixer and read here on the audio thread. Access is synchronized via
SDL_LockAudioStream / SDL_UnlockAudioStream (see SNDDMA_BeginPainting /
SNDDMA_Submit).
===============
*/
static void SDLCALL SNDDMA_AudioStreamCallback(
    void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    int bytesPerSample;
    int pos;
    int tobufend;
    int len1, len2;

    (void)userdata;
    (void)total_amount;

    if (!snd_inited || dmasize == 0 || additional_amount <= 0)
        return; /* returning without putting data = silence */

    bytesPerSample = dma.samplebits / 8;
    pos = dmapos * bytesPerSample;

    if (pos >= dmasize)
        dmapos = pos = 0;

    tobufend = dmasize - pos;
    len1 = additional_amount;
    len2 = 0;

    /* Handle circular buffer wraparound */
    if (len1 > tobufend)
    {
        len1 = tobufend;
        len2 = additional_amount - len1;
    }

    SDL_PutAudioStreamData(stream, dma.buffer + pos, len1);

    if (len2 > 0)
    {
        SDL_PutAudioStreamData(stream, dma.buffer, len2);
        dmapos = len2 / bytesPerSample;
    }
    else
    {
        dmapos += len1 / bytesPerSample;
    }

    if (dmapos >= dmasize / bytesPerSample)
        dmapos = 0;
}

/*
===============
SNDDMA_Init
===============
*/
bool SNDDMA_Init(void)
{
    int tmp;
    int freq;
    int channels;
    int mixSamps;
    int devSamps;
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    SDL_AudioFormat format;

    if (snd_inited)
        return true;

    if (!s_sdlBits)
    {
        s_sdlBits = Cvar_Get("s_sdlBits", "16", CVAR_ARCHIVE);
        s_sdlSpeed = Cvar_Get("s_sdlSpeed", "0", CVAR_ARCHIVE);
        s_sdlChannels = Cvar_Get("s_sdlChannels", "2", CVAR_ARCHIVE);
        s_sdlDevSamps = Cvar_Get("s_sdlDevSamps", "0", CVAR_ARCHIVE);
        s_sdlMixSamps = Cvar_Get("s_sdlMixSamps", "0", CVAR_ARCHIVE);
    }

    Com_Printf("SDL_Init( SDL_INIT_AUDIO )... ");

    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        /* SDL3: SDL_Init returns bool (true=success, false=failure) */
        if (!SDL_Init(SDL_INIT_AUDIO))
        {
            Com_Printf("FAILED (%s)\n", SDL_GetError());
            return false;
        }
    }

    Com_Printf("OK\n");

    const char *driverName = SDL_GetCurrentAudioDriver();
    Com_Printf("SDL audio driver is \"%s\".\n", driverName ? driverName : "(unknown)");

    /* Determine desired audio format */
    tmp = (int)s_sdlBits->value;
    if ((tmp != 16) && (tmp != 8))
        tmp = 16;

    format = (tmp == 16) ? SDL_AUDIO_S16 : SDL_AUDIO_U8;

    freq = (int)s_sdlSpeed->value;
    if (!freq)
        freq = 22050;

    channels = (int)s_sdlChannels->value;
    if (channels < 1 || channels > 2)
        channels = 2;

    /* Build the desired audio spec */
    memset(&desired, 0, sizeof(desired));
    desired.format = format;
    desired.channels = channels;
    desired.freq = freq;

    /*
     * Open audio device with stream using SDL3 API.
     * SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK selects the system default output.
     * The callback pushes data from the DMA buffer to the audio stream.
     */
    audioStream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, SNDDMA_AudioStreamCallback, NULL);

    if (audioStream == NULL)
    {
        Com_Printf("SDL_OpenAudioDeviceStream FAILED (%s)\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    /* Get the SOURCE format of the stream — this is what the callback must
     * provide and what the DMA buffer should be filled with.  We must NOT use
     * the destination (device) format here, because SDL3 may convert (e.g.
     * S16 -> F32 for WASAPI) and the DMA mixer writes in the source format. */
    if (!SDL_GetAudioStreamFormat(audioStream, &obtained, NULL))
    {
        Com_Printf("SDL_GetAudioStreamFormat FAILED (%s)\n", SDL_GetError());
        SDL_DestroyAudioStream(audioStream);
        audioStream = NULL;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    Com_Printf("SDL audio format: %d bit, %d channels, %d Hz\n", (int)SDL_AUDIO_BITSIZE(obtained.format),
        (int)obtained.channels, obtained.freq);

    /* Calculate mix buffer size - must be large to prevent underruns */
    devSamps = 0;
    if (s_sdlDevSamps->value)
        devSamps = (int)s_sdlDevSamps->value;
    else
    {
        if (obtained.freq <= 11025)
            devSamps = 256;
        else if (obtained.freq <= 22050)
            devSamps = 512;
        else if (obtained.freq <= 44100)
            devSamps = 1024;
        else
            devSamps = 2048;
    }

    mixSamps = (int)s_sdlMixSamps->value;
    if (!mixSamps)
        mixSamps = devSamps * (int)obtained.channels * 10;

    /* Round up to power of two - the mixer expects this */
    if (mixSamps & (mixSamps - 1))
    {
        int val = 1;
        while (val < mixSamps)
            val <<= 1;
        mixSamps = val;
    }

    /* Set up the DMA buffer */
    dmapos = 0;
    dma.samplebits = (int)SDL_AUDIO_BITSIZE(obtained.format);
    dma.channels = obtained.channels;
    dma.samples = mixSamps;
    dma.submission_chunk = 1;
    dma.speed = obtained.freq;
    dmasize = dma.samples * (dma.samplebits / 8);
    dma.buffer = (byte *)calloc(1, dmasize);

    if (!dma.buffer)
    {
        Com_Printf("Failed to allocate DMA buffer (%d bytes)\n", dmasize);
        SDL_DestroyAudioStream(audioStream);
        audioStream = NULL;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    Com_Printf("Starting SDL audio playback...\n");
    /* SDL3: audio devices start paused, must explicitly resume */
    SDL_ResumeAudioStreamDevice(audioStream);

    Com_Printf("SDL audio initialized.\n");
    snd_inited = true;
    return true;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos(void) { return dmapos; }

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void)
{
    Com_Printf("Closing SDL audio device...\n");

    if (audioStream)
    {
        SDL_DestroyAudioStream(audioStream);
        audioStream = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    free(dma.buffer);
    dma.buffer = NULL;
    dmapos = dmasize = 0;
    snd_inited = false;
    Com_Printf("SDL audio device shut down.\n");
}

/*
===============
SNDDMA_Submit

Unlock the audio stream so the callback can fire and feed data to the device.
Called after the sound mixer has finished writing to the DMA buffer.
===============
*/
void SNDDMA_Submit(void)
{
    if (audioStream)
        SDL_UnlockAudioStream(audioStream);
}

/*
===============
SNDDMA_BeginPainting

Lock the audio stream to prevent the callback from reading the DMA buffer
while the sound mixer writes new samples into it.
===============
*/
void SNDDMA_BeginPainting(void)
{
    if (audioStream)
        SDL_LockAudioStream(audioStream);
}
