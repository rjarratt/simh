/* mu5_btu.c: MU5 Block Transfer Unit

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

This is the MU5 Block Transfer Unit. It can perform up to 4 concurrent
transfers and uses one unit per transfer.

Known Limitations
-----------------

*/

#include <assert.h>
#include "sim_defs.h"
#include "sim_disk.h"
#include "mu5_defs.h"
#include "mu5_cpu.h"
#include "mu5_exch.h"
#include "mu5_sac.h"
#include "mu5_btu.h"

#define LOG_BTU_REQUEST           (1 << 0)

static t_stat btu_reset(DEVICE *dptr);
t_stat btu_svc(UNIT *uptr);

static void btu_start_polling_if_attached(UNIT *uptr);
static void btu_schedule_next_poll(UNIT *uptr);

//static t_uint64 btu_read_vx_store(t_addr addr);
//static void btu_write_vx_store(t_addr addr, t_uint64 value);
//static void btu_setup_vx_store_location(uint8 line, t_uint64(*readCallback)(uint8), void(*writeCallback)(uint8, t_uint64));
//static t_uint64 btu_read_disc_address_callback(uint8 line);
//static void btu_write_disc_address_callback(uint8 line, t_uint64 value);
//static t_uint64 btu_read_store_address_callback(uint8 line);
//static void btu_write_store_address_callback(uint8 line, t_uint64 value);
//static t_uint64 btu_read_disc_status_callback(uint8 line);
//static void btu_write_disc_status_callback(uint8 line, t_uint64 value);
//static t_uint64 btu_read_current_positions_callback(uint8 line);
//static t_uint64 btu_read_complete_address_callback(uint8 line);
//static void btu_write_complete_address_callback(uint8 line, t_uint64 value);

static uint32 source_address[BTU_NUM_UNITS];
static uint32 destination_address[BTU_NUM_UNITS];
static uint32 size[BTU_NUM_UNITS];
static uint32 transfer_status[BTU_NUM_UNITS];
static uint32 btu_ripf;
static uint32 transfer_complete;

static UNIT btu_unit[] =
{
    { UDATA(&btu_svc, 0, 0) },
    { UDATA(&btu_svc, 0, 0) },
    { UDATA(&btu_svc, 0, 0) },
    { UDATA(&btu_svc, 0, 0) }
};

static BITFIELD size_bits[] = {
    BITF(N,16),
    BITF(UI,4),
    ENDBITS
};

static BITFIELD transfer_status_bits[] = {
    BITNCF(1),
    BIT(PDT),
    BIT(TC),
    BIT(TIP),
    ENDBITS
};

static REG btu_reg[] =
{
    { URDATAD(SOURCEADDR,         source_address, 16, 28, 4, BTU_NUM_UNITS, 0, "source address, units 0 to 3") },
    { URDATAD(DESTINATIONADDR,    destination_address, 16, 28, 4, BTU_NUM_UNITS, 0, "destination address, units 0 to 3") },
    { URDATAD(DESTINATIONADDR,    destination_address, 16, 28, 4, BTU_NUM_UNITS, 0, "destination address, units 0 to 3") },
    { URDATADF(SIZE,               size, 16,20, 12, BTU_NUM_UNITS, 0, "transfer size, units 0 to 3", size_bits) },
    { URDATADF(TRANSFERSTATUS,     transfer_status, 16, 4, 28, BTU_NUM_UNITS, 0, "transfer status, units 0 to 3", transfer_status_bits) },
    { GRDATAD(BTURIPF,            btu_ripf,      16,  31, 1, "request inhibit") },
    { GRDATAD(TRANSFERCOMPLETE,   transfer_complete, 16,  28, 4, "transfer complete") },
    { NULL }
};

static MTAB btu_mod[] =
{
    { 0 }
};

static DEBTAB btu_debtab[] =
{
    { "EVENT",          SIM_DBG_EVENT,        "event dispatch activities" },
    { "SELFTESTDETAIL", LOG_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_SELFTEST_FAIL,    "self test failure output" },
    { "REQUEST",        LOG_BTU_REQUEST,      "i/o requests" },
    { NULL,           0 }
};

static const char* btu_description(DEVICE *dptr) {
    return "Block Transfer Unit";
}

DEVICE btu_dev = {
    "BTU",             /* name */
    btu_unit,          /* units */
    btu_reg,           /* registers */
    btu_mod,           /* modifiers */
    BTU_NUM_UNITS,     /* numunits */
    16,                /* aradix */
    32,                /* awidth */
    4,                 /* aincr */
    16,                /* dradix */
    32,                /* dwidth */
    NULL,              /* examine */
    NULL,              /* deposit */
    &btu_reset,        /* reset */
    NULL,              /* boot */
    NULL,              /* attach */
    NULL,              /* detach */
    NULL,              /* ctxt */
    DEV_DEBUG,         /* flags */
    0,                 /* dctrl */
    btu_debtab,        /* debflags */
    NULL,              /* msize */
    NULL,              /* lname */
    NULL,              /* help */
    NULL,              /* attach_help */
    NULL,              /* help_ctx */
    &btu_description,  /* description */
    NULL               /* brk_types */
};

static VXSTORE_LINE VxStore[32];

/* reset routine */
static t_stat btu_reset(DEVICE *dptr)
{
    int i;
    t_stat result = SCPE_OK;
    btu_reset_state();
    for (i = 0; i < BTU_NUM_UNITS; i++)
    {
//        btu_start_polling(&btu_unit[i]);
    }
    return result;
}

static t_stat btu_svc(UNIT *uptr)
{
    t_stat result = SCPE_OK;

    //btu_schedule_next_poll(uptr);
    return result;
}

void btu_reset_state(void)
{
    int i;

    for (i = 0; i < BTU_NUM_UNITS; i++)
    {
    }

////    btu_setup_vx_store_location(DRUM_VX_STORE_DISC_ADDRESS, btu_read_disc_address_callback, btu_write_disc_address_callback);
////    btu_setup_vx_store_location(DRUM_VX_STORE_STORE_ADDRESS, btu_read_store_address_callback, btu_write_store_address_callback);
////    btu_setup_vx_store_location(DRUM_VX_STORE_DISC_STATUS, btu_read_disc_status_callback, btu_write_disc_status_callback);
////    btu_setup_vx_store_location(DRUM_VX_STORE_CURRENT_POSITIONS, btu_read_current_positions_callback, NULL);
////    btu_setup_vx_store_location(DRUM_VX_STORE_COMPLETE_ADDRESS, btu_read_complete_address_callback, btu_write_complete_address_callback);
}

t_uint64 btu_exch_read(t_addr addr)
{
    t_uint64 result = 0;
    if (RA_VX_MASK & addr)
    {
//        result = btu_read_vx_store((addr & RA_X_MASK) >> 1);
    }

    return result;
}

void btu_exch_write(t_addr addr, t_uint64 value)
{
    if (RA_VX_MASK & addr)
    {
//        btu_write_vx_store((addr & RA_X_MASK) >> 1, value);
    }
}

static t_uint64 btu_read_vx_store(t_addr addr)
{
    t_uint64 result = 0;
    uint8 line = VX_LINE(addr);
    VXSTORE_LINE *vx_line;

    if (VX_BLOCK(addr) == 0)
    {
        vx_line = &VxStore[line];
        if (vx_line->ReadCallback != NULL)
        {
            result = vx_line->ReadCallback(line);
        }
    }

    return result;
}

static void btu_write_vx_store(t_addr addr, t_uint64 value)
{
    uint8 line = VX_LINE(addr);
    VXSTORE_LINE *vx_line;

    if (VX_BLOCK(addr) == 0)
    {
        vx_line = &VxStore[line];
        if (vx_line->WriteCallback != NULL)
        {
            vx_line->WriteCallback(line, value);
        }
    }
}