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

This is the MU5 Store Access Control unit.

Known Limitations
-----------------
The following V Store lines are not implemented: CPR X FIELD, SAC PARITY, SAC MODE, UNIT STATUS, 1905E INTERRUPT.

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

typedef struct VSTORE_LINE
{
    t_uint64(*ReadCallback)(uint8 line);
    void(*WriteCallback)(uint8 line, t_uint64 value);
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
static uint32 CPRNotEquivalencePSX;
static uint16 CPRNotEquivalenceS;
static uint8 AccessViolation;
static uint8 SystemErrorInterrupt;

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

static t_uint64 prop_read_process_number_callback(uint8 line);
static void prop_write_process_number_callback(uint8 line, t_uint64 value);

static void sac_write_cpr_search_callback(uint8 line, t_uint64 value);
static void sac_write_cpr_number_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_ra_callback(uint8 line);
static void sac_write_cpr_ra_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_va_callback(uint8 line);
static void sac_write_cpr_va_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_ignore_callback(uint8 line);
static void sac_write_cpr_ignore_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_find_callback(uint8 line);
static void sac_write_cpr_find_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_altered_callback(uint8 line);
static void sac_write_cpr_altered_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_referenced_callback(uint8 line);
static void sac_write_cpr_referenced_callback(uint8 line, t_uint64 value);
static void sac_write_cpr_find_mask_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_access_violation_callback(uint8 line);
static void sac_write_access_violation_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_system_error_interrupts_callback(uint8 line);
static void sac_write_system_error_interrupts_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_not_equivalence_psx_callback(uint8 line);
static void sac_write_cpr_not_equivalence_psx_callback(uint8 line, t_uint64 value);
static t_uint64 sac_read_cpr_not_equivalence_s_callback(uint8 line);

static void sac_reset_cpr(uint8 n);
static uint32 sac_search_cprs(uint32 mask, uint32 va);
static uint32 sac_match_cprs(uint32 va, int *numMatches, int *firstMatchIndex, uint32 *segmentMask);
static int sac_check_access(uint8 requestedAccess, uint8 permittedAccess);
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
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, sac_read_cpr_find_callback, sac_write_cpr_find_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, sac_read_cpr_altered_callback, sac_write_cpr_altered_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, sac_read_cpr_referenced_callback, sac_write_cpr_referenced_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, NULL, sac_write_cpr_find_mask_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_PSX, sac_read_cpr_not_equivalence_psx_callback, sac_write_cpr_not_equivalence_psx_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_S, sac_read_cpr_not_equivalence_s_callback, NULL);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, sac_read_access_violation_callback, sac_write_access_violation_callback);
    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_SYSTEM_ERROR_INTERRUPTS, sac_read_system_error_interrupts_callback, sac_write_system_error_interrupts_callback);
    CPRNumber = 0;
    CPRFind = 0;
    CPRFindMask = 0;
    CPRIgnore = 0xFFFFFFFF;
    CPRAltered = 0;
    CPRReferenced = 0;
    CPRNotEquivalencePSX = 0;
    CPRNotEquivalenceS = 0;
    AccessViolation = 0;
    SystemErrorInterrupt = 0;
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

uint32 sac_read_32_bit_word_for_obey(t_addr address)
{
    t_addr mappedAddress = sac_map_address(address, SAC_OBEY_ACCESS);
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

uint16 sac_read_16_bit_word_for_obey(t_addr address)
{
    uint32 fullWord = sac_read_32_bit_word_for_obey(address >> 1);
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

void sac_setup_v_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(uint8), void(*writeCallback)(uint8,t_uint64))
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
        l->WriteCallback(line, value);
    }
}

t_uint64 sac_read_v_store(uint8 block, uint8 line)
{
    t_uint64 result = 0;
    VSTORE_LINE *l = &VStore[block][line];
    if (l->ReadCallback != NULL)
    {
        result = l->ReadCallback(line);
    }

    return result;
}

static t_uint64 prop_read_process_number_callback(uint8 line)
{
    return PROPProcessNumber & 0xF;
}

static void prop_write_process_number_callback(uint8 line, t_uint64 value)
{
    PROPProcessNumber = value & 0xF;
}

static void sac_write_cpr_search_callback(uint8 line, t_uint64 value)
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

    CPRFind |= sac_search_cprs(mask, value & VA_MASK);
}

static void sac_write_cpr_number_callback(uint8 line, t_uint64 value)
{
    CPRNumber = value & 0x1F;
}

static t_uint64 sac_read_cpr_ra_callback(uint8 line)
{
    return cpr[CPRNumber] & RA_MASK;
}

static void sac_write_cpr_ra_callback(uint8 line, t_uint64 value)
{
    cpr[CPRNumber] = (cpr[CPRNumber] & 0xFFFFFFFF00000000) | (value & RA_MASK);
}

static t_uint64 sac_read_cpr_va_callback(uint8 line)
{
    return (cpr[CPRNumber] >> 32) & VA_MASK;
}

static void sac_write_cpr_va_callback(uint8 line, t_uint64 value)
{
    cpr[CPRNumber] = ((value & VA_MASK) << 32) | (cpr[CPRNumber] & RA_MASK);
    sac_reset_cpr(CPRNumber);
}

static t_uint64 sac_read_cpr_ignore_callback(uint8 line)
{
    return CPRIgnore & 0xFFFFFFFF;
}

static void sac_write_cpr_ignore_callback(uint8 line, t_uint64 value)
{
    CPRIgnore = value & 0xFFFFFFFF;
}

static t_uint64 sac_read_cpr_find_callback(uint8 line)
{
    return CPRFind;
}

static void sac_write_cpr_find_callback(uint8 line, t_uint64 value)
{
    /* this action is not documented in the MU5 Programming Manual, but the line is marked R/W. In emails with RNI he believes that anything written
       to this line would cause a reset because the CPR FIND line is *updated* by CPR SEARCH not re-written */
    CPRFind = 0;
}

static t_uint64 sac_read_cpr_altered_callback(uint8 line)
{
    return CPRAltered;
}

static void sac_write_cpr_altered_callback(uint8 line, t_uint64 value)
{
    CPRAltered = value & 0xFFFFFFFF;
}

static t_uint64 sac_read_cpr_referenced_callback(uint8 line)
{
    return CPRReferenced;
}
static void sac_write_cpr_referenced_callback(uint8 line, t_uint64 value)
{
    CPRReferenced = value & 0xFFFFFFFF;
}

static void sac_write_cpr_find_mask_callback(uint8 line, t_uint64 value)
{
    CPRFindMask = value & 0x7FFFFFF;
}

static t_uint64 sac_read_access_violation_callback(uint8 line)
{
    return AccessViolation;
}

static void sac_write_access_violation_callback(uint8 line, t_uint64 value)
{
    AccessViolation = 0;
}

static t_uint64 sac_read_system_error_interrupts_callback(uint8 line)
{
    return SystemErrorInterrupt;
}

static void sac_write_system_error_interrupts_callback(uint8 line, t_uint64 value)
{
    SystemErrorInterrupt = SystemErrorInterrupt & 0x20; /* bit 58 cannot be reset */
}

static t_uint64 sac_read_cpr_not_equivalence_psx_callback(uint8 line)
{
    return CPRNotEquivalencePSX & 0x3FFFFFFF;
}

static void sac_write_cpr_not_equivalence_psx_callback(uint8 line, t_uint64 value)
{
    CPRNotEquivalencePSX = 0;
    CPRNotEquivalenceS = 0;
}

static t_uint64 sac_read_cpr_not_equivalence_s_callback(uint8 line)
{
    return CPRNotEquivalenceS & 0x3FFF;
}


static void sac_reset_cpr(uint8 n)
{
    CPRIgnore &= ~(1 << CPRNumber);
    CPRAltered &= ~(1 << CPRNumber);
    CPRReferenced &= ~(1 << CPRNumber);
    CPRFind &= ~(1 << CPRNumber);
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

static uint32 sac_match_cprs(uint32 va, int *numMatches, int *firstMatchIndex, uint32 *segmentMask)
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
                *segmentMask = mask;
            }
        }

        iresult = iresult << 1;
    }

    *numMatches = numMatchesResult;
    *firstMatchIndex = firstMatchIndexResult;

    return result;
}

static int sac_check_access(uint8 requestedAccess, uint8 permittedAccess)
{
    uint8 augmentedRequestedAccess = requestedAccess;
    uint8 augmentedPermittedAccess = permittedAccess;

    if (!(cpu_get_ms() & MS_MASK_EXEC))
    {
        augmentedRequestedAccess |= SAC_USER_ACCESS;
    }

    if (permittedAccess & SAC_WRITE_ACCESS)
    {
        augmentedPermittedAccess |= SAC_READ_ACCESS;
    }

    return augmentedRequestedAccess == (augmentedRequestedAccess & augmentedPermittedAccess & 0xF);
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
        uint32 segmentMask;
        uint32 va = (address >> 4) & 0x3FFFFFF;
        uint32 seg = (address >> 16) & 0x3FFF;
        if (seg < 8192)
        {
            va = ((uint32)PROPProcessNumber << 26) | va;
        }

        matchMask = sac_match_cprs(va, &numMatches, &firstMatchIndex, &segmentMask);
        if (numMatches == 1)
        {
            result = (cpr[firstMatchIndex] >> 4) & 0xFFFFFF;
            result += address & ((segmentMask << 4) | 0xF);

            if (!sac_check_access(access, (cpr[firstMatchIndex] >> 28) & 0xF))
            {
                cpu_set_interrupt(INT_PROGRAM_FAULTS);
                if (access & SAC_OBEY_ACCESS)
                {
                    AccessViolation |= 0x6;
                }
                else
                {
                    AccessViolation |= 0x2;
                }
            }
        }
        else if (numMatches == 0)
        {
            CPRNotEquivalencePSX = va;
            CPRNotEquivalenceS = seg;
            cpu_set_interrupt(INT_CPR_NOT_EQUIVALENCE);
        }
        else
        {
            cpu_set_interrupt(INT_SYSTEM_ERROR);
            SystemErrorInterrupt |= 0x40;
        }

        if (access & (SAC_OBEY_ACCESS | SAC_READ_ACCESS))
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