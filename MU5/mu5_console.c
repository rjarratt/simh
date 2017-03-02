/* mu5_console.c: MU5 Console unit

Copyright (c) 2016-2017, Robert Jarratt

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

#define MASK_FCI 0x1
#define MASK_TCI 0x2
#define MASK_TEII 0x4
#define MASK_SCI 0x8

#define MASK_TTY_OUTPUT 0x20

static t_stat console_reset(DEVICE *dptr);
static t_stat console_svc(UNIT *uptr);
static void console_schedule_next_poll(UNIT *uptr);

static t_uint64 console_read_console_interrupt_callback(uint8 line);
static void console_write_console_interrupt_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_teletype_data_callback(uint8 line);
static void console_write_teletype_data_callback(uint8 line, t_uint64 value);
static t_uint64 console_read_teletype_control_callback(uint8 line);
static void console_write_teletype_control_callback(uint8 line, t_uint64 value);

static uint8 ConsoleInterrupt;
static uint8 TeletypeData;
static uint8 TeletypeControl;
static int TeletypeOperationInProgress;

static UNIT console_unit =
{
    UDATA(console_svc, UNIT_FIX | UNIT_BINK, MAXMEMORY)
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
    { "SELFTESTDETAIL", LOG_CPU_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_CPU_SELFTEST_FAIL,  "self test failure output" },
    { NULL,           0 }
};

static const char* console_description(DEVICE *dptr) {
    return "Console Unit";
}

DEVICE console_dev = {
    "CONSOLE",            /* name */
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
    ConsoleInterrupt = 0;
    TeletypeData = 0;
    TeletypeControl = 0;
    TeletypeOperationInProgress = 0;
    sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_CONSOLE_INTERRUPT, console_read_console_interrupt_callback, console_write_console_interrupt_callback);
    sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_DATA, console_read_teletype_data_callback, console_write_teletype_data_callback);
    sac_setup_v_store_location(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_CONTROL, console_read_teletype_control_callback, console_write_teletype_control_callback);
    sim_cancel(&console_unit);
    sim_activate(&console_unit, 1);
    return result;
}

static t_stat console_svc(UNIT *uptr)
{
    if (TeletypeOperationInProgress)
    {
        if (TeletypeControl & MASK_TTY_OUTPUT)
        {
            printf("%c", TeletypeData);
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
