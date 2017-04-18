/* mu5_cpu.h: MU5 CPU definitions

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

#define MS_MASK_LEVEL0 0x0001
#define MS_MASK_LEVEL1 0x0002
#define MS_MASK_EXEC 0x0004
#define MS_MASK_A_SYS_ERR_EXEC 0x0008
#define MS_MASK_B_D_SYS_ERR_EXEC 0x0010
#define MS_MASK_INH_INS_COUNT 0x0020
#define MS_MASK_BCPR 0x0080
#define MS_MASK_INH_PROG_FLT 0x0400

/* Program fault status register masks */
#define PFS_B_FAULT 0x0080


void cpu_reset_state(void);
void cpu_execute_next_order(void);
void cpu_set_register(REG *reg, t_uint64 value); /* for selftest purposes only, so register setting includes calling the callbacks if appropriate, to make interrupt testing easier */
void cpu_set_interrupt(uint8 number);
uint8 cpu_get_interrupt_number(void);
void cpu_set_access_violation_interrupt(void);
uint16 cpu_get_ms(void);
int cpu_ms_is_all(uint16 bits);
int cpu_ms_is_any(uint16 bits);

