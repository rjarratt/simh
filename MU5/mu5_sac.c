/* mu5_sac.c: MU5 Store Access Control unit

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
*/

#include "mu5_defs.h"
#include "mu5_sac.h"

#define NUM_CPRS 32
#define V_STORE_BLOCKS 8
#define V_STORE_BLOCK_SIZE 256

typedef struct VSTORE_LINE
{
    t_uint64(*ReadCallback)(void);
    void(*WriteCallback)(t_uint64);
} VSTORE_LINE;

static uint32 LocalStore[MAXMEMORY];
static VSTORE_LINE VStore[V_STORE_BLOCKS][V_STORE_BLOCK_SIZE];

static UNIT sac_unit =
{
    UDATA(NULL, UNIT_FIX | UNIT_BINK, MAXMEMORY)
};

static t_uint64 cpr[NUM_CPRS];

static REG sac_reg[] =
{
    { BRDATAD(CPR, cpr, 16, 64, NUM_CPRS, "CPR register") },
    { NULL }
};

static MTAB sac_mod[] =
{
    { 0 }
};

static DEBTAB sac_debtab[] =
{
    { "EVENT",        SIM_DBG_EVENT,     "event dispatch activities" },
    { "SELFTEST",     LOG_CPU_SELFTEST,  "self test output" },
    { "SELFTESTFAIL", LOG_CPU_SELFTEST_FAIL,  "self test failure output" },
    { NULL,           0 }
};

static const char* sac_description(DEVICE *dptr) {
    return "Store Access Control Unit";
}

static t_stat sac_reset(DEVICE *dptr);

DEVICE sac_dev = {
    "SAC",            /* name */
    &sac_unit,        /* units */
    sac_reg,          /* registers */
    sac_mod,          /* modifiers */
    1,                /* numunits */
    16,               /* aradix */
    32,               /* awidth */
    1,                /* aincr */
    16,               /* dradix */
    32,               /* dwidth */
    NULL,             /* examine */
    NULL,             /* deposit */
    &sac_reset,       /* reset */
    NULL,             /* boot */
    NULL,             /* attach */
    NULL,             /* detach */
    NULL,             /* ctxt */
    DEV_DEBUG,        /* flags */
    0,                /* dctrl */
    sac_debtab,       /* debflags */
    NULL,             /* msize */
    NULL,             /* lname */
    NULL,             /* help */
    NULL,             /* attach_help */
    NULL,             /* help_ctx */
    &sac_description, /* description */
    NULL              /* brk_types */
};

/* reset routine */
static t_stat sac_reset(DEVICE *dptr)
{
    t_stat result = SCPE_OK;
    sac_reset_state();
    return result;
}

void sac_reset_state(void)
{
	memset(LocalStore, 0, sizeof(uint32) * MAXMEMORY);
    memset(VStore, 0, sizeof(VStore));
}

t_uint64 sac_read_64_bit_word(t_addr address)
{
    t_uint64 result = ((t_uint64)sac_read_32_bit_word(address) << 32) | sac_read_32_bit_word(address + 1);
    return result;
}

void sac_write_64_bit_word(t_addr address, t_uint64 value)
{
    sac_write_32_bit_word(address, (value >> 32) & 0xFFFFFFFF);
    sac_write_32_bit_word(address + 1, value & 0xFFFFFFFF);
}

uint32 sac_read_32_bit_word(t_addr address)
{
    uint32 result = LocalStore[address];
    return result;
}

void sac_write_32_bit_word(t_addr address, uint32 value)
{
    LocalStore[address] = value;
}

uint16 sac_read_16_bit_word(t_addr address)
{
    uint32 fullWord = sac_read_32_bit_word(address >> 1);
    uint16 result = (address & 1) ? fullWord & 0xFFFF : fullWord >> 16;
    return result;
}

void sac_write_16_bit_word(t_addr address, uint16 value)
{
    uint32 fullWord = sac_read_32_bit_word(address >> 1);
    if (address & 1)
    {
        fullWord = (fullWord & 0xFFFF0000) | value;
    }
    else
    {
        fullWord = (value << 16) | (fullWord & 0xFFFF);
    }
    sac_write_32_bit_word(address >> 1, fullWord);
}

uint8 sac_read_8_bit_word(t_addr address)
{
    uint32 fullWord = sac_read_32_bit_word(address >> 2);
    uint8 byteNumber = 3 - (address & 0x3);
    uint8 result = fullWord >> (byteNumber << 3);
    return result;
}

void sac_write_8_bit_word(t_addr address, uint8 value)
{
    uint32 fullWord = sac_read_32_bit_word(address >> 2);
    uint8 byteNumber = 3 - (address & 0x3);
    uint32 mask = 0xFF << (byteNumber << 3);
    uint32 shiftedValue = (uint32)value << (byteNumber << 3);
    fullWord = (fullWord & ~mask) | shiftedValue;
    sac_write_32_bit_word(address >> 2, fullWord);
}

void sac_setup_v_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(void), void(*writeCallback)(t_uint64))
{
    VSTORE_LINE *l = &VStore[block][line];
    l->ReadCallback = readCallback;
    l->WriteCallback = writeCallback;
}

void sac_write_v_store(uint8 block, uint8 line, t_uint64 value)
{
    VSTORE_LINE *l = &VStore[block][line];
    if (l->WriteCallback != NULL)
    {
        l->WriteCallback(value);
    }
}

t_uint64 sac_read_v_store(uint8 block, uint8 line)
{
    t_uint64 result = 0;
    VSTORE_LINE *l = &VStore[block][line];
    if (l->ReadCallback != NULL)
    {
        result = l->ReadCallback();
    }

    return result;
}