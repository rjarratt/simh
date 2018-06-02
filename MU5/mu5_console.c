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

#include <assert.h>
#include "mu5_defs.h"
#include "mu5_cpu.h"
#include "mu5_sac.h"
#include "mu5_console.h"
#if defined(HAVE_LIBSDL)
#include <SDL.h>
#include <SDL_audio.h>
#endif

#define MASK_FCI 0x1
#define MASK_TCI 0x2
#define MASK_TEII 0x4
#define MASK_SCI 0x8

#define MASK_TTY_INPUT 0x20

#define AUDIO_SAMPLE_RATE     22050
#define AUDIO_OUT_BUFFER_SIZE  2048 /* Also the lag as a proportion of the sample rate */

static t_stat console_reset(DEVICE *dptr);
static t_stat console_detach(UNIT* uptr);
static t_stat console_svc(UNIT *uptr);
static t_stat console_set_hooter(UNIT* uptr, int32 val, CONST char* cptr, void* desc);
static t_stat console_show_hooter(FILE* st, UNIT* uptr, int32 val, CONST void* desc);

static void console_schedule_next_poll(UNIT *uptr);

static void console_v_store_register_read_callback(struct REG *reg, int index);
static void console_v_store_register_write_callback(t_value old_val, struct REG *reg, int index);
static void console_time_upper_read_callback(struct REG *reg, int index);
static void console_time_lower_read_callback(struct REG *reg, int index);

static t_uint64 console_read_console_interrupt_callback(uint8 line);
static void console_write_console_interrupt_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_time_upper_callback(uint8 line);
static t_uint64 console_read_time_lower_callback(uint8 line);
static t_uint64 console_read_date_lower_callback(uint8 line);
static t_uint64 console_read_date_upper_hooter_callback(uint8 line);
static void console_write_date_upper_hooter_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_teletype_data_callback(uint8 line);
static void console_write_teletype_data_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_teletype_control_callback(uint8 line);
static void console_write_teletype_control_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_engineers_handswitches_callback(uint8 line);
static void console_write_engineers_handswitches_callback(uint8 line, t_uint64 value);

static uint8 console_convert_to_bcd(uint8 n);
static struct tm *console_get_local_time(void);

static t_stat StartAudioOutput(void);
static void StopAudioOutput(void);

static int TeletypeOperationInProgress;
static t_uint64 *ConsoleInterrupt;
static t_uint64 *TeletypeData;
static t_uint64 *TeletypeControl;
static t_uint64 *EngineersHandswitches;
static t_uint64 *TimeLower;
static t_uint64 *TimeUpper;
static t_uint64 *DateLower;
static t_uint64 *DateUpper;
static t_uint64 *ConsoleHoot;

static int enable_hooter = 0;
static volatile int terminate_thread = 0;

#if defined(HAVE_LIBSDL)
static volatile SDL_AudioDeviceID audio_device_handle = 0;
SDL_Thread *audio_thread_handle = NULL;
static int AudioThreadFunction(void *);
#elif defined (_WIN32)
static HWAVEOUT hWaveOut = INVALID_HANDLE_VALUE;
static volatile PBYTE pBuffer1, pBuffer2;
static volatile PWAVEHDR pWaveHdr1, pWaveHdr2;
static HANDLE audio_thread_handle = INVALID_HANDLE_VALUE;
static DWORD WINAPI audio_thread_function(void *param);
static void audio_cleanup_buffers(void);
#endif

static UNIT console_unit =
{
	UDATA(console_svc, 0, 0)
};

static REG console_reg[] =
{
    { STRDATADFC(V,        VStore[CONSOLE_V_STORE_BLOCK],    16, 64, 0, V_STORE_BLOCK_SIZE, sizeof(VSTORE_LINE), 0, "V Store", NULL, console_v_store_register_read_callback, console_v_store_register_write_callback) },
	{ HRDATADFC(TIMEUPPER, VStore[CONSOLE_V_STORE_BLOCK][2],     64, "Time Upper register", NULL, console_time_upper_read_callback, NULL), REG_HRO }, /* Needed because remote console can't sample array registers. Hidden because it is not the mechanism to be used for setting it, only used for sampling by remote console */
	{ HRDATADFC(TIMELOWER, VStore[CONSOLE_V_STORE_BLOCK][3],     64, "Time Lower register", NULL, console_time_lower_read_callback, NULL), REG_HRO }, /* Needed because remote console can't sample array registers. Hidden because it is not the mechanism to be used for setting it, only used for sampling by remote console */
	{ NULL }
};

static MTAB console_mod[] =
{
    { MTAB_XTD | MTAB_VDV | MTAB_VALR, 0, "HOOTER", "HOOTER={ON|OFF}", &console_set_hooter, &console_show_hooter, NULL, "Hooter" },
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
    "CON",                /* name */
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
    &console_detach,      /* detach */
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
	StopAudioOutput();
    console_reset_state();
	sim_cancel(&console_unit);
	sim_activate(&console_unit, 1);
	return result;
}

static t_stat console_detach(UNIT* uptr)
{
	StopAudioOutput();
	return SCPE_OK;
}

void console_reset_state(void)
{
    TeletypeOperationInProgress = 0;
    ConsoleInterrupt = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_CONSOLE_INTERRUPT, console_read_console_interrupt_callback, console_write_console_interrupt_callback);
	TimeUpper = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TIME_UPPER, console_read_time_upper_callback, NULL);
	TimeLower = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TIME_LOWER, console_read_time_lower_callback, NULL);
	DateLower = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_DATE_LOWER, console_read_date_lower_callback, NULL);
    DateUpper = ConsoleHoot = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_DATE_UPPER_HOOTER, console_read_date_upper_hooter_callback, console_write_date_upper_hooter_callback);
    TeletypeData = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_DATA, console_read_teletype_data_callback, console_write_teletype_data_callback);
    TeletypeControl = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_CONTROL, console_read_teletype_control_callback, console_write_teletype_control_callback);
    EngineersHandswitches = sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_ENGINEERS_HANDSWITCHES, console_read_engineers_handswitches_callback, console_write_engineers_handswitches_callback);
    *ConsoleInterrupt = 0;
    *TeletypeData = 0;
    *TeletypeControl = 0;
}

static t_stat console_svc(UNIT *uptr)
{
	if (TeletypeOperationInProgress)
	{
		if (!(*TeletypeControl & MASK_TTY_INPUT))
		{
			sim_putchar((int32)(*TeletypeData));
			*ConsoleInterrupt |= MASK_TCI;
            cpu_set_console_peripheral_window_interrupt(0);
        }

		TeletypeOperationInProgress = 0;
	}
	console_schedule_next_poll(uptr);
	return SCPE_OK;
}

static t_stat console_set_hooter(UNIT* uptr, int32 val, CONST char* cptr, void* desc)
{
    t_stat result = SCPE_OK;

    if (cptr == NULL)
    {
        result = SCPE_ARG;
    }
    else if (!strcmp(cptr, "ON"))
    {
        enable_hooter = 1;
        StartAudioOutput();
    }
    else if (!strcmp(cptr, "OFF"))
    {
        enable_hooter = 0;
        StopAudioOutput();
    }
    else
    {
        result = SCPE_ARG;
    }

    return result;
}

static t_stat console_show_hooter(FILE* st, UNIT* uptr, int32 val, CONST void* desc)
{
    fprintf(st, "hooter=%s", enable_hooter ? "on" : "off");
    return SCPE_OK;
}

static void console_schedule_next_poll(UNIT *uptr)
{
	sim_activate_after_abs(uptr, 72727); /* 110 baud is 1 character every 1/13.75 seconds, which is 72727 microseconds */
}

static t_uint64 console_read_console_interrupt_callback(uint8 line)
{
    return *ConsoleInterrupt & 0xF;
}

static void console_write_console_interrupt_callback(uint8 line, t_uint64 value)
{
    if (value & MASK_TCI)
    {
        *ConsoleInterrupt &= ~MASK_TCI;
    }
    else if (value & MASK_FCI)
    {
        *ConsoleInterrupt &= ~(MASK_SCI | MASK_FCI);
    }
}

static t_uint64 console_read_time_upper_callback(uint8 line)
{
	struct tm *t = console_get_local_time();
    *TimeUpper = console_convert_to_bcd(t->tm_hour) << 8 | console_convert_to_bcd(t->tm_min);
	*TimeLower = console_convert_to_bcd(t->tm_sec) << 8; /* TODO: milleseconds */
	return *TimeUpper;
}

static t_uint64 console_read_time_lower_callback(uint8 line)
{
	return *TimeLower;
}

static t_uint64 console_read_date_lower_callback(uint8 line)
{
	struct tm *t = console_get_local_time();
	*DateLower = console_convert_to_bcd(t->tm_mon + 1) << 8 | console_convert_to_bcd(t->tm_mday);

	return *DateLower;
}

static t_uint64 console_read_date_upper_hooter_callback(uint8 line)
{
	/* assume we are in the early 2000's when 50 years is close to the time when MU5 existed. 50 years is a span where the years share the same days of the week for the same dates */
	uint8 year = (console_get_local_time()->tm_year - 50) % 100;
	*DateUpper = console_convert_to_bcd(year);
	return *DateUpper;
}

static void console_write_date_upper_hooter_callback(uint8 line, t_uint64 value)
{
	if ((value & 0x1) == 0)
	{
		*ConsoleHoot = 0;
	}
	else
	{
		*ConsoleHoot = 255;
	}
}

static t_uint64 console_read_teletype_data_callback(uint8 line)
{
    return *TeletypeData;
}

static void console_write_teletype_data_callback(uint8 line, t_uint64 value)
{
    *TeletypeData = value & MASK_8;
    TeletypeOperationInProgress = 1;
}

static t_uint64 console_read_teletype_control_callback(uint8 line)
{
    return *TeletypeControl;
}

static void console_write_teletype_control_callback(uint8 line, t_uint64 value)
{
    *TeletypeControl = value & MASK_8;
}

static t_uint64 console_read_engineers_handswitches_callback(uint8 line)
{
	return *EngineersHandswitches;
}

/* The real machine does NOT support writing to this V-Store location, but this is a mechanism for the handswitches to be set from the ini file without a special command
   TODO: Add special command for setting the handswitches
*/
static void console_write_engineers_handswitches_callback(uint8 line, t_uint64 value)
{
	*EngineersHandswitches = value & MASK_16;
}

static uint8 console_convert_to_bcd(uint8 n)
{
	uint8 result = (n / 10) << 4 | (n % 10);
	return result;
}

static struct tm *console_get_local_time(void)
{
	time_t now;
	time(&now);
	return localtime(&now);
}

static void console_v_store_register_read_callback(struct REG *reg, int index)
{
    assert(reg->width == 64);
    sac_read_v_store(CONSOLE_V_STORE_BLOCK, index);
}

static void console_v_store_register_write_callback(t_value old_val, struct REG *reg, int index)
{
    assert(reg->width == 64);
    sac_write_v_store(CONSOLE_V_STORE_BLOCK, index, ((VSTORE_LINE *)reg->loc + index)->value);
}

static void console_time_upper_read_callback(struct REG *reg, int index)
{
	console_v_store_register_read_callback(reg, 2);
}

static void console_time_lower_read_callback(struct REG *reg, int index)
{
	console_v_store_register_read_callback(reg, 3);
}

/* TODO: Audio code still has a crackle, possibly because at buffer changeover the old buffer has finished playing before the new one is queued */
#if defined (HAVE_LIBSDL)
static t_stat StartAudioOutput(void)
{
	t_stat result;
	SDL_AudioSpec want, have;

	terminate_thread = 0;

	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		SDL_Init(SDL_INIT_AUDIO);
	}

    if (audio_thread_handle == NULL)
	{
		audio_thread_handle = SDL_CreateThread(AudioThreadFunction, "AudioThread", NULL);
		if (audio_thread_handle == NULL)
		{
			result = SCPE_OPENERR;
			SDL_Log("Failed to create audio thread: %s", SDL_GetError());
		}
		else
		{
			SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
		}
	}

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = AUDIO_SAMPLE_RATE;
	want.format = AUDIO_U8;
	want.channels = 1;
	want.samples = AUDIO_OUT_BUFFER_SIZE;
	want.callback = NULL;

	audio_device_handle = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (audio_device_handle == 0)
	{
		result = SCPE_OPENERR;
		SDL_Log("Failed to open audio: %s", SDL_GetError());
	}
	else
	{
		SDL_PauseAudioDevice(audio_device_handle, 0); /* start audio playing. */
    }
	return SCPE_OK;
}

static void StopAudioOutput(void)
{
	terminate_thread = 1;
	if (audio_thread_handle != NULL)
	{
		SDL_WaitThread(audio_thread_handle, NULL);
		audio_thread_handle = NULL;
	}

	if (audio_device_handle != 0)
	{
		SDL_CloseAudioDevice(audio_device_handle);
	}
}

static int AudioThreadFunction(void *data)
{
	int i = 0;
	t_uint64 countsPerSecond;
	t_uint64 lastSampleCount;
	t_uint64 currentCount;
	t_uint64 countsPerSample;
	t_uint64 countDiff;
	uint8 buffer[AUDIO_OUT_BUFFER_SIZE];

	countsPerSecond = SDL_GetPerformanceFrequency();
	countsPerSample = countsPerSecond / AUDIO_SAMPLE_RATE;
	lastSampleCount = SDL_GetPerformanceCounter();

    /* Wait for device to be opened */
    while (audio_device_handle == 0 && !terminate_thread)
    {
        sim_os_ms_sleep(0);
    }

	while (!terminate_thread)
	{
		do
		{
			currentCount = SDL_GetPerformanceCounter();
			countDiff = currentCount - lastSampleCount;
			if (countDiff > countsPerSample)
			{
				buffer[i++] = (uint8)(*ConsoleHoot);
				lastSampleCount += countsPerSample;
				countDiff -= countsPerSample;
				if (i >= AUDIO_OUT_BUFFER_SIZE)
				{
					i = 0;
					SDL_QueueAudio(audio_device_handle, buffer, sizeof(buffer));
				}
			}
            sim_os_ms_sleep(0);
		} while (countDiff > countsPerSample);
	}

	return 0;
}

#elif defined (_WIN32)
/* Using waveform audio APIs: https://msdn.microsoft.com/en-us/library/dd742883(v=vs.85).aspx */

static t_stat StartAudioOutput(void)
{
	t_stat result = SCPE_OK;
	WAVEFORMATEX waveformat;

	if (audio_thread_handle == INVALID_HANDLE_VALUE)
	{
		audio_thread_handle = CreateThread(NULL, 0, audio_thread_function, NULL, CREATE_SUSPENDED, NULL);
		if (audio_thread_handle == NULL)
		{
			audio_thread_handle = INVALID_HANDLE_VALUE;
			result = SCPE_OPENERR;
		}
		else
		{
			SetThreadPriority(audio_thread_handle, THREAD_PRIORITY_HIGHEST);
		}
	}

	pWaveHdr1 = malloc(sizeof(WAVEHDR));
	pWaveHdr2 = malloc(sizeof(WAVEHDR));
	pBuffer1 = malloc(AUDIO_OUT_BUFFER_SIZE);
	pBuffer2 = malloc(AUDIO_OUT_BUFFER_SIZE);

	if (!pWaveHdr1 || !pWaveHdr2 || !pBuffer1 || !pBuffer2)
	{
		audio_cleanup_buffers();
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
			audio_cleanup_buffers();
			hWaveOut = INVALID_HANDLE_VALUE;
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
		pWaveHdr1->dwFlags = 0;
		pWaveHdr1->dwLoops = 1;
		pWaveHdr1->lpNext = NULL;
		pWaveHdr1->reserved = 0;

		waveOutPrepareHeader(hWaveOut, pWaveHdr1, sizeof(WAVEHDR));

		pWaveHdr2->lpData = pBuffer2;
		pWaveHdr2->dwBufferLength = AUDIO_OUT_BUFFER_SIZE;
		pWaveHdr2->dwBytesRecorded = 0;
		pWaveHdr2->dwUser = 0;
		pWaveHdr2->dwFlags = 0;
		pWaveHdr2->dwLoops = 1;
		pWaveHdr2->lpNext = NULL;
		pWaveHdr2->reserved = 0;

		waveOutPrepareHeader(hWaveOut, pWaveHdr2, sizeof(WAVEHDR));

		terminate_thread = 0;
		ResumeThread(audio_thread_handle);
	}

	return result;
}

static DWORD WINAPI audio_thread_function(void *param)
{
	PWAVEHDR currentWaveHdr = pWaveHdr1;
	int i = 0;
	LARGE_INTEGER countsPerSecond;
	LARGE_INTEGER lastSampleCount;
	LARGE_INTEGER currentCount;
	LONGLONG countsPerSample;
	LONGLONG countDiff;

	QueryPerformanceFrequency(&countsPerSecond);
	countsPerSample = countsPerSecond.QuadPart / AUDIO_SAMPLE_RATE;
	QueryPerformanceCounter(&lastSampleCount);

	while (!terminate_thread)
	{
		do
		{
			QueryPerformanceCounter(&currentCount);
			countDiff = currentCount.QuadPart - lastSampleCount.QuadPart;
			if (countDiff > countsPerSample)
			{
				currentWaveHdr->lpData[i++] = *ConsoleHoot;
				lastSampleCount.QuadPart += countsPerSample;
				countDiff -= countsPerSample;
				if (i >= AUDIO_OUT_BUFFER_SIZE)
				{
					i = 0;
					waveOutWrite(hWaveOut, currentWaveHdr, sizeof(WAVEHDR));
					if (currentWaveHdr == pWaveHdr1)
					{
						currentWaveHdr = pWaveHdr2;
					}
					else
					{
						currentWaveHdr = pWaveHdr1;
					}
				}
			}
            sim_os_ms_sleep(0);
		} while (countDiff > countsPerSample);
	}

	return 0;
}

static void audio_cleanup_buffers(void)
{
	if (!pWaveHdr1) free(pWaveHdr1);
	if (!pWaveHdr2) free(pWaveHdr2);
	if (!pBuffer1)  free(pBuffer1);
	if (!pBuffer2)  free(pBuffer2);
}

static void StopAudioOutput(void)
{
	terminate_thread = 1;
	if (audio_thread_handle != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(audio_thread_handle, 1000) != WAIT_OBJECT_0)
		{
			TerminateThread(audio_thread_handle, -1);
		}
		CloseHandle(audio_thread_handle);
		audio_thread_handle = INVALID_HANDLE_VALUE;
	}

	if (hWaveOut != INVALID_HANDLE_VALUE)
	{
		waveOutReset(hWaveOut);
		waveOutUnprepareHeader(hWaveOut, pWaveHdr1, sizeof(WAVEHDR));
		waveOutUnprepareHeader(hWaveOut, pWaveHdr2, sizeof(WAVEHDR));
		waveOutClose(hWaveOut);
		hWaveOut = INVALID_HANDLE_VALUE;
	}

	audio_cleanup_buffers();
}
#else
static t_stat StartAudioOutput(void)
{
	return SCPE_NOFNC;
}

void StopAudioOutput(void)
{
}

#endif