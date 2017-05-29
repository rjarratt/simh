/* mu5_defs.h: MU5 simulator definitions

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

#pragma once

#include "sim_defs.h"

#define REG_A "A"
#define REG_AEX "AEX"
#define REG_AOD "AOD"
#define REG_X "X"
#define REG_B "B"
#define REG_BOD "BOD"
#define REG_D "D"
#define REG_XD "XD"
#define REG_DT "DT"
#define REG_XDT "XDT"
#define REG_DOD "DOD"
#define REG_NB "NB"
#define REG_XNB "XNB"
#define REG_SN "SN"
#define REG_SF "SF"
#define REG_MS "MS"
#define REG_CO "CO"
#define REG_DL "DL"

/* flag used to mark a register as needing a callback when it is written*/
#define REG_CALLBACK 0x8000000

/* Debug flags */
#define LOG_ERROR           (1 << 12)
#define LOG_SELFTEST        (1 << 13)
#define LOG_SELFTEST_DETAIL (1 << 14)
#define LOG_SELFTEST_FAIL   (1 << 15)

#define MAX_LOCAL_MEMORY  (32768)    /* RNI told me Local Store consisted of four 4096-word memory units, each word containing 64 data bits + 8 parity bits. This is the size in 32-bit words */
#define MAX_MASS_MEMORY  (262144)    /* RNI told me Local Store consisted of four 4096-word memory units, each word containing 64 data bits + 8 parity bits. This is the size in 32-bit words */

/* The Exchange Unit numbers below are presumed but not confirmed. RNI believes the numbers are in the order in which they appear in Fig. 6.12 on p133 of the book. However in an email
   he also said "The BTU was also a unit, as was the SPM.I think the SPM was unit 10 or 11."
*/
#define UNIT_FIXED_HEAD_DISC 0
#define UNIT_PDP11 1
#define UNIT_MU5_PROCESSOR 2
#define UNIT_LOCAL_STORE 3
#define UNIT_1905E 4
#define UNIT_MASS_STORE 5
#define UNIT_BTU 6 /* A total guess */
#define UNIT_SPM 10 /* or could be 11 */

/* Interrupt numbers */
#define INT_SYSTEM_ERROR 0
#define INT_CPR_NOT_EQUIVALENCE 1
#define INT_EXCHANGE 2
#define INT_PERIPHERAL_WINDOW 3
#define INT_INSTRUCTION_COUNT_ZERO 4
#define INT_ILLEGAL_ORDERS 5
#define INT_PROGRAM_FAULTS 6
#define INT_SOFTWARE_INTERRUPT 7

/* Descriptor types */
#define DESCRIPTOR_TYPE_GENERAL_VECTOR 0
#define DESCRIPTOR_TYPE_GENERAL_STRING 1
#define DESCRIPTOR_TYPE_ADDRESS_VECTOR 2
#define DESCRIPTOR_TYPE_MISCELLANEOUS  3

/* Descriptor sizes */
#define DESCRIPTOR_SIZE_1_BIT 0
#define DESCRIPTOR_SIZE_4_BIT 2
#define DESCRIPTOR_SIZE_8_BIT 3
#define DESCRIPTOR_SIZE_16_BIT 4
#define DESCRIPTOR_SIZE_32_BIT 5
#define DESCRIPTOR_SIZE_64_BIT 6

#define RA_LOCAL(address) ((UNIT_LOCAL_STORE << 20) | (address & 0xFFFFF))
#define RA_LOCAL_BYTE(address) ((UNIT_LOCAL_STORE << 22) | (address & 0xFFFFF))

typedef struct
{
	uint16 backing_value;
	void(*callback)(uint16 old_value, uint16 new_value);
} uint16_register_backing;

typedef struct
{
	uint32 backing_value;
	void(*callback)(uint32 old_value, uint32 new_value);
} uint32_register_backing;

typedef struct
{
	t_uint64 backing_value;
	void(*callback)(t_uint64 old_value, t_uint64 new_value);
} t_uint64_register_backing;
