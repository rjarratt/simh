/* mu5_exch.c: MU5 Exchange Unit

Copyright (c) 2016-2018, Robert Jarratt

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

This is the MU5 Exchange unit. It is a passive SIMH device because it was not
considered a unit in MU5 (see the Block 5 description in Section 9.4 of the
Basic Programming Manual) and its state was accessed through the BTU.
Furthermore, because the emulator is synchronous the Exchange can be
emulated as a passive unit and does not need a service routine, so again a
device is not needed. It is only a SIMH device so it can have its own debug
settings.

Addresses presented to the Exchange are all real addresses in 32-bit units.
The format is as per Fig. 6.10(b) on p128 of the book, and further explained
on p134 of the book, namely:

   4  1            23
+----+-+-----------------------+
+UNIT|V|        ADDRESS        |
+----+-+-----------------------+

*/

#include <assert.h>
#include "sim_defs.h"
#include "sim_disk.h"
#include "mu5_defs.h"
#include "mu5_exch.h"
#include "mu5_sac.h"
#include "mu5_drum.h"

#define LOG_EXCH_REAL_ACCESSES   (1 << 0)

static int8 exch_get_unit(t_addr addr);
static t_addr exch_get_unit_address(t_addr addr);

static UNIT exch_unit =
{
    UDATA(NULL, 0, 0)
};

static REG exch_reg[] =
{
    { NULL }
};

static MTAB exch_mod[] =
{
    { 0 }
};

static DEBTAB exch_debtab[] =
{
    { "EVENT",          SIM_DBG_EVENT,     "event dispatch activities" },
    { "SELFTESTDETAIL", LOG_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_SELFTEST_FAIL,  "self test failure output" },
    { "ERROR",          LOG_ERROR, "significant errors" },
    { "REAL",           LOG_EXCH_REAL_ACCESSES, "real address accesses" },
    { NULL,           0 }
};

static const char* exch_description(DEVICE *dptr) {
    return "Exchange Unit";
}

DEVICE exch_dev = {
    "EXCH",            /* name */
    &exch_unit,        /* units */
    exch_reg,          /* registers */
    exch_mod,          /* modifiers */
    1,                 /* numunits */
    16,                /* aradix */
    32,                /* awidth */
    1,                 /* aincr */
    16,                /* dradix */
    32,                /* dwidth */
    NULL,              /* examine */
    NULL,              /* deposit */
    NULL,              /* reset */
    NULL,              /* boot */
    NULL,              /* attach */
    NULL,              /* detach */
    NULL,              /* ctxt */
    DEV_DEBUG,         /* flags */
    0,                 /* dctrl */
    exch_debtab,       /* debflags */
    NULL,              /* msize */
    NULL,              /* lname */
    NULL,              /* help */
    NULL,              /* attach_help */
    NULL,              /* help_ctx */
    &exch_description,  /* description */
    NULL               /* brk_types */
};

t_uint64 exch_read(t_addr address)
{
	t_uint64 result = 0;
	int8 unit = exch_get_unit(address);
	t_addr unit_addr = exch_get_unit_address(address);

    switch (unit)
    {
        case UNIT_FIXED_HEAD_DISC:
        {
            result = drum_exch_read(unit_addr);

            sim_debug(LOG_EXCH_REAL_ACCESSES, &exch_dev, "Read drum real address %08X, result=%016llX\n", address, result);
            break;
        }

        case UNIT_LOCAL_STORE:
        {
            result = sac_local_store_exch_read(unit_addr);
            sim_debug(LOG_EXCH_REAL_ACCESSES, &exch_dev, "Read local store real address %08X, result=%016llX\n", address, result);
            break;
        }

        case UNIT_MASS_STORE:
        {
            result = sac_mass_store_exch_read(unit_addr);
            sim_debug(LOG_EXCH_REAL_ACCESSES, &exch_dev, "Read mass store real address %08X, result=%016llX\n", address, result);
            break;
        }

        default:
        {
            result = 0;
            sim_debug(LOG_ERROR, &exch_dev, "Read unknown (%hhu) store real address %08X, result=%016llX\n", unit, address, result);
            break;
        }
    }

    return result;
}

void exch_write(t_addr address, t_uint64 value)
{
	int8 unit = exch_get_unit(address);
	t_addr unit_addr = exch_get_unit_address(address);
    switch (unit)
    {
        case UNIT_FIXED_HEAD_DISC:
        {
            drum_exch_write(unit_addr, value);
            sim_debug(LOG_EXCH_REAL_ACCESSES, &exch_dev, "Write drum real address %08X, value=%016llX\n", address, value);
            break;
        }

        case UNIT_LOCAL_STORE:
        {
            sac_local_store_exch_write(unit_addr, value);
            sim_debug(LOG_EXCH_REAL_ACCESSES, &exch_dev, "Write local store real address %08X, value=%016llX\n", address, value);
            break;
        }

        case UNIT_MASS_STORE:
        {
            sac_mass_store_exch_write(unit_addr, value);
            sim_debug(LOG_EXCH_REAL_ACCESSES, &exch_dev, "Write mass store real address %08X, value=%016llX\n", address, value);
            break;
        }

        default:
        {
            sim_debug(LOG_ERROR, &exch_dev, "Write unknown (%hhu) store real address %08X, value=%016llX\n", unit, address, value);
            break;
        }
    }
}

static int8 exch_get_unit(t_addr addr)
{
	return (addr >> 24) & 0xF;
}

static t_addr exch_get_unit_address(t_addr addr)
{
	return addr & 0xFFFFFF;
}

