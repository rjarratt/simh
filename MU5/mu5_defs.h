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

/* Debug flags */
#define LOG_ERROR           (1 << 12)
#define LOG_SELFTEST        (1 << 13)
#define LOG_SELFTEST_DETAIL (1 << 14)
#define LOG_SELFTEST_FAIL   (1 << 15)

#define MAX_LOCAL_MEMORY  (32768)    /* RNI told me Local Store consisted of four 4096-word memory units, each word containing 64 data bits + 8 parity bits. This is the size in 32-bit words */
#define MAX_MASS_MEMORY  (262144) /* p55 of the book says it allowed 4 128K-word units, but there were only ever 2 units, so a total of 256K 32-bit words, or 1MByte */

#define MASK_8 0xFF
#define MASK_16 0xFFFF
#define MASK_32 0xFFFFFFFF

/* The Exchange Unit numbers below are presumed but not confirmed. RNI believes the numbers are in the order in which they appear in Fig. 6.12 on p133 of the book, he is also confident that the
   fixed head disc was indeed unit 0. However in an email he also said "The BTU was also a unit, as was the SPM.I think the SPM was unit 10 or 11."
*/
#define UNIT_FIXED_HEAD_DISC 0 /* Also known as the Drum */
#define UNIT_PDP11 1
#define UNIT_MU5_PROCESSOR 2
#define UNIT_1905E 4
#define UNIT_MASS_STORE 5
#define UNIT_BTU 6 /* A total guess */
#define UNIT_LOCAL_STORE 9 /* Confirmed by AEK and RNI. In the hardware the zeroes were inverted to make it 0xF for faster matching. */
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

#define RA_IN_CPR_MASK 0xFFFFF /* Only most significant 20 bits, the least significant 4 bits represent the smallest page size and are not held in the RA field of the CPR */
#define RA_BIT_LENGTH 24
#define RA_MASK 0xFFFFFF
#define RA_MASK_FULL 0xFFFFFFF /* including unit */
#define RA_X_MASK 0x7FFFFF
#define RA_VX_MASK 0x0800000

#define CPR_VA(P,S,X) (((P) << 26 ) | ((S) << 12) | (X))
#define CPR_RA_LOCAL(AC,A,LZ) ((((AC) & 0xF) << 28) | (UNIT_LOCAL_STORE << 24) | (((A) & RA_IN_CPR_MASK) << 4) | ((LZ) & 0xF))
#define CPR_RA_MASS(AC,A,LZ) ((((AC) & 0xF) << 28) | (UNIT_MASS_STORE << 24) | (((A) & RA_IN_CPR_MASK) << 4) | ((LZ) & 0xF))
#define RA_X(address) ((address) & RA_X_MASK)
#define RA_VX(address) (RA_VX_MASK | ((address) & RA_X_MASK))
#define RA_VX_DRUM(address) (RA_VX_MASK | (UNIT_FIXED_HEAD_DISC << RA_BIT_LENGTH) | (((address) << 1) & RA_X_MASK)) /* address is in 64-bit units */
#define RA_LOCAL(address) ((UNIT_LOCAL_STORE << RA_BIT_LENGTH) | ((address) & RA_MASK))
#define RA_MASS(address) ((UNIT_MASS_STORE << RA_BIT_LENGTH) | ((address) & RA_MASK))
#define RA_LOCAL_BYTE(address) ((UNIT_LOCAL_STORE << (RA_BIT_LENGTH + 2)) | ((address) & RA_MASK))
#define RA_VX_MU5(address) (RA_VX_MASK | (UNIT_MU5_PROCESSOR << RA_BIT_LENGTH) | (((address) << 1) & RA_MASK)) /* address is in 64-bit units */
#define RA_VX_BTU(address) (RA_VX_MASK | (UNIT_BTU << RA_BIT_LENGTH) | (((address) << 1) & RA_X_MASK)) /* address is in 64-bit units */

#define VX_ADDR(block,line) (((block) << 5) | ((line) & 0x1F))
#define VX_LINE(addr) ((addr) & 0x1F)
#define VX_BLOCK(addr) (((addr) & ~RA_VX_MASK) >> 5)