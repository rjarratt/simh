/* mu5_drum.c: MU5 Fixed Head Disk Device Unit

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

This is the MU5 Fixed Head Disc unit. It was also called the Drum store to
avoid confusion with other disks used for file storage (see p33 of the book).
MU5 allowed up to 4 fixed head discs, but in actual fact never had more than
2. Each drum rotates in 20.5ms.

The service routine operates at the real time speed of the drum in that it is
scheduled once per block, of which there were 37 per band (or track in modern
parlance).

Notes on V and Vx Store Access
------------------------------




Known Limitations
-----------------

*/

#include <assert.h>
#include "sim_defs.h"
#include "sim_disk.h"
#include "mu5_defs.h"
#include "mu5_sac.h"
#include "mu5_drum.h"

#define BLOCK_TIME_USECS (20000 / DRUM_BLOCKS_PER_BAND) /* Rotation is 20ms */
#define D0_ABSENT_MASK (1 << 17)
#define D1_ABSENT_MASK (1 << 21)
#define D2_ABSENT_MASK (1 << 25)
#define D3_ABSENT_MASK (1 << 29)

static t_stat drum_reset(DEVICE *dptr);
t_stat drum_svc(UNIT *uptr);
t_stat drum_attach(UNIT *uptr, CONST char *cptr);
t_stat drum_detach(UNIT *uptr);

static void drum_start_polling_if_attached(UNIT *uptr);
static void drum_schedule_next_poll(UNIT *uptr);
static uint8 drum_get_current_position(int unit_num);
static void drum_set_current_position(int unit_num, uint8 value);
static void drum_update_current_position(int unit_num);
static int drum_get_unit_num(UNIT *uptr);
static void drum_set_unit_present(int unit_num, int present);
static void drum_set_illegal_request();

static t_uint64 drum_read_vx_store(t_addr addr);
static void drum_write_vx_store(t_addr addr, t_uint64 value);
static void drum_setup_vx_store_location(uint8 line, t_uint64(*readCallback)(uint8), void(*writeCallback)(uint8, t_uint64));
static t_uint64 drum_read_disc_address_callback(uint8 line);
static void drum_write_disc_address_callback(uint8 line, t_uint64 value);


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
	BITF(INPPARITYERR,2), /* Input parity error */
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
uint32 reg_disc_status = D0_ABSENT_MASK | D1_ABSENT_MASK | D2_ABSENT_MASK | D3_ABSENT_MASK;
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
	{ "EVENT",          SIM_DBG_EVENT,     "event dispatch activities" },
	{ "SELFTESTDETAIL", LOG_SELFTEST_DETAIL,  "self test detailed output" },
	{ "SELFTESTFAIL",   LOG_SELFTEST_FAIL,  "self test failure output" },
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

static VXSTORE_LINE VxStore[32];

/* reset routine */
static t_stat drum_reset(DEVICE *dptr)
{
	int i;
	t_stat result = SCPE_OK;
	drum_reset_state();
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
	int unit_num = drum_get_unit_num(uptr);
	drum_update_current_position(unit_num);
	drum_schedule_next_poll(uptr);
	return result;
}

t_stat drum_attach(UNIT *uptr, CONST char *cptr)
{
	t_stat r;
	size_t xferElementSize = drum_dev.dwidth / 8;
	size_t sectorSizeBytes = DRUM_WORDS_PER_BLOCK * drum_dev.aincr * xferElementSize;
	int unit_num = drum_get_unit_num(uptr);

	r = sim_disk_attach(uptr, cptr, sectorSizeBytes, xferElementSize, 1, 0, "DRUM", 0, 0);

	drum_set_unit_present(unit_num, 1);

	drum_start_polling_if_attached(uptr);
	return r;
}

t_stat drum_detach(UNIT *uptr)
{
	t_stat r;
	int unit_num = drum_get_unit_num(uptr);

	r = sim_disk_detach(uptr);
	drum_set_unit_present(unit_num, 0);

	return r;
}

void drum_reset_state(void)
{
	reg_disc_address = 0;
	reg_store_address = 0;
	reg_disc_status = D0_ABSENT_MASK | D1_ABSENT_MASK | D2_ABSENT_MASK | D3_ABSENT_MASK;
	reg_current_positions = 0;
	reg_complete_address = 0;
	reg_lockout_01 = 0;
	reg_lockout_23 = 0;
	reg_request_self_test = 0;
	reg_self_test_command = 0;
	reg_self_test_state = 0;

	drum_setup_vx_store_location(0, drum_read_disc_address_callback, drum_write_disc_address_callback);
}

t_uint64 drum_exch_read(t_addr addr)
{
	t_uint64 result = 0;
	if (RA_VX_MASK & addr)
	{
		result = drum_read_vx_store((addr & RA_X_MASK) >> 1);
	}
	else
	{
		drum_set_illegal_request();
	}

	return result;
}

void drum_exch_write(t_addr addr, t_uint64 value)
{
	if (RA_VX_MASK & addr)
	{
		drum_write_vx_store((addr & RA_X_MASK) >> 1, value);
	}
	else
	{
		drum_set_illegal_request();
	}
}

static t_uint64 drum_read_vx_store(t_addr addr)
{
	t_uint64 result = 0;
	uint8 line = addr & 0xFFFFFFE0;
	VXSTORE_LINE *vx_line = &VxStore[line];
	if (vx_line->ReadCallback != NULL)
	{
		result = vx_line->ReadCallback(line);
	}

	return 0;
}

static void drum_write_vx_store(t_addr addr, t_uint64 value)
{
	uint8 line = addr & 0xFFFFFFE0;
	VXSTORE_LINE *vx_line = &VxStore[line];
	if (vx_line->WriteCallback != NULL)
	{
		vx_line->WriteCallback(line, value);
	}
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

static void drum_update_current_position(int unit_num)
{
	drum_set_current_position(unit_num, (drum_get_current_position(unit_num) + 1) % DRUM_BLOCKS_PER_BAND);
}

static int drum_get_unit_num(UNIT *uptr)
{
	return (int)(uptr - drum_unit);
}

static void drum_set_unit_present(int unit_num, int present)
{
	static uint32 masks[] = { D0_ABSENT_MASK, D1_ABSENT_MASK, D2_ABSENT_MASK, D3_ABSENT_MASK };
	if (present)
	{
		reg_disc_status &= ~masks[unit_num];
	}
	else
	{
		reg_disc_status |= masks[unit_num];
	}
}

static void drum_set_illegal_request()
{
	reg_disc_status |= DRUM_DISC_STATUS_ILLEGAL_REQUEST;
}

static void drum_setup_vx_store_location(uint8 line, t_uint64(*readCallback)(uint8), void(*writeCallback)(uint8,t_uint64))
{
    VXSTORE_LINE *l = &VxStore[line];
    l->ReadCallback = readCallback;
    l->WriteCallback = writeCallback;
}

static t_uint64 drum_read_disc_address_callback(uint8 line)
{
	return reg_disc_address & 0xC03FFF3F;
}

static void drum_write_disc_address_callback(uint8 line, t_uint64 value)
{
	reg_disc_address = value & 0xC03FFF3F;
}