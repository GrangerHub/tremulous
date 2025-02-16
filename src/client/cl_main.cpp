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
// cl_main.c  -- client main loop

#include "client.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif

#include <climits>

#include "sys/sys_loadlib.h"
#include "sys/sys_local.h"

#include "cl_updates.h"
#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif

#ifdef USE_MUMBLE
cvar_t *cl_useMumble;
cvar_t *cl_mumbleScale;
#endif

#ifdef USE_VOIP
cvar_t *cl_voipUseVAD;
cvar_t *cl_voipVADThreshold;
cvar_t *cl_voipSend;
cvar_t *cl_voipSendTarget;
cvar_t *cl_voipGainDuringCapture;
cvar_t *cl_voipCaptureMult;
cvar_t *cl_voipShowMeter;
cvar_t *cl_voipProtocol;
cvar_t *cl_voip;
#endif

#ifdef USE_RENDERER_DLOPEN
cvar_t *cl_renderer;
#endif

cvar_t *cl_nodelta;
cvar_t *cl_debugMove;

cvar_t *cl_noprint;
cvar_t *cl_motd;

cvar_t *rcon_client_password;
cvar_t *rconAddress;

cvar_t *cl_timeout;
cvar_t *cl_maxpackets;
cvar_t *cl_packetdup;
cvar_t *cl_timeNudge;
cvar_t *cl_showTimeDelta;
cvar_t *cl_freezeDemo;

cvar_t *cl_shownet;
cvar_t *cl_showSend;
cvar_t *cl_timedemo;
cvar_t *cl_timedemoLog;
cvar_t *cl_autoRecordDemo;
cvar_t *cl_aviFrameRate;
cvar_t *cl_aviMotionJpeg;
cvar_t *cl_forceavidemo;

cvar_t *cl_freelook;
cvar_t *cl_sensitivity;

cvar_t *cl_mouseAccel;
cvar_t *cl_mouseAccelOffset;
cvar_t *cl_mouseAccelStyle;
cvar_t *cl_showMouseRate;

cvar_t *m_pitch;
cvar_t *m_yaw;
cvar_t *m_forward;
cvar_t *m_side;
cvar_t *m_filter;

cvar_t *j_pitch;
cvar_t *j_yaw;
cvar_t *j_forward;
cvar_t *j_side;
cvar_t *j_up;
cvar_t *j_pitch_axis;
cvar_t *j_yaw_axis;
cvar_t *j_forward_axis;
cvar_t *j_side_axis;
cvar_t *j_up_axis;

cvar_t *cl_activeAction;

cvar_t *cl_motdString;

cvar_t *cl_allowDownload;
cvar_t *com_downloadPrompt;
cvar_t *cl_conXOffset;
cvar_t *cl_inGameVideo;

cvar_t *cl_serverStatusResendTime;

cvar_t *cl_lanForcePackets;

cvar_t *cl_guidServerUniq;

cvar_t *cl_clantag;

cvar_t *cl_consoleKeys;

cvar_t *cl_rate;

cvar_t *cl_rsaAuth;

clientActive_t cl;
clientConnection_t clc;
clientStatic_t cls;

char cl_reconnectArgs[MAX_OSPATH];
char cl_oldGame[MAX_QPATH];
bool cl_oldGameSet;

// Structure containing functions exported from refresh DLL
refexport_t re;
#ifdef USE_RENDERER_DLOPEN
static void *rendererLib = NULL;
#endif

ping_t cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s {
    char string[BIG_INFO_STRING];
    netadr_t address;
    int time, startTime;
    bool pending;
    bool print;
    bool retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];

static void CL_InitRef(void);

#if defined __USEA3D && defined __A3D_GEOM
void hA3Dg_ExportRenderGeom(refexport_t *incoming_re);
#endif

static bool noGameRestart = false;

static void CL_DownloadUpdate_f() { CL_DownloadRelease(); }

static void CL_InstallUpdate_f()
{
    if (Cmd_Argc() > 1)
        CL_ExecuteInstaller();
    else
        CL_ExecuteInstaller();
}

static void CL_CheckForUpdate_f() { CL_GetLatestRelease(); }

static void CL_BrowseHomepath_f() { FS_BrowseHomepath(); }

static void CL_BrowseDemos_f() { FS_OpenBaseGamePath( "demos/" ); }

static void CL_BrowseScreenShots_f() { FS_OpenBaseGamePath( "screenshots/" ); }

#ifdef USE_MUMBLE
static void CL_UpdateMumble(void)
{
    vec3_t pos, forward, up;
    float scale = cl_mumbleScale->value;
    float tmp;

    if (!cl_useMumble->integer) return;

    // !!! FIXME: not sure if this is even close to correct.
    if (clc.netchan.alternateProtocol == 2)
    {
        AngleVectors(cl.snap.alternatePs.viewangles, forward, NULL, up);

        pos[0] = cl.snap.alternatePs.origin[0] * scale;
        pos[1] = cl.snap.alternatePs.origin[2] * scale;
        pos[2] = cl.snap.alternatePs.origin[1] * scale;
    }
    else
    {
        AngleVectors(cl.snap.ps.viewangles, forward, NULL, up);

        pos[0] = cl.snap.ps.origin[0] * scale;
        pos[1] = cl.snap.ps.origin[2] * scale;
        pos[2] = cl.snap.ps.origin[1] * scale;
    }

    tmp = forward[1];
    forward[1] = forward[2];
    forward[2] = tmp;

    tmp = up[1];
    up[1] = up[2];
    up[2] = tmp;

    if (cl_useMumble->integer > 1)
    {
        fprintf(stderr, "%f %f %f, %f %f %f, %f %f %f\n",
                pos[0], pos[1], pos[2],
                forward[0], forward[1], forward[2],
                up[0], up[1], up[2]);
    }

    mumble_update_coordinates(pos, forward, up);
}
#endif

#ifdef USE_VOIP
static void CL_UpdateVoipIgnore(const char *idstr, bool ignore)
{
    if ((*idstr >= '0') && (*idstr <= '9'))
    {
        const int id = atoi(idstr);
        if ((id >= 0) && (id < MAX_CLIENTS))
        {
            clc.voipIgnore[id] = ignore;
            CL_AddReliableCommand(va("voip %s %d", ignore ? "ignore" : "unignore", id), false);
            Com_Printf("VoIP: %s ignoring player #%d\n", ignore ? "Now" : "No longer", id);
            return;
        }
    }
    Com_Printf("VoIP: invalid player ID#\n");
}

static void CL_UpdateVoipGain(const char *idstr, float gain)
{
    if ((*idstr >= '0') && (*idstr <= '9'))
    {
        const int id = atoi(idstr);
        if (gain < 0.0f) gain = 0.0f;
        if ((id >= 0) && (id < MAX_CLIENTS))
        {
            clc.voipGain[id] = gain;
            Com_Printf("VoIP: player #%d gain now set to %f\n", id, gain);
        }
    }
}

void CL_Voip_f(void)
{
    const char *cmd = Cmd_Argv(1);
    const char *reason = NULL;

    if (clc.state != CA_ACTIVE)
        reason = "Not connected to a server";
    else if (!clc.voipCodecInitialized)
        reason = "Voip codec not initialized";
    else if (!clc.voipEnabled)
        reason = "Server doesn't support VoIP";

    if (reason != NULL)
    {
        Com_Printf("VoIP: command ignored: %s\n", reason);
        return;
    }

    if (strcmp(cmd, "ignore") == 0)
    {
        CL_UpdateVoipIgnore(Cmd_Argv(2), true);
    }
    else if (strcmp(cmd, "unignore") == 0)
    {
        CL_UpdateVoipIgnore(Cmd_Argv(2), false);
    }
    else if (strcmp(cmd, "gain") == 0)
    {
        if (Cmd_Argc() > 3)
        {
            CL_UpdateVoipGain(Cmd_Argv(2), atof(Cmd_Argv(3)));
        }
        else if (Q_isanumber(Cmd_Argv(2)))
        {
            int id = atoi(Cmd_Argv(2));
            if (id >= 0 && id < MAX_CLIENTS)
            {
                Com_Printf(
                    "VoIP: current gain for player #%d "
                    "is %f\n",
                    id, clc.voipGain[id]);
            }
            else
            {
                Com_Printf("VoIP: invalid player ID#\n");
            }
        }
        else
        {
            Com_Printf("usage: voip gain <playerID#> [value]\n");
        }
    }
    else if (strcmp(cmd, "muteall") == 0)
    {
        Com_Printf("VoIP: muting incoming voice\n");
        CL_AddReliableCommand("voip muteall", false);
        clc.voipMuteAll = true;
    }
    else if (strcmp(cmd, "unmuteall") == 0)
    {
        Com_Printf("VoIP: unmuting incoming voice\n");
        CL_AddReliableCommand("voip unmuteall", false);
        clc.voipMuteAll = false;
    }
    else
    {
        Com_Printf(
            "usage: voip [un]ignore <playerID#>\n"
            "       voip [un]muteall\n"
            "       voip gain <playerID#> [value]\n");
    }
}

static void CL_VoipNewGeneration(void)
{
    // don't have a zero generation so new clients won't match, and don't
    //  wrap to negative so MSG_ReadLong() doesn't "fail."
    clc.voipOutgoingGeneration++;
    if (clc.voipOutgoingGeneration <= 0) clc.voipOutgoingGeneration = 1;
    clc.voipPower = 0.0f;
    clc.voipOutgoingSequence = 0;

    opus_encoder_ctl(clc.opusEncoder, OPUS_RESET_STATE);
}

/*
===============
CL_VoipParseTargets

sets clc.voipTargets according to cl_voipSendTarget
Generally we don't want who's listening to change during a transmission,
so this is only called when the key is first pressed
===============
*/
static void CL_VoipParseTargets(void)
{
    const char *target = cl_voipSendTarget->string;
    char *end;
    int val;

    ::memset(clc.voipTargets, 0, sizeof(clc.voipTargets));
    clc.voipFlags &= ~VOIP_SPATIAL;

    while (target)
    {
        while (*target == ',' || *target == ' ') target++;

        if (!*target) break;

        if (isdigit(*target))
        {
            val = strtol(target, &end, 10);
            target = end;
        }
        else
        {
            if (!Q_stricmpn(target, "all", 3))
            {
                ::memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
                return;
            }
            if (!Q_stricmpn(target, "spatial", 7))
            {
                clc.voipFlags |= VOIP_SPATIAL;
                target += 7;
                continue;
            }
            else
            {
                if (!Q_stricmpn(target, "attacker", 8))
                {
                    val = VM_Call(cls.cgame, CG_LAST_ATTACKER);
                    target += 8;
                }
                else if (!Q_stricmpn(target, "crosshair", 9))
                {
                    val = VM_Call(cls.cgame, CG_CROSSHAIR_PLAYER);
                    target += 9;
                }
                else
                {
                    while (*target && *target != ',' && *target != ' ') target++;

                    continue;
                }

                if (val < 0) continue;
            }
        }

        if (val < 0 || val >= MAX_CLIENTS)
        {
            Com_Printf(S_COLOR_YELLOW
                "WARNING: VoIP "
                "target %d is not a valid client "
                "number\n",
                val);

            continue;
        }

        clc.voipTargets[val / 8] |= 1 << (val % 8);
    }
}

/*
===============
CL_CaptureVoip

Record more audio from the hardware if required and encode it into Opus
 data for later transmission.
===============
*/
static void CL_CaptureVoip(void)
{
    const float audioMult = cl_voipCaptureMult->value;
    const bool useVad = (cl_voipUseVAD->integer != 0);
    bool initialFrame = false;
    bool finalFrame = false;

#if USE_MUMBLE
    // if we're using Mumble, don't try to handle VoIP transmission ourselves.
    if (cl_useMumble->integer) return;
#endif

    // If your data rate is too low, you'll get Connection Interrupted warnings
    //  when VoIP packets arrive, even if you have a broadband connection.
    //  This might work on rates lower than 25000, but for safety's sake, we'll
    //  just demand it. Who doesn't have at least a DSL line now, anyhow? If
    //  you don't, you don't need VoIP.  :)
    if (cl_voip->modified || cl_rate->modified)
    {
        if ((cl_voip->integer) && (cl_rate->integer < 25000))
        {
            Com_Printf(S_COLOR_YELLOW "Your network rate is too slow for VoIP.\n");
            Com_Printf("Set 'Data Rate' to 'LAN/Cable/xDSL' in 'Setup/System/Network'.\n");
            Com_Printf("Until then, VoIP is disabled.\n");
            Cvar_Set("cl_voip", "0");
        }
        Cvar_Set("cl_voipProtocol", cl_voip->integer ? "opus" : "");
        cl_voip->modified = false;
        cl_rate->modified = false;
    }

    if (!clc.voipCodecInitialized) return;  // just in case this gets called at a bad time.

    if (clc.voipOutgoingDataSize > 0) return;  // packet is pending transmission, don't record more yet.

    if (cl_voipUseVAD->modified)
    {
        Cvar_Set("cl_voipSend", (useVad) ? "1" : "0");
        cl_voipUseVAD->modified = false;
    }

    if ((useVad) && (!cl_voipSend->integer)) Cvar_Set("cl_voipSend", "1");  // lots of things reset this.

    if (cl_voipSend->modified)
    {
        bool dontCapture = false;
        if (clc.state != CA_ACTIVE)
            dontCapture = true;  // not connected to a server.
        else if (!clc.voipEnabled)
            dontCapture = true;  // server doesn't support VoIP.
        else if (clc.demoplaying)
            dontCapture = true;  // playing back a demo.
        else if (cl_voip->integer == 0)
            dontCapture = true;  // client has VoIP support disabled.
        else if (audioMult == 0.0f)
            dontCapture = true;  // basically silenced incoming audio.

        cl_voipSend->modified = false;

        if (dontCapture)
        {
            Cvar_Set("cl_voipSend", "0");
            return;
        }

        if (cl_voipSend->integer)
        {
            initialFrame = true;
        }
        else
        {
            finalFrame = true;
        }
    }

    // try to get more audio data from the sound card...

    if (initialFrame)
    {
        S_MasterGain(Com_Clamp(0.0f, 1.0f, cl_voipGainDuringCapture->value));
        S_StartCapture();
        CL_VoipNewGeneration();
        CL_VoipParseTargets();
    }

    if ((cl_voipSend->integer) || (finalFrame))
    {  // user wants to capture audio?
        int samples = S_AvailableCaptureSamples();
        const int packetSamples = (finalFrame) ? VOIP_MAX_FRAME_SAMPLES : VOIP_MAX_PACKET_SAMPLES;

        // enough data buffered in audio hardware to process yet?
        if (samples >= packetSamples)
        {
            // audio capture is always MONO16.
            static int16_t sampbuffer[VOIP_MAX_PACKET_SAMPLES];
            float voipPower = 0.0f;
            int voipFrames;
            int i, bytes;

            if (samples > VOIP_MAX_PACKET_SAMPLES) samples = VOIP_MAX_PACKET_SAMPLES;

            // !!! FIXME: maybe separate recording from encoding, so voipPower
            // !!! FIXME:  updates faster than 4Hz?

            samples -= samples % VOIP_MAX_FRAME_SAMPLES;
            if (samples != 120 && samples != 240 && samples != 480 && samples != 960 && samples != 1920 &&
                samples != 2880)
            {
                Com_Printf("Voip: bad number of samples %d\n", samples);
                return;
            }
            voipFrames = samples / VOIP_MAX_FRAME_SAMPLES;

            S_Capture(samples, (byte *)sampbuffer);  // grab from audio card.

            // check the "power" of this packet...
            for (i = 0; i < samples; i++)
            {
                const float flsamp = (float)sampbuffer[i];
                const float s = fabs(flsamp);
                voipPower += s * s;
                sampbuffer[i] = (int16_t)((flsamp)*audioMult);
            }

            // encode raw audio samples into Opus data...
            bytes = opus_encode(clc.opusEncoder, sampbuffer, samples, (unsigned char *)clc.voipOutgoingData,
                sizeof(clc.voipOutgoingData));
            if (bytes <= 0)
            {
                Com_DPrintf("VoIP: Error encoding %d samples\n", samples);
                bytes = 0;
            }

            clc.voipPower = (voipPower / (32768.0f * 32768.0f * ((float)samples))) * 100.0f;

            if ((useVad) && (clc.voipPower < cl_voipVADThreshold->value))
            {
                CL_VoipNewGeneration();  // no "talk" for at least 1/4 second.
            }
            else
            {
                clc.voipOutgoingDataSize = bytes;
                clc.voipOutgoingDataFrames = voipFrames;

                Com_DPrintf("VoIP: Send %d frames, %d bytes, %f power\n", voipFrames, bytes, clc.voipPower);

#if 0
				static FILE *encio = NULL;
				if (encio == NULL) encio = fopen("voip-outgoing-encoded.bin", "wb");
				if (encio != NULL) { fwrite(clc.voipOutgoingData, bytes, 1, encio); fflush(encio); }
				static FILE *decio = NULL;
				if (decio == NULL) decio = fopen("voip-outgoing-decoded.bin", "wb");
				if (decio != NULL) { fwrite(sampbuffer, voipFrames * VOIP_MAX_FRAME_SAMPLES * 2, 1, decio); fflush(decio); }
#endif
            }
        }
    }

    // User requested we stop recording, and we've now processed the last of
    //  any previously-buffered data. Pause the capture device, etc.
    if (finalFrame)
    {
        S_StopCapture();
        S_MasterGain(1.0f);
        clc.voipPower = 0.0f;  // force this value so it doesn't linger.
    }
}
#endif

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(const char *cmd, bool isDisconnectCmd)
{
    int unacknowledged = clc.reliableSequence - clc.reliableAcknowledge;

    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    // also leave one slot open for the disconnect command in this case.

    if ((isDisconnectCmd && unacknowledged > MAX_RELIABLE_COMMANDS) ||
        (!isDisconnectCmd && unacknowledged >= MAX_RELIABLE_COMMANDS))
    {
        if (com_errorEntered)
            return;
        else
            Com_Error(ERR_DROP, "Client command overflow");
    }

    Q_strncpyz(
        clc.reliableCommands[++clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1)], cmd, sizeof(*clc.reliableCommands));
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(msg_t *msg, int headerBytes)
{
    int len, swlen;

    // write the packet sequence
    len = clc.serverMessageSequence;
    swlen = LittleLong(len);
    FS_Write(&swlen, 4, clc.demofile);
    // skip the packet sequencing information
    len = msg->cursize - headerBytes;
    swlen = LittleLong(len);
    FS_Write(&swlen, 4, clc.demofile);
    FS_Write(msg->data + headerBytes, len, clc.demofile);
}

/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f(void)
{
    int len;

    if (!clc.demorecording)
    {
        Com_Printf("Not recording a demo.\n");
        return;
    }

    // finish up
    len = -1;
    FS_Write(&len, 4, clc.demofile);
    FS_Write(&len, 4, clc.demofile);
    FS_FCloseFile(clc.demofile);
    clc.demofile = 0;
    clc.demorecording = false;
    clc.spDemoRecording = false;
    Com_Printf("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
*/
static void CL_DemoFilename(int number, char *fileName, int fileNameSize)
{
    int a, b, c, d;

    if (number < 0 || number > 9999) number = 9999;

    a = number / 1000;
    number -= a * 1000;
    b = number / 100;
    number -= b * 100;
    c = number / 10;
    number -= c * 10;
    d = number;

    Com_sprintf(fileName, fileNameSize, "demo%i%i%i%i", a, b, c, d);
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
static char demoName[MAX_QPATH];  // compiler bug workaround

static void CL_Record_f(void)
{
    char name[MAX_OSPATH];
    byte bufData[MAX_MSGLEN];
    msg_t buf;
    int i;
    int len;
    entityState_t *ent;
    entityState_t nullstate;

    if (Cmd_Argc() > 2)
    {
        Com_Printf("record <demoname>\n");
        return;
    }

    if (clc.demorecording)
    {
        if (!clc.spDemoRecording)
        {
            Com_Printf("Already recording.\n");
        }
        return;
    }

    if (clc.state != CA_ACTIVE)
    {
        Com_Printf("You must be in a level to record.\n");
        return;
    }

    // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
    if (NET_IsLocalAddress(clc.serverAddress) && !Cvar_VariableValue("g_synchronousClients"))
    {
        Com_Printf(S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
    }

    if (Cmd_Argc() == 2)
    {
        const char *s = Cmd_Argv(1);
        Q_strncpyz(demoName, s, sizeof(demoName));
        Com_sprintf(
          name, sizeof(name),
          "demos/%s.%s%d", demoName, DEMOEXT,
            (clc.netchan.alternateProtocol == 0 ?
              PROTOCOL_VERSION : clc.netchan.alternateProtocol == 1 ? 70 : 69));
    }
    else
    {
        int number;

        // scan for a free demo name
        for (number = 0; number <= 9999; number++)
        {
            CL_DemoFilename(number, demoName, sizeof(demoName));
            Com_sprintf(name, sizeof(name), "demos/%s.%s%d", demoName, DEMOEXT, PROTOCOL_VERSION);

            if (!FS_FileExists(name)) break;  // file doesn't exist
        }
    }

    // open the demo file

    Com_Printf("recording to %s.\n", name);
    clc.demofile = FS_FOpenFileWrite(name);
    if (!clc.demofile)
    {
        Com_Printf("ERROR: couldn't open.\n");
        return;
    }
    clc.demorecording = true;
    if (Cvar_VariableValue("ui_recordSPDemo"))
    {
        clc.spDemoRecording = true;
    }
    else
    {
        clc.spDemoRecording = false;
    }

    Q_strncpyz(clc.demoName, demoName, sizeof(clc.demoName));

    // don't start saving messages until a non-delta compressed message is received
    clc.demowaiting = true;

    // write out the gamestate message
    MSG_Init(&buf, bufData, sizeof(bufData));
    MSG_Bitstream(&buf);

    // NOTE, MRE: all server->client messages now acknowledge
    MSG_WriteLong(&buf, clc.reliableSequence);

    MSG_WriteByte(&buf, svc_gamestate);
    MSG_WriteLong(&buf, clc.serverCommandSequence);

    // configstrings
    for (i = 0; i < MAX_CONFIGSTRINGS; i++)
    {
        if (!cl.gameState.stringOffsets[i])
        {
            continue;
        }
        const char *s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
        MSG_WriteByte(&buf, svc_configstring);
        MSG_WriteShort(&buf, i);
        MSG_WriteBigString(&buf, s);
    }

    // baselines
    ::memset(&nullstate, 0, sizeof(nullstate));
    for (i = 0; i < MAX_GENTITIES; i++)
    {
        ent = &cl.entityBaselines[i];
        if (!ent->number)
        {
            continue;
        }
        MSG_WriteByte(&buf, svc_baseline);
        MSG_WriteDeltaEntity(clc.netchan.alternateProtocol, &buf, &nullstate, ent, true);
    }

    MSG_WriteByte(&buf, svc_EOF);

    // finished writing the gamestate stuff

    // write the client num
    MSG_WriteLong(&buf, clc.clientNum);
    // write the checksum feed
    MSG_WriteLong(&buf, clc.checksumFeed);

    // finished writing the client packet
    MSG_WriteByte(&buf, svc_EOF);

    // write it to the demo file
    len = LittleLong(clc.serverMessageSequence - 1);
    FS_Write(&len, 4, clc.demofile);

    len = LittleLong(buf.cursize);
    FS_Write(&len, 4, clc.demofile);
    FS_Write(buf.data, buf.cursize, clc.demofile);

    // the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoFrameDurationSDev
=================
*/
static float CL_DemoFrameDurationSDev(void)
{
    int i;
    int numFrames;
    float mean = 0.0f;
    float variance = 0.0f;

    if ((clc.timeDemoFrames - 1) > MAX_TIMEDEMO_DURATIONS)
        numFrames = MAX_TIMEDEMO_DURATIONS;
    else
        numFrames = clc.timeDemoFrames - 1;

    for (i = 0; i < numFrames; i++) mean += clc.timeDemoDurations[i];
    mean /= numFrames;

    for (i = 0; i < numFrames; i++)
    {
        float x = clc.timeDemoDurations[i];

        variance += ((x - mean) * (x - mean));
    }
    variance /= numFrames;

    return sqrt(variance);
}

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted(void)
{
    char buffer[MAX_STRING_CHARS];

    if (cl_timedemo && cl_timedemo->integer)
    {
        int time;

        time = Sys_Milliseconds() - clc.timeDemoStart;
        if (time > 0)
        {
            // Millisecond times are frame durations:
            // minimum/average/maximum/std deviation
            Com_sprintf(buffer, sizeof(buffer), "%i frames %3.1f seconds %3.1f fps %d.0/%.1f/%d.0/%.1f ms\n",
                clc.timeDemoFrames, time / 1000.0, clc.timeDemoFrames * 1000.0 / time, clc.timeDemoMinDuration,
                time / (float)clc.timeDemoFrames, clc.timeDemoMaxDuration, CL_DemoFrameDurationSDev());
            Com_Printf("%s", buffer);

            // Write a log of all the frame durations
            if (cl_timedemoLog && strlen(cl_timedemoLog->string) > 0)
            {
                int i;
                int numFrames;
                fileHandle_t f;

                if ((clc.timeDemoFrames - 1) > MAX_TIMEDEMO_DURATIONS)
                    numFrames = MAX_TIMEDEMO_DURATIONS;
                else
                    numFrames = clc.timeDemoFrames - 1;

                f = FS_FOpenFileWrite(cl_timedemoLog->string);
                if (f)
                {
                    FS_Printf(f, "# %s", buffer);

                    for (i = 0; i < numFrames; i++) FS_Printf(f, "%d\n", clc.timeDemoDurations[i]);

                    FS_FCloseFile(f);
                    Com_Printf("%s written\n", cl_timedemoLog->string);
                }
                else
                {
                    Com_Printf("Couldn't open %s for writing\n", cl_timedemoLog->string);
                }
            }
        }
    }

    CL_Disconnect(true);
    CL_NextDemo();
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage(void)
{
    int r;
    msg_t buf;
    byte bufData[MAX_MSGLEN];
    int s;

    if (!clc.demofile)
    {
        CL_DemoCompleted();
        return;
    }

    // get the sequence number
    r = FS_Read(&s, 4, clc.demofile);
    if (r != 4)
    {
        CL_DemoCompleted();
        return;
    }
    clc.serverMessageSequence = LittleLong(s);

    // init the message
    MSG_Init(&buf, bufData, sizeof(bufData));

    // get the length
    r = FS_Read(&buf.cursize, 4, clc.demofile);
    if (r != 4)
    {
        CL_DemoCompleted();
        return;
    }
    buf.cursize = LittleLong(buf.cursize);
    if (buf.cursize == -1)
    {
        CL_DemoCompleted();
        return;
    }
    if (buf.cursize > buf.maxsize)
    {
        Com_Error(ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
    }
    r = FS_Read(buf.data, buf.cursize, clc.demofile);
    if (r != buf.cursize)
    {
        Com_Printf("Demo file was truncated.\n");
        CL_DemoCompleted();
        return;
    }

    clc.lastPacketTime = cls.realtime;
    buf.readcount = 0;
    CL_ParseServerMessage(&buf);
}

/*
====================
CL_WalkDemoExt
====================
*/
static int CL_WalkDemoExt(const char *arg, char *name, int *demofile)
{
    int i;
    *demofile = 0;

    Com_sprintf(name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT, PROTOCOL_VERSION);
    FS_FOpenFileRead(name, demofile, true);

    if (*demofile)
    {
        Com_Printf("Demo file: %s\n", name);
        return PROTOCOL_VERSION;
    }

    Com_Printf("Not found: %s\n", name);

    for (i = 0; demo_protocols[i]; ++i)
    {
        if (demo_protocols[i] == PROTOCOL_VERSION) continue;

        Com_sprintf(name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT, demo_protocols[i]);
        FS_FOpenFileRead(name, demofile, true);
        if (*demofile)
        {
            Com_Printf("Demo file: %s\n", name);

            return demo_protocols[i];
        }
        else
            Com_Printf("Not found: %s\n", name);
    }

    return -1;
}

/*
====================
CL_CompleteDemoName
====================
*/
static void CL_CompleteDemoName(char *args, int argNum)
{
    if (argNum == 2)
    {
        char demoExt[16];

        Com_sprintf(demoExt, sizeof(demoExt), ".%s%d", DEMOEXT, PROTOCOL_VERSION);
        Field_CompleteFilename("demos", demoExt, true, true);
    }
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f(void)
{
    char name[MAX_OSPATH];
    const char *ext_test;
    int protocol, i;
    char retry[MAX_OSPATH];

    if (Cmd_Argc() != 2)
    {
        Com_Printf("demo <demoname>\n");
        return;
    }

    // make sure a local server is killed
    // 2 means don't force disconnect of local client
    Cvar_Set("sv_killserver", "2");

    // open the demo file
    const char *arg = Cmd_Argv(1);

    CL_Disconnect(true);

    // check for an extension .DEMOEXT_?? (?? is protocol)
    ext_test = strrchr(arg, '.');

    if (ext_test && !Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1))
    {
        protocol = atoi(ext_test + ARRAY_LEN(DEMOEXT));

        for (i = 0; demo_protocols[i]; i++)
        {
            if (demo_protocols[i] == protocol) break;
        }

        if (demo_protocols[i] || protocol == PROTOCOL_VERSION)
        {
            Com_sprintf(name, sizeof(name), "demos/%s", arg);
            FS_FOpenFileRead(name, &clc.demofile, true);
        }
        else
        {
            int len;

            Com_Printf("Protocol %d not supported for demos\n", protocol);
            len = ext_test - arg;

            if (len >= ARRAY_LEN(retry)) len = ARRAY_LEN(retry) - 1;

            Q_strncpyz(retry, arg, len + 1);
            retry[len] = '\0';
            protocol = CL_WalkDemoExt(retry, name, &clc.demofile);
        }
    }
    else
        protocol = CL_WalkDemoExt(arg, name, &clc.demofile);

    if (!clc.demofile)
    {
        Com_Error(ERR_DROP, "couldn't open %s", name);
        return;
    }
    Q_strncpyz(clc.demoName, arg, sizeof(clc.demoName));

    clc.state = CA_CONNECTED;
    clc.demoplaying = true;
    Q_strncpyz(clc.servername, arg, sizeof(clc.servername));
    clc.netchan.alternateProtocol = (protocol == 69 ? 2 : protocol == 70 ? 1 : 0);

    // read demo messages until connected
    while (clc.state >= CA_CONNECTED && clc.state < CA_PRIMED)
    {
        CL_ReadDemoMessage();
    }
    // don't get the first snapshot this frame, to prevent the long
    // time from the gamestate load from messing causing a time skip
    clc.firstDemoFrameSkipped = false;
}

/*
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
static void CL_StartDemoLoop(void)
{
    // start the demo loop again
    Cbuf_AddText("d1\n");
    Key_SetCatcher(Key_GetCatcher() & KEYCATCH_CONSOLE);
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo(void)
{
    char v[MAX_STRING_CHARS];

    Q_strncpyz(v, Cvar_VariableString("nextdemo"), sizeof(v));
    v[MAX_STRING_CHARS - 1] = 0;
    Com_DPrintf("CL_NextDemo: %s\n", v);
    if (!v[0])
    {
        return;
    }

    Cvar_Set("nextdemo", "");
    Cbuf_AddText(v);
    Cbuf_AddText("\n");
    Cbuf_Execute();
}

/*
==================
CL_DemoState

Returns the current state of the demo system
==================
*/
demoState_t CL_DemoState(void)
{
    if (clc.demoplaying)
    {
        return DS_PLAYBACK;
    }
    else if (clc.demorecording)
    {
        return DS_RECORDING;
    }
    else
    {
        return DS_NONE;
    }
}

/*
==================
CL_DemoPos

Returns the current position of the demo
==================
*/
int CL_DemoPos(void)
{
    if (clc.demoplaying || clc.demorecording)
    {
        return FS_FTell(clc.demofile);
    }
    else
    {
        return 0;
    }
}

/*
==================
CL_DemoName

Returns the name of the demo
==================
*/
void CL_DemoName(char *buffer, int size)
{
    if (clc.demoplaying || clc.demorecording)
    {
        Q_strncpyz(buffer, clc.demoName, size);
    }
    else if (size >= 1)
    {
        buffer[0] = '\0';
    }
}

//======================================================================

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState(void)
{
    //	S_StopAllSounds();

    ::memset(&cl, 0, sizeof(cl));
}

/*
====================
CL_UpdateGUID

update cl_guid using QKEY_FILE and optional prefix
====================
*/
static void CL_UpdateGUID(const char *prefix, int prefix_len)
{
    fileHandle_t f;
    int len;

    len = FS_SV_FOpenFileRead(QKEY_FILE, &f);
    FS_FCloseFile(f);

    if (len != QKEY_SIZE)
        Cvar_Set("cl_guid", "");
    else
        Cvar_Set("cl_guid", Com_MD5File(QKEY_FILE, QKEY_SIZE, prefix, prefix_len));
}

static void CL_OldGame(void)
{
    if (cl_oldGameSet)
    {
        // change back to previous fs_game
        cl_oldGameSet = false;
        Cvar_Set2("fs_game", cl_oldGame, true);
        FS_ConditionalRestart(clc.checksumFeed, false);
    }
}

/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(bool showMainMenu)
{
    if (!com_cl_running || !com_cl_running->integer)
    {
        return;
    }

    // shutting down the client so enter full screen ui mode
    Cvar_Set("r_uiFullScreen", "1");

    if (clc.demorecording)
    {
        CL_StopRecord_f();
    }

    if (clc.download)
    {
        FS_FCloseFile(clc.download);
        clc.download = 0;
    }
    *clc.downloadTempName = *clc.downloadName = 0;
    Cvar_Set("cl_downloadName", "");

#ifdef USE_MUMBLE
    if (cl_useMumble->integer && mumble_islinked())
    {
        Com_Printf("Mumble: Unlinking from Mumble application\n");
        mumble_unlink();
    }
#endif

#ifdef USE_VOIP
    if (cl_voipSend->integer)
    {
        int tmp = cl_voipUseVAD->integer;
        cl_voipUseVAD->integer = 0;  // disable this for a moment.
        clc.voipOutgoingDataSize = 0;  // dump any pending VoIP transmission.
        Cvar_Set("cl_voipSend", "0");
        CL_CaptureVoip();  // clean up any state...
        cl_voipUseVAD->integer = tmp;
    }

    if (clc.voipCodecInitialized)
    {
        int i;
        opus_encoder_destroy(clc.opusEncoder);
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            opus_decoder_destroy(clc.opusDecoder[i]);
        }
    }
    Cmd_RemoveCommand("voip");
#endif

    if (clc.demofile)
    {
        FS_FCloseFile(clc.demofile);
        clc.demofile = 0;
    }

    if (cls.ui && showMainMenu)
    {
        VM_Call(cls.ui, UI_SET_ACTIVE_MENU - (cls.uiInterface == 2 ? 2 : 0), UIMENU_NONE);
    }

    SCR_StopCinematic();
    S_ClearSoundBuffer();

    // send a disconnect message to the server
    // send it a few times in case one is dropped
    if (clc.state >= CA_CONNECTED)
    {
        CL_AddReliableCommand("disconnect", true);
        CL_WritePacket();
        CL_WritePacket();
        CL_WritePacket();
    }

    // Remove pure paks
    FS_PureServerSetLoadedPaks("", "");
    FS_PureServerSetReferencedPaks("", "");

    CL_ClearState();

    // wipe the client connection
    ::memset(&clc, 0, sizeof(clc));

    clc.state = CA_DISCONNECTED;

    CL_ProtocolSpecificCommandsInit();

    // allow cheats locally
    Cvar_Set("sv_cheats", "1");

    // not connected to a pure server anymore
    cl_connectedToPureServer = false;

#ifdef USE_VOIP
    // not connected to voip server anymore.
    clc.voipEnabled = false;
#endif

    // Stop recording any video
    if (CL_VideoRecording())
    {
        // Finish rendering current frame
        SCR_UpdateScreen();
        CL_CloseAVI();
    }

    CL_UpdateGUID(NULL, 0);

    if (!noGameRestart)
        CL_OldGame();
    else
        noGameRestart = false;
}

/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer(const char *string)
{
    const char *cmd = Cmd_Argv(0);

    // ignore key up commands
    if (cmd[0] == '-')
    {
        return;
    }

    if (clc.demoplaying || clc.state < CA_CONNECTED || cmd[0] == '+')
    {
        Com_Printf("Unknown command \"%s" S_COLOR_WHITE "\"\n", cmd);
        return;
    }

    if (Cmd_Argc() > 1)
    {
        CL_AddReliableCommand(string, false);
    }
    else
    {
        CL_AddReliableCommand(cmd, false);
    }
}

/*
===================
CL_RequestMotd

===================
*/
static void CL_RequestMotd(void)
{
    char info[MAX_INFO_STRING];

    if (!cl_motd->integer)
    {
        return;
    }
    Com_Printf("Resolving %s\n", MASTER_SERVER_NAME);

    switch (NET_StringToAdr(MASTER_SERVER_NAME, &cls.updateServer, NA_UNSPEC))
    {
        case 0:
            Com_Printf("Couldn't resolve master address\n");
            return;

        case 2:
            cls.updateServer.port = BigShort(PORT_MASTER);
        default:
            break;
    }

    Com_Printf("%s resolved to %s\n", MASTER_SERVER_NAME, NET_AdrToStringwPort(cls.updateServer));

    info[0] = 0;

    Com_sprintf(cls.updateChallenge, sizeof(cls.updateChallenge), "%i", ((rand() << 16) ^ rand()) ^ Com_Milliseconds());

    Info_SetValueForKey(info, "challenge", cls.updateChallenge);

    NET_OutOfBandPrint(NS_CLIENT, cls.updateServer, "getmotd%s", info);
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ShowIP_f
==================
*/
static void CL_ShowIP_f(void) { Sys_ShowIP(); }

/*
==================
CL_ForwardToServer_f
==================
*/
static void CL_ForwardToServer_f(void)
{
    if (clc.state != CA_ACTIVE || clc.demoplaying)
    {
        Com_Printf("Not connected to a server.\n");
        return;
    }

    // don't forward the first argument
    if (Cmd_Argc() > 1)
    {
        CL_AddReliableCommand(Cmd_Args(), false);
    }
}

/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f(void)
{
    SCR_StopCinematic();
    if (clc.state != CA_DISCONNECTED && clc.state != CA_CINEMATIC)
    {
        Com_Error(ERR_DISCONNECT, "Disconnected from server");
    }
}

/*
================
CL_Reconnect_f

================
*/
static void CL_Reconnect_f(void)
{
    if (!strlen(cl_reconnectArgs)) return;
    Cbuf_AddText(va("connect %s\n", cl_reconnectArgs));
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f(void)
{
    const char *server;
    int alternateProtocol;
    const char *serverString;
    int argc = Cmd_Argc();
    netadrtype_t family = NA_UNSPEC;

    if (argc < 2 || argc > 4)
    {
        Com_Printf("usage: connect [-4|-6] server [-g|-1]\n");
        return;
    }

    alternateProtocol = 0;
    if (argc == 2)
    {
    }
    else if (!strcmp(Cmd_Argv(argc - 1), "-g"))
    {
        alternateProtocol = 1;
        --argc;
    }
    else if (!strcmp(Cmd_Argv(argc - 1), "-1"))
    {
        alternateProtocol = 2;
        --argc;
    }
    else if (argc == 4)
    {
        --argc;
    }

    if (argc == 2)
        server = Cmd_Argv(1);
    else
    {
        if (!strcmp(Cmd_Argv(1), "-4"))
            family = NA_IP;
        else if (!strcmp(Cmd_Argv(1), "-6"))
            family = NA_IP6;
        else
            Com_Printf("warning: only -4 or -6 as address type understood.\n");

        server = Cmd_Argv(2);
    }

    // save arguments for reconnect
    Q_strncpyz(cl_reconnectArgs, Cmd_Args(), sizeof(cl_reconnectArgs));

    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;

    if (com_sv_running->integer && !strcmp(server, "localhost"))
    {
        // if running a local server, kill it
        SV_Shutdown("Server quit");
    }

    // make sure a local server is killed
    Cvar_Set("sv_killserver", "1");
    SV_Frame(0);

    noGameRestart = true;
    CL_Disconnect(true);

    Q_strncpyz(clc.servername, server, sizeof(clc.servername));

    if (!NET_StringToAdr(clc.servername, &clc.serverAddress, family))
    {
        Com_Printf("Bad server address\n");
        clc.state = CA_DISCONNECTED;
        CL_ProtocolSpecificCommandsInit();
        return;
    }
    if (clc.serverAddress.port == 0)
    {
        clc.serverAddress.port = BigShort(PORT_SERVER);
    }
    clc.serverAddress.alternateProtocol = alternateProtocol;

    serverString = NET_AdrToStringwPort(clc.serverAddress);

    Com_Printf("%s resolved to %s\n", clc.servername, serverString);

    if (cl_guidServerUniq->integer)
        CL_UpdateGUID(serverString, strlen(serverString));
    else
        CL_UpdateGUID(NULL, 0);

    clc.challenge2[0] = '\0';
    clc.sendSignature = false;

    // if we aren't playing on a lan, we need to authenticate
    // with the cd key
    if (NET_IsLocalAddress(clc.serverAddress))
    {
        clc.state = CA_CHALLENGING;
        clc.sendSignature = true;
    }
    else
    {
        clc.state = CA_CONNECTING;

        // Set a client challenge number that ideally is mirrored back by the server.
        clc.challenge = ((rand() << 16) ^ rand()) ^ Com_Milliseconds();
    }

    Key_SetCatcher(Key_GetCatcher() & KEYCATCH_CONSOLE);
    clc.connectTime = -99999;  // CL_CheckForResend() will fire immediately
    clc.connectPacketCount = 0;
}

#define MAX_RCON_MESSAGE 1024

/*
==================
CL_CompleteRcon
==================
*/
static void CL_CompleteRcon(char *args, int argNum)
{
    if (argNum == 2)
    {
        // Skip "rcon "
        char *p = Com_SkipTokens(args, 1, " ");

        if (p > args) Field_CompleteCommand(p, true, true);
    }
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
static void CL_Rcon_f(void)
{
    char message[MAX_RCON_MESSAGE];
    netadr_t to;

    if (!rcon_client_password->string[0])
    {
        Com_Printf(
            "You must set 'rconpassword' before\n"
            "issuing an rcon command.\n");
        return;
    }

    message[0] = -1;
    message[1] = -1;
    message[2] = -1;
    message[3] = -1;
    message[4] = 0;

    Q_strcat(message, MAX_RCON_MESSAGE, "rcon ");

    Q_strcat(message, MAX_RCON_MESSAGE, rcon_client_password->string);
    Q_strcat(message, MAX_RCON_MESSAGE, " ");

    // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
    Q_strcat(message, MAX_RCON_MESSAGE, Cmd_Cmd() + 5);

    if (clc.state >= CA_CONNECTED)
    {
        to = clc.netchan.remoteAddress;
    }
    else
    {
        if (!strlen(rconAddress->string))
        {
            Com_Printf(
                "You must either be connected,\n"
                "or set the 'rconAddress' cvar\n"
                "to issue rcon commands\n");

            return;
        }
        NET_StringToAdr(rconAddress->string, &to, NA_UNSPEC);
        if (to.port == 0)
        {
            to.port = BigShort(PORT_SERVER);
        }
    }

    NET_SendPacket(NS_CLIENT, strlen(message) + 1, message, to);
}

/*
=================
CL_SendPureChecksums
=================
*/
static void CL_SendPureChecksums(void)
{
    char cMsg[MAX_INFO_VALUE];

    // if we are pure we need to send back a command with our referenced pk3 checksums
    Com_sprintf(cMsg, sizeof(cMsg), "cp %d %s", cl.serverId, FS_ReferencedPakPureChecksums());

    CL_AddReliableCommand(cMsg, false);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
static void CL_ResetPureClientAtServer(void) { CL_AddReliableCommand("vdr", false); }

/*
=================
CL_Snd_Restart

Restart the sound subsystem
=================
*/
static void CL_Snd_Shutdown(void)
{
    S_Shutdown();
    cls.soundStarted = false;
}
/*
==================
CL_PK3List_f
==================
*/
static void CL_OpenedPK3List_f(void) { Com_Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames(false)); }

/*
==================
CL_PureList_f
==================
*/
static void CL_ReferencedPK3List_f(void) { Com_Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames(false)); }

/*
==================
CL_Configstrings_f
==================
*/
static void CL_Configstrings_f(void)
{
    int i;
    int ofs;

    if (clc.state != CA_ACTIVE)
    {
        Com_Printf("Not connected to a server.\n");
        return;
    }

    for (i = 0; i < MAX_CONFIGSTRINGS; i++)
    {
        ofs = cl.gameState.stringOffsets[i];
        if (!ofs)
        {
            continue;
        }
        Com_Printf("%4i: %s\n", i, cl.gameState.stringData + ofs);
    }
}

/*
==============
CL_Clientinfo_f
==============
*/
static void CL_Clientinfo_f(void)
{
    Com_Printf("--------- Client Information ---------\n");
    Com_Printf("state: %i\n", clc.state);
    Com_Printf("Server: %s\n", clc.servername);
    Com_Printf("User info settings:\n");
    Info_Print(Cvar_InfoString(CVAR_USERINFO));
    Com_Printf("--------------------------------------\n");
}

//====================================================================

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
static void CL_DownloadsComplete(void)
{
    Com_Printf("Downloads complete\n");

    // if we downloaded with cURL
    if (clc.cURLUsed)
    {
        clc.cURLUsed = false;
        CL_cURL_Shutdown();
        if (clc.cURLDisconnected)
        {
            if (clc.downloadRestart)
            {
                if (!clc.activeCURLNotGameRelated) FS_Restart(clc.checksumFeed);
                clc.downloadRestart = false;
            }
            clc.cURLDisconnected = false;
            if (!clc.activeCURLNotGameRelated) CL_Reconnect_f();
            return;
        }
    }

    // if we downloaded files we need to restart the file system
    if (clc.downloadRestart)
    {
        clc.downloadRestart = false;

        FS_Restart(clc.checksumFeed);  // We possibly downloaded a pak, restart the file system to load it

        // inform the server so we get new gamestate info
        CL_AddReliableCommand("donedl", false);

        // by sending the donedl command we request a new gamestate
        // so we don't want to load stuff yet
        return;
    }
    else
    {
        FS_ClearPakReferences(0);
    }

    // let the client game init and load data
    clc.state = CA_LOADING;

    // Pump the loop, this may change gamestate!
    Com_EventLoop();

    // if the gamestate was changed by calling Com_EventLoop
    // then we loaded everything already and we don't want to do it again.
    if (clc.state != CA_LOADING)
    {
        return;
    }

    // starting to load a map so we get out of full screen ui mode
    Cvar_Set("r_uiFullScreen", "0");

    // flush client memory and start loading stuff
    // this will also (re)load the UI
    // if this is a local client then only the client part of the hunk
    // will be cleared, note that this is done after the hunk mark has been set
    CL_FlushMemory();

    // initialize the CGame
    cls.cgameStarted = true;
    CL_InitCGame();

    // set pure checksums
    CL_SendPureChecksums();

    CL_WritePacket();
    CL_WritePacket();
    CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
static void CL_BeginDownload(const char *localName, const char *remoteName)
{
    Com_DPrintf(
        "***** CL_BeginDownload *****\n"
        "Localname: %s\n"
        "Remotename: %s\n"
        "****************************\n",
        localName, remoteName);

    Q_strncpyz(clc.downloadName, localName, sizeof(clc.downloadName));
    Com_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", localName);

    // Set so UI gets access to it
    Cvar_Set("cl_downloadName", remoteName);
    Cvar_Set("cl_downloadSize", "0");
    Cvar_Set("cl_downloadCount", "0");
    Cvar_SetValue("cl_downloadTime", cls.realtime);

    clc.downloadBlock = 0;  // Starting new file
    clc.downloadCount = 0;

    // Stop any errant looping sounds that may be playing
    S_ClearLoopingSounds(true);

    CL_AddReliableCommand(va("download %s", remoteName), false);
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void)
{
    char *s;
    char *remoteName, *localName;
    bool useCURL = false;
    int prompt;
    int remaining;

    // A download has finished, check whether this matches a referenced checksum
    if (*clc.downloadName && !clc.activeCURLNotGameRelated)
    {
        char *zippath = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), clc.downloadName, "");
        zippath[strlen(zippath) - 1] = '\0';

        if (!FS_CompareZipChecksum(zippath)) Com_Error(ERR_DROP, "Incorrect checksum for file: %s", clc.downloadName);
    }

    *clc.downloadTempName = *clc.downloadName = 0;
    Cvar_Set("cl_downloadName", "");

    // We are looking to start a download here
    if (*clc.downloadList)
    {
        // Calculated
        remaining = 1;  // 1 to be rounded up, '@' may not be in pair
        s = clc.downloadList;
        while (*s)
          if (*(s++) == '@')
            remaining++;
        remaining /= 2;
        clc.downloadDone = clc.downloadTotal - remaining;
        Cvar_SetValue("cl_downloadDone", clc.downloadDone);

        // Prompt if we do not allow automatic downloads
        prompt = com_downloadPrompt->integer;
        if (!(prompt & DLP_TYPE_MASK) && !(cl_allowDownload->integer & DLF_ENABLE))
        {
            char files[MAX_INFO_STRING] = "";
            char *name, *head;
            const char *pure_msg;
            const char *url_msg = "";
            int i = 0, others = 0, swap = 0, max_list = 12;

            // Set the download URL message
            if ((clc.sv_allowDownload & DLF_ENABLE) && !(clc.sv_allowDownload & DLF_NO_REDIRECT))
            {
                url_msg = va("The server redirects to the following URL:\n%s", clc.sv_dlURL);
                max_list -= 6;
            }

            // Make a pretty version of the download list
            name = clc.downloadList;
            if (*name == '@') name++;

            do
            {
                // Copy remote name
                head = name;
                while (*head && *head != '@') head++;

                swap = *head;
                *head = 0;

                if (i++ < max_list)
                {
                    if (i > 1) Q_strcat(files, sizeof(files), ", ");
                    Q_strcat(files, sizeof(files), name);
                }
                else
                {
                    others++;
                }

                *head = swap;
                if (!swap) break;

                // Skip local name
                head++;
                while (*head && *head != '@') head++;

                name = head + 1;
            } while (*head);

            if (others)
            {
                Q_strcat(files, sizeof(files), va("(%d other file%s)\n", others, others > 1 ? "s" : ""));
            }

            // Set the pure message
            if (cl_connectedToPureServer)
            {
                if (!(clc.sv_allowDownload & DLF_ENABLE) ||
                    ((clc.sv_allowDownload & DLF_NO_UDP) && (clc.sv_allowDownload & DLF_NO_REDIRECT)))
                {
                    pure_msg =
                        "You are missing files required by the server. "
                        "The server does not allow downloading. "
                        "You must install these files manually:";
                }
                else
                {
                    pure_msg =
                        "You are missing files required by the server. "
                        "You must download these files or disconnect:";
                }
            }
            else
            {
                pure_msg =
                    "You are missing optional files provided by the "
                    "server. You may not need them to play but can "
                    "choose to download them anyway:";
            }

            Cvar_Set("com_downloadPromptText", va("%s\n\n%s\n%s", pure_msg, files, url_msg));
            Cvar_Set("com_downloadPrompt", va("%d", DLP_SHOW));
            return;
        }

        if (!(prompt & DLP_PROMPTED)) Cvar_Set("com_downloadPrompt", va("%d", prompt | DLP_PROMPTED));

        prompt &= DLP_TYPE_MASK;

        s = clc.downloadList;

        // format is:
        //  @remotename@localname@remotename@localname, etc.

        if (*s == '@') s++;
        remoteName = s;

        if ((s = strchr(s, '@')) == NULL)
        {
            CL_DownloadsComplete();
            return;
        }

        *s++ = 0;
        localName = s;
        if ((s = strchr(s, '@')) != NULL)
            *s++ = 0;
        else
            s = localName + strlen(localName);  // point at the nul byte

        if (((cl_allowDownload->integer & DLF_ENABLE) && !(cl_allowDownload->integer & DLF_NO_REDIRECT)) ||
            prompt == DLP_CURL)
        {
            Com_Printf("Trying CURL download: %s; %s\n", localName, remoteName);
            if (clc.sv_allowDownload & DLF_NO_REDIRECT)
            {
                Com_Printf(
                    "WARNING: server does not "
                    "allow download redirection "
                    "(sv_allowDownload is %d)\n",
                    clc.sv_allowDownload);
            }
            else if (!*clc.sv_dlURL)
            {
                Com_Printf(
                    "WARNING: server allows "
                    "download redirection, but does not "
                    "have sv_dlURL set\n");
            }
            else if (!CL_cURL_Init())
            {
                Com_Printf(
                    "WARNING: could not load "
                    "cURL library\n");
            }
            else
            {
                CL_cURL_BeginDownload(localName, va("%s/%s", clc.sv_dlURL, remoteName));
                useCURL = true;
            }
        }
        else if (!(clc.sv_allowDownload & DLF_NO_REDIRECT))
        {
            Com_Printf(
                "WARNING: server allows download "
                "redirection, but it disabled by client "
                "configuration (cl_allowDownload is %d)\n",
                cl_allowDownload->integer);
        }
        if (!useCURL)
        {
            Com_Printf("Trying UDP download: %s; %s\n", localName, remoteName);

            if ((!(cl_allowDownload->integer & DLF_ENABLE) || (cl_allowDownload->integer & DLF_NO_UDP)) &&
                prompt != DLP_UDP)
            {
                if (cl_connectedToPureServer)
                {
                    Com_Error(ERR_DROP,
                        "Automatic downloads are "
                        "disabled on your client (cl_allowDownload is %d). "
                        "You can enable automatic downloads in the Options "
                        "menu.",
                        cl_allowDownload->integer);
                    return;
                }

                Com_Printf("WARNING: UDP downloads are disabled.\n");
                CL_DownloadsComplete();
                return;
            }
            else
            {
                CL_BeginDownload(localName, remoteName);
            }
        }
        clc.downloadRestart = true;

        // move over the rest
        memmove(clc.downloadList, s, strlen(s) + 1);

        return;
    }

    CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void)
{
    char *s;

    if (FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), true))
    {
        Com_Printf("Need paks: %s\n", clc.downloadList);

        Cvar_Set("com_downloadPrompt", "0");
        if (*clc.downloadList)
        {
            clc.downloadDone = 0;
            clc.downloadTotal = 0;
            s = clc.downloadList;
            while (*s)
              if (*(s++) == '@')
                clc.downloadTotal++;
            clc.downloadTotal /= 2;

            // if autodownloading is not enabled on the server
            clc.state = CA_CONNECTED;
            *clc.downloadTempName = *clc.downloadName = 0;
            Cvar_Set("cl_downloadName", "");
            Cvar_Set("cl_downloadDone", "0");
            Cvar_SetValue("cl_downloadTotal", clc.downloadTotal);
            CL_NextDownload();
            return;
        }
    }
    CL_DownloadsComplete();
}

/*
===============
CL_UnloadRSAKeypair
===============
*/
static void CL_UnloadRSAKeypair(void)
{
    rsa_public_key_clear(&cls.rsa.public_key);
    rsa_private_key_clear(&cls.rsa.private_key);
}

/*
===============
CL_WriteRSAPublicKey
===============
*/
static bool CL_WriteRSAPublicKey(void)
{
    struct nettle_buffer key_buffer;
    fileHandle_t f;

    f = FS_SV_FOpenFileWrite(RSA_PUBLIC_KEY_FILE);
    if (!f) return false;

    nettle_buffer_init(&key_buffer);
    if (!rsa_keypair_to_sexp(&key_buffer, NULL, &cls.rsa.public_key, NULL))
    {
        FS_FCloseFile(f);
        nettle_buffer_clear(&key_buffer);
        return false;
    }

    FS_Write(key_buffer.contents, key_buffer.size, f);
    FS_FCloseFile(f);

    nettle_buffer_clear(&key_buffer);
    return true;
}

/*
===============
CL_WriteRSAPrivateKey
===============
*/
static bool CL_WriteRSAPrivateKey(void)
{
    struct nettle_buffer key_buffer;
    fileHandle_t f;

#ifndef _WIN32
    int old_umask = umask(0377);
#endif
    f = FS_SV_FOpenFileWrite(RSA_PRIVATE_KEY_FILE);
#ifndef _WIN32
    umask(old_umask);
#endif
    if (!f) return false;

    nettle_buffer_init(&key_buffer);
    if (!rsa_keypair_to_sexp(&key_buffer, NULL, &cls.rsa.public_key, &cls.rsa.private_key))
    {
        FS_FCloseFile(f);
        nettle_buffer_clear(&key_buffer);
        return false;
    }

    FS_Write(key_buffer.contents, key_buffer.size, f);
    FS_FCloseFile(f);

    nettle_buffer_clear(&key_buffer);
    return true;
}

/*
===============
CL_GenerateRSAKeypair

public_key and private_key must already be inititalized before calling this
function.  This is done by CL_LoadRSAKeypair.
===============
*/
static void CL_GenerateRSAKeypair(void)
{
    mpz_set_ui(cls.rsa.public_key.e, RSA_PUBLIC_EXPONENT);

    int success = rsa_generate_keypair(
        &cls.rsa.public_key, &cls.rsa.private_key, NULL, qnettle_random, NULL, NULL, RSA_KEY_LENGTH, 0);
    if (success)
        if (CL_WriteRSAPrivateKey())
            if (CL_WriteRSAPublicKey())
            {
                Com_Printf("RSA keypair generated\n");
                return;
            }

    // failure
    CL_UnloadRSAKeypair();
    Com_Printf("Error generating RSA keypair, setting cl_rsaAuth to 0\n");
    Cvar_Set("cl_rsaAuth", "0");
}

/*
===============
CL_LoadRSAKeypair

Attempt to load RSA keys from RSA_PRIVATE_KEY_FILE
If this fails, generate a new keypair
===============
*/
static void CL_LoadRSAKeypair(void)
{
    int len;
    fileHandle_t f;
    uint8_t *buf;

    rsa_public_key_init(&cls.rsa.public_key);
    rsa_private_key_init(&cls.rsa.private_key);

    Com_DPrintf("Loading RSA private key from %s\n", RSA_PRIVATE_KEY_FILE);

    len = FS_SV_FOpenFileRead(RSA_PRIVATE_KEY_FILE, &f);
    if (!f)
    {
        Com_DPrintf("RSA private key not found, generating\n");
        CL_GenerateRSAKeypair();
        return;
    }

    if (len < 1)
    {
        Com_DPrintf("RSA private key empty, generating\n");
        FS_FCloseFile(f);
        CL_GenerateRSAKeypair();
        return;
    }

    buf = (uint8_t *)Z_Malloc(len);
    FS_Read(buf, len, f);
    FS_FCloseFile(f);

    if (!rsa_keypair_from_sexp(&cls.rsa.public_key, &cls.rsa.private_key, 0, len, buf))
    {
        memset(buf, 0, len);
        Z_Free(buf);
        CL_UnloadRSAKeypair();
        Com_Error(ERR_FATAL, "Invalid RSA private key found.");
        return;
    }

    memset(buf, 0, len);
    Z_Free(buf);

    len = FS_SV_FOpenFileRead(RSA_PUBLIC_KEY_FILE, &f);
    if (!f || len < 1) CL_WriteRSAPublicKey();
    if (f) FS_FCloseFile(f);

    Com_DPrintf("RSA private key loaded\n");
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
static void CL_CheckForResend(void)
{
    int port;
    char info[MAX_INFO_STRING];
    char data[MAX_MSGLEN];

    // don't send anything if playing back a demo
    if (clc.demoplaying)
    {
        return;
    }

    // resend if we haven't gotten a reply yet
    if (clc.state != CA_CONNECTING && clc.state != CA_CHALLENGING)
    {
        return;
    }

    if (cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT)
    {
        return;
    }

    clc.connectTime = cls.realtime;  // for retransmit requests
    clc.connectPacketCount++;

    switch (clc.state)
    {
        case CA_CONNECTING:
            // requesting a challenge

            // The challenge request shall be followed by a client challenge so no malicious server can hijack this
            // connection.
            // Add the gamename so the server knows we're running the correct game or can reject the client
            // with a meaningful message
            if (clc.serverAddress.alternateProtocol == 2)
            {
                Com_sprintf(data, sizeof(data), "getchallenge");
            }
            else if (clc.serverAddress.alternateProtocol == 1)
            {
                Com_sprintf(data, sizeof(data), "getchallenge %d", clc.challenge);
            }
            else
                Com_sprintf(data, sizeof(data), "getchallenge %d %s", clc.challenge, com_gamename->string);

            NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "%s", data);
            break;

        case CA_CHALLENGING:
            // sending back the challenge
            port = Cvar_VariableValue("net_qport");

            Q_strncpyz(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));
            Info_SetValueForKey(info, "protocol",
                va("%i",
                    (clc.serverAddress.alternateProtocol == 0 ? PROTOCOL_VERSION
                                                              : clc.serverAddress.alternateProtocol == 1 ? 70 : 69)));
            Info_SetValueForKey(info, "qport", va("%i", port));
            Info_SetValueForKey(info, "challenge", va("%i", clc.challenge));

            if (cl_rsaAuth->integer && clc.sendSignature)
            {
                char public_key[RSA_STRING_LENGTH];
                char signature[RSA_STRING_LENGTH];
                struct sha256_ctx sha256_hash;
                mpz_t n;

                Info_SetValueForKey(info, "challenge2", clc.challenge2);

                sha256_init(&sha256_hash);
                sha256_update(&sha256_hash, strlen(info), (uint8_t *)info);

                mpz_init(n);
                rsa_sha256_sign(&cls.rsa.private_key, &sha256_hash, n);
                mpz_get_str(signature, 16, n);
                mpz_clear(n);

                mpz_get_str(public_key, 16, cls.rsa.public_key.n);

                Com_sprintf(data, sizeof(data), "connect \"%s\" %s %s", info, public_key, signature);
            }
            else
            {
                Com_sprintf(data, sizeof(data), "connect \"%s\"", info);
            }

            NET_OutOfBandData(NS_CLIENT, clc.serverAddress, (byte *)data, strlen(data));
            // the most current userinfo has been sent, so watch for any
            // newer changes to userinfo variables
            cvar_modifiedFlags &= ~CVAR_USERINFO;
            break;

        default:
            Com_Error(ERR_FATAL, "CL_CheckForResend: bad clc.state");
    }
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading(void)
{
    if (com_dedicated->integer)
    {
        clc.state = CA_DISCONNECTED;
        Key_SetCatcher(KEYCATCH_CONSOLE);
        CL_ProtocolSpecificCommandsInit();
        return;
    }

    if (!com_cl_running->integer)
    {
        return;
    }

    Key_SetCatcher(Key_GetCatcher() & KEYCATCH_CONSOLE);

    // if we are already connected to the local host, stay connected
    if (clc.state >= CA_CONNECTED && !Q_stricmp(clc.servername, "localhost"))
    {
        clc.state = CA_CONNECTED;  // so the connect screen is drawn
        ::memset(cls.updateInfoString, 0, sizeof(cls.updateInfoString));
        ::memset(clc.serverMessage, 0, sizeof(clc.serverMessage));
        ::memset(&cl.gameState, 0, sizeof(cl.gameState));
        clc.lastPacketSentTime = -9999;
        SCR_UpdateScreen();
    }
    else
    {
        CL_Disconnect(true);
        Q_strncpyz(clc.servername, "localhost", sizeof(clc.servername));
        clc.state = CA_CHALLENGING;  // so the connect screen is drawn
        Key_SetCatcher(Key_GetCatcher() & KEYCATCH_CONSOLE);
        SCR_UpdateScreen();
        clc.connectTime = -RETRANSMIT_TIMEOUT;
        NET_StringToAdr(clc.servername, &clc.serverAddress, NA_UNSPEC);
        // we don't need a challenge on the localhost

        CL_CheckForResend();
    }
}

/*
===================
CL_MotdPacket

===================
*/
static void CL_MotdPacket(netadr_t from, const char *info)
{
    const char *v;

    // if not from our server, ignore it
    if (!NET_CompareAdr(from, cls.updateServer))
    {
        Com_DPrintf("MOTD packet from unexpected source\n");
        return;
    }

    Com_DPrintf("MOTD packet: %s\n", info);
    while (*info != '\\') info++;

    // check challenge
    v = Info_ValueForKey(info, "challenge");
    if (strcmp(v, cls.updateChallenge))
    {
        Com_DPrintf(
            "MOTD packet mismatched challenge: "
            "'%s' != '%s'\n",
            v, cls.updateChallenge);
        return;
    }

    v = Info_ValueForKey(info, "motd");

    Q_strncpyz(cls.updateInfoString, info, sizeof(cls.updateInfoString));
    Cvar_Set("cl_motdString", v);
}

/*
===================
CL_InitServerInfo
===================
*/
static void CL_InitServerInfo(serverInfo_t *server, netadr_t *address)
{
    server->adr = *address;
    server->clients = 0;
    server->hostName[0] = '\0';
    server->mapName[0] = '\0';
    server->label[0] = '\0';
    server->maxClients = 0;
    server->maxPing = 0;
    server->minPing = 0;
    server->ping = -1;
    server->game[0] = '\0';
    server->gameType = 0;
    server->netType = 0;
}

/*
===================
CL_GSRSequenceInformation

Parses this packet's index and the number of packets from a master server's
response. Updates the packet count and returns the index. Advances the data
pointer as appropriate (but only when parsing was successful)

The sequencing information isn't terribly useful at present (we can skip
duplicate packets, but we don't bother to make sure we've got all of them).
===================
*/
static int CL_GSRSequenceInformation(int alternateProtocol, byte **data)
{
    char *p = (char *)*data, *e;
    int ind, num;
    // '\0'-delimited fields: this packet's index, total number of packets
    if (*p++ != '\0') return -1;

    ind = strtol(p, (char **)&e, 10);
    if (*e++ != '\0') return -1;

    num = strtol(e, (char **)&p, 10);
    if (*p++ != '\0') return -1;

    if (num <= 0 || ind <= 0 || ind > num) return -1;  // nonsensical response

    if (cls.numAlternateMasterPackets[alternateProtocol] > 0 && num != cls.numAlternateMasterPackets[alternateProtocol])
    {
        // Assume we sent two getservers and somehow they changed in
        // between - only use the results that arrive later
        Com_DPrintf("Master changed its mind about%s packet count!\n",
            (alternateProtocol == 0 ? "" : alternateProtocol == 1 ? " alternate-1" : " alternate-2"));
        cls.receivedAlternateMasterPackets[alternateProtocol] = 0;
        // cls.numglobalservers = 0;
        // cls.numGlobalServerAddresses = 0;
    }
    cls.numAlternateMasterPackets[alternateProtocol] = num;

    // successfully parsed
    *data = (byte *)p;
    return ind;
}

/*
===================
CL_GSRFeaturedLabel

Parses from the data an arbitrary text string labelling the servers in the
following getserversresponse packet.
The result is copied to *buf, and *data is advanced as appropriate
===================
*/
static void CL_GSRFeaturedLabel(byte **data, char *buf, int size)
{
    char *l = buf;

    // copy until '\0' which indicates field break
    // or slash which indicates beginning of server list
    while (**data && **data != '\\' && **data != '/')
    {
        if (l < &buf[size - 1])
            *l = **data;
        else if (l == &buf[size - 1])
            Com_DPrintf(S_COLOR_YELLOW
                "Warning: "
                "CL_GSRFeaturedLabel: overflow\n");
        l++, (*data)++;
    }

    if (l < &buf[size - 1])
        *l = '\0';
    else
        buf[size - 1] = '\0';
}

#define MAX_SERVERSPERPACKET 256

/*
===================
CL_ServersResponsePacket
===================
*/
static void CL_ServersResponsePacket(const netadr_t *from, msg_t *msg, bool extended)
{
    int i, count, total;
    netadr_t addresses[MAX_SERVERSPERPACKET];
    int numservers;
    byte *buffptr;
    byte *buffend;
    char label[MAX_FEATLABEL_CHARS] = "";

    Com_DPrintf("CL_ServersResponsePacket from %s %s\n",
            NET_AdrToStringwPort(*from),
            extended ? " (extended)" : "");

    if (cls.numglobalservers == -1)
    {
        // state to detect lack of servers or lack of response
        cls.numglobalservers = 0;
        cls.numGlobalServerAddresses = 0;
        for (i = 0; i < 3; ++i)
        {
            cls.numAlternateMasterPackets[i] = 0;
            cls.receivedAlternateMasterPackets[i] = 0;
        }
    }

    // parse through server response string
    numservers = 0;
    buffptr = msg->data;
    buffend = buffptr + msg->cursize;

    // skip header
    buffptr += 4;

    // advance to initial token
    // I considered using strchr for this but I don't feel like relying
    // on its behaviour with '\0'
    while (*buffptr && *buffptr != '\\' && *buffptr != '/')
    {
        buffptr++;

        if (buffptr >= buffend) break;
    }

    if (*buffptr == '\0')
    {
        int ind = CL_GSRSequenceInformation(from->alternateProtocol, &buffptr);
        if (ind >= 0)
        {
            // this denotes the start of new-syntax stuff
            // have we already received this packet?
            if (cls.receivedAlternateMasterPackets[from->alternateProtocol] & (1 << (ind - 1)))
            {
                Com_DPrintf(
                    "CL_ServersResponsePacket: "
                    "received packet %d again, ignoring\n",
                    ind);
                return;
            }
            // TODO: detect dropped packets and make another
            // request
            Com_DPrintf(
                "CL_ServersResponsePacket:%s packet "
                "%d of %d\n",
                (from->alternateProtocol == 0 ? "" : from->alternateProtocol == 1 ? " alternate-1" : " alternate-2"),
                ind, cls.numAlternateMasterPackets[from->alternateProtocol]);
            cls.receivedAlternateMasterPackets[from->alternateProtocol] |= (1 << (ind - 1));

            CL_GSRFeaturedLabel(&buffptr, label, sizeof(label));
        }
        // now skip to the server list
        for (; buffptr < buffend && *buffptr != '\\' && *buffptr != '/'; buffptr++)
            ;
    }

    while (buffptr + 1 < buffend)
    {
        // IPv4 address
        if (*buffptr == '\\')
        {
            buffptr++;

            if (buffend - buffptr < sizeof(addresses[numservers].ip) + sizeof(addresses[numservers].port) + 1) break;

            for (i = 0; i < sizeof(addresses[numservers].ip); i++) addresses[numservers].ip[i] = *buffptr++;

            addresses[numservers].type = NA_IP;
        }
        // IPv6 address, if it's an extended response
        else if (extended && *buffptr == '/')
        {
            buffptr++;

            if (buffend - buffptr < sizeof(addresses[numservers].ip6) + sizeof(addresses[numservers].port) + 1) break;

            for (i = 0; i < sizeof(addresses[numservers].ip6); i++) addresses[numservers].ip6[i] = *buffptr++;

            addresses[numservers].type = NA_IP6;
            addresses[numservers].scope_id = from->scope_id;
        }
        else
            // syntax error!
            break;

        // parse out port
        addresses[numservers].port = (*buffptr++) << 8;
        addresses[numservers].port += *buffptr++;
        addresses[numservers].port = BigShort(addresses[numservers].port);

        // syntax check
        if (*buffptr != '\\' && *buffptr != '/') break;

        addresses[numservers].alternateProtocol = from->alternateProtocol;

        numservers++;
        if (numservers >= MAX_SERVERSPERPACKET) break;
    }

    count = cls.numglobalservers;

    for (i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++)
    {
        // build net address
        serverInfo_t *server = &cls.globalServers[count];

        CL_InitServerInfo(server, &addresses[i]);
        Q_strncpyz(server->label, label, sizeof(server->label));
        // advance to next slot
        count++;
    }

    // if getting the global list
    if (count >= MAX_GLOBAL_SERVERS && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS)
    {
        // if we couldn't store the servers in the main list anymore
        for (; i < numservers && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS; i++)
        {
            // just store the addresses in an additional list
            cls.globalServerAddresses[cls.numGlobalServerAddresses++] = addresses[i];
        }
    }

    cls.numglobalservers = count;
    total = count + cls.numGlobalServerAddresses;

    Com_Printf("%d servers parsed (total %d)\n", numservers, total);
}
/*
==================
CL_CheckTimeout

==================
*/
static void CL_CheckTimeout(void)
{
    //
    // check timeout
    //
    if ((!CL_CheckPaused() || !sv_paused->integer) && clc.state >= CA_CONNECTED && clc.state != CA_CINEMATIC &&
        cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000)
    {
        if (++cl.timeoutcount > 5)
        {  // timeoutcount saves debugger
            Com_Printf("\nServer connection timed out.\n");
            CL_Disconnect(true);
            return;
        }
    }
    else
    {
        cl.timeoutcount = 0;
    }
}

/*
==================
CL_CheckPaused
Check whether client has been paused.
==================
*/
bool CL_CheckPaused(void)
{  // if cl_paused->modified is set, the cvar has only been changed in
    // this frame. Keep paused in this frame to ensure the server doesn't
    // lag behind.
    if (cl_paused->integer || cl_paused->modified) return true;

    return false;
}

//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
static void CL_CheckUserinfo(void)
{
    // don't add reliable commands when not yet connected
    if (clc.state < CA_CONNECTED) return;

    // don't overflow the reliable command buffer when paused
    if (CL_CheckPaused()) return;

    // send a reliable userinfo update if needed
    if (cvar_modifiedFlags & CVAR_USERINFO)
    {
        cvar_modifiedFlags &= ~CVAR_USERINFO;
        CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString(CVAR_USERINFO)), false);
    }
}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int msec)
{
    if (!com_cl_running->integer) return;

    // We may have a download prompt ready
    if ((com_downloadPrompt->integer & DLP_TYPE_MASK) && !(com_downloadPrompt->integer & DLP_PROMPTED))
    {
        Com_Printf("Download prompt returned %d\n", com_downloadPrompt->integer);
        CL_NextDownload();
    }
    else if (com_downloadPrompt->integer & DLP_SHOW)
    {
        // If the UI VM does not support the download prompt, we need to catch
        // the prompt here and replicate regular behavior.
        // One frame will always run between requesting and showing the prompt.

        if (com_downloadPrompt->integer & DLP_STALE)
        {
            Com_Printf("WARNING: UI VM does not support download prompt\n");
            Cvar_Set("com_downloadPrompt", va("%d", DLP_IGNORE));
            CL_NextDownload();
        }
        else
        {
            Cvar_Set("com_downloadPrompt", va("%d", com_downloadPrompt->integer | DLP_STALE));
        }
    }

    if (clc.downloadCURLM)
    {
        CL_cURL_PerformDownload();
        // we can't process frames normally when in disconnected download mode
        // since the ui vm expects clc.state to be CA_CONNECTED
        if (clc.cURLDisconnected)
        {
            cls.realFrametime = msec;
            cls.frametime = msec;
            cls.realtime += cls.frametime;

            SCR_UpdateScreen();
            S_Update();
            Con_RunConsole();

            cls.framecount++;
            return;
        }
    }

    if (clc.state == CA_DISCONNECTED && !(Key_GetCatcher() & KEYCATCH_UI) && !com_sv_running->integer && cls.ui)
    {
        // if disconnected, bring up the menu
        S_StopAllSounds();
        VM_Call(cls.ui, UI_SET_ACTIVE_MENU - (cls.uiInterface == 2 ? 2 : 0), UIMENU_MAIN);
    }

    // if recording an avi, lock to a fixed fps
    if (CL_VideoRecording() && cl_aviFrameRate->integer && msec)
    {
        // save the current screen
        if (clc.state == CA_ACTIVE || cl_forceavidemo->integer)
        {
            float fps = MIN(cl_aviFrameRate->value * com_timescale->value, 1000.0f);
            float frameDuration = MAX(1000.0f / fps, 1.0f) + clc.aviVideoFrameRemainder;

            CL_TakeVideoFrame();

            msec = (int)frameDuration;
            clc.aviVideoFrameRemainder = frameDuration - msec;
        }
    }

    if (cl_autoRecordDemo->integer)
    {
        if (clc.state == CA_ACTIVE && !clc.demorecording && !clc.demoplaying)
        {
            // If not recording a demo, and we should be, start one
            qtime_t now;
            Com_RealTime(&now);

            const char *nowString = va("%04d%02d%02d%02d%02d%02d", 1900 + now.tm_year, 1 + now.tm_mon, now.tm_mday,
                now.tm_hour, now.tm_min, now.tm_sec);

            char serverName[MAX_OSPATH];
            Q_strncpyz(serverName, clc.servername, MAX_OSPATH);

            // Replace the ":" in the address as it is not a valid
            // file name character
            char *p = strstr(serverName, ":");
            if (p) *p = '.';

            char mapName[MAX_QPATH];
            Q_strncpyz(mapName, COM_SkipPath(cl.mapname), sizeof(cl.mapname));
            COM_StripExtension(mapName, mapName, sizeof(mapName));
            Cbuf_ExecuteText(EXEC_NOW, va("record \"%s-%s-%s\"\n", nowString, serverName, mapName));
        }
        else if (clc.state != CA_ACTIVE && clc.demorecording)
        {
            // Recording, but not CA_ACTIVE, so stop recording
            CL_StopRecord_f();
        }
    }

    // save the msec before checking pause
    cls.realFrametime = msec;

    // decide the simulation time
    cls.frametime = msec;

    cls.realtime += cls.frametime;

    if (cl_timegraph->integer)
    {
        SCR_DebugGraph(cls.realFrametime * 0.25);
    }

    // see if we need to update any userinfo
    CL_CheckUserinfo();

    // if we haven't gotten a packet in a long time, drop the connection
    CL_CheckTimeout();

    // send intentions now
    CL_SendCmd();

    // resend a connection request if necessary
    CL_CheckForResend();

    // decide on the serverTime to render
    CL_SetCGameTime();

    // update the screen
    SCR_UpdateScreen();

    // update audio
    S_Update();

#ifdef USE_VOIP
    CL_CaptureVoip();
#endif

#ifdef USE_MUMBLE
    CL_UpdateMumble();
#endif

    // advance local effects for next frame
    SCR_RunCinematic();

    Con_RunConsole();

    cls.framecount++;
}

//============================================================================

/*
================
CL_RefPrintf

DLL glue
================
*/
static __attribute__((format(printf, 2, 3))) void QDECL CL_RefPrintf(int print_level, const char *fmt, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    switch (print_level)
    {
        case PRINT_ALL:
            Com_Printf("%s", msg);
            break;

        case PRINT_WARNING:
            Com_Printf(S_COLOR_YELLOW "%s", msg);
            break;

        case PRINT_DEVELOPER:
            Com_DPrintf(S_COLOR_RED "%s", msg);
            break;
    }
}

/*
============
CL_ShutdownRef
============
*/
static void CL_ShutdownRef(void)
{
    if (re.Shutdown) re.Shutdown(true);

    ::memset(&re, 0, sizeof(re));

#ifdef USE_RENDERER_DLOPEN
    if (rendererLib)
    {
        Sys_UnloadLibrary(rendererLib);
        rendererLib = NULL;
    }
#endif
}

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(bool shutdownRef)
{
    if (CL_VideoRecording()) CL_CloseAVI();

    if (clc.demorecording) CL_StopRecord_f();

    CL_cURL_Shutdown();

    // clear sounds
    S_DisableSounds();
    // shutdown CGame
    CL_ShutdownCGame();
    // shutdown UI
    CL_ShutdownUI();

    // shutdown the renderer
    if (shutdownRef)
        CL_ShutdownRef();
    else if (re.Shutdown)
        re.Shutdown(false);  // don't destroy window or context

    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.rendererStarted = false;
    cls.soundRegistered = false;
}

/*
=================
CL_ClearMemory

Called by Com_GameRestart
=================
*/
static void CL_ClearMemory(bool shutdownRef)
{
    // shutdown all the client stuff
    CL_ShutdownAll(shutdownRef);

    // if not running a server clear the whole hunk
    if (!com_sv_running->integer)
    {
        // clear the whole hunk
        Hunk_Clear();
        // clear collision map data
        CM_ClearMap();
    }
    else
    {
        // clear all the client data on the hunk
        Hunk_ClearToMark();
    }
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void)
{
    CL_ClearMemory(false);
    CL_StartHunkUsers(false);
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
static void CL_Vid_Restart_f(void)
{
    // Settings may have changed so stop recording now
    if (CL_VideoRecording())
    {
        CL_CloseAVI();
    }

    if (clc.demorecording) CL_StopRecord_f();

    // don't let them loop during the restart
    S_StopAllSounds();

    if (!FS_ConditionalRestart(clc.checksumFeed, true))
    {
        // if not running a server clear the whole hunk
        if (com_sv_running->integer)
        {
            // clear all the client data on the hunk
            Hunk_ClearToMark();
        }
        else
        {
            // clear the whole hunk
            Hunk_Clear();
        }

        // shutdown the UI
        CL_ShutdownUI();
        // shutdown the CGame
        CL_ShutdownCGame();
        // shutdown the renderer and clear the renderer interface
        CL_ShutdownRef();
        // client is no longer pure untill new checksums are sent
        CL_ResetPureClientAtServer();
        // clear pak references
        FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
        // reinitialize the filesystem if the game directory or checksum has changed

        cls.rendererStarted = false;
        cls.uiStarted = false;
        cls.cgameStarted = false;
        cls.soundRegistered = false;

        // unpause so the cgame definately gets a snapshot and renders a frame
        Cvar_Set("cl_paused", "0");

        // initialize the renderer interface
        CL_InitRef();

        // startup all the client stuff
        CL_StartHunkUsers(false);

        // start the cgame if connected
        if (clc.state > CA_CONNECTED && clc.state != CA_CINEMATIC)
        {
            cls.cgameStarted = true;
            CL_InitCGame();
            // send pure checksums
            CL_SendPureChecksums();
        }
    }
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
static void CL_Snd_Restart_f(void)
{
    CL_Snd_Shutdown();
    // sound will be reinitialized by vid_restart
    CL_Vid_Restart_f();
}


/*
============
CL_InitRenderer
============
*/
static void CL_InitRenderer(void)
{
    // this sets up the renderer and calls R_Init
    re.BeginRegistration(&cls.glconfig);

    // load character sets
    cls.charSetShader = re.RegisterShader("gfx/2d/bigchars");
    cls.whiteShader = re.RegisterShader("white");
    cls.consoleShader = re.RegisterShader("console");
    g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
    g_consoleField.widthInChars = g_console_field_width;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers(bool rendererOnly)
{
    if (!com_cl_running)
    {
        return;
    }

    if (!com_cl_running->integer)
    {
        return;
    }

    if (!cls.rendererStarted)
    {
        cls.rendererStarted = true;
        CL_InitRenderer();
    }

    if (rendererOnly)
    {
        return;
    }

    if (!cls.soundStarted)
    {
        cls.soundStarted = true;
        S_Init();
    }

    if (!cls.soundRegistered)
    {
        cls.soundRegistered = true;
        S_BeginRegistration();
    }

    if (com_dedicated->integer)
    {
        return;
    }

    if (!cls.uiStarted)
    {
        cls.uiStarted = true;
        CL_InitUI();
    }
}

/*
============
CL_RefMalloc
============
*/
static void *CL_RefMalloc(int size) { return Z_TagMalloc(size, TAG_RENDERER); }

/*
============
CL_ScaledMilliseconds
============
*/
int CL_ScaledMilliseconds(void) { return Sys_Milliseconds() * com_timescale->value; }

//===========================================================================================

static void CL_SetModel_f(void)
{
    char name[256];

    const char *arg = Cmd_Argv(1);
    if (arg[0])
    {
        Cvar_Set("model", arg);
        Cvar_Set("headmodel", arg);
    }
    else
    {
        Cvar_VariableStringBuffer("model", name, sizeof(name));
        Com_Printf("model is set to %s\n", name);
    }
}

//===========================================================================================

/*
===============
CL_Video_f

video
video [filename]
===============
*/
static void CL_Video_f(void)
{
    char filename[MAX_OSPATH];

    if (!clc.demoplaying)
    {
        Com_Printf("The video command can only be used when playing back demos\n");
        return;
    }

    if (Cmd_Argc() == 2)
    {
        // explicit filename
        Com_sprintf(filename, MAX_OSPATH, "videos/%s.avi", Cmd_Argv(1));
    }
    else
    {
        int i, last;

        // scan for a free filename
        for (i = 0; i <= 9999; i++)
        {
            int a, b, c, d;

            last = i;

            a = last / 1000;
            last -= a * 1000;
            b = last / 100;
            last -= b * 100;
            c = last / 10;
            last -= c * 10;
            d = last;

            Com_sprintf(filename, MAX_OSPATH, "videos/video%d%d%d%d.avi", a, b, c, d);

            if (!FS_FileExists(filename)) break;  // file doesn't exist
        }

        if (i > 9999)
        {
            Com_Printf(S_COLOR_RED "ERROR: no free file names to create video\n");
            return;
        }
    }

    CL_OpenAVIForWriting(filename);
}

/*
===============
CL_StopVideo_f
===============
*/
static void CL_StopVideo_f(void) { CL_CloseAVI(); }
/*
===============
CL_GenerateQKey

test to see if a valid QKEY_FILE exists.  If one does not, try to generate
it by filling it with 2048 bytes of random data.
===============
*/
static void CL_GenerateQKey(void)
{
    int len = 0;
    unsigned char buff[QKEY_SIZE];
    fileHandle_t f;

    len = FS_SV_FOpenFileRead(QKEY_FILE, &f);
    FS_FCloseFile(f);
    if (len == QKEY_SIZE)
    {
        Com_Printf("QKEY found.\n");
        return;
    }
    else
    {
        if (len > 0)
        {
            Com_Printf("QKEY file size != %d, regenerating\n", QKEY_SIZE);
        }

        Com_Printf("QKEY building random string\n");
        Com_RandomBytes(buff, sizeof(buff));

        f = FS_SV_FOpenFileWrite(QKEY_FILE);
        if (!f)
        {
            Com_Printf("QKEY could not open %s for write\n", QKEY_FILE);
            return;
        }
        FS_Write(buff, sizeof(buff), f);
        FS_FCloseFile(f);
        Com_Printf("QKEY generated\n");
    }
}

static void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping)
{
    if (server)
    {
        if (info)
        {
            server->clients = atoi(Info_ValueForKey(info, "clients"));
            Q_strncpyz(server->hostName, Info_ValueForKey(info, "hostname"), MAX_HOSTNAME_LENGTH);
            Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH);
            server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
            server->gameType = atoi(Info_ValueForKey(info, "gametype"));
            server->netType = atoi(Info_ValueForKey(info, "nettype"));
            server->minPing = atoi(Info_ValueForKey(info, "minping"));
            server->maxPing = atoi(Info_ValueForKey(info, "maxping"));
            const char *game = Info_ValueForKey(info, "game");
            Q_strncpyz(server->game, (game[0]) ? game : BASEGAME, MAX_NAME_LENGTH);
        }
        server->ping = ping;
    }
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping)
{
    int i;

    for (i = 0; i < MAX_OTHER_SERVERS; i++)
    {
        if (NET_CompareAdr(from, cls.localServers[i].adr))
        {
            CL_SetServerInfo(&cls.localServers[i], info, ping);
        }
    }

    for (i = 0; i < MAX_GLOBAL_SERVERS; i++)
    {
        if (NET_CompareAdr(from, cls.globalServers[i].adr))
        {
            CL_SetServerInfo(&cls.globalServers[i], info, ping);
        }
    }

    for (i = 0; i < MAX_OTHER_SERVERS; i++)
    {
        if (NET_CompareAdr(from, cls.favoriteServers[i].adr))
        {
            CL_SetServerInfo(&cls.favoriteServers[i], info, ping);
        }
    }
}

/*
===================
CL_ServerInfoPacket
===================
*/
static void CL_ServerInfoPacket(netadr_t from, msg_t *msg)
{
    int i, type;
    char info[MAX_INFO_STRING];
    char *infoString;
    int prot;
    char *gamename;
    bool gameMismatch;

    infoString = MSG_ReadString(msg);

    if (from.alternateProtocol == 0)
    {
        // if this isn't the correct gamename, ignore it
        gamename = Info_ValueForKey(infoString, "gamename");

        gameMismatch = !*gamename || strcmp(gamename, com_gamename->string) != 0;

        if (gameMismatch)
        {
            Com_DPrintf("Game mismatch in info packet: %s\n", infoString);
            return;
        }
    }

    // if this isn't the correct protocol version, ignore it
    prot = atoi(Info_ValueForKey(infoString, "protocol"));
    if (prot != (from.alternateProtocol == 0 ? PROTOCOL_VERSION : from.alternateProtocol == 1 ? 70 : 69))
    {
        Com_DPrintf("Different protocol info packet: %s\n", infoString);
        return;
    }

    // iterate servers waiting for ping response
    for (i = 0; i < MAX_PINGREQUESTS; i++)
    {
        if (cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr(from, cl_pinglist[i].adr))
        {
            // calc ping time
            cl_pinglist[i].time = Sys_Milliseconds() - cl_pinglist[i].start;
            Com_DPrintf("ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString(from));

            // save of info
            Q_strncpyz(cl_pinglist[i].info, infoString, sizeof(cl_pinglist[i].info));

            // tack on the net type
            // NOTE: make sure these types are in sync with the netnames strings in the UI
            switch (from.type)
            {
                case NA_BROADCAST:
                case NA_IP:
                    type = 1;
                    break;
                case NA_IP6:
                    type = 2;
                    break;
                default:
                    type = 0;
                    break;
            }
            Info_SetValueForKey(cl_pinglist[i].info, "nettype", va("%d", type));
            CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

            return;
        }
    }

    // if not just sent a local broadcast or pinging local servers
    if (cls.pingUpdateSource != AS_LOCAL)
    {
        return;
    }

    for (i = 0; i < MAX_OTHER_SERVERS; i++)
    {
        // empty slot
        if (cls.localServers[i].adr.port == 0)
        {
            break;
        }

        // avoid duplicate
        if (NET_CompareAdr(from, cls.localServers[i].adr))
        {
            return;
        }
    }

    if (i == MAX_OTHER_SERVERS)
    {
        Com_DPrintf("MAX_OTHER_SERVERS hit, dropping infoResponse\n");
        return;
    }

    // add this to the list
    cls.numlocalservers = i + 1;
    CL_InitServerInfo(&cls.localServers[i], &from);

    Q_strncpyz(info, MSG_ReadString(msg), MAX_INFO_STRING);
    if (strlen(info))
    {
        if (info[strlen(info) - 1] != '\n')
        {
            Q_strcat(info, sizeof(info), "\n");
        }
        Com_Printf("%s: %s", NET_AdrToStringwPort(from), info);
    }
}

/*
===================
CL_ServerStatusResponse
===================
*/
static void CL_ServerStatusResponse(netadr_t from, msg_t *msg)
{
    char info[MAX_INFO_STRING];
    int i, l, score, ping;
    int len;
    serverStatus_t *serverStatus;

    serverStatus = NULL;
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
    {
        if (NET_CompareAdr(from, cl_serverStatusList[i].address))
        {
            serverStatus = &cl_serverStatusList[i];
            break;
        }
    }
    // if we didn't request this server status
    if (!serverStatus)
    {
        return;
    }

    const char *s = MSG_ReadStringLine(msg);

    len = 0;
    Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "%s", s);

    if (serverStatus->print)
    {
        Com_Printf("Server settings:\n");
        // print cvars
        while (*s)
        {
            for (i = 0; i < 2 && *s; i++)
            {
                if (*s == '\\') s++;
                l = 0;
                while (*s)
                {
                    info[l++] = *s;
                    if (l >= MAX_INFO_STRING - 1) break;
                    s++;
                    if (*s == '\\')
                    {
                        break;
                    }
                }
                info[l] = '\0';
                if (i)
                {
                    Com_Printf("%s\n", info);
                }
                else
                {
                    Com_Printf("%-24s", info);
                }
            }
        }
    }

    len = strlen(serverStatus->string);
    Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

    if (serverStatus->print)
    {
        Com_Printf("\nPlayers:\n");
        Com_Printf("num: score: ping: name:\n");
    }
    for (i = 0, s = MSG_ReadStringLine(msg); *s; s = MSG_ReadStringLine(msg), i++)
    {
        len = strlen(serverStatus->string);
        Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\%s", s);

        if (serverStatus->print)
        {
            score = ping = 0;
            sscanf(s, "%d %d", &score, &ping);
            s = strchr(s, ' ');
            if (s) s = strchr(s + 1, ' ');
            if (s)
                s++;
            else
                s = "unknown";
            Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
        }
    }
    len = strlen(serverStatus->string);
    Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

    serverStatus->time = Com_Milliseconds();
    serverStatus->address = from;
    serverStatus->pending = false;
    if (serverStatus->print)
    {
        serverStatus->retrieved = true;
    }
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket(netadr_t from, msg_t *msg)
{
    int challenge = 0;

    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg);  // skip the -1

    const char *s = MSG_ReadStringLine(msg);

    Cmd_TokenizeString(s);

    const char *c = Cmd_Argv(0);

    Com_DPrintf("CL packet %s: %s\n", NET_AdrToStringwPort(from), c);

    // challenge from the server we are connecting to
    if (!Q_stricmp(c, "challengeResponse"))
    {
        int ver;

        if (clc.state != CA_CONNECTING)
        {
            Com_DPrintf("Unwanted challenge response received. Ignored.\n");
            return;
        }

        const char *strver = Cmd_Argv(3);
        if (*strver)
        {
            ver = atoi(strver);

            if (ver != PROTOCOL_VERSION)
            {
                Com_Printf(S_COLOR_YELLOW
                    "Warning: Server reports protocol version %d, we have %d. "
                    "Trying anyways.\n",
                    ver, PROTOCOL_VERSION);
            }
        }
        if (clc.serverAddress.alternateProtocol == 0)
        {
            c = Cmd_Argv(2);
            if (*c) challenge = atoi(c);

            if (!*c || challenge != clc.challenge)
            {
                Com_Printf("Bad challenge for challengeResponse. Ignored.\n");
                return;
            }
        }

        // start sending challenge response instead of challenge request packets
        clc.challenge = atoi(Cmd_Argv(1));
        clc.state = CA_CHALLENGING;
        clc.connectPacketCount = 0;
        clc.connectTime = -99999;

        if (cl_rsaAuth->integer)
        {
            s = Cmd_Argv(4);
            if (*s)
            {
                Q_strncpyz(clc.challenge2, s, sizeof(clc.challenge2));
                clc.sendSignature = true;
            }
        }

        // take this address as the new server address.  This allows
        // a server proxy to hand off connections to multiple servers
        clc.serverAddress = from;
        Com_DPrintf("challengeResponse: %d\n", clc.challenge);
        return;
    }

    // server connection
    if (!Q_stricmp(c, "connectResponse"))
    {
        if (clc.state >= CA_CONNECTED)
        {
            Com_Printf("Dup connect received. Ignored.\n");
            return;
        }
        if (clc.state != CA_CHALLENGING)
        {
            Com_Printf("connectResponse packet while not connecting. Ignored.\n");
            return;
        }
        if (!NET_CompareAdr(from, clc.serverAddress))
        {
            Com_Printf("connectResponse from wrong address. Ignored.\n");
            return;
        }

        if (clc.serverAddress.alternateProtocol == 0)
        {
            c = Cmd_Argv(1);

            if (*c)
                challenge = atoi(c);
            else
            {
                Com_Printf("Bad connectResponse received. Ignored.\n");
                return;
            }

            if (challenge != clc.challenge)
            {
                Com_Printf("ConnectResponse with bad challenge received. Ignored.\n");
                return;
            }
        }

        Netchan_Setup(clc.serverAddress.alternateProtocol, NS_CLIENT, &clc.netchan, from,
            Cvar_VariableValue("net_qport"), clc.challenge);

        clc.state = CA_CONNECTED;
        clc.lastPacketSentTime = -9999;  // send first packet immediately
        return;
    }

    // server responding to an info broadcast
    if (!Q_stricmp(c, "infoResponse"))
    {
        CL_ServerInfoPacket(from, msg);
        return;
    }

    // server responding to a get playerlist
    if (!Q_stricmp(c, "statusResponse"))
    {
        CL_ServerStatusResponse(from, msg);
        return;
    }

    // echo request from server
    if (!Q_stricmp(c, "echo"))
    {
        NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
        return;
    }

    // global MOTD from trem master
    if (!Q_stricmp(c, "motd"))
    {
        CL_MotdPacket(from, s);
        return;
    }

    // echo request from server
    if (!Q_stricmp(c, "print"))
    {
        s = MSG_ReadString(msg);

        Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));

        while (clc.serverMessage[strlen(clc.serverMessage) - 1] == '\n')
            clc.serverMessage[strlen(clc.serverMessage) - 1] = '\0';

        Com_Printf("%s", s);

        return;
    }

    // list of servers sent back by a master server (classic)
    if (!Q_strncmp(c, "getserversResponse", 18))
    {
        CL_ServersResponsePacket(&from, msg, false);

        return;
    }

    // list of servers sent back by a master server (extended)
    if (!Q_strncmp(c, "getserversExtResponse", 21))
    {
        CL_ServersResponsePacket(&from, msg, true);
        return;
    }

    Com_DPrintf("Unknown connectionless packet command.\n");
}

/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent(netadr_t from, msg_t *msg)
{
    int headerBytes;

    clc.lastPacketTime = cls.realtime;

    if (msg->cursize >= 4 && *(int *)msg->data == -1)
    {
        CL_ConnectionlessPacket(from, msg);
        return;
    }

    if (clc.state < CA_CONNECTED)
    {
        return;  // can't be a valid sequenced packet
    }

    if (msg->cursize < 4)
    {
        Com_Printf("%s: Runt packet\n", NET_AdrToStringwPort(from));
        return;
    }

    //
    // packet from server
    //
    if (!NET_CompareAdr(from, clc.netchan.remoteAddress))
    {
        Com_DPrintf("%s:sequenced packet without connection\n", NET_AdrToStringwPort(from));
        // FIXME: send a client disconnect?
        return;
    }

    if (!CL_Netchan_Process(&clc.netchan, msg))
    {
        return;  // out of order, duplicated, etc
    }

    // the header is different lengths for reliable and unreliable messages
    headerBytes = msg->readcount;

    // track the last message received so it can be returned in
    // client messages, allowing the server to detect a dropped
    // gamestate
    clc.serverMessageSequence = LittleLong(*(int *)msg->data);

    clc.lastPacketTime = cls.realtime;
    CL_ParseServerMessage(msg);

    //
    // we don't know if it is ok to save a demo message until
    // after we have parsed the frame
    //
    if (clc.demorecording && !clc.demowaiting)
    {
        CL_WriteDemoMessage(msg, headerBytes);
    }
}

/*
===================
CL_GetServerStatus
===================
*/
static serverStatus_t *CL_GetServerStatus(netadr_t from)
{
    int i, oldest, oldestTime;

    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
    {
        if (NET_CompareAdr(from, cl_serverStatusList[i].address))
        {
            return &cl_serverStatusList[i];
        }
    }
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
    {
        if (cl_serverStatusList[i].retrieved)
        {
            return &cl_serverStatusList[i];
        }
    }
    oldest = -1;
    oldestTime = 0;
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
    {
        if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime)
        {
            oldest = i;
            oldestTime = cl_serverStatusList[i].startTime;
        }
    }
    return &cl_serverStatusList[oldest];
}

/*
===================
CL_ServerStatus
===================
*/
bool CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen)
{
    int i;
    netadr_t to;
    serverStatus_t *serverStatus;

    // if no server address then reset all server status requests
    if (!serverAddress)
    {
        for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
        {
            cl_serverStatusList[i].address.port = 0;
            cl_serverStatusList[i].retrieved = true;
        }
        return false;
    }
    // get the address
    if (!NET_StringToAdr(serverAddress, &to, NA_UNSPEC))
    {
        return false;
    }
    serverStatus = CL_GetServerStatus(to);
    // if no server status string then reset the server status request for this address
    if (!serverStatusString)
    {
        serverStatus->retrieved = true;
        return false;
    }

    // if this server status request has the same address
    if (NET_CompareAdr(to, serverStatus->address))
    {
        // if we received a response for this server status request
        if (!serverStatus->pending)
        {
            Q_strncpyz(serverStatusString, serverStatus->string, maxLen);
            serverStatus->retrieved = true;
            serverStatus->startTime = 0;
            return true;
        }
        // resend the request regularly
        else if (serverStatus->startTime < Com_Milliseconds() - cl_serverStatusResendTime->integer)
        {
            serverStatus->print = false;
            serverStatus->pending = true;
            serverStatus->retrieved = false;
            serverStatus->time = 0;
            serverStatus->startTime = Com_Milliseconds();
            NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
            return false;
        }
    }
    // if retrieved
    else if (serverStatus->retrieved)
    {
        serverStatus->address = to;
        serverStatus->print = false;
        serverStatus->pending = true;
        serverStatus->retrieved = false;
        serverStatus->startTime = Com_Milliseconds();
        serverStatus->time = 0;
        NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
        return false;
    }
    return false;
}

/*
==================
CL_LocalServers_f
==================
*/
static void CL_LocalServers_f(void)
{
    const char *message;
    int i, j;
    netadr_t to;

    Com_Printf("Scanning for servers on the local network...\n");

    // reset the list, waiting for response
    cls.numlocalservers = 0;
    cls.pingUpdateSource = AS_LOCAL;

    for (i = 0; i < MAX_OTHER_SERVERS; i++)
    {
        bool b = cls.localServers[i].visible;
        ::memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
        cls.localServers[i].visible = b;
    }
    ::memset(&to, 0, sizeof(to));

    // The 'xxx' in the message is a challenge that will be echoed back
    // by the server.  We don't care about that here, but master servers
    // can use that to prevent spoofed server responses from invalid ip
    message = "\377\377\377\377getinfo xxx";

    // send each message twice in case one is dropped
    for (i = 0; i < 2; i++)
    {
        // send a broadcast packet on each server port
        // we support multiple server ports so a single machine
        // can nicely run multiple servers
        for (j = 0; j < NUM_SERVER_PORTS; j++)
        {
            to.port = BigShort((short)(PORT_SERVER + j));

            to.type = NA_BROADCAST;
            NET_SendPacket(NS_CLIENT, strlen(message), message, to);
            to.type = NA_MULTICAST6;
            NET_SendPacket(NS_CLIENT, strlen(message), message, to);
        }
    }
}

/*
==================
CL_GlobalServers_f
==================
*/
static void CL_GlobalServers_f(void)
{
    int netAlternateProtocols, a;
    int i;
    char command[1024];
    const char *masteraddress;

    int masterNum;
    int count = Cmd_Argc();
    if ( count < 2 || (masterNum = atoi(Cmd_Argv(1))) < 0 || masterNum > MAX_MASTER_SERVERS )
    {
        Com_Printf("usage: globalservers <master# 0-%d> [keywords]\n", MAX_MASTER_SERVERS);
        return;
    }

    netAlternateProtocols = Cvar_VariableIntegerValue("net_alternateProtocols");

    for (a = 0; a < 3; ++a)
    {
        // indent
        if (a == 0 && (netAlternateProtocols & NET_DISABLEPRIMPROTO)) continue;
        if (a == 1 && !(netAlternateProtocols & NET_ENABLEALT1PROTO)) continue;
        if (a == 2 && !(netAlternateProtocols & NET_ENABLEALT2PROTO)) continue;

        // request from all master servers
        if ( masterNum == 0 )
        {
            int numAddress = 0;

            for ( int i = 1; i <= MAX_MASTER_SERVERS; i++ )
            {
                sprintf(command, "sv_master%d", i);
                masteraddress = Cvar_VariableString(command);

                if(!*masteraddress)
                    continue;

                numAddress++;

                Com_sprintf(command, sizeof(command), "globalservers %d %s %s\n", i, Cmd_Argv(2), Cmd_ArgsFrom(3));
                Cbuf_AddText(command);
            }

            if ( !numAddress )
                Com_Printf("CL_GlobalServers_f: Error: No master server addresses.\n");

            return;
        }

        sprintf(command, "sv_%smaster%d", (a == 0 ? "" : a == 1 ? "alt1" : "alt2"), masterNum);
        masteraddress = Cvar_VariableString(command);

        if (!*masteraddress)
        {
            Com_Printf("CL_GlobalServers_f: Error: No%s master server address given.\n",
                (a == 0 ? "" : a == 1 ? " alternate-1" : " alternate-2"));
            continue;
        }

        // reset the list, waiting for response
        // -1 is used to distinguish a "no response"
        netadr_t to;
        int i = NET_StringToAdr(masteraddress, &to, NA_UNSPEC);

        if ( i == 0 )
        {
            Com_Printf("CL_GlobalServers_f: Error: could not resolve address of%s master %s\n",
                (a == 0 ? "" : a == 1 ? " alternate-1" : " alternate-2"), masteraddress);
            continue;
        }
        else if ( i == 2 )
        {
            to.port = BigShort(a == 0 ? PORT_MASTER : a == 1 ? ALT1PORT_MASTER : ALT2PORT_MASTER);
        }
        to.alternateProtocol = a;

        Com_Printf("Requesting servers from%s master %s...\n",
                a == 0 ? "" : a == 1 ? " alternate-1" : " alternate-2",
                masteraddress);

        cls.numglobalservers = -1;
        cls.pingUpdateSource = AS_GLOBAL;

        Com_sprintf(command, sizeof(command), "getserversExt %s %i%s",
                com_gamename->string,
                a == 0 ? PROTOCOL_VERSION : a == 1 ? 70 : 69,
                Cvar_VariableIntegerValue("net_enabled") & NET_ENABLEV4 ? "" : " ipv6");

        for (i = 3; i < count; i++)
        {
            Q_strcat(command, sizeof(command), " ");
            Q_strcat(command, sizeof(command), Cmd_Argv(i));
        }

        NET_OutOfBandPrint(NS_SERVER, to, "%s", command);
        // outdent
    }
    CL_RequestMotd();
}

/*
==================
CL_GetPing
==================
*/
void CL_GetPing(int n, char *buf, int buflen, int *pingtime)
{
    const char *str;
    int time;
    int maxPing;

    if (n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
    {
        // empty or invalid slot
        buf[0] = '\0';
        *pingtime = 0;
        return;
    }

    str = NET_AdrToStringwPort(cl_pinglist[n].adr);
    Q_strncpyz(buf, str, buflen);

    time = cl_pinglist[n].time;
    if (!time)
    {
        // check for timeout
        time = Sys_Milliseconds() - cl_pinglist[n].start;
        maxPing = Cvar_VariableIntegerValue("cl_maxPing");
        if (maxPing < 100)
        {
            maxPing = 100;
        }
        if (time < maxPing)
        {
            // not timed out yet
            time = 0;
        }
    }

    CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

    *pingtime = time;
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo(int n, char *buf, int buflen)
{
    if (n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
    {
        // empty or invalid slot
        if (buflen) buf[0] = '\0';
        return;
    }

    Q_strncpyz(buf, cl_pinglist[n].info, buflen);
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing(int n)
{
    if (n < 0 || n >= MAX_PINGREQUESTS) return;

    cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount(void)
{
    int i;
    int count;
    ping_t *pingptr;

    count = 0;
    pingptr = cl_pinglist;

    for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
    {
        if (pingptr->adr.port)
        {
            count++;
        }
    }

    return (count);
}

/*
==================
CL_GetFreePing
==================
*/
static ping_t *CL_GetFreePing(void)
{
    ping_t *pingptr;
    ping_t *best;
    int oldest;
    int i;
    int time;

    pingptr = cl_pinglist;
    for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
    {
        // find free ping slot
        if (pingptr->adr.port)
        {
            if (!pingptr->time)
            {
                if (Sys_Milliseconds() - pingptr->start < 500)
                {
                    // still waiting for response
                    continue;
                }
            }
            else if (pingptr->time < 500)
            {
                // results have not been queried
                continue;
            }
        }

        // clear it
        pingptr->adr.port = 0;
        return (pingptr);
    }

    // use oldest entry
    pingptr = cl_pinglist;
    best = cl_pinglist;
    oldest = INT_MIN;
    for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
    {
        // scan for oldest
        time = Sys_Milliseconds() - pingptr->start;
        if (time > oldest)
        {
            oldest = time;
            best = pingptr;
        }
    }

    return (best);
}

/*
==================
CL_Ping_f
==================
*/
static void CL_Ping_f(void)
{
    netadr_t to;
    ping_t *pingptr;
    const char *server;
    int argc;
    netadrtype_t family = NA_UNSPEC;

    argc = Cmd_Argc();

    if (argc != 2 && argc != 3)
    {
        Com_Printf("usage: ping [-4|-6] server\n");
        return;
    }

    if (argc == 2)
        server = Cmd_Argv(1);
    else
    {
        if (!strcmp(Cmd_Argv(1), "-4"))
            family = NA_IP;
        else if (!strcmp(Cmd_Argv(1), "-6"))
            family = NA_IP6;
        else
            Com_Printf("warning: only -4 or -6 as address type understood.\n");

        server = Cmd_Argv(2);
    }

    ::memset(&to, 0, sizeof(netadr_t));

    if (!NET_StringToAdr(server, &to, family))
    {
        return;
    }

    pingptr = CL_GetFreePing();

    memcpy(&pingptr->adr, &to, sizeof(netadr_t));
    pingptr->start = Sys_Milliseconds();
    pingptr->time = 0;

    CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

    NET_OutOfBandPrint(NS_CLIENT, to, "getinfo xxx");
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
bool CL_UpdateVisiblePings_f(int source)
{
    int slots, i;
    char buff[MAX_STRING_CHARS];
    int pingTime;
    int max;
    bool status = false;

    if (source < 0 || source > AS_FAVORITES)
    {
        return false;
    }

    cls.pingUpdateSource = source;

    slots = CL_GetPingQueueCount();
    if (slots < MAX_PINGREQUESTS)
    {
        serverInfo_t *server = NULL;

        switch (source)
        {
            case AS_LOCAL:
                server = &cls.localServers[0];
                max = cls.numlocalservers;
                break;
            case AS_GLOBAL:
                server = &cls.globalServers[0];
                max = cls.numglobalservers;
                break;
            case AS_FAVORITES:
                server = &cls.favoriteServers[0];
                max = cls.numfavoriteservers;
                break;
            default:
                return false;
        }
        for (i = 0; i < max; i++)
        {
            if (server[i].visible)
            {
                if (server[i].ping == -1)
                {
                    int j;

                    if (slots >= MAX_PINGREQUESTS)
                    {
                        break;
                    }
                    for (j = 0; j < MAX_PINGREQUESTS; j++)
                    {
                        if (!cl_pinglist[j].adr.port)
                        {
                            continue;
                        }
                        if (NET_CompareAdr(cl_pinglist[j].adr, server[i].adr))
                        {
                            // already on the list
                            break;
                        }
                    }
                    if (j >= MAX_PINGREQUESTS)
                    {
                        status = true;
                        for (j = 0; j < MAX_PINGREQUESTS; j++)
                        {
                            if (!cl_pinglist[j].adr.port)
                            {
                                break;
                            }
                        }
                        memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
                        cl_pinglist[j].start = Sys_Milliseconds();
                        cl_pinglist[j].time = 0;
                        NET_OutOfBandPrint(NS_CLIENT, cl_pinglist[j].adr, "getinfo xxx");
                        slots++;
                    }
                }
                // if the server has a ping higher than cl_maxPing or
                // the ping packet got lost
                else if (server[i].ping == 0)
                {
                    // if we are updating global servers
                    if (source == AS_GLOBAL)
                    {
                        //
                        if (cls.numGlobalServerAddresses > 0)
                        {
                            // overwrite this server with one from the additional global servers
                            cls.numGlobalServerAddresses--;
                            CL_InitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
                            // NOTE: the server[i].visible flag stays untouched
                        }
                    }
                }
            }
        }
    }

    if (slots)
    {
        status = true;
    }
    for (i = 0; i < MAX_PINGREQUESTS; i++)
    {
        if (!cl_pinglist[i].adr.port)
        {
            continue;
        }
        CL_GetPing(i, buff, MAX_STRING_CHARS, &pingTime);
        if (pingTime != 0)
        {
            CL_ClearPing(i);
            status = true;
        }
    }

    return status;
}

/*
==================
CL_ServerStatus_f
==================
*/
static void CL_ServerStatus_f(void)
{
    netadr_t to, *toptr = NULL;
    const char *server;
    serverStatus_t *serverStatus;
    int argc;
    netadrtype_t family = NA_UNSPEC;

    argc = Cmd_Argc();

    if (argc != 2 && argc != 3)
    {
        if (clc.state != CA_ACTIVE || clc.demoplaying)
        {
            Com_Printf("Not connected to a server.\n");
            Com_Printf("usage: serverstatus [-4|-6] server\n");
            return;
        }

        toptr = &clc.serverAddress;
    }

    if (!toptr)
    {
        ::memset(&to, 0, sizeof(netadr_t));

        if (argc == 2)
            server = Cmd_Argv(1);
        else
        {
            if (!strcmp(Cmd_Argv(1), "-4"))
                family = NA_IP;
            else if (!strcmp(Cmd_Argv(1), "-6"))
                family = NA_IP6;
            else
                Com_Printf("warning: only -4 or -6 as address type understood.\n");

            server = Cmd_Argv(2);
        }

        toptr = &to;
        if (!NET_StringToAdr(server, toptr, family)) return;
    }

    NET_OutOfBandPrint(NS_CLIENT, *toptr, "getstatus");

    serverStatus = CL_GetServerStatus(*toptr);
    serverStatus->address = *toptr;
    serverStatus->print = true;
    serverStatus->pending = true;
}

/*
============
CL_InitRef
============
*/
static void CL_InitRef(void)
{
    refimport_t ri;
    refexport_t *ret;
#ifdef USE_RENDERER_DLOPEN
    GetRefAPI_t GetRefAPI;
    char dllName[MAX_OSPATH];
#endif

    Com_Printf("----- Initializing Renderer ----\n");

#ifdef USE_RENDERER_DLOPEN
    cl_renderer = Cvar_Get("cl_renderer", "opengl1", CVAR_ARCHIVE | CVAR_LATCH);

    Com_sprintf(dllName, sizeof(dllName), "renderer_%s" DLL_EXT, cl_renderer->string);

    if (!(rendererLib = Sys_LoadDll(dllName, false)) && strcmp(cl_renderer->string, cl_renderer->resetString))
    {
        Com_Printf("failed:\n\"%s\"\n", Sys_LibraryError());
        Cvar_ForceReset("cl_renderer");

        Com_sprintf(dllName, sizeof(dllName), "renderer_opengl1" DLL_EXT);
        rendererLib = Sys_LoadDll(dllName, false);
    }

    if (!rendererLib)
    {
        Com_Printf("failed:\n\"%s\"\n", Sys_LibraryError());
        Com_Error(ERR_FATAL, "Failed to load renderer");
    }

    GetRefAPI = (GetRefAPI_t)Sys_LoadFunction(rendererLib, "GetRefAPI");
    if (!GetRefAPI)
    {
        Com_Error(ERR_FATAL, "Can't load symbol GetRefAPI: '%s'", Sys_LibraryError());
    }
#endif

    ri.Cmd_AddCommand = Cmd_AddCommand;
    ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
    ri.Cmd_Argc = Cmd_Argc;
    ri.Cmd_Argv = Cmd_Argv;
    ri.Cmd_ExecuteText = Cbuf_ExecuteText;
    ri.Printf = CL_RefPrintf;
    ri.Error = Com_Error;
    ri.Milliseconds = CL_ScaledMilliseconds;
    ri.Malloc = CL_RefMalloc;
    ri.Free = Z_Free;
#ifdef HUNK_DEBUG
    ri.Hunk_AllocDebug = Hunk_AllocDebug;
#else
    ri.Hunk_Alloc = Hunk_Alloc;
#endif
    ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
    ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;

    ri.CM_ClusterPVS = CM_ClusterPVS;
    ri.CM_DrawDebugSurface = CM_DrawDebugSurface;

    ri.FS_ReadFile = FS_ReadFile;
    ri.FS_FreeFile = FS_FreeFile;
    ri.FS_WriteFile = FS_WriteFile;
    ri.FS_FreeFileList = FS_FreeFileList;
    ri.FS_ListFiles = FS_ListFiles;
    ri.FS_FileIsInPAK = FS_FileIsInPAK;
    ri.FS_FileExists = FS_FileExists;
    ri.Cvar_Get = Cvar_Get;
    ri.Cvar_Set = Cvar_Set;
    ri.Cvar_SetValue = Cvar_SetValue;
    ri.Cvar_CheckRange = Cvar_CheckRange;
    ri.Cvar_SetDescription = Cvar_SetDescription;
    ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;

    // cinematic stuff

    ri.CIN_UploadCinematic = CIN_UploadCinematic;
    ri.CIN_PlayCinematic = CIN_PlayCinematic;
    ri.CIN_RunCinematic = CIN_RunCinematic;

    ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

    ri.IN_Init = IN_Init;
    ri.IN_Shutdown = IN_Shutdown;
    ri.IN_Restart = IN_Restart;

    ri.Sys_GLimpSafeInit = Sys_GLimpSafeInit;
    ri.Sys_GLimpInit = Sys_GLimpInit;
    ri.Sys_LowPhysicalMemory = Sys_LowPhysicalMemory;

    ret = GetRefAPI(REF_API_VERSION, &ri);

#if defined __USEA3D && defined __A3D_GEOM
    hA3Dg_ExportRenderGeom(ret);
#endif

    Com_Printf("-------------------------------\n");

    if (!ret)
    {
        Com_Error(ERR_FATAL, "Couldn't initialize refresh");
    }

    re = *ret;

    // unpause so the cgame definately gets a snapshot and renders a frame
    Cvar_Set("cl_paused", "0");
}

/*
====================
CL_ProtocolSpecificCommandsInit

For adding/remove commands that depend on a/some
specific protocols, whenever the protcol may change
====================
*/
void CL_ProtocolSpecificCommandsInit(void)
{
  Con_MessageModesInit();
}

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
    Com_Printf("----- Client Initialization -----\n");

    Con_Init();

    if (!com_fullyInitialized)
    {
        CL_ClearState();
        clc.state = CA_DISCONNECTED;  // no longer CA_UNINITIALIZED
        cl_oldGameSet = false;
    }

    CL_InitInput();

    //
    // register our variables
    //
    cl_noprint = Cvar_Get("cl_noprint", "0", 0);
    cl_motd = Cvar_Get("cl_motd", "1", 0);

    cl_timeout = Cvar_Get("cl_timeout", "200", 0);

    cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
    cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
    cl_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
    cl_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
    cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
    rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
    cl_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);

    cl_timedemo = Cvar_Get("timedemo", "0", 0);
    cl_timedemoLog = Cvar_Get("cl_timedemoLog", "", CVAR_ARCHIVE);
    cl_autoRecordDemo = Cvar_Get("cl_autoRecordDemo", "0", CVAR_ARCHIVE);
    cl_aviFrameRate = Cvar_Get("cl_aviFrameRate", "25", CVAR_ARCHIVE);
    cl_aviMotionJpeg = Cvar_Get("cl_aviMotionJpeg", "1", CVAR_ARCHIVE);
    cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

    rconAddress = Cvar_Get("rconAddress", "", 0);

    cl_yawspeed = Cvar_Get("cl_yawspeed", "140", CVAR_ARCHIVE);
    cl_pitchspeed = Cvar_Get("cl_pitchspeed", "140", CVAR_ARCHIVE);
    cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);

    cl_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
    cl_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

    cl_run = Cvar_Get("cl_run", "1", CVAR_ARCHIVE);
    cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE);
    cl_mouseAccel = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE);
    cl_freelook = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE);

    // 0: legacy mouse acceleration
    // 1: new implementation
    cl_mouseAccelStyle = Cvar_Get("cl_mouseAccelStyle", "0", CVAR_ARCHIVE);
    // offset for the power function (for style 1, ignored otherwise)
    // this should be set to the max rate value
    cl_mouseAccelOffset = Cvar_Get("cl_mouseAccelOffset", "5", CVAR_ARCHIVE);
    Cvar_CheckRange(cl_mouseAccelOffset, 0.001f, 50000.0f, false);

    cl_showMouseRate = Cvar_Get("cl_showmouserate", "0", 0);

    cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE);

    if (cl_allowDownload->integer != -1) cl_allowDownload->integer = DLF_ENABLE;

    com_downloadPrompt = Cvar_Get("com_downloadPrompt", "0", CVAR_ROM);
    Cvar_Get("com_downloadPromptText", "", CVAR_TEMP);

    cl_conXOffset = Cvar_Get("cl_conXOffset", "0", 0);
#ifdef __APPLE__
    // In game video is REALLY slow in Mac OS X right now due to driver slowness
    cl_inGameVideo = Cvar_Get("r_inGameVideo", "0", CVAR_ARCHIVE);
#else
    cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);
#endif

    cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);

    m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
    m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
    m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE);
    m_side = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE);
#ifdef __APPLE__
    // Input is jittery on OS X w/o this
    m_filter = Cvar_Get("m_filter", "1", CVAR_ARCHIVE);
#else
    m_filter = Cvar_Get("m_filter", "0", CVAR_ARCHIVE);
#endif

    j_pitch = Cvar_Get("j_pitch", "0.022", CVAR_ARCHIVE);
    j_yaw = Cvar_Get("j_yaw", "-0.022", CVAR_ARCHIVE);
    j_forward = Cvar_Get("j_forward", "-0.25", CVAR_ARCHIVE);
    j_side = Cvar_Get("j_side", "0.25", CVAR_ARCHIVE);
    j_up = Cvar_Get("j_up", "0", CVAR_ARCHIVE);

    j_pitch_axis = Cvar_Get("j_pitch_axis", "3", CVAR_ARCHIVE);
    j_yaw_axis = Cvar_Get("j_yaw_axis", "2", CVAR_ARCHIVE);
    j_forward_axis = Cvar_Get("j_forward_axis", "1", CVAR_ARCHIVE);
    j_side_axis = Cvar_Get("j_side_axis", "0", CVAR_ARCHIVE);
    j_up_axis = Cvar_Get("j_up_axis", "4", CVAR_ARCHIVE);

    Cvar_CheckRange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    Cvar_CheckRange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    Cvar_CheckRange(j_forward_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    Cvar_CheckRange(j_side_axis, 0, MAX_JOYSTICK_AXIS - 1, true);
    Cvar_CheckRange(j_up_axis, 0, MAX_JOYSTICK_AXIS - 1, true);

    cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

    Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);

    cl_lanForcePackets = Cvar_Get("cl_lanForcePackets", "1", CVAR_ARCHIVE);

    cl_guidServerUniq = Cvar_Get("cl_guidServerUniq", "1", CVAR_ARCHIVE);

    cl_rsaAuth = Cvar_Get("cl_rsaAuth", "0", CVAR_INIT | CVAR_PROTECTED);

    // ~ and `, as keys and characters
    cl_consoleKeys = Cvar_Get("cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE);

    cl_clantag = Cvar_Get ("cl_clantag", "", CVAR_ARCHIVE);

    // userinfo
    Cvar_Get(
      "name", "UnnamedPlayer",
      CVAR_USERINFO | CVAR_ARCHIVE| CVAR_REMOVE_UNUSED_COLOR_STRINGS);
    cl_rate = Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("snaps", "40", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("color1", "4", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);

    Cvar_Get("password", "", CVAR_USERINFO);

#ifdef USE_MUMBLE
    cl_useMumble = Cvar_Get("cl_useMumble", "0", CVAR_ARCHIVE | CVAR_LATCH);
    cl_mumbleScale = Cvar_Get("cl_mumbleScale", "0.0254", CVAR_ARCHIVE);
#endif

#ifdef USE_VOIP
    cl_voipSend = Cvar_Get("cl_voipSend", "0", 0);
    cl_voipSendTarget = Cvar_Get("cl_voipSendTarget", "spatial", 0);
    cl_voipGainDuringCapture = Cvar_Get("cl_voipGainDuringCapture", "0.2", CVAR_ARCHIVE);
    cl_voipCaptureMult = Cvar_Get("cl_voipCaptureMult", "2.0", CVAR_ARCHIVE);
    cl_voipUseVAD = Cvar_Get("cl_voipUseVAD", "0", CVAR_ARCHIVE);
    cl_voipVADThreshold = Cvar_Get("cl_voipVADThreshold", "0.25", CVAR_ARCHIVE);
    cl_voipShowMeter = Cvar_Get("cl_voipShowMeter", "1", CVAR_ARCHIVE);

    cl_voip = Cvar_Get("cl_voip", "1", CVAR_ARCHIVE);
    Cvar_CheckRange(cl_voip, 0, 1, true);
    cl_voipProtocol = Cvar_Get("cl_voipProtocol", cl_voip->integer ? "opus" : "", CVAR_USERINFO | CVAR_ROM);
#endif

    // cgame might not be initialized before menu is used
    Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);
    // Make sure cg_stereoSeparation is zero as that variable is deprecated and should not be used anymore.
    Cvar_Get("cg_stereoSeparation", "0", CVAR_ROM);

    //
    // register our commands
    //
    Cmd_AddCommand("cmd", CL_ForwardToServer_f);
    Cmd_AddCommand("configstrings", CL_Configstrings_f);
    Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
    Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
    Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
    Cmd_AddCommand("disconnect", CL_Disconnect_f);
    Cmd_AddCommand("record", CL_Record_f);
    Cmd_AddCommand("demo", CL_PlayDemo_f);
    Cmd_SetCommandCompletionFunc("demo", CL_CompleteDemoName);
    Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
    Cmd_AddCommand("stoprecord", CL_StopRecord_f);
    Cmd_AddCommand("connect", CL_Connect_f);
    Cmd_AddCommand("reconnect", CL_Reconnect_f);
    Cmd_AddCommand("localservers", CL_LocalServers_f);
    Cmd_AddCommand("globalservers", CL_GlobalServers_f);
    Cmd_AddCommand("rcon", CL_Rcon_f);
    Cmd_SetCommandCompletionFunc("rcon", CL_CompleteRcon);
    Cmd_AddCommand("ping", CL_Ping_f);
    Cmd_AddCommand("serverstatus", CL_ServerStatus_f);
    Cmd_AddCommand("showip", CL_ShowIP_f);
    Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f);
    Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f);
    Cmd_AddCommand("model", CL_SetModel_f);
    Cmd_AddCommand("video", CL_Video_f);
    Cmd_AddCommand("stopvideo", CL_StopVideo_f);
    Cmd_AddCommand("downloadUpdate", CL_DownloadUpdate_f);
    Cmd_AddCommand("installUpdate", CL_InstallUpdate_f);
    Cmd_AddCommand("checkForUpdate", CL_CheckForUpdate_f);
    Cmd_AddCommand("browseHomepath", CL_BrowseHomepath_f);
    Cmd_AddCommand("browseDemos", CL_BrowseDemos_f);
    Cmd_AddCommand("browseScreenShots", CL_BrowseScreenShots_f);

    CL_InitRef();

    SCR_Init();

    //	Cbuf_Execute ();

    Cvar_Set("cl_running", "1");

    if (cl_rsaAuth->integer) CL_LoadRSAKeypair();

    CL_GenerateQKey();
    Cvar_Get("cl_guid", "", CVAR_USERINFO | CVAR_ROM);
    if (clc.state == CA_DISCONNECTED) CL_UpdateGUID(NULL, 0);

    Com_Printf("----- Client Initialization Complete -----\n");
}

/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown(const char *finalmsg, bool disconnect, bool quit)
{
    static bool recursive = false;
    int realtime;

    // check whether the client is running at all.
    if (!(com_cl_running && com_cl_running->integer)) return;

    Com_Printf("----- Client Shutdown (%s) -----\n", finalmsg);

    if (recursive)
    {
        Com_Printf("WARNING: Recursive shutdown\n");
        return;
    }
    recursive = true;

    Com_Bucket_Destroy_All_Buckets( );

    noGameRestart = quit;

    if (disconnect) CL_Disconnect(true);

    CL_ClearMemory(true);
    CL_Snd_Shutdown();

    Cmd_RemoveCommand("cmd");
    Cmd_RemoveCommand("configstrings");
    Cmd_RemoveCommand("clientinfo");
    Cmd_RemoveCommand("snd_restart");
    Cmd_RemoveCommand("vid_restart");
    Cmd_RemoveCommand("disconnect");
    Cmd_RemoveCommand("record");
    Cmd_RemoveCommand("demo");
    Cmd_RemoveCommand("cinematic");
    Cmd_RemoveCommand("stoprecord");
    Cmd_RemoveCommand("connect");
    Cmd_RemoveCommand("reconnect");
    Cmd_RemoveCommand("localservers");
    Cmd_RemoveCommand("globalservers");
    Cmd_RemoveCommand("rcon");
    Cmd_RemoveCommand("ping");
    Cmd_RemoveCommand("serverstatus");
    Cmd_RemoveCommand("showip");
    Cmd_RemoveCommand("fs_openedList");
    Cmd_RemoveCommand("fs_referencedList");
    Cmd_RemoveCommand("model");
    Cmd_RemoveCommand("video");
    Cmd_RemoveCommand("stopvideo");

    CL_ShutdownInput();
    Con_Shutdown();

    Cvar_Set("cl_running", "0");

    recursive = false;

    if (cl_rsaAuth->integer) CL_UnloadRSAKeypair();

    realtime = cls.realtime;
    ::memset(&cls, 0, sizeof(cls));
    cls.realtime = realtime;
    Key_SetCatcher(0);

    Com_Printf("-----------------------\n");
}
