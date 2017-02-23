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
#include "mu5_cpu.h"
#include "mu5_sac.h"

#define NUM_CPRS 32
#define V_STORE_BLOCKS 8
#define V_STORE_BLOCK_SIZE 256
#define VA_MASK 0x3FFFFFFF
#define VA_P_MASK (0xF << 26)
#define VA_X_MASK (0xFFF)
#define RA_MASK 0x7FFFFFFF
#define CPR_FIND_MASK_P_MASK 0x4000000
#define CPR_FIND_MASK_S_MASK 0x3FFF000
#define CPR_FIND_MASK_X_MASK 0x0000001

#define SAC_EXEC_ACCESS 0x8
#define SAC_READ_ACCESS 0x4
#define SAC_WRITE_ACCESS 0x2
#define SAC_OBEY_ACCESS 0x1

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

static uint8 PROPProcessNumber;
static uint8 CPRNumber;
static uint32 CPRFind;
static uint32 CPRFindMask;
static uint32 CPRIgnore;
static uint32 CPRAltered;
static uint32 CPRReferenced;

static t_uint64 cpr[NUM_CPRS];

BITFIELD cpr_bits[] = {
    BITF(LZ,4),        /* RA page size as a number of mask bits 0-12 */
    BITF(ADDRESS,19),  /* RA address */
    BIT(V),            /* RA Address V-Store Bit*/
    BITF(UNIT,4),      /* RA unit part of address */
    BITF(ACCESS,4),    /* RA Access permissions */
    BITF(X,12),        /* VA X */
    BITF(SEGMENT,14),  /* VA Segment */
    BITF(PROCESS,4),   /* VA Process */
    BITNCF(2),
    ENDBITS
};

static REG sac_reg[] =
{
    { BRDATADF(CPR, cpr, 16, 64, NUM_CPRS, "CPR register", cpr_bits) },
    { NULL }
};

static MTAB sac_mod[] =
{
    { 0 }
};

static DEBTAB sac_debtab[] =
{
    { "EVENT",          SIM_DBG_EVENT,     "event dispatch activities" },
    { "SELFTESTDETAIL", LOG_CPU_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_CPU_SELFTEST_FAIL,  "self test failure output" },
    { NULL,           0 }
};

static const char* sac_description(DEVICE *dptr) {
    return "Store Access Control Unit";
}

static t_stat sac_reset(DEVICE *dptr);

static t_uint64 prop_read_process_number_callback(void);
static void prop_write_process_number_callback(t_uint64 value);

static void sac_write_cpr_search_callback(t_uint64 value);
static void sac_write_cpr_number_callback(t_uint64 value);
static t_uint64 sac_read_cpr_ra_callback(void);
static void sac_write_cpr_ra_callback(t_uint64 value);
static t_uint64 sac_read_cpr_va_callback(void);
static void sac_write_cpr_va_callback(t_uint64 value);
static t_uint64 sac_read_cpr_ignore_callback(void);
static void sac_write_cpr_ignore_callback(t_uint64 value);
static t_uint64 sac_read_cpr_find_callback(void);
static t_uint64 sac_read_cpr_altered_callback(void);
static void sac_write_cpr_altered_callback(t_uint64 value);
static t_uint64 sac_read_cpr_referenced_callback(void);
static void sac_write_cpr_referenced_callback(t_uint64 value);
static void sac_write_cpr_find_mask_callback(t_uint64 value);

static void sac_reset_cpr(uint8 n);
static uint32 sac_search_cprs(uint32 mask, uint32 va);
static uint32 sac_match_cprs(uint8 access, uint32 va, int *numMatches, int *firstMatchIndex);
static t_addr sac_map_address(t_addr address, uint8 access);

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
    memset(cpr, 0, sizeof(cpr));

    sac_setup_v_store_location(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, prop_read_process_number_callback, prop_write_process_number_callback);

    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, NULL, sac_write_cpr_search_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, NULL, sac_write_cpr_number_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, sac_read_cpr_ra_callback, sac_write_cpr_ra_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, sac_read_cpr_va_callback, sac_write_cpr_va_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, sac_read_cpr_ignore_callback, sac_write_cpr_ignore_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, sac_read_cpr_find_callback, NULL);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, sac_read_cpr_altered_callback, sac_write_cpr_altered_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, sac_read_cpr_referenced_callback, sac_write_cpr_referenced_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, NULL, sac_write_cpr_find_mask_callback);
    CPRNumber = 0;
    CPRFind = 0;
    CPRFindMask = 0;
    CPRIgnore = 0xFFFFFFFF;
    CPRAltered = 0;
    CPRReferenced = 0;
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
    t_addr mappedAddress = sac_map_address(address, SAC_READ_ACCESS);
    uint32 result = LocalStore[mappedAddress];
    return result;
}

void sac_write_32_bit_word(t_addr address, uint32 value)
{
    t_addr mappedAddress = sac_map_address(address, SAC_WRITE_ACCESS);
    LocalStore[mappedAddress] = value;
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

/* Intended for self test code only, not to be used by anything else */
uint32 sac_read_32_bit_word_real_address(t_addr address)
{
    uint32 result = LocalStore[address];
    return result;
}

/* Intended for self test code only, not to be used by anything else */
void sac_write_32_bit_word_real_address(t_addr address, uint32 value)
{
    LocalStore[address] = value;
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

static t_uint64 prop_read_process_number_callback(void)
{
    return PROPProcessNumber & 0xF;
}

static void prop_write_process_number_callback(t_uint64 value)
{
    PROPProcessNumber = value & 0xF;
}

static void sac_write_cpr_search_callback(t_uint64 value)
{
    uint32 mask = CPRFindMask & CPR_FIND_MASK_S_MASK;
    if (CPRFindMask & CPR_FIND_MASK_P_MASK)
    {
        mask = mask | VA_P_MASK;
    }
    if (CPRFindMask & CPR_FIND_MASK_X_MASK)
    {
        mask = mask | VA_X_MASK;
    }

    CPRFind = sac_search_cprs(mask, value & VA_MASK);
}

static void sac_write_cpr_number_callback(t_uint64 value)
{
    CPRNumber = value & 0x1F;
}

static t_uint64 sac_read_cpr_ra_callback(void)
{
    return cpr[CPRNumber] & RA_MASK;
}

static void sac_write_cpr_ra_callback(t_uint64 value)
{
    cpr[CPRNumber] = (cpr[CPRNumber] & 0xFFFFFFFF00000000) | (value & RA_MASK);
}

static t_uint64 sac_read_cpr_va_callback(void)
{
    return (cpr[CPRNumber] >> 32) & VA_MASK;
}

static void sac_write_cpr_va_callback(t_uint64 value)
{
    cpr[CPRNumber] = ((value & VA_MASK) << 32) | (cpr[CPRNumber] & RA_MASK);
    sac_reset_cpr(CPRNumber);
}

static t_uint64 sac_read_cpr_ignore_callback(void)
{
    return CPRIgnore & 0xFFFFFFFF;
}

static void sac_write_cpr_ignore_callback(t_uint64 value)
{
    CPRIgnore = value & 0xFFFFFFFF;
}

static t_uint64 sac_read_cpr_find_callback(void)
{
    return CPRFind;
}

static t_uint64 sac_read_cpr_altered_callback(void)
{
    return CPRAltered;
}

static void sac_write_cpr_altered_callback(t_uint64 value)
{
    CPRAltered = value & 0xFFFFFFFF;
}

static t_uint64 sac_read_cpr_referenced_callback(void)
{
    return CPRReferenced;
}
static void sac_write_cpr_referenced_callback(t_uint64 value)
{
    CPRReferenced = value & 0xFFFFFFFF;
}

static void sac_write_cpr_find_mask_callback(t_uint64 value)
{
    CPRFindMask = value & 0x7FFFFFF;
}

static void sac_reset_cpr(uint8 n)
{
    CPRIgnore &= ~(1 << CPRNumber);
    CPRAltered &= ~(1 << CPRNumber);
    CPRReferenced &= ~(1 << CPRNumber);
}

static uint32 sac_search_cprs(uint32 mask, uint32 va)
{
    int i;
    uint32 result = 0;
    uint32 iresult = 1;
    for (i = 0; i < NUM_CPRS; i++)
    {
        if (!(CPRIgnore & iresult))
        {
            if ((va & ~mask) == ((cpr[i] >> 32) & ~mask))
            {
                result = result | iresult;
            }
        }

        iresult = iresult << 1;
    }

    return result;
}

static uint32 sac_match_cprs(uint8 access, uint32 va, int *numMatches, int *firstMatchIndex)
{
    int i;
    uint32 result = 0;
    uint32 mask = 0;
    uint32 maskLen;
    uint32 iresult = 1;
    int numMatchesResult = 0;
    int firstMatchIndexResult = -1;
    for (i = 0; i < NUM_CPRS; i++)
    {
        if (!(CPRIgnore & iresult))
        {
            maskLen = cpr[i] & 0xF;
            mask = ~((0xFFF << maskLen)) & 0xFFF;
            if ((va & ~mask) == ((cpr[i] >> 32) & ~mask))
            {
                numMatchesResult++;
                if (numMatchesResult == 1)
                {
                    firstMatchIndexResult = i;
                }

                result = result | iresult;
            }
        }

        iresult = iresult << 1;
    }

    if (numMatches != NULL)
    {
        *numMatches = numMatchesResult;
    }

    if (firstMatchIndex != NULL)
    {
        *firstMatchIndex = firstMatchIndexResult;
    }

    return result;
}

static t_addr sac_map_address(t_addr address, uint8 access)
{
    t_addr result = 0;
    if (cpu_get_ms() & MS_MASK_BCPR)
    {
        result = address;
    }
    else
    {
        int numMatches;
        int firstMatchIndex;
        uint32 matchMask;
        uint32 va = (address >> 4) & 0x3FFFFFF;
        uint32 seg = (address >> 16) & 0x3FFF;
        if (seg < 8192)
        {
            va = ((uint32)PROPProcessNumber << 26) | va;
        }

        matchMask = sac_match_cprs(0, va, &numMatches, &firstMatchIndex);
        if (numMatches == 1)
        {
            result = (cpr[firstMatchIndex] >> 4) & 0xFFFFFF;
            result += address & 0xF;
        }
        else if (numMatches == 0)
        {
            cpu_set_interrupt(INT_CPR_NOT_EQUIVALENCE);
        }
        else
        {
            printf("Multi-equivalence\n");
        }

        if (access & SAC_READ_ACCESS)
        {
            CPRReferenced |= matchMask;
        }
        else if (access & SAC_WRITE_ACCESS)
        {
            CPRAltered |= matchMask;
        }
    }

    return result;
}