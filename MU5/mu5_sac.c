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

This is the MU5 Store Access Control unit. The Local Store consisted of four
4096-word memory units, each word containing 64 data bits + 8 parity bits.
The Mass Store consisted of two 128K-word memory units, each word containing
36 bits. The Fixed-head Disc consisted of two 2.4 Mbyte units.

In the real MU5 the local store was accessed directly by SAC, all the other stores
were accessed via the Exchange. This module effectively includes the Exchange by
accessing all the other stores too.

There are believed to be 4 hard-wired CPRs. When asked in April 2017 RNI was
not sure which ones and did not know their values. He said: "Assume 4 for now
and assume they are numbers 28-31. Writing to them would have no effect."

Addresses
---------

The rules are as follows:

1. All accesses to Local Store, Mass Store and the other units use addresses
   of 64-bit words. The Local Store is accessed directly without going through
   the Exchange. All other units, including the Mass Store are accessed
   through the Exchange.

2. For 64-bit and 32-bit named variables, real addresses are always in 32-bit
   word increments. All units therefore receive addresses in 32-bit increments
   but ignore the least significant address bit and treat the address as the
   address of a 64-bit word.

2. For 64-bit and 32-bit named variables, real addresses are always in 32-bit
   word increments. To access the store the least significant bit is removed
   and the remainder of the address is presented to the store. This means that
   64-bit named variables are aligned on a 64-bit boundary. For 32-bit variables
   the least significant bit of the 32-bit word address is used to select the
   appropriate half of the word after it has come back from the store.


3. For descriptors, the real addresses are always in 8-bit word increments,
   but vectors always start on a 32-bit word boundary. As the store access
   is always in 64-bit units the 3rd least significant bit is used to
   choose which half of the 64-bit word is to be used as the start of the
   vector. While processing a vector the elements can follow without needing
   alignment beyond the 8-bit word. This means that 64-bit elements can
   straddle 64-bit alignment boundaries.

4. For CO, the real addresses are always in 16-bit word increments. To access
   the store, the two least significant bits are removed from the address
   presented to the store to get the address of a 64-bit word. The processor
   then uses the two least significant bits of the address to choose the 16-bit
   portion of the word that has been selected.

5. Any real address can be used to access any unit. This means that theoretically
   a CPR could map a virtual address to an address in the Fixed Head Disc Vx
   Store, although in practice this makes little sense.

Notes on V and Vx Store Access
------------------------------

V-store is only accessed using k'=7 operands, with System Store also accessible through segment 8192.
Vx-store is accessed by real addresses with the V bit (see section 2.13 of the Programming Manual) set,
so can be accessed by type 3.0 descriptors, or if the CPRs are bypassed (MS8=1). Vx-store cannot be
accessed using a virtual address, partly because the real address component of the CPR RA field is not wide
enough to hold the V bit, but mainly because the hardware did not support this as the designers felt that
it should not be accessible through user memory addressing.

When a V-store access to the System Store is made (block 0), this is turned into a virtual address access
for segment 8192. In practice this means that a CPR must always be defined that maps segment 8192 to an
area of the local store. So the System Store can also be accessed using any operand type that creates a
virtual address in segment 8192.

A note on the V-store terminology (RNI 4th June 2017):
The V-store terminology derives from Atlas, in which the V-store was an early (possibly the first)
implementation of memory-mapped I/O. Previous machines had generally included special functions to deal
with I/O but Atlas had the added need to manipulate the registers that implemented the paging system,
so the idea of using a separate addressing mechanism for all the special purpose registers, and then
being able to use standard functions to manipulate them, was seen as a victory over the problem - hence V-store.


Known Limitations
-----------------
The following V Store lines are not implemented: CPR X FIELD, SAC PARITY, SAC MODE, UNIT STATUS, 1905E INTERRUPT.

*/

#include <assert.h>
#include "mu5_defs.h"
#include "mu5_cpu.h"
#include "mu5_sac.h"
#include "mu5_drum.h"

#define NUM_CPRS 32
#define CPR_VA_MASK 0x3FFFFFFF
#define CPR_VA_P_MASK (0xF << 26)
#define CPR_VA_X_MASK (0xFFF)
#define CPR_RA_MASK 0xFFFFFFFF
#define CPR_FIND_MASK_P_MASK 0x4000000
#define CPR_FIND_MASK_S_MASK 0x3FFF000
#define CPR_FIND_MASK_X_MASK 0x0000001

#define LOG_SAC_REAL_ACCESSES   (1 << 0)
#define LOG_SAC_MEMORY_TRACE    (1 << 1)
#define LOG_SAC_CPR_ERROR       (1 << 2)

#define CPR_MASK(n) (0x80000000 >> n)

static t_stat sac_reset(DEVICE *dptr);

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
static int sac_match_cpr(int cpr_num, uint32 *mask, uint32 va, uint32 *match_result);
static uint32 sac_search_cprs(uint32 mask, uint32 va);
static uint32 sac_match_cprs(uint32 va, int *numMatches, int *firstMatchIndex, uint32 *segmentMask);
static int sac_check_access(uint8 requestedAccess, uint8 permittedAccess);
static int sac_map_address(t_addr address, uint8 access, t_addr *mappedAddress);
static uint8 sac_get_real_address_unit(t_addr address);
static uint32 sac_read_local_store(t_addr address);
static void sac_write_local_store(t_addr address, uint32 value);
static uint32 sac_read_mass_store(t_addr address);
static void sac_write_mass_store(t_addr address, uint32 value);

static void sac_v_store_register_read_callback(struct REG *reg, int index);
static void sac_v_store_register_write_callback(t_value old_val, struct REG *reg, int index);

static uint32 LocalStore[MAX_LOCAL_MEMORY];
static uint32 MassStore[MAX_MASS_MEMORY];
VSTORE_LINE VStore[V_STORE_BLOCKS][V_STORE_BLOCK_SIZE];

static UNIT sac_unit =
{
    UDATA(NULL, UNIT_FIX | UNIT_BINK, MAX_LOCAL_MEMORY)
};

extern t_uint64 *PROPProcessNumber;
static t_uint64 *CPRNumber;
static t_uint64 *CPRRa;
static t_uint64 *CPRVa;
static t_uint64 *CPRFind;
static t_uint64 *CPRFindMask;
static t_uint64 *CPRIgnore;
static t_uint64 *CPRAltered;
static t_uint64 *CPRReferenced;
static t_uint64 *CPRNotEquivalencePSX;
static t_uint64 *CPRNotEquivalenceS;
static t_uint64 *AccessViolation;
static t_uint64 *SystemErrorInterrupt;

static int OverrideAccessCheck;

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
    { STRDATADFC(V, VStore[SAC_V_STORE_BLOCK], 16, 64, 0, V_STORE_BLOCK_SIZE, sizeof(VSTORE_LINE), 0, "V Store", NULL, sac_v_store_register_read_callback, sac_v_store_register_write_callback) },
	{ NULL }
};

static MTAB sac_mod[] =
{
    { 0 }
};

static DEBTAB sac_debtab[] =
{
    { "EVENT",          SIM_DBG_EVENT,     "event dispatch activities" },
    { "SELFTESTDETAIL", LOG_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_SELFTEST_FAIL,  "self test failure output" },
	{ "ERROR",          LOG_ERROR, "significant errors" },
	{ "REAL",           LOG_SAC_REAL_ACCESSES, "real address accesses" },
    { "CPR",            LOG_SAC_CPR_ERROR, "CPR errors" },
	{ NULL,           0 }
};

static const char* sac_description(DEVICE *dptr) {
    return "Store Access Control Unit";
}

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
	if (sim_switches & SWMASK('P')) /* P is the power up switch, MU5 used core so the memory really was non-volatile, but here we want to make the machine have a clean start when the emulator is first started */
	{
		memset(LocalStore, 0, sizeof(uint32) * MAX_LOCAL_MEMORY);
		memset(MassStore, 0, sizeof(uint32) * MAX_MASS_MEMORY);
	}
	memset(VStore, 0, sizeof(VStore));
    memset(cpr, 0, sizeof(cpr));

	/* set up the reserved CPRs. These are partially made-up values, I don't know the originals. AEK's thesis lists 4 fixed CPRs, while a picture taken
       of a sheet on the MU5 console still in the Department lists some of the common segments, some of which appear to correspond to the fixed CPRs. The
       list below is the list from AEK's thesis, along with my guess at which entry from the sheet of common segments they apply to.

       1. Locked down code for the supervisor-supervisor.
       2. Locked down data for the supervisor-supervisor.
       3. Data block referencing the currently running process and its page tables.
       4. Mass Store access (CPR31). This looks to be segment 8300 from the photograph of the common segments.

       NB Size 4 = 256 words or 1K bytes, which is the only value MUSS ever actually used.
      
    */

    /* at the moment these are all the same value as I don't know what they should be, just make sure the
	   CPR IGNORE ignores all but one of them to avoid a multiple equivalence error */
	cpr[28] = ((t_uint64)CPR_VA(0x0, 8193, 0x0) << 32) | CPR_RA_LOCAL(SAC_OBEY_ACCESS | SAC_READ_ACCESS, 0x07D00, 0x4); /* MUSS code (first point, interrupt level, locked in core, the rest paged (Item 1 in the list above) */
	cpr[29] = ((t_uint64)CPR_VA(0x0, 8194, 0x0) << 32) | CPR_RA_LOCAL(SAC_READ_ACCESS | SAC_WRITE_ACCESS, 0x07E00, 0x4); /* MUSS Interrupt Level Names and Run Time Stack (Item 2 in the list above) */
	cpr[30] = ((t_uint64)CPR_VA(0x0, 8192, 0x0) << 32) | CPR_RA_LOCAL(SAC_READ_ACCESS | SAC_WRITE_ACCESS, 0x07F00, 0x4); /* Segment 8192 needs to be mapped, this slot previously for item 3 above (segment 8196) */
	cpr[31] = ((t_uint64)CPR_VA(0x0, 8300, 0x0) << 32) | CPR_RA_MASS(SAC_ALL_EXEC_ACCESS, 0x00000, 0xC);  /* Mass store (less frequently used tables locked in slow core) (Item 4 in the list above)*/

    sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, NULL, sac_write_cpr_search_callback);
    CPRNumber = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, NULL, sac_write_cpr_number_callback);
    CPRRa = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, sac_read_cpr_ra_callback, sac_write_cpr_ra_callback);
    CPRVa = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, sac_read_cpr_va_callback, sac_write_cpr_va_callback);
    CPRIgnore = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, sac_read_cpr_ignore_callback, sac_write_cpr_ignore_callback);
    CPRFind = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, sac_read_cpr_find_callback, sac_write_cpr_find_callback);
    CPRAltered = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, sac_read_cpr_altered_callback, sac_write_cpr_altered_callback);
    CPRReferenced = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, sac_read_cpr_referenced_callback, sac_write_cpr_referenced_callback);
    CPRFindMask = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, NULL, sac_write_cpr_find_mask_callback);
    CPRNotEquivalencePSX = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_PSX, sac_read_cpr_not_equivalence_psx_callback, sac_write_cpr_not_equivalence_psx_callback);
    CPRNotEquivalenceS = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_S, sac_read_cpr_not_equivalence_s_callback, NULL);
    AccessViolation = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, sac_read_access_violation_callback, sac_write_access_violation_callback);
    SystemErrorInterrupt = sac_setup_v_store_location(SAC_V_STORE_BLOCK, SAC_V_STORE_SYSTEM_ERROR_INTERRUPTS, sac_read_system_error_interrupts_callback, sac_write_system_error_interrupts_callback);
    *CPRNumber = 0;
    *CPRFind = 0;
    *CPRFindMask = 0;
    *CPRIgnore = 0xFFFFFFF0; /* Enable the reserved CPRs 28-31 */
    *CPRAltered = 0;
    *CPRReferenced = 0;
    *CPRNotEquivalencePSX = 0;
    *CPRNotEquivalenceS = 0;
    *AccessViolation = 0;
    *SystemErrorInterrupt = 0;

	OverrideAccessCheck = 0;
}

void sac_set_loading(void)
{
	OverrideAccessCheck = 1;
}

void sac_clear_loading(void)
{
	OverrideAccessCheck = 0;
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
	t_addr mappedAddress;
	uint32 result = 0;
	if (sac_map_address(address, SAC_READ_ACCESS, &mappedAddress))
	{
		result = sac_read_32_bit_word_real_address(mappedAddress);
	}

    return result;
}

uint32 sac_read_32_bit_word_for_obey(t_addr address)
{
	t_addr mappedAddress;
	uint32 result = 0;
	if (sac_map_address(address, SAC_OBEY_ACCESS, &mappedAddress))
	{
		result = sac_read_32_bit_word_real_address(mappedAddress);
	}

	return result;
}

void sac_write_32_bit_word(t_addr address, uint32 value)
{
	t_addr mappedAddress;
	if (sac_map_address(address, SAC_WRITE_ACCESS, &mappedAddress))
	{
		sac_write_32_bit_word_real_address(mappedAddress, value);
	}
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

t_uint64 sac_read_64_bit_word_real_address(t_addr address)
{
	t_uint64 result = ((t_uint64)sac_read_32_bit_word_real_address(address) << 32) | sac_read_32_bit_word_real_address(address + 1);
	return result;
}

void sac_write_64_bit_word_real_address(t_addr address, t_uint64 value)
{
	sac_write_32_bit_word_real_address(address, (value >> 32) & MASK_32);
	sac_write_32_bit_word_real_address(address + 1, value & MASK_32);
}

uint32 sac_read_32_bit_word_real_address(t_addr address)
{
    uint32 result = 0;
    t_addr addr20 = address & RA_MASK;
    uint8 unit = sac_get_real_address_unit(address);
    switch (unit)
    {
		case UNIT_FIXED_HEAD_DISC:
		{
			if (address & 1)
			{
				result = drum_exch_read(address) & MASK_32;
			}
			else
			{
				result = (drum_exch_read(address) >> 32) & MASK_32;
			}

			sim_debug(LOG_SAC_REAL_ACCESSES, &sac_dev, "Read drum real address %08X, result=%08X\n", address, result);
			break;
		}

		case UNIT_LOCAL_STORE:
		{
			result = sac_read_local_store(addr20);
			sim_debug(LOG_SAC_REAL_ACCESSES, &sac_dev, "Read local store real address %08X, result=%08X\n", address, result);
			break;
		}

		case UNIT_MASS_STORE:
        {
            result = sac_read_mass_store(addr20);
            sim_debug(LOG_SAC_REAL_ACCESSES, &sac_dev, "Read mass store real address %08X, result=%08X\n", address, result);
            break;
        }

        default:
        {
            result = 0;
            sim_debug(LOG_ERROR, &sac_dev, "Read unknown (%hhu) store real address %08X, result=%08X\n", unit, address, result);
            break;
        }
    }

    return result;
}

void sac_write_32_bit_word_real_address(t_addr address, uint32 value)
{
    t_addr addr20 = address & RA_MASK;
    uint8 unit = sac_get_real_address_unit(address);
	switch (unit)
    {
		case UNIT_FIXED_HEAD_DISC:
		{
			/* TODO: This code won't work with full 64-bit writes */
			if (address & 1)
			{
				drum_exch_write(address, value);
			}
			else
			{
				drum_exch_write(address, (t_uint64)value << 32);
			}

			sim_debug(LOG_SAC_REAL_ACCESSES, &sac_dev, "Write drum real address %08X, value=%08X\n", address, value);
			break;
		}

		case UNIT_LOCAL_STORE:
		{
			sac_write_local_store(addr20, value);
			sim_debug(LOG_SAC_REAL_ACCESSES, &sac_dev, "Write local store real address %08X, value=%08X\n", address, value);
			break;
		}

		case UNIT_MASS_STORE:
        {
            sac_write_mass_store(addr20, value);
            sim_debug(LOG_SAC_REAL_ACCESSES, &sac_dev, "Write mass store real address %08X, value=%08X\n", address, value);
            break;
        }

        default:
        {
            sim_debug(LOG_ERROR, &sac_dev, "Write unknown (%hhu) store real address %08X, value=%08X\n", unit, address, value);
            break;
        }
    }
}

void sac_write_8_bit_word_real_address(t_addr address, uint8 value)
{
    uint32 fullWord = sac_read_32_bit_word_real_address(address >> 2);
    uint8 byteNumber = 3 - (address & 0x3);
    uint32 mask = 0xFF << (byteNumber << 3);
    uint32 shiftedValue = (uint32)value << (byteNumber << 3);
    fullWord = (fullWord & ~mask) | shiftedValue;
    sac_write_32_bit_word_real_address(address >> 2, fullWord);
}

t_uint64 *sac_setup_v_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(uint8), void(*writeCallback)(uint8,t_uint64))
{
    VSTORE_LINE *l = &VStore[block][line];
    l->ReadCallback = readCallback;
    l->WriteCallback = writeCallback;
    return &l->value;
}

void sac_write_v_store(uint8 block, uint8 line, t_uint64 value)
{
    VSTORE_LINE *l;
    assert(block < V_STORE_BLOCKS);
    l = &VStore[block][line];
	if (block == SYSTEM_V_STORE_BLOCK)
	{
		t_addr addr = addr = 0x20000000 | (block << 9) | (line << 1); /* 64-bit address so block and line shifted left by 1 */
		sac_write_64_bit_word(addr, value);
	}
	else if (l->WriteCallback != NULL)
    {
        l->WriteCallback(line, value);
    }
}

t_uint64 sac_read_v_store(uint8 block, uint8 line)
{
    t_uint64 result = 0;
    VSTORE_LINE *l;
    assert(block < V_STORE_BLOCKS);
    l = &VStore[block][line];
	if (block == SYSTEM_V_STORE_BLOCK)
	{
		t_addr addr = addr = 0x20000000 | (block << 9) | (line << 1); /* 64-bit address so block and line shifted left by 1 */
		result = sac_read_64_bit_word(addr);
	}
	else if (l->ReadCallback != NULL)
    {
        result = l->ReadCallback(line);
    }

    return result;
}

static void sac_v_store_register_read_callback(struct REG *reg, int index)
{
    assert(reg->width == 64);
    sac_read_v_store(SAC_V_STORE_BLOCK, index);
}

static void sac_v_store_register_write_callback(t_value old_val, struct REG *reg, int index)
{
    assert(reg->width == 64);
    sac_write_v_store(SAC_V_STORE_BLOCK, index, ((VSTORE_LINE *)reg->loc + index)->value);
}

static void sac_write_cpr_search_callback(uint8 line, t_uint64 value)
{
    uint32 mask = *CPRFindMask & CPR_FIND_MASK_S_MASK;
    if (*CPRFindMask & CPR_FIND_MASK_P_MASK)
    {
        mask = mask | CPR_VA_P_MASK;
    }
    if (*CPRFindMask & CPR_FIND_MASK_X_MASK)
    {
        mask = mask | CPR_VA_X_MASK;
    }

    *CPRFind |= sac_search_cprs(mask, value & CPR_VA_MASK);
}

static void sac_write_cpr_number_callback(uint8 line, t_uint64 value)
{
    *CPRNumber = value & 0x1F;
}

static t_uint64 sac_read_cpr_ra_callback(uint8 line)
{
    *CPRRa = cpr[*CPRNumber] & CPR_RA_MASK;
    return *CPRRa;
}

static void sac_write_cpr_ra_callback(uint8 line, t_uint64 value)
{
	if (*CPRNumber < 28)
	{
		cpr[*CPRNumber] = (cpr[*CPRNumber] & 0xFFFFFFFF00000000) | (value & CPR_RA_MASK);
	}
}

static t_uint64 sac_read_cpr_va_callback(uint8 line)
{
    *CPRVa = (cpr[*CPRNumber] >> 32) & CPR_VA_MASK;
    return *CPRVa;
}

static void sac_write_cpr_va_callback(uint8 line, t_uint64 value)
{
	if (*CPRNumber < 28)
	{
		cpr[*CPRNumber] = ((value & CPR_VA_MASK) << 32) | (cpr[*CPRNumber] & CPR_RA_MASK);
		sac_reset_cpr((uint8)(*CPRNumber));
	}
}

static t_uint64 sac_read_cpr_ignore_callback(uint8 line)
{
    return *CPRIgnore & 0xFFFFFFFF;
}

static void sac_write_cpr_ignore_callback(uint8 line, t_uint64 value)
{
    *CPRIgnore = value & 0xFFFFFFFF;
}

static t_uint64 sac_read_cpr_find_callback(uint8 line)
{
    return *CPRFind;
}

static void sac_write_cpr_find_callback(uint8 line, t_uint64 value)
{
    /* this action is not documented in the MU5 Programming Manual, but the line is marked R/W. In emails with RNI he believes that anything written
       to this line would cause a reset because the CPR FIND line is *updated* by CPR SEARCH not re-written */
    *CPRFind = 0;
}

static t_uint64 sac_read_cpr_altered_callback(uint8 line)
{
    return *CPRAltered;
}

static void sac_write_cpr_altered_callback(uint8 line, t_uint64 value)
{
    *CPRAltered = value & 0xFFFFFFFF;
}

static t_uint64 sac_read_cpr_referenced_callback(uint8 line)
{
    return *CPRReferenced;
}
static void sac_write_cpr_referenced_callback(uint8 line, t_uint64 value)
{
    *CPRReferenced = value & 0xFFFFFFFF;
}

static void sac_write_cpr_find_mask_callback(uint8 line, t_uint64 value)
{
    *CPRFindMask = value & 0x7FFFFFF;
}

static t_uint64 sac_read_access_violation_callback(uint8 line)
{
    return *AccessViolation;
}

static void sac_write_access_violation_callback(uint8 line, t_uint64 value)
{
    *AccessViolation = 0;
}

static t_uint64 sac_read_system_error_interrupts_callback(uint8 line)
{
    return *SystemErrorInterrupt;
}

static void sac_write_system_error_interrupts_callback(uint8 line, t_uint64 value)
{
    *SystemErrorInterrupt = *SystemErrorInterrupt & 0x20; /* bit 58 cannot be reset */
}

static t_uint64 sac_read_cpr_not_equivalence_psx_callback(uint8 line)
{
    return *CPRNotEquivalencePSX & 0x3FFFFFFF;
}

static void sac_write_cpr_not_equivalence_psx_callback(uint8 line, t_uint64 value)
{
    *CPRNotEquivalencePSX = 0;
    *CPRNotEquivalenceS = 0;
}

static t_uint64 sac_read_cpr_not_equivalence_s_callback(uint8 line)
{
    return *CPRNotEquivalenceS & 0x3FFF;
}


static void sac_reset_cpr(uint8 n)
{
    *CPRIgnore &= ~CPR_MASK(n);
    *CPRAltered &= ~CPR_MASK(n);
    *CPRReferenced &= ~CPR_MASK(n);
    *CPRFind &= ~CPR_MASK(n);
}

static int sac_match_cpr(int cpr_num, uint32 *mask, uint32 va, uint32 *match_result)
{
	int result = 0;
	uint32 result_mask = CPR_MASK(cpr_num);

	if (!(*CPRIgnore & result_mask))
	{
		if (cpr_num == 31)
		{
			*mask |= 0xF000; /* Allow CPR 31 to map 1MB segment as per AEK thesis */
		}
		if ((va & ~*mask) == ((cpr[cpr_num] >> 32) & ~*mask))
		{
			result = 1;
			*match_result = *match_result | result_mask;
		}
	}

	return result;
}

static uint32 sac_search_cprs(uint32 mask, uint32 va)
{
    int i;
    uint32 result = 0;
    for (i = 0; i < NUM_CPRS; i++)
    {
		sac_match_cpr(i, &mask, va, &result);
    }

    return result;
}

static uint32 sac_match_cprs(uint32 va, int *numMatches, int *firstMatchIndex, uint32 *segmentMask)
{
	  int i;
	  uint32 result = 0;
	  uint32 mask = 0;
	  uint32 maskLen;
	  int numMatchesResult = 0;
	  int firstMatchIndexResult = -1;
	  for (i = 0; i < NUM_CPRS; i++)
	  {
		  maskLen = cpr[i] & 0xF;
		  mask = ~((0xFFF << maskLen)) & 0xFFF;
		  if (sac_match_cpr(i, &mask, va, &result))
		  {
		  	numMatchesResult++;
		  	if (numMatchesResult == 1)
		  	{
		  		firstMatchIndexResult = i;
		  	}
            else
            {
                sim_debug(LOG_SAC_CPR_ERROR, &sac_dev, "CPR multiple equivalence with CPR %d and CPR %d for virtual address 0x%08X\n", firstMatchIndexResult, i, va);
            }

		  	*segmentMask = mask;
		  }
	  }

	  *numMatches = numMatchesResult;
	  *firstMatchIndex = firstMatchIndexResult;

	  return result;
}

static int sac_check_access(uint8 requestedAccess, uint8 permittedAccess)
{
	int result;
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

    result = OverrideAccessCheck | (augmentedRequestedAccess == (augmentedRequestedAccess & augmentedPermittedAccess & 0xF));
	return result;
}

static int sac_map_address(t_addr address, uint8 access, t_addr *mappedAddress)
{
	int result = 0;
	uint32 va = (address >> 4) & 0x3FFFFFF;
	uint32 seg = (address >> 16) & 0x3FFF;
	*mappedAddress = 0;
    if (cpu_get_ms() & MS_MASK_BCPR)
    {
        *mappedAddress = address;
		result = 1;
    }
    else
    {
        int numMatches;
        int firstMatchIndex;
        uint32 matchMask;
        uint32 segmentMask;
        if (seg < 8192)
        {
            va = ((uint32)(*PROPProcessNumber) << 26) | va;
        }

        matchMask = sac_match_cprs(va, &numMatches, &firstMatchIndex, &segmentMask);
        if (numMatches == 1)
        {
            *mappedAddress = (cpr[firstMatchIndex] >> 4) & 0xFFFFFF;
            *mappedAddress += address & ((segmentMask << 4) | 0xF);

            if (!sac_check_access(access, (cpr[firstMatchIndex] >> 28) & 0xF))
            {
                if (access & SAC_OBEY_ACCESS)
                {
                    *AccessViolation |= 0x6;
                }
                else
                {
                    *AccessViolation |= 0x2;
                }

                cpu_set_access_violation_interrupt();
            }
			else
			{
				result = 1;
			}
        }
        else if (numMatches == 0)
        {
            sim_debug(LOG_SAC_CPR_ERROR, &sac_dev, "CPR non-equivalence for virtual address 0x%08X\n", va);
            *CPRNotEquivalencePSX = va;
            *CPRNotEquivalenceS = seg;
            cpu_set_cpr_non_equivalence_interrupt();
        }
        else
        {
            cpu_set_cpr_multiple_equivalence_interrupt();
            *SystemErrorInterrupt |= 0x40; // TODO: check this in light of interrupt or-tree work.
        }

        if (access & (SAC_OBEY_ACCESS | SAC_READ_ACCESS))
        {
            *CPRReferenced |= matchMask;
        }
        else if (access & SAC_WRITE_ACCESS)
        {
            *CPRAltered |= matchMask;
        }
    }

    return result;
}

static uint8 sac_get_real_address_unit(t_addr address)
{
    uint8 unit = (address >> 20) & 0xF;
    return unit;
}

static uint32 sac_read_local_store(t_addr address)
{
	uint32 result;
	assert(address < MAX_LOCAL_MEMORY);
	result = LocalStore[address];

	return result;
}

static void sac_write_local_store(t_addr address, uint32 value)
{
	assert(address < MAX_LOCAL_MEMORY);
	LocalStore[address] = value;
}

static uint32 sac_read_mass_store(t_addr address)
{
	uint32 result;
	result = MassStore[address % MAX_MASS_MEMORY];

	return result;
}

static void sac_write_mass_store(t_addr address, uint32 value)
{
	MassStore[address % MAX_MASS_MEMORY] = value;
}
