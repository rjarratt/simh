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

#define NUM_VX_BLOCKS (BTU_NUM_UNITS + 2)

static t_stat btu_reset(DEVICE *dptr);
t_stat btu_svc(UNIT *uptr);

static void btu_start_polling_if_attached(UNIT *uptr);
static void btu_schedule_next_poll(UNIT *uptr);

static t_uint64 btu_read_vx_store(t_addr addr);
static void btu_write_vx_store(t_addr addr, t_uint64 value);
static void btu_setup_vx_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(uint8, uint8), void(*writeCallback)(uint8, uint8, t_uint64));
static t_uint64 btu_read_source_address_callback(uint8 block, uint8 line);
static void btu_write_source_address_callback(uint8 block, uint8 line, t_uint64 value);
static t_uint64 btu_read_destination_address_callback(uint8 block, uint8 line);
static void btu_write_destination_address_callback(uint8 block, uint8 line, t_uint64 value);
static t_uint64 btu_read_size_callback(uint8 block, uint8 line);
static void btu_write_size_callback(uint8 block, uint8 line, t_uint64 value);
static t_uint64 btu_read_transfer_status_callback(uint8 block, uint8 line);
static void btu_write_transfer_status_callback(uint8 block, uint8 line, t_uint64 value);
static t_uint64 btu_read_btu_ripf_callback(uint8 block, uint8 line);
static void btu_write_btu_ripf_callback(uint8 block, uint8 line, t_uint64 value);
static t_uint64 btu_read_transfer_complete_callback(uint8 block, uint8 line);

static uint32 reg_source_address[BTU_NUM_UNITS];
static uint32 reg_destination_address[BTU_NUM_UNITS];
static uint32 reg_size[BTU_NUM_UNITS];
static uint32 reg_transfer_status[BTU_NUM_UNITS];
static uint16 reg_btu_ripf;
static uint32 reg_transfer_complete;

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
    { URDATAD(SOURCEADDR,         reg_source_address, 16, 28, 4, BTU_NUM_UNITS, 0, "source address, units 0 to 3") },
    { URDATAD(DESTINATIONADDR,    reg_destination_address, 16, 28, 4, BTU_NUM_UNITS, 0, "destination address, units 0 to 3") },
    { URDATADF(SIZE,              reg_size, 16,20, 12, BTU_NUM_UNITS, 0, "transfer size, units 0 to 3", size_bits) },
    { URDATADF(TRANSFERSTATUS,    reg_transfer_status, 16, 4, 28, BTU_NUM_UNITS, 0, "transfer status, units 0 to 3", transfer_status_bits) },
    { GRDATAD(BTURIPF,            reg_btu_ripf,      16,  1, 31, "request inhibit") },
    { GRDATAD(TRANSFERCOMPLETE,   reg_transfer_complete, 16,  28, 4, "transfer complete") },
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

static VXSTORE_LINE VxStore[NUM_VX_BLOCKS][32];

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

    memset(reg_source_address, 0, sizeof(reg_source_address));
    memset(reg_destination_address, 0, sizeof(reg_destination_address));
    memset(reg_size, 0, sizeof(reg_size));
    memset(reg_transfer_status, 0, sizeof(reg_transfer_status));
    reg_btu_ripf = 0;
    reg_transfer_complete = 0;

    for (i = 0; i < BTU_NUM_UNITS; i++)
    {
        btu_setup_vx_store_location(i, BTU_VX_STORE_SOURCE_ADDRESS_LINE, btu_read_source_address_callback, btu_write_source_address_callback);
        btu_setup_vx_store_location(i, BTU_VX_STORE_DESTINATION_ADDRESS_LINE, btu_read_destination_address_callback, btu_write_destination_address_callback);
        btu_setup_vx_store_location(i, BTU_VX_STORE_SIZE_LINE, btu_read_size_callback, btu_write_size_callback);
        btu_setup_vx_store_location(i, BTU_VX_STORE_TRANSFER_STATUS_LINE, btu_read_transfer_status_callback, btu_write_transfer_status_callback);
    }

    btu_setup_vx_store_location(BTU_VX_STORE_PARITY_BLOCK, BTU_VX_STORE_BTU_RIPF_LINE, btu_read_btu_ripf_callback, btu_write_btu_ripf_callback);
    btu_setup_vx_store_location(BTU_VX_STORE_PARITY_BLOCK, BTU_VX_STORE_TRANSFER_COMPLETE_LINE, btu_read_transfer_complete_callback, NULL);

}

t_uint64 btu_exch_read(t_addr addr)
{
    t_uint64 result = 0;
    if (RA_VX_MASK & addr)
    {
        result = btu_read_vx_store((addr & RA_X_MASK) >> 1);
    }

    return result;
}

void btu_exch_write(t_addr addr, t_uint64 value)
{
    if (RA_VX_MASK & addr)
    {
        btu_write_vx_store((addr & RA_X_MASK) >> 1, value);
    }
}

static t_uint64 btu_read_vx_store(t_addr addr)
{
    t_uint64 result = 0;
    uint8 block = VX_BLOCK(addr);
    uint8 line = VX_LINE(addr);
    VXSTORE_LINE *vx_line;

    if (block <= NUM_VX_BLOCKS)
    {
        vx_line = &VxStore[block][line];
        if (vx_line->ReadCallback != NULL)
        {
            result = vx_line->ReadCallback(block, line);
        }
    }

    return result;
}

static void btu_write_vx_store(t_addr addr, t_uint64 value)
{
    uint8 block = VX_BLOCK(addr);
    uint8 line = VX_LINE(addr);
    VXSTORE_LINE *vx_line;

    if (block <= NUM_VX_BLOCKS)
    {
        vx_line = &VxStore[block][line];
        if (vx_line->WriteCallback != NULL)
        {
            vx_line->WriteCallback(block, line, value);
        }
    }
}

static void btu_setup_vx_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(uint8, uint8), void(*writeCallback)(uint8, uint8, t_uint64))
{
    VXSTORE_LINE *l = &VxStore[block][line];
    l->ReadCallback = readCallback;
    l->WriteCallback = writeCallback;
}

static t_uint64 btu_read_source_address_callback(uint8 block, uint8 line)
{
    return reg_source_address[block] & 0x0FFFFFFF;
}

static void btu_write_source_address_callback(uint8 block, uint8 line, t_uint64 value)
{
    reg_source_address[block] = value & 0x0FFFFFFF;
}

static t_uint64 btu_read_destination_address_callback(uint8 block, uint8 line)
{
    return reg_destination_address[block] & 0x0FFFFFFF;
}

static void btu_write_destination_address_callback(uint8 block, uint8 line, t_uint64 value)
{
    reg_destination_address[block] = value & 0x0FFFFFFF;
}

static t_uint64 btu_read_size_callback(uint8 block, uint8 line)
{
    return reg_size[block] & 0x000FFFFF;
}

static void btu_write_size_callback(uint8 block, uint8 line, t_uint64 value)
{
    reg_size[block] = value & 0x000FFFFF;
}

static t_uint64 btu_read_transfer_status_callback(uint8 block, uint8 line)
{
    return reg_transfer_status[block] & 0xE;
}

static void btu_write_transfer_status_callback(uint8 block, uint8 line, t_uint64 value)
{
    reg_transfer_status[block] = value & 0xE;
}

static t_uint64 btu_read_btu_ripf_callback(uint8 block, uint8 line)
{
    return reg_btu_ripf & 0x1;
}

static void btu_write_btu_ripf_callback(uint8 block, uint8 line, t_uint64 value)
{
    reg_btu_ripf = value & 0x1;
}

static t_uint64 btu_read_transfer_complete_callback(uint8 block, uint8 line)
{
    return reg_transfer_complete & 0xF;
}
