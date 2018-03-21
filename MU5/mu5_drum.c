/* mu5_sac.c: MU5 Fixed Head Disk Device Unit

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

This is the MU5 Store Access Control unit. The Local Store consisted of four
4096-word memory units, each word containing 64 data bits + 8 parity bits.
The Mass Store consisted of two 128K-word memory units, each word containing
36 bits. The Fixed-head Disc consisted of two 2.4 Mbyte units.

There are believed to be 4 hard-wired CPRs. When asked in April 2017 RNI was
not sure which ones and did not know their values. He said: "Assume 4 for now
and assume they are numbers 28-31. Writing to them would have no effect."

Notes on V and Vx Store Access
------------------------------




Known Limitations
-----------------

*/

#include <assert.h>
#include "sim_defs.h"
#include "sim_disk.h"
#include "mu5_defs.h"
#include "mu5_drum.h"

#define BLOCK_TIME_USECS (20000 / DRUM_BLOCKS_PER_BAND) /* Rotation is 20ms */

static t_stat drum_reset(DEVICE *dptr);
t_stat drum_svc(UNIT *uptr);
t_stat drum_attach(UNIT *uptr, CONST char *cptr);
t_stat drum_detach(UNIT *uptr);

static void drum_start_polling_if_attached(UNIT *uptr);
static void drum_schedule_next_poll(UNIT *uptr);
static uint8 drum_get_current_position(int unit_num);
static void drum_set_current_position(int unit_num, uint8 value);

static UNIT drum_unit[] =
{
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) },
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) },
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) },
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) }
};

BITFIELD disc_address_bits[] = {
	BITF(SIZE,6),   /* Number of blocks requested for transfer */
	BITNCF(2),
	BITF(BLOCK,6),  /* Block number (0-36) */
	BITF(BAND,6),   /* Band */
	BITF(D,2),      /* Disc number */
	BITNCF(8),
	BIT(RW),        /* 1=Read, 0=Write */
	BIT(P),         /* Internal read */
	ENDBITS
};

BITFIELD store_address_bits[] = {
	BITF(ADDRESS,24),   /* Mass/Local Real Address */
	BITF(UNIT,4),       /* Unit */
	BITNCF(4),
	ENDBITS
};

BITFIELD disc_status_bits[] = {
	BITF(UNIT, 4),        /* Disc unit number (=0) */
	BIT(ENDXFER),         /* End transfer. 1=ENDED */
	BIT(IPARITY),         /* Ignore parity fault */
	BIT(ROWPARITY),       /* Row parity error (internal to disc) */
	BIT(COLPARITY),       /* Column parity error (internal to disc) */
	BIT(DATALATE),        /* Data late (hardware error) */
	BIT(BOUNDLOCKEDOUT),  /* Bound locked out */
	BIT(INPPARITYERR),    /* Input parity error */
	BIT(INPPARITYERRSRC), /* Input parity error source (data, address, control info) */
	BIT(ILLREQ),          /* Illegal request to the disc */
	BIT(D0ST),            /* Disc 0 on self test */
	BITNCF(2),
	BIT(D0ABSENT),        /* Disc 0 absent */
	BIT(D1ST),            /* Disc 1 on self test */
	BITNCF(2),
	BIT(D1ABSENT),        /* Disc 1 absent */
	BIT(D2ST),            /* Disc 2 on self test */
	BITNCF(2),
	BIT(D2ABSENT),        /* Disc 2 absent */
	BIT(D3ST),            /* Disc 3 on self test */
	BITNCF(2),
	BIT(D3ABSENT),        /* Disc 3 absent */
	BIT(SELFTEST),        /* Decode Vx line 7 */
	BIT(DECODE),          /* Decode the rest of the status line, indicates if further action is required */
	ENDBITS
};

BITFIELD current_positions_bits[] = {
	BITF(D0,6),
	BITF(D1,6),
	BITF(D2,6),
	BITF(D3,6),
	BIT(D0PD),
	BIT(D1PD),
	BITNCF(6),
	ENDBITS
};

BITFIELD complete_address_bits[] = {
	BITF(ADDRESS,28),
	BITNCF(4),
	ENDBITS
};

BITFIELD lockout_01_bits[] = {
	BITF(LOCKOUTDISC1,16),
	BITF(LOCKOUTDISC2,16),
	ENDBITS
};

BITFIELD lockout_23_bits[] = {
	BITF(LOCKOUTDISCs23,16),
	BITNCF(16),
	ENDBITS
};

BITFIELD request_self_test_bits[] = {
	BIT(CPUPERM),
	BIT(REQST0),
	BIT(REQST1),
	BIT(REQST2),
	BIT(REQST3),
	ENDBITS
};

BITFIELD self_test_command_bits[] = {
	BITF(DISKNUMBER,2),
	BIT(SELFTEST),
	BIT(ADREQ),
	BITF(MARGINS,3),
	ENDBITS
};

BITFIELD self_test_state_bits[] = {
	BIT(SELFPHASEERR),
	BIT(ADDRERR),
	BIT(SURFACEERR),
	BIT(PRINTAD),
	BIT(MAXMINSIG),
	ENDBITS
};

uint32 reg_disc_address;
uint32 reg_store_address;
uint32 reg_disc_status;
uint32 reg_current_positions;
uint32 reg_complete_address;
uint32 reg_lockout_01;
uint32 reg_lockout_23;
uint32 reg_request_self_test;
uint32 reg_self_test_command;
uint32 reg_self_test_state;

static REG drum_reg[] =
{
	{ GRDATADF(DISCADDRESS,      reg_disc_address,      16,  32, 0, "Disc address", disc_address_bits) },
	{ GRDATADF(STOREADDRESS,     reg_store_address,     16,  32, 0, "Store address", store_address_bits) },
	{ GRDATADF(DISCSTATUS,       reg_disc_status,       16,  32, 0, "Disc status", disc_status_bits) },
	{ GRDATADF(CURRENTPOSITIONS, reg_current_positions, 16,  32, 0, "Current positions", current_positions_bits) },
	{ GRDATADF(COMPLETEADDRESS,  reg_complete_address,  16,  32, 0, "Complete address", complete_address_bits) },
	{ GRDATADF(LOCKOUT01,        reg_lockout_01,        16,  32, 0, "Lockout 01", lockout_01_bits) },
	{ GRDATADF(LOCKOUT23,        reg_lockout_23,        16,  32, 0, "Lockout 23", lockout_23_bits) },
	{ GRDATADF(REQSELFTEST,      reg_request_self_test, 16,  32, 0, "Request self test", request_self_test_bits) },
	{ GRDATADF(SELFTESTCMD,      reg_self_test_command, 16,  32, 0, "Self test command", self_test_command_bits) },
	{ GRDATADF(SELFTESTSTATE,    reg_self_test_state,   16,  32, 0, "Self test state", self_test_state_bits) },
	{ NULL }
};

static MTAB drum_mod[] =
{
	{ 0 }
};

static DEBTAB drum_debtab[] =
{
	{ NULL,           0 }
};

static const char* drum_description(DEVICE *dptr) {
	return "Fixed-Head Disc Store Unit";
}

DEVICE drum_dev = {
	"DRUM",            /* name */
	drum_unit,         /* units */
	drum_reg,          /* registers */
	drum_mod,          /* modifiers */
	DRUM_NUM_UNITS,    /* numunits */
	16,                /* aradix */
	32,                /* awidth */
	4,                 /* aincr */
	16,                /* dradix */
	32,                /* dwidth */
	NULL,              /* examine */
	NULL,              /* deposit */
	&drum_reset,       /* reset */
	NULL,              /* boot */
	&drum_attach,      /* attach */
	&drum_detach,      /* detach */
	NULL,              /* ctxt */
	DEV_DEBUG | DEV_DISK,         /* flags */
	0,                 /* dctrl */
	drum_debtab,       /* debflags */
	NULL,              /* msize */
	NULL,              /* lname */
	NULL,              /* help */
	NULL,              /* attach_help */
	NULL,              /* help_ctx */
	&drum_description, /* description */
	NULL               /* brk_types */
};

/* reset routine */
static t_stat drum_reset(DEVICE *dptr)
{
	int i;
	t_stat result = SCPE_OK;
	for (i = 0; i < DRUM_NUM_UNITS; i++)
	{
		sim_disk_reset(&drum_unit[i]);
		drum_start_polling_if_attached(&drum_unit[i]);
	}
	return result;
}

static t_stat drum_svc(UNIT *uptr)
{
	t_stat result = SCPE_OK;
	int unit_num = (int)(uptr - drum_unit);
	drum_set_current_position(unit_num, (drum_get_current_position(unit_num) + 1) % DRUM_BLOCKS_PER_BAND);
	drum_schedule_next_poll(uptr);
	return result;
}

t_stat drum_attach(UNIT *uptr, CONST char *cptr)
{
	t_stat r;
	size_t xferElementSize = drum_dev.dwidth / 8;
	size_t sectorSizeBytes = DRUM_WORDS_PER_BLOCK * drum_dev.aincr * xferElementSize;

	r = sim_disk_attach(uptr, cptr, sectorSizeBytes, xferElementSize, 1, 0, "DRUM", 0, 0);
	
	drum_start_polling_if_attached(uptr);
	return r;
}

t_stat drum_detach(UNIT *uptr)
{
	t_stat r;

	r = sim_disk_detach(uptr);                             /* detach unit */
	return r;
}

static void drum_start_polling_if_attached(UNIT *uptr)
{
	sim_cancel(uptr);
	if (uptr->flags & UNIT_ATT)
	{
		drum_schedule_next_poll(uptr);
	}
}


static void drum_schedule_next_poll(UNIT *uptr)
{
	sim_activate_after(uptr, BLOCK_TIME_USECS);
}

static uint8 drum_get_current_position(int unit_num)
{
	uint8 pos = reg_current_positions >> (unit_num * 6) & 0x3F;
	return pos;
}

static void drum_set_current_position(int unit_num, uint8 value)
{
	uint32 temp = (value & 0x3F) << (unit_num * 6);
	uint32 mask = ~(0x3F << (unit_num * 6));
	reg_current_positions = (reg_current_positions & mask) | temp;
}
