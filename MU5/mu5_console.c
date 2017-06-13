/* mu5_console.c: MU5 Console unit

Copyright (c) 2016-2017, Robert Jarratt
Portions (c) Charles Petzold, 1998 (the audio code, taken from https://www-user.tu-chemnitz.de/~heha/petzold/ch22c.htm)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
ROBERT JARRATT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Robert Jarratt shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from Robert Jarratt.

This is the MU5 Console unit.

Known Limitations
-----------------
Only does Teletype output.

*/

#include "mu5_defs.h"
#include "mu5_sac.h"
#include "mu5_console.h"
#if defined (_WIN32)
#include <windows.h>
#endif

#define MASK_FCI 0x1
#define MASK_TCI 0x2
#define MASK_TEII 0x4
#define MASK_SCI 0x8

#define MASK_TTY_INPUT 0x20

#define AUDIO_SAMPLE_RATE     11025
#define AUDIO_OUT_BUFFER_SIZE  1

static t_stat console_reset(DEVICE *dptr);
static t_stat console_svc(UNIT *uptr);
static void console_schedule_next_poll(UNIT *uptr);

static t_uint64 console_read_console_interrupt_callback(uint8 line);
static void console_write_console_interrupt_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_date_upper_hooter_callback(uint8 line);
static void console_write_date_upper_hooter_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_teletype_data_callback(uint8 line);
static void console_write_teletype_data_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_teletype_control_callback(uint8 line);
static void console_write_teletype_control_callback(uint8 line, t_uint64 value);

static t_stat OpenAudioOutput(void);
static void PlayAudioOutput(uint8 hoot);
void CloseAudioOutput(void);

static uint8 ConsoleInterrupt;
static uint8 TeletypeData;
static uint8 TeletypeControl;
static int TeletypeOperationInProgress;

#if defined (_WIN32)
static HWAVEOUT     hWaveOut;
static PBYTE        pBuffer1, pBuffer2;
static PWAVEHDR     pWaveHdr1, pWaveHdr2;
static WAVEFORMATEX waveformat;
#endif

static UNIT console_unit =
{
    UDATA(console_svc, UNIT_FIX | UNIT_BINK, 0)
};

static REG console_reg[] =
{
    { NULL }
};

static MTAB console_mod[] =
{
    { 0 }
};

static DEBTAB console_debtab[] =
{
    { "EVENT",          SIM_DBG_EVENT,     "event dispatch activities" },
    { "SELFTESTDETAIL", LOG_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_SELFTEST_FAIL,  "self test failure output" },
    { NULL,           0 }
};

static const char* console_description(DEVICE *dptr) {
    return "Console Unit";
}

DEVICE console_dev = {
    "TTY",                /* name */
    &console_unit,        /* units */
    console_reg,          /* registers */
    console_mod,          /* modifiers */
    1,                    /* numunits */
    16,                   /* aradix */
    32,                   /* awidth */
    1,                    /* aincr */
    16,                   /* dradix */
    32,                   /* dwidth */
    NULL,                 /* examine */
    NULL,                 /* deposit */
    &console_reset,       /* reset */
    NULL,                 /* boot */
    NULL,                 /* attach */
    NULL,                 /* detach */
    NULL,                 /* ctxt */
    DEV_DEBUG,            /* flags */
    0,                    /* dctrl */
    console_debtab,       /* debflags */
    NULL,                 /* msize */
    NULL,                 /* lname */
    NULL,                 /* help */
    NULL,                 /* attach_help */
    NULL,                 /* help_ctx */
    &console_description, /* description */
    NULL                  /* brk_types */
};

static t_stat console_reset(DEVICE *dptr)
{
    t_stat result = SCPE_OK;
	OpenAudioOutput(); // TODO: Proper open somewhere
    console_reset_state();
    sim_cancel(&console_unit);
    sim_activate(&console_unit, 1);
    return result;
}

void console_reset_state(void)
{
    ConsoleInterrupt = 0;
    TeletypeData = 0;
    TeletypeControl = 0;
    TeletypeOperationInProgress = 0;
    sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_CONSOLE_INTERRUPT, console_read_console_interrupt_callback, console_write_console_interrupt_callback);
	sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_DATE_UPPER_HOOTER, console_read_date_upper_hooter_callback, console_write_date_upper_hooter_callback);
    sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_DATA, console_read_teletype_data_callback, console_write_teletype_data_callback);
    sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_CONTROL, console_read_teletype_control_callback, console_write_teletype_control_callback);
}

static t_stat console_svc(UNIT *uptr)
{
    if (TeletypeOperationInProgress)
    {
        if (!(TeletypeControl & MASK_TTY_INPUT))
        {
            sim_putchar(TeletypeData);
            ConsoleInterrupt |= MASK_TCI;
            /* TODO actually set interrupt here, for now just enable polling */
        }
        TeletypeOperationInProgress = 0;
    }
    console_schedule_next_poll(uptr);
    return SCPE_OK;
}

static void console_schedule_next_poll(UNIT *uptr)
{
    sim_activate_after_abs(uptr, 72727); /* 110 baud is 1 character every 1/13.75 seconds, which is 72727 microseconds */
}

static t_uint64 console_read_console_interrupt_callback(uint8 line)
{
    return ConsoleInterrupt & 0xF;
}

static void console_write_console_interrupt_callback(uint8 line, t_uint64 value)
{
    if (value & MASK_TCI)
    {
        ConsoleInterrupt &= ~MASK_TCI;
    }
    else if (value & MASK_FCI)
    {
        ConsoleInterrupt &= ~(MASK_SCI | MASK_FCI);
    }
}

static t_uint64 console_read_date_upper_hooter_callback(uint8 line)
{
	// TODO: not implemented yet
	return 0;
}

static void console_write_date_upper_hooter_callback(uint8 line, t_uint64 value)
{
	if ((value & 0x1) == 0)
	{
		PlayAudioOutput(0);
	}
	else
	{
		PlayAudioOutput(255);
	}
}

static t_uint64 console_read_teletype_data_callback(uint8 line)
{
    return TeletypeData;
}

static void console_write_teletype_data_callback(uint8 line, t_uint64 value)
{
    TeletypeData = value & 0xFF;
    TeletypeOperationInProgress = 1;
}

static t_uint64 console_read_teletype_control_callback(uint8 line)
{
    return TeletypeControl;
}

static void console_write_teletype_control_callback(uint8 line, t_uint64 value)
{
    TeletypeControl = value & 0xFF;
}

#if defined (_WIN32)

static t_stat OpenAudioOutput(void)
{
	t_stat result = SCPE_OK;

	pWaveHdr1 = malloc(sizeof(WAVEHDR));
	pWaveHdr2 = malloc(sizeof(WAVEHDR));
	pBuffer1 = malloc(AUDIO_OUT_BUFFER_SIZE);
	pBuffer2 = malloc(AUDIO_OUT_BUFFER_SIZE);

	if (!pWaveHdr1 || !pWaveHdr2 || !pBuffer1 || !pBuffer2)
	{
		if (!pWaveHdr1) free(pWaveHdr1);
		if (!pWaveHdr2) free(pWaveHdr2);
		if (!pBuffer1)  free(pBuffer1);
		if (!pBuffer2)  free(pBuffer2);
		result = SCPE_MEM;
	}
	else
	{
		waveformat.wFormatTag = WAVE_FORMAT_PCM;
		waveformat.nChannels = 1;
		waveformat.nSamplesPerSec = AUDIO_SAMPLE_RATE;
		waveformat.nAvgBytesPerSec = AUDIO_SAMPLE_RATE;
		waveformat.nBlockAlign = 1;
		waveformat.wBitsPerSample = 8;
		waveformat.cbSize = 0;

		if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveformat, (DWORD_PTR)NULL, (DWORD_PTR)NULL, CALLBACK_NULL) != MMSYSERR_NOERROR)
		{
			free(pWaveHdr1);
			free(pWaveHdr2);
			free(pBuffer1);
			free(pBuffer2);
			hWaveOut = NULL;
			result = SCPE_OPENERR;
		}
	}

	if (result == SCPE_OK)
	{
		// Set up headers and prepare them

		pWaveHdr1->lpData = pBuffer1;
		pWaveHdr1->dwBufferLength = AUDIO_OUT_BUFFER_SIZE;
		pWaveHdr1->dwBytesRecorded = 0;
		pWaveHdr1->dwUser = 0;
		pWaveHdr1->dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
		pWaveHdr1->dwLoops = MAXINT32 /* 1 */;
		pWaveHdr1->lpNext = NULL;
		pWaveHdr1->reserved = 0;

		waveOutPrepareHeader(hWaveOut, pWaveHdr1, sizeof(WAVEHDR));

		pWaveHdr2->lpData = pBuffer2;
		pWaveHdr2->dwBufferLength = AUDIO_OUT_BUFFER_SIZE;
		pWaveHdr2->dwBytesRecorded = 0;
		pWaveHdr2->dwUser = 0;
		pWaveHdr2->dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
		pWaveHdr2->dwLoops = MAXINT32 /* 1 */;
		pWaveHdr2->lpNext = NULL;
		pWaveHdr2->reserved = 0;

		waveOutPrepareHeader(hWaveOut, pWaveHdr2, sizeof(WAVEHDR));
	}

	return result;
}

static void PlayAudioOutput(uint8 hoot)
{
	waveOutReset(hWaveOut);
	pBuffer1[0] = hoot;
	waveOutWrite(hWaveOut, pWaveHdr1, sizeof(WAVEHDR));
}

void CloseAudioOutput(void)
{
	waveOutReset(hWaveOut);
	waveOutUnprepareHeader(hWaveOut, pWaveHdr1, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, pWaveHdr2, sizeof(WAVEHDR));

	free(pWaveHdr1);
	free(pWaveHdr2);
	free(pBuffer1);
	free(pBuffer2);
	waveOutClose(hWaveOut);
}
#else
static t_stat OpenAudioOutput(void)
{
	return SCPE_NOFNC;
}

static void PlayAudioOutput(uint8 hoot)
{
}

void CloseAudioOutput(void)
{
}

#endif