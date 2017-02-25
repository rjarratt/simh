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
#define LOG_CPU_PERF            (1 << 0)
#define LOG_CPU_DECODE          (1 << 1)
#define LOG_CPU_SELFTEST        (1 << 2)
#define LOG_CPU_SELFTEST_DETAIL (1 << 3)
#define LOG_CPU_SELFTEST_FAIL   (1 << 4)

//#define MAXMEMORY  (32768)    /* RNI told me Local Store consisted of four 4096-word memory units, each word containing 64 data bits + 8 parity bits */
#define MAXMEMORY  (0x160000)    /* TODO: Until I implement virtual memory make the memory big enough for the addresses in the demo programs to work as physical addresses */

#define INT_SYSTEM_ERROR 0
#define INT_CPR_NOT_EQUIVALENCE 1
#define INT_EXCHANGE 2
#define INT_PERIPHERAL_WINDOW 3
#define INT_INSTRUCTION_COUNT_ZERO 4
#define INT_ILLEGAL_ORDERS 5
#define INT_PROGRAM_FAULTS 6
#define INT_SOFTWARE_INTERRUPT 7

#define DESCRIPTOR_TYPE_GENERAL_VECTOR 0
#define DESCRIPTOR_TYPE_GENERAL_STRING 1
#define DESCRIPTOR_TYPE_ADDRESS_VECTOR 2
#define DESCRIPTOR_TYPE_MISCELLANEOUS  3

#define DESCRIPTOR_SIZE_1_BIT 0
#define DESCRIPTOR_SIZE_4_BIT 2
#define DESCRIPTOR_SIZE_8_BIT 3
#define DESCRIPTOR_SIZE_16_BIT 4
#define DESCRIPTOR_SIZE_32_BIT 5
#define DESCRIPTOR_SIZE_64_BIT 6
