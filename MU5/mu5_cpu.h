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

/* Machine Status register masks */

#define MS_MASK_LEVEL0           0x0001
#define MS_MASK_LEVEL1           0x0002
#define MS_MASK_EXEC             0x0004
#define MS_MASK_A_SYS_ERR_EXEC   0x0008
#define MS_MASK_B_D_SYS_ERR_EXEC 0x0010
#define MS_MASK_INH_INS_COUNT    0x0020
#define MS_MASK_BCPR             0x0080
#define MS_MASK_INH_PROG_FLT     0x4000

/* System Error Status register masks */

#define SYSTEM_ERROR_STATUS_MASK_CPR_MULTI_EQV           0x0400
#define SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR            0x0080
#define SYSTEM_ERROR_STATUS_MASK_ACC_ERROR               0x0040
#define SYSTEM_ERROR_STATUS_MASK_ILLEGAL_FUNCTION_ERROR  0x0020
#define SYSTEM_ERROR_STATUS_MASK_NAME_ADDER_OVF_ERROR    0x0010
#define SYSTEM_ERROR_STATUS_MASK_CONTROL_ADDER_OVF_ERROR 0x0008
#define SYSTEM_ERROR_STATUS_MASK_CPR_EXEC_ILLEGAL        0x0004
#define SYSTEM_ERROR_STATUS_MASK_CPR_NEQV                0x0002

#define SYSTEM_ERROR_STATUS_BIT_CPR_MULTI_EQV           (63 - 53)
#define SYSTEM_ERROR_STATUS_BIT_B_OR_D_ERROR            (63 - 56)
#define SYSTEM_ERROR_STATUS_BIT_ACC_ERROR               (63 - 57)
#define SYSTEM_ERROR_STATUS_BIT_ILLEGAL_FUNCTION_ERROR  (63 - 58)
#define SYSTEM_ERROR_STATUS_BIT_NAME_ADDER_OVF_ERROR    (63 - 59)
#define SYSTEM_ERROR_STATUS_BIT_CONTROL_ADDER_OVF_ERROR (63 - 60)
#define SYSTEM_ERROR_STATUS_BIT_CPR_EXEC_ILLEGAL        (63 - 61)
#define SYSTEM_ERROR_STATUS_BIT_CPR_NEQV                (63 - 62)

/* Program Fault Status register masks */
#define PROGRAM_FAULT_STATUS_MASK_ILLEGAL_FUNCTION_ERROR       0x8000
#define PROGRAM_FAULT_STATUS_MASK_NAME_ADDER_OVF_ERROR         0x4000
#define PROGRAM_FAULT_STATUS_MASK_CONTROL_ADDER_OVF_ERROR      0x2000
#define PROGRAM_FAULT_STATUS_MASK_ILLEGAL_V_STORE_ACCESS_ERROR 0x1000
#define PROGRAM_FAULT_STATUS_MASK_CPR_ILLEGAL_ACCESS_ERROR     0x0800
#define PROGRAM_FAULT_STATUS_MASK_SYSTEM_PERFORMANCE_MONITOR   0x0200
#define PROGRAM_FAULT_STATUS_MASK_B_ERROR                      0x0080
#define PROGRAM_FAULT_STATUS_MASK_D_ERROR                      0x0040
#define PROGRAM_FAULT_STATUS_MASK_ACC_ERROR                    0x0020

#define PROGRAM_FAULT_STATUS_BIT_ILLEGAL_FUNCTION_ERROR       (63 - 48)
#define PROGRAM_FAULT_STATUS_BIT_NAME_ADDER_OVF_ERROR         (63 - 49)
#define PROGRAM_FAULT_STATUS_BIT_CONTROL_ADDER_OVF_ERROR      (63 - 50)
#define PROGRAM_FAULT_STATUS_BIT_ILLEGAL_V_STORE_ACCESS_ERROR (63 - 51)
#define PROGRAM_FAULT_STATUS_BIT_CPR_ILLEGAL_ACCESS_ERROR     (63 - 52)
#define PROGRAM_FAULT_STATUS_BIT_SYSTEM_PERFORMANCE_MONITOR   (63 - 54)
#define PROGRAM_FAULT_STATUS_BIT_B_ERROR                      (63 - 56)
#define PROGRAM_FAULT_STATUS_BIT_D_ERROR                      (63 - 57)
#define PROGRAM_FAULT_STATUS_BIT_ACC_ERROR                    (63 - 58)

void cpu_reset_state(void);
void cpu_execute_next_order(void);
void cpu_set_register(REG *reg, t_uint64 value); /* for selftest purposes only, so register setting includes calling the callbacks if appropriate, to make interrupt testing easier */
void cpu_set_interrupt(uint8 number);
uint8 cpu_get_interrupt_number(void);
void cpu_spm_interrupt(void);
void cpu_set_access_violation_interrupt(void);
void cpu_set_cpr_non_equivalence_interrupt(void);
void cpu_set_cpr_multiple_equivalence_interrupt(void);
uint16 cpu_get_ms(void);
int cpu_ms_is_all(uint16 bits);
int cpu_ms_is_any(uint16 bits);
t_uint64 cpu_exch_read(t_addr addr);
void cpu_exch_write(t_addr addr, t_uint64 value);
void cpu_set_console_peripheral_window_interrupt(t_uint64 message);