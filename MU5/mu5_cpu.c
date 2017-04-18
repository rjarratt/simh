/* mu5_cpu.c: MU5 CPU

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

MU5 is a big-endian machine. Instructions are addressed in 16-bit units,
the memory is addressed in 32-bit words, but it is fundamentally a 64-bit
machine. The memory loading command takes a byte address the origin and
32-bit word addresses are calculated from there. The program counter has
to be loaded as 16-bit word address though, so to calculate it, take the
byte address and shift it right by 1 bit.

Known Limitations
-----------------
Z register is not implemented.
B DIV implementation is a guess (not defined in MU5 Basic Programming Manual)
No floating point orders
No decimal orders
Interrupt processing is incomplete, all the points where an interrupt is generated need to record the cause, other details also incomplete.
Type 3 descriptors not implemented yet.
Read only descriptors (1978 manual), RNI email of 4/3/17 says write access to these should generate illegal order interrupt
No instruction counter

To Do
-----
Page 2.10.1. Action section says that truncating a non-zero bit on store orders will cause an interrupt.

RNI: The Name Base is conceptually 16 bits long but the l.s. bit is always zero, so the name for a 32-bit variable is shifted
down 1 place and the l.s. bit is used to select the appropriate half of the 64-bit word accessed from the name store.
So for a V-store access, the name part of the instruction just points to a 64-bit word.

*/

#include <assert.h>
#include "mu5_defs.h"
#include "mu5_cpu.h"
#include "mu5_sac.h"
#include "mu5_test.h"
#include "mu5_cpu_test.h"
#include "mu5_sac_test.h"
#include "mu5_console_test.h"

/* This structure is used to allow instruction execution to be table driven. It allows for tables to be nested. At each level the
   execute() function does anything it needs to do before invoking the function on the inner table. The leaf tables have a NULL for
   the inner table.
*/
typedef struct DISPATCH_ENTRY
{
    void(*execute)(uint16 order, struct DISPATCH_ENTRY *innerTable);
    struct DISPATCH_ENTRY *innerTable;
} DISPATCH_ENTRY;

/* The CPU did not have a STOP instruction. For the purposes of emulation I want to be able to stop it anyway so the emulation stops.
This is what RNI told me:

It wasn't (stopped), otherwise it wouldn't have been able to respond to
interrupts. Instead, it did a "loop stop". An instruction made up
entirely of 0's is a relative jump by zero, i.e. it has no effect other
than to keep the CPU running. I had to invent a STOP instruction for my
HASE simulation model. Of course MU5 itself could be stopped from the
console by pressing the reset button. Being an asynchronous machine, it
often stopped during commissioning, so the restart procedure was to
press the "reset", "interrupt" and "go" buttons. The "go" button
inserted a pulse into the PROP timing mechanism. The interrupt in this
case was the "engineer's interrupt", which had the highest priority and
caused two fixed instructions wired into the Instruction Buffer Unit to
be executed. I can't remember the details but they resulted in an
absolute jump to the start of the OS code. The first thing this did was
to set the MS register to some appropriate setting.

*/

#define MASK_8 0xFF
#define MASK_16 0xFFFF
#define MASK_32 0xFFFFFFFF

#define SCALE_32 1
#define SCALE_64 2

#define SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR            0x0080
#define SYSTEM_ERROR_STATUS_MASK_ACC_ERROR               0x0040
#define SYSTEM_ERROR_STATUS_MASK_ILLEGAL_FUNCTION_ERROR  0x0020
#define SYSTEM_ERROR_STATUS_MASK_NAME_ADDER_OVF_ERROR    0x0010
#define SYSTEM_ERROR_STATUS_MASK_CONTROL_ADDER_OVF_ERROR 0x0008
#define SYSTEM_ERROR_STATUS_MASK_CPR_ERROR               0x0004

#define PROGRAM_FAULT_STATUS_MASK_ILLEGAL_FUNCTION_ERROR  0x8000
#define PROGRAM_FAULT_STATUS_MASK_NAME_ADDER_OVF_ERROR    0x4000
#define PROGRAM_FAULT_STATUS_MASK_CONTROL_ADDER_OVF_ERROR 0x2000
#define PROGRAM_FAULT_STATUS_ILLEGAL_V_STORE_ACCESS_ERROR 0x1000
#define PROGRAM_FAULT_STATUS_MASK_CPR_ERROR               0x0800
#define PROGRAM_FAULT_STATUS_MASK_B_ERROR                 0x0080
#define PROGRAM_FAULT_STATUS_MASK_D_ERROR                 0x0040
#define PROGRAM_FAULT_STATUS_MASK_ACC_ERROR               0x0020

static int cpu_stopped = 0;

int32 sim_emax;

static UNIT cpu_unit =
{
    UDATA(NULL, UNIT_FIX | UNIT_BINK, MAXMEMORY)
};

BITFIELD aod_bits[] = {
    BIT(DBLLEN),  /* Double length +/- */
    BIT(IROUND),  /* Inhibit rounding */
    BIT(ZDIV),    /* Zero divide indicator */
    BIT(DECOVF),  /* Decimal overflow indicator */
    BIT(FXPOVF),  /* Fixed point overflow indicator */
    BIT(FLPUNF),  /* Floating point underflow indicator */
    BIT(FLPOVF),  /* Floating point overflow indicator */
    BIT(IZDIV),   /* Inhibit zero divide interrupt */
    BIT(IDECOVF), /* Inhibit decimal overflow interrupt */
    BIT(IFXPOVF), /* Inhibit fixed point overflow interrupt */
    BIT(IFLPUNF), /* Inhibit floating point underflow interrupt */
    BIT(IFLPOVF), /* Inhibit floating point overflow interrupt */
    BIT(OPSIZ64), /* Operand size (0/1 meaning 32/64 bits */
    BITNCF(51),
    ENDBITS
};

static t_uint64 mask_aod         = 0x0000000000001FFF;
static t_uint64 mask_aod_zdiv    = 0x0000000000000004;
static t_uint64 mask_aod_fxpovf  = 0x0000000000000010;
static t_uint64 mask_aod_izdiv   = 0x0000000000000080;
static t_uint64 mask_aod_ifxpovf = 0x0000000000000200;
static t_uint64 mask_aod_opsiz64 = 0x0000000000001000;

BITFIELD bod_bits[] = {
    BITNCF(26),
    BIT(BOVF),   /* B Overflow */
    BITNCF(4),
    BIT(IBOVF),  /* Inhibit B overflow interrupt */
    ENDBITS
};

static uint32 mask_bod_bovf  = 0x04000000;
static uint32 mask_bod_ibovf = 0x80000000;

BITFIELD ms_bits[] = {
    BIT(L0IF),    /* Level 0 interrupt flip-flop */
    BIT(L1IF),    /* Level 1 interrupt flip-flop */
    BIT(EXEC),    /* Exec Mode flip-flop */
    BIT(AF),      /* A faults to System Error in Exec Mode */
    BIT(BDF),     /* B and D faults to System Error in Exec Mode */
    BIT(ICI),     /* Instruction counter inhibit */
    BIT(BNS),     /* Bypass name store */
    BIT(BCPR),    /* Bypass CPRs */

    BIT(BN),      /* Boolean */
    BIT(T2),      /* T2 - less than 0 */
    BIT(T1),      /* T1 - not equal 0 */
    BIT(T0),      /* T0 - overflow */
    BITNCF(2),    /* Spare in section 6, SPM & Spare in section 7 */
    BIT(IPF),     /* Inhibit program fault interrupts */
    BIT(DS),      /* Force DR instead of S */
    ENDBITS
};

static uint16 mask_ms_exec = MS_MASK_EXEC;
static uint16 mask_ms_bcpr = MS_MASK_BCPR;
static uint16 mask_ms_b_d_sys_err_exec = MS_MASK_B_D_SYS_ERR_EXEC;
static uint16 mask_ms_a_sys_err_exec = MS_MASK_A_SYS_ERR_EXEC;
static uint16 mask_ms_bn = 0x0100;
static uint16 mask_ms_t2   = 0x0200;
static uint16 mask_ms_t1   = 0x0400;
static uint16 mask_ms_t0   = 0x0800;

BITFIELD dod_bits[] = {
    BIT(XCH),  /* XCHK digit */
    BIT(ITS),  /* Illegal Type/Size */
    BIT(EMS),  /* Excutive Mode Subtype used in non-executive mode */
    BIT(SSS),  /* Short Source String in store to store order */
    BIT(NZT), /* Non-Zero Truncation when storing secondary operand */
    BIT(BCH),  /* Bound Check Fail during secondary operand access */
    BIT(SSSI), /* SSS Interrupt Inhibit */
    BIT(NZTI), /* NZT Interrupt Inhibit */
    BIT(BCHI), /* BCH Interrupt Inhibit */
    BITNCF(23),
    ENDBITS
};

static uint32 mask_dod_xch  = 0x00000001;
static uint32 mask_dod_its  = 0x00000002;
static uint32 mask_dod_sss  = 0x00000008;
static uint32 mask_dod_bch  = 0x00000020;
static uint32 mask_dod_sssi = 0x00000040;
static uint32 mask_dod_bchi = 0x00000100;

/* Register backing values */
static uint32                    reg_b_backing_value;     /* B register */
static uint32_register_backing   reg_bod_backing_value;   /* BOD register */
static t_uint64                  reg_a_backing_value;     /* A register */
static t_uint64_register_backing reg_aod_backing_value;   /* AOD register */
static t_uint64                  reg_aex_backing_value;   /* AEX register */
static uint32                    reg_x_backing_value;     /* X register */
static uint16                    reg_ms_backing_value;    /* MS register */
static uint16                    reg_nb_backing_value;    /* NB register */
static uint32                    reg_xnb_backing_value;   /* XNB register */
static uint16                    reg_sn_backing_value;    /* SN register */
static uint16                    reg_sf_backing_value;    /* SF register */
static uint32                    reg_co_backing_value;    /* CO Register */
static t_uint64                  reg_d_backing_value;     /* D Register */
static t_uint64                  reg_xd_backing_value;    /* XD Register */
static uint32_register_backing   reg_dod_backing_value;   /* DOD Register */
static uint32                    reg_dt_backing_value;    /* DT Register */
static uint32                    reg_xdt_backing_value;   /* XDT Register */
static uint32                    reg_dl_backing_value;    /* Pseudo register for display lamps */

static REG cpu_reg[] =
{
    { HRDATAD(B,    reg_b_backing_value,       32,    "B register") },
    { GRDATADF(BOD, reg_bod_backing_value, 16, 32, 0, "BOD register", bod_bits), REG_CALLBACK },
    { HRDATAD(A,    reg_a_backing_value,       64,    "Accumulator") },
    { GRDATADF(AOD, reg_aod_backing_value, 16, 64, 0, "AOD register", aod_bits), REG_CALLBACK },
    { HRDATAD(AEX,  reg_aex_backing_value,     64,    "Accumulator extension") },
    { HRDATAD(X,    reg_x_backing_value,       32,    "X register") },
    { GRDATADF(MS,  reg_ms_backing_value, 16,  16, 0, "Machine status register", ms_bits) },
    { HRDATAD(NB,   reg_nb_backing_value,      16,    "Name Base register") },
    { HRDATAD(XNB,  reg_xnb_backing_value,     32,    "X register") },
    { HRDATAD(SN,   reg_sn_backing_value,      16,    "Name Segment Number register") },
    { HRDATAD(SF,   reg_sf_backing_value,      16,    "Stack Front register") },
    { HRDATAD(CO,   reg_co_backing_value,      32,    "Program counter") },
    { HRDATAD(D,    reg_d_backing_value,       64,    "Data descriptor register") },
    { HRDATAD(XD,   reg_xd_backing_value,      64,    "XD register") },
    { GRDATADF(DOD, reg_dod_backing_value, 16, 32, 0, "DOD register", dod_bits), REG_CALLBACK },
    { HRDATAD(DT,   reg_dt_backing_value,      32,    "DT register") },
    { HRDATAD(XDT,  reg_xdt_backing_value,     32,    "XDT register") },
    { HRDATAD(DL,   reg_dl_backing_value,      32,    "Pseudo register for display lamps") },
    { NULL }
};

REG *reg_b    = &cpu_reg[0];
REG *reg_bod  = &cpu_reg[1];
REG *reg_a    = &cpu_reg[2];
REG *reg_aod  = &cpu_reg[3];
REG *reg_aex  = &cpu_reg[4];
REG *reg_x    = &cpu_reg[5];
REG *reg_ms   = &cpu_reg[6];
REG *reg_nb   = &cpu_reg[7];
REG *reg_xnb  = &cpu_reg[8];
REG *reg_sn   = &cpu_reg[9];
REG *reg_sf   = &cpu_reg[10];
REG *reg_co   = &cpu_reg[11];
REG *reg_d    = &cpu_reg[12];
REG *reg_xd   = &cpu_reg[13];
REG *reg_dod  = &cpu_reg[14];
REG *reg_dt   = &cpu_reg[15];
REG *reg_xdt  = &cpu_reg[16];
REG *reg_dl   = &cpu_reg[17];

static uint8 interrupt;
REG *sim_PC = &cpu_reg[11];

static MTAB cpu_mod[] =
{
    { 0 }
};

/* Debug Flags */
static DEBTAB cpu_debtab[] =
{
    { "PERF",           LOG_CPU_PERF,             "CPU performance" },
    { "EVENT",          SIM_DBG_EVENT,            "event dispatch activities" },
    { "DECODE",         LOG_CPU_DECODE,           "decode instructions" },
    { "SELFTEST",       LOG_CPU_SELFTEST,         "self test summary output" },
    { "SELFTESTDETAIL", LOG_CPU_SELFTEST_DETAIL,  "self test detailed output" },
    { "SELFTESTFAIL",   LOG_CPU_SELFTEST_FAIL,    "self test failure output" },
    { NULL,             0 }
};

static const char* cpu_description(DEVICE *dptr) {
    return "Central Processing Unit";
}

uint8 PROPProcessNumber;
static uint16 PROPProgramFaultStatus;
static uint16 PROPSystemErrorStatus;
static uint16 PROPInstructionCounter;

t_stat sim_instr(void);

static t_stat cpu_ex(t_value *vptr, t_addr addr, UNIT *uptr, int32 sw);
static t_stat cpu_dep(t_value val, t_addr addr, UNIT *uptr, int32 sw);
static t_stat cpu_reset(DEVICE *dptr);

/* SCP provides get_rval and put_rval, but these are too slow */
static SIM_INLINE void cpu_set_register_16(REG *reg, uint16 value);
static SIM_INLINE void cpu_set_register_32(REG *reg, uint32 value);
static SIM_INLINE void cpu_set_register_64(REG *reg, t_uint64 value);
static SIM_INLINE void cpu_set_register_bit_16(REG *reg, uint16 mask, int value);
static SIM_INLINE void cpu_set_register_bit_32(REG *reg, uint32 mask, int value);
static SIM_INLINE void cpu_set_register_bit_64(REG *reg, t_uint64 mask, int value);
static SIM_INLINE uint16 cpu_get_register_16(REG *reg);
static SIM_INLINE uint32 cpu_get_register_32(REG *reg);
static SIM_INLINE t_uint64 cpu_get_register_64(REG *reg);
static SIM_INLINE int cpu_get_register_bit_16(REG *reg, uint16 mask);
static SIM_INLINE int cpu_get_register_bit_32(REG *reg, uint32 mask);
static SIM_INLINE int cpu_get_register_bit_64(REG *reg, t_uint64 mask);
static void cpu_set_ms(uint16 value);
static void cpu_set_nb(uint16 value);
static void cpu_set_xnb(uint32 value);
static void cpu_set_sf(uint16 value);
static void cpu_set_co(uint32 value);
static void cpu_set_ms_bn(int value);
static void cpu_aod_callback(t_uint64 old_value, t_uint64 new_value);
static void cpu_bod_callback(uint32 old_value, uint32 new_value);
static void cpu_dod_callback(uint32 old_value, uint32 new_value);
static uint32 cpu_co_add(uint32 co, t_int64 operand);
static t_uint64 cpu_get_link(void);
static void cpu_set_link(t_uint64 link);
static uint16 cpu_calculate_base_offset_from_addr(t_addr base, t_int64 offset, uint8 scale);
static t_addr cpu_get_name_segment_address_from_addr(t_addr base, int16 offset, uint8 scale);
static uint16 cpu_calculate_base_offset_from_reg_16(REG *reg, t_int64 offset, uint8 scale);
static uint32 cpu_calculate_base_offset_from_reg_32(REG *reg, t_int64 offset, uint8 scale);
static t_addr cpu_get_name_segment_address_from_reg(REG *reg, int16 offset, uint8 scale);
static int cpu_is_executive_mode(void);

static void cpu_clear_interrupt(uint8 number);
static void cpu_clear_all_interrupts(void);
static int cpu_check_condition_with_inhibit(REG *reg, uint32 condition_mask, uint32 inhibit_mask);
static void cpu_evaluate_interrupts(void);
static void cpu_set_system_error_interrupt(uint16 reason);
static void cpu_set_program_fault_interrupt(uint16 reason);
static void cpu_set_illegal_order_interrupt(uint16 reason);
static void cpu_set_A_interrupt(void);
static void cpu_set_B_interrupt(void);
static void cpu_set_D_interrupt(void);
static void cpu_set_its_interrupt(void);
static void cpu_set_sss_interrupt(void);
static void cpu_set_bounds_check_interrupt(void);
static void cpu_set_name_adder_overflow_interrupt(void);
static void cpu_set_control_adder_overflow_interrupt(void);
static void cpu_set_illegal_function_interrupt(void);
static uint8 cpu_get_cr(uint16 order);
static uint8 cpu_get_f(uint16 order);
static uint16 cpu_get_k(uint16 order);
static uint16 cpu_get_n(uint16 order);
static uint16 cpu_get_extended_k(uint16 order);
static uint16 cpu_get_extended_n(uint16 order);
static int cpu_operand_is_secondary(uint16 order);
static t_uint64 cpu_get_operand_6_bit_literal(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_extended_literal(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_addr cpu_get_operand_extended_variable_address(uint16 order, uint32 instructionAddress, int *instructionLength, uint8 scale, int *is64bit);
static t_uint64 cpu_get_operand_internal_register(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_set_operand_internal_register(uint16 order, t_uint64 value);
static t_addr cpu_get_operand_address_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_addr cpu_get_operand_address_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength);
static int32 cpu_scale_descriptor_modifier(t_uint64 descriptor, uint32 modifier);
static t_addr cpu_get_operand_byte_address_by_descriptor_vector_access(t_uint64 descriptor, uint32 modifier);
static t_uint64 cpu_get_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier);
static void cpu_set_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier, t_uint64 value);
static void cpu_process_source_to_destination_descriptor_vector(t_uint64 operand, t_uint64(*func)(t_uint64 source, t_uint64 destination, t_uint64 operand));
static t_uint64 cpu_get_operand_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength);
static void cpu_set_operand_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, t_uint64 value);
static t_uint64 cpu_get_operand_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength);
static void cpu_set_operand_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, t_uint64 value);
static t_uint64 cpu_get_operand_from_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, uint32 modifier);
static void cpu_set_operand_from_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, uint32 modifier, t_uint64 value);
static t_uint64 cpu_get_operand(uint16 order);
static void cpu_set_operand(uint16 order, t_uint64 value);
static t_uint64 cpu_sign_extend(t_uint64 value, int8 bits);
static t_uint64 cpu_sign_extend_6_bit(t_uint64 value);
static t_uint64 cpu_sign_extend_32_bit(t_uint64 value);
static void cpu_push_value(t_uint64 value);
static t_addr cpu_pop_address(void);
static t_uint64 cpu_pop_value(void);
static void cpu_test_value(t_int64 value);

static t_uint64 prop_read_process_number_callback(uint8 line);
static void prop_write_process_number_callback(uint8 line, t_uint64 value);
static t_uint64 prop_read_program_fault_status_callback(uint8 line);
static void prop_write_program_fault_status_callback(uint8 line, t_uint64 value);
static t_uint64 prop_read_system_error_status_callback(uint8 line);
static void prop_write_system_error_status_callback(uint8 line, t_uint64 value);
static t_uint64 prop_read_instruction_counter(uint8 line);
static void prop_write_instruction_counter(uint8 line, t_uint64 value);

static void cpu_execute_dummy_order(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_start_interrupt_processing(void);

/* cr functions */
static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable);

/* organisational order functions */
static void cpu_jump_relative(uint16 order, int performJump);
static int cpu_calculate_condition(uint8 conditionCode);
static void cpu_process_condition(int condition, int8 function);
static void cpu_execute_organisational_relative_jump(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_exit(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_absolute_jump(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_return(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_xcn(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_stacklink(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_MS_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_DL_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_spm(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_setlink(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_load_XNB(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_load_SN(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_XNB_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_XNB_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_SF_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_SF_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_SF_load_NB_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_SF_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_load_SF_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_eq(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_ne(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_ge(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_lt(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_le(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_gt(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_ovf(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_bn(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_bn_by_operand(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_bn_by_order(uint16 order, DISPATCH_ENTRY *innerTable);

/* store-to-store order functions */
static uint8 cpu_get_descriptor_type(t_uint64 descriptor);
static uint8 cpu_get_descriptor_subtype(t_uint64 descriptor);
static uint8 cpu_get_descriptor_size(t_uint64 descriptor);
static uint8 cpu_get_descriptor_unscaled(t_uint64 descriptor);
static uint8 cpu_get_descriptor_bound_check_inhibit(t_uint64 descriptor);
static uint32 cpu_get_descriptor_bound(t_uint64 descriptor);
static uint32 cpu_get_descriptor_origin(t_uint64 descriptor);
static void cpu_set_descriptor_bound(t_uint64 *descriptor, uint32 bound);
static void cpu_set_descriptor_origin(t_uint64 *descriptor, uint32 origin);
static void cpu_descriptor_modify(REG *descriptorReg, int32 modifier, int originOnly);
static int cpu_check_bound(t_uint64 descriptor, int32 modifier);
static void cpu_execute_descriptor_modify(uint16 order, REG *descriptorReg);

static void cpu_parse_sts_string_to_string_operand(t_uint64 operand, uint8 *mask, uint8 *filler);
static void cpu_parse_sts_string_to_string_operand_from_order(uint16 order, uint8 *mask, uint8 *filler);
static int cpu_check_string_descriptor(t_uint64 descriptor);
static int cpu_check_32bit_descriptor(t_uint64 descriptor);
static t_uint64 cpu_sts1_smve_element_operation(t_uint64 source, t_uint64 destination, t_uint64 operand);
static t_uint64 cpu_sts1_slgc_element_operation(t_uint64 source, t_uint64 destination, t_uint64 operand);
static void cpu_execute_sts1_xdo_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_xd_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_stack(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_xd_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_xdb_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_xchk(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_smod(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_xmod(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_slgc(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_smvb(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_smve(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_smvf(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_talu(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_scmp(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts1_sub1(uint16 order, DISPATCH_ENTRY *innerTable);

static void cpu_execute_sts2_do_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_d_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_d_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_d_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_db_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_mdr(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_mod(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_rmod(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_blgc(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_bmvb(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_bmve(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_bscn(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_bcmp(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_sub2(uint16 order, DISPATCH_ENTRY *innerTable);

/* B order functions */
static int cpu_test_b_overflow(t_uint64 value);
static void cpu_check_b_overflow(t_uint64 value);
static void cpu_test_b_value(t_int64 value);
static void cpu_execute_b_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_load_and_decrement(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_add(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_sub(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_mul(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_div(uint16 order, DISPATCH_ENTRY *innerTable); /* did not exist in real MU5, for HASE simulator comparison */
static void cpu_execute_b_xor(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_or(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_shift_left(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_and(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable); /* did not exist in real MU5, for HASE simulator comparison */
static void cpu_execute_b_compare(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_b_compare_and_increment(uint16 order, DISPATCH_ENTRY *innerTable);

/* fixed point signed order functions */
static void cpu_set_divide_by_zero(void);
static void cpu_check_x_overflow(t_uint64 result);
static void cpu_execute_fp_signed_load_single(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_add(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_sub(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_mul(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_div(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_xor(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_or(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_shift_left(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_and(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_compare(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_convert(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_signed_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable);

/* fixed point unsigned order functions */
static void cpu_execute_fp_unsigned_load_single(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_add(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_sub(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_mul(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_div(uint16 order, DISPATCH_ENTRY *innerTable); /* did not exist in real MU5, for HASE simulator comparison */
static void cpu_execute_fp_unsigned_xor(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_or(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_shift_left(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_and(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_compare(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_unsigned_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable); /* did not exist in real MU5, for HASE simulator comparison */

/* fixed point decimal order functions */
static void cpu_execute_fp_decimal_load_double(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_decimal_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_decimal_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_fp_decimal_compare(uint16 order, DISPATCH_ENTRY *innerTable);

/* floating point order functions */
static t_uint64 cpu_get_acc_value(void);
static void cpu_execute_flp_load_single(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_load_double(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_store(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_unsigned_xor(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_unsigned_or(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_shift_circ(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_unsigned_and(uint16 order, DISPATCH_ENTRY *innerTable);

DEVICE cpu_dev = {
    "CPU",            /* name */
    &cpu_unit,        /* units */
    cpu_reg,          /* registers */
    cpu_mod,          /* modifiers */
    1,                /* numunits */
    16,               /* aradix */
    32,               /* awidth */
    1,                /* aincr */
    16,               /* dradix */
    32,               /* dwidth */
    &cpu_ex,          /* examine */
    &cpu_dep,         /* deposit */
    &cpu_reset,       /* reset */
    NULL,             /* boot */
    NULL,             /* attach */
    NULL,             /* detach */
    NULL,             /* ctxt */
    DEV_DEBUG,        /* flags */
    0,                /* dctrl */
    cpu_debtab,       /* debflags */
    NULL,             /* msize */
    NULL,             /* lname */
    NULL,             /* help */
    NULL,             /* attach_help */
    NULL,             /* help_ctx */
    &cpu_description, /* description */
    NULL              /* brk_types */
};

static DISPATCH_ENTRY organisationalDispatchTable[] =
{
    { cpu_execute_organisational_relative_jump,         NULL }, /* 0 */
    { cpu_execute_organisational_exit,                  NULL }, /* 1 */
    { cpu_execute_dummy_order,                          NULL }, /* 2 */
    { cpu_execute_dummy_order,                          NULL }, /* 3 */
    { cpu_execute_organisational_absolute_jump,         NULL }, /* 4 */
    { cpu_execute_organisational_return,                NULL }, /* 5 */
    { cpu_execute_dummy_order,                          NULL }, /* 6 */
    { cpu_execute_dummy_order,                          NULL }, /* 7 */
    { cpu_execute_organisational_xcn,                   NULL }, /* 8 */
    { cpu_execute_organisational_xcn,                   NULL }, /* 9 */
    { cpu_execute_organisational_xcn,                   NULL }, /* 10 */
    { cpu_execute_organisational_xcn,                   NULL }, /* 11*/
    { cpu_execute_organisational_xcn,                   NULL }, /* 12 */
    { cpu_execute_organisational_xcn,                   NULL }, /* 13 */
    { cpu_execute_organisational_xcn,                   NULL }, /* 14 */
    { cpu_execute_organisational_stacklink,             NULL }, /* 15 */
    { cpu_execute_organisational_MS_load,               NULL }, /* 16 */
    { cpu_execute_organisational_DL_load,               NULL }, /* 17 */
    { cpu_execute_organisational_spm,                   NULL }, /* 18 */
    { cpu_execute_organisational_setlink,               NULL }, /* 19 */
    { cpu_execute_organisational_load_XNB,              NULL }, /* 20 */
    { cpu_execute_organisational_load_SN,               NULL }, /* 21 */
    { cpu_execute_organisational_XNB_plus,              NULL }, /* 22 */
    { cpu_execute_organisational_XNB_store,             NULL }, /* 23 */
    { cpu_execute_organisational_SF_load,               NULL }, /* 24 */
    { cpu_execute_organisational_SF_plus,               NULL }, /* 25 */
    { cpu_execute_organisational_SF_load_NB_plus,       NULL }, /* 26 */
    { cpu_execute_organisational_SF_store,              NULL }, /* 27 */
    { cpu_execute_organisational_NB_load,               NULL }, /* 28 */
    { cpu_execute_organisational_NB_load_SF_plus,       NULL }, /* 29 */
    { cpu_execute_organisational_NB_plus,               NULL }, /* 30 */
    { cpu_execute_organisational_NB_store,              NULL }, /* 31 */
    { cpu_execute_organisational_branch_eq,             NULL }, /* 32 */
    { cpu_execute_organisational_branch_ne,             NULL }, /* 33 */
    { cpu_execute_organisational_branch_ge,             NULL }, /* 34 */
    { cpu_execute_organisational_branch_lt,             NULL }, /* 35 */
    { cpu_execute_organisational_branch_le,             NULL }, /* 36 */
    { cpu_execute_organisational_branch_gt,             NULL }, /* 37 */
    { cpu_execute_organisational_branch_ovf,            NULL }, /* 38 */
    { cpu_execute_organisational_branch_bn,             NULL }, /* 39 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 40 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 41 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 42 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 43 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 44 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 45 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 46 */
    { cpu_execute_organisational_bn_by_operand,         NULL }, /* 47 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 48 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 49 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 50 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 51 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 52 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 53 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 54 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 55 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 56 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 57 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 58 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 59 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 60 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 61 */
    { cpu_execute_organisational_bn_by_order,           NULL }, /* 62 */
    { cpu_execute_organisational_bn_by_order,           NULL }  /* 63 */
};

static DISPATCH_ENTRY sts1DispatchTable[] =
{
    { cpu_execute_sts1_xdo_load, NULL },   /* 0 */
    { cpu_execute_sts1_xd_load,  NULL },   /* 1 */
    { cpu_execute_sts1_stack,    NULL },   /* 2 */
    { cpu_execute_sts1_xd_store, NULL },   /* 3 */
    { cpu_execute_sts1_xdb_load, NULL },   /* 4 */
    { cpu_execute_sts1_xchk,     NULL },   /* 5 */
    { cpu_execute_sts1_smod,     NULL },   /* 6 */
    { cpu_execute_sts1_xmod,     NULL },   /* 7 */
    { cpu_execute_sts1_slgc,     NULL },   /* 8 */
    { cpu_execute_sts1_smvb,     NULL },   /* 9 */
    { cpu_execute_sts1_smve,     NULL },   /* 10 */ /* Remove when don't need to compare to HASE simulator, was added there by mistake, never implemented in MU5 */
    { cpu_execute_sts1_smvf,     NULL },   /* 11*/
    { cpu_execute_sts1_talu,     NULL },   /* 12 */
    { cpu_execute_dummy_order,   NULL },   /* 13 */
    { cpu_execute_sts1_scmp,     NULL },   /* 14 */
    { cpu_execute_sts1_sub1,     NULL },   /* 15 */
};

static DISPATCH_ENTRY sts2DispatchTable[] =
{
    { cpu_execute_sts2_do_load,          NULL },   /* 0 */
    { cpu_execute_sts2_d_load,           NULL },   /* 1 */
    { cpu_execute_sts2_d_stack_and_load, NULL },   /* 2 */
    { cpu_execute_sts2_d_store,          NULL },   /* 3 */
    { cpu_execute_sts2_db_load,          NULL },   /* 4 */
    { cpu_execute_sts2_mdr,              NULL },   /* 5 */
    { cpu_execute_sts2_mod,              NULL },   /* 6 */
    { cpu_execute_sts2_rmod,             NULL },   /* 7 */
    { cpu_execute_sts2_blgc,             NULL },   /* 8 */
    { cpu_execute_sts2_bmvb,             NULL },   /* 9 */
    { cpu_execute_sts2_bmve,             NULL },   /* 10 */
    { cpu_execute_sts1_smvf,             NULL },   /* 11*/
    { cpu_execute_dummy_order,           NULL },   /* 12 */
    { cpu_execute_sts2_bscn,             NULL },   /* 13 */
    { cpu_execute_sts2_bcmp,             NULL },   /* 14 */
    { cpu_execute_sts2_sub2,             NULL },   /* 15 */
};

static DISPATCH_ENTRY bDispatchTable[] =
{
    { cpu_execute_b_load,                  NULL },   /* 0 */
    { cpu_execute_b_load_and_decrement,    NULL },   /* 1 */
    { cpu_execute_b_stack_and_load,        NULL },   /* 2 */
    { cpu_execute_b_store,                 NULL },   /* 3 */
    { cpu_execute_b_add,                   NULL },   /* 4 */
    { cpu_execute_b_sub,                   NULL },   /* 5 */
    { cpu_execute_b_mul,                   NULL },   /* 6 */
    { cpu_execute_b_div,                   NULL },   /* 7 */ /* Remove when don't need to compare to HASE simulator, was added there by mistake, never implemented in MU5 */
    { cpu_execute_b_xor,                   NULL },   /* 8 */
    { cpu_execute_b_or,                    NULL },   /* 9 */
    { cpu_execute_b_shift_left,            NULL },   /* 10 */
    { cpu_execute_b_and,                   NULL },   /* 11*/
    { cpu_execute_b_reverse_sub,           NULL },   /* 12 */
    { cpu_execute_b_compare,               NULL },   /* 13 */
    { cpu_execute_b_compare_and_increment, NULL },   /* 14 */
    { cpu_execute_b_reverse_div,           NULL },   /* 15 */ /* Remove when don't need to compare to HASE simulator, was added there by mistake, never implemented in MU5 */
};

static DISPATCH_ENTRY accFPSignedDispatchTable[] =
{
    { cpu_execute_fp_signed_load_single,    NULL }, /* 0 */
    { cpu_execute_dummy_order,              NULL }, /* 1 */
    { cpu_execute_fp_signed_stack_and_load, NULL }, /* 2 */
    { cpu_execute_fp_signed_store,          NULL }, /* 3 */
    { cpu_execute_fp_signed_add,            NULL }, /* 4 */
    { cpu_execute_fp_signed_sub,            NULL }, /* 5 */
    { cpu_execute_fp_signed_mul,            NULL }, /* 6 */
    { cpu_execute_fp_signed_div,            NULL }, /* 7 */
    { cpu_execute_fp_signed_xor,            NULL }, /* 8 */
    { cpu_execute_fp_signed_or,             NULL }, /* 9 */
    { cpu_execute_fp_signed_shift_left,     NULL }, /* 10 */
    { cpu_execute_fp_signed_and,            NULL }, /* 11*/
    { cpu_execute_fp_signed_reverse_sub,    NULL }, /* 12 */
    { cpu_execute_fp_signed_compare,        NULL }, /* 13 */
    { cpu_execute_fp_signed_convert,        NULL }, /* 14 */
    { cpu_execute_fp_signed_reverse_div,    NULL }, /* 15 */
};

static DISPATCH_ENTRY accFPUnsignedDispatchTable[] =
{
    { cpu_execute_fp_unsigned_load_single,    NULL }, /* 0 */
    { cpu_execute_dummy_order,                NULL }, /* 1 */
    { cpu_execute_fp_unsigned_stack_and_load, NULL }, /* 2 */
    { cpu_execute_fp_unsigned_store,          NULL }, /* 3 */
    { cpu_execute_fp_unsigned_add,            NULL }, /* 4 */
    { cpu_execute_fp_unsigned_sub,            NULL }, /* 5 */
    { cpu_execute_fp_unsigned_mul,            NULL }, /* 6 */
    { cpu_execute_fp_unsigned_div,            NULL }, /* 7 */ /* Remove when don't need to compare to HASE simulator, was added there by mistake, never implemented in MU5 */
    { cpu_execute_fp_unsigned_xor,            NULL }, /* 8 */
    { cpu_execute_fp_unsigned_or,             NULL }, /* 9 */
    { cpu_execute_fp_unsigned_shift_left,     NULL }, /* 10 */
    { cpu_execute_fp_unsigned_and,            NULL }, /* 11*/
    { cpu_execute_fp_unsigned_reverse_sub,    NULL }, /* 12 */
    { cpu_execute_fp_unsigned_compare,        NULL }, /* 13 */
    { cpu_execute_dummy_order,                NULL }, /* 14 */
    { cpu_execute_fp_unsigned_reverse_div,    NULL }, /* 15 */ /* Remove when don't need to compare to HASE simulator, was added there by mistake, never implemented in MU5 */
};

static DISPATCH_ENTRY accFPDecimalDispatchTable[] =
{
    { cpu_execute_dummy_order,               NULL }, /* 0 */
    { cpu_execute_fp_decimal_load_double,    NULL }, /* 1 */
    { cpu_execute_fp_decimal_stack_and_load, NULL }, /* 2 */
    { cpu_execute_fp_decimal_store,          NULL }, /* 3 */
    { cpu_execute_dummy_order,               NULL }, /* 4 */
    { cpu_execute_dummy_order,               NULL }, /* 5 */
    { cpu_execute_dummy_order,               NULL }, /* 6 */
    { cpu_execute_dummy_order,               NULL }, /* 7 */
    { cpu_execute_dummy_order,               NULL }, /* 8 */
    { cpu_execute_dummy_order,               NULL }, /* 9 */
    { cpu_execute_dummy_order,               NULL }, /* 10 */
    { cpu_execute_dummy_order,               NULL }, /* 11*/
    { cpu_execute_fp_decimal_compare,        NULL }, /* 12 */
    { cpu_execute_dummy_order,               NULL }, /* 13 */
    { cpu_execute_dummy_order,               NULL }, /* 14 */
    { cpu_execute_dummy_order,               NULL }, /* 15 */
};

static DISPATCH_ENTRY floatingPointDispatchTable[] =
{
    { cpu_execute_flp_load_single,    NULL }, /* 0 */
    { cpu_execute_flp_load_double,    NULL }, /* 1 */
    { cpu_execute_flp_stack_and_load, NULL }, /* 2 */
    { cpu_execute_flp_store,          NULL }, /* 3 */
    { cpu_execute_dummy_order,        NULL }, /* 4 */
    { cpu_execute_dummy_order,        NULL }, /* 5 */
    { cpu_execute_dummy_order,        NULL }, /* 6 */
    { cpu_execute_dummy_order,        NULL }, /* 7 */
    { cpu_execute_flp_unsigned_xor,   NULL }, /* 8 */
    { cpu_execute_flp_unsigned_or,    NULL }, /* 9 */
    { cpu_execute_flp_shift_circ,     NULL }, /* 10 */
    { cpu_execute_flp_unsigned_and,   NULL }, /* 11*/
    { cpu_execute_dummy_order,        NULL }, /* 12 */
    { cpu_execute_dummy_order,        NULL }, /* 13 */
    { cpu_execute_dummy_order,        NULL }, /* 14 */
    { cpu_execute_dummy_order,        NULL }, /* 15 */
};

static DISPATCH_ENTRY crDispatchTable[] =
{
    { cpu_execute_cr_level, organisationalDispatchTable }, /* 0 */
    { cpu_execute_cr_level, bDispatchTable },              /* 1 */
    { cpu_execute_cr_level, sts1DispatchTable },           /* 2 */
    { cpu_execute_cr_level, sts2DispatchTable },           /* 3 */
    { cpu_execute_cr_level, accFPSignedDispatchTable },    /* 4 */
    { cpu_execute_cr_level, accFPUnsignedDispatchTable },  /* 5 */
    { cpu_execute_cr_level, accFPDecimalDispatchTable },   /* 6 */
    { cpu_execute_cr_level, floatingPointDispatchTable }   /* 7 */
};

t_stat sim_instr(void)
{
    t_stat reason = SCPE_OK;
    cpu_stopped = 0;

    while (!cpu_stopped)
    {
        if (sim_interval <= 0)
        {
#if !UNIX_PLATFORM
            if ((reason = sim_poll_kbd()) == SCPE_STOP) {   /* poll on platforms without reliable signalling */
                break;
            }
#endif
            if ((reason = sim_process_event()) != SCPE_OK)
            {
                break;
            }
        }

        if ((sim_brk_summ & SWMASK('E')) && sim_brk_test(cpu_get_register_32(reg_co), SWMASK('E')))
        {
            reason = SCPE_STOP;
            break;
        }

        cpu_execute_next_order();

        sim_interval--;
    }

    if (cpu_stopped)
    {
        reason = SCPE_STOP;
    }

    sim_debug(LOG_CPU_PERF, &cpu_dev, "CPU ran at %.1f MIPS\n", sim_timer_inst_per_sec() / 1000000);

    return reason;
}

/* Loads two formats of file. Both formats are sequences of 16-bit words. If the -o option is specified then an absolute binary file loaded starting at the specified physical origin. If the
 -o option is not specified then the file is a multi-segment binary file loaded into virtual addresses as specified in the file. The multi-segement file uses a format that has been made-up, as I
 don't know how this was done in MU5 itself. Each segment in the file starts with a 16-bit segment number and a 16-bit length (in 16-bit words), followed by the binary. This format allows files
 produced by my XPL cross-compiler to be loaded. */
t_stat sim_load(FILE *ptr, CONST char *cptr, CONST char *fnam, int flag)
{
    t_stat r = SCPE_OK;
	uint16 i;
	uint32 segment;
	uint16 segment_length;
    int b;
    t_addr origin, limit;

    if (flag) /* dump? */
    {
        r = sim_messagef(SCPE_NOFNC, "Command Not Implemented\n");
    }
    else
    {
        origin = 0;
        limit = (t_addr)cpu_unit.capac * sizeof(uint32); /* Capacity is in 32-bit words, we need bytes here */
        if (sim_switches & SWMASK('O')) /* Origin option */
        {
            origin = (t_addr)get_uint(cptr, 16, limit, &r);
            if (r != SCPE_OK)
            {
                r = SCPE_ARG;
            }
        }
    }

    if (r == SCPE_OK)
    {
		if (sim_switches & SWMASK('O'))
		{
			while ((b = Fgetc(ptr)) != EOF)
			{
				if (origin >= limit)
				{
					r = SCPE_NXM;
					break;
				}

				sac_write_8_bit_word_real_address(origin++, b & 0xFF);
			}
		}
		else
		{
			do
			{
				segment = (uint8)Fgetc(ptr) << 8 | (uint8)Fgetc(ptr);
				segment_length = (uint8)Fgetc(ptr) << 8 | (uint8)Fgetc(ptr);
				if (feof(ptr))
				{
					break;
				}

				origin = segment << 18;
				printf("Loading segment %u of length %hu at virtual address %08X\n", segment, segment_length, origin);
				for (i = 0; i < 2 * segment_length; i++)
				{
					b = Fgetc(ptr);
					sac_write_8_bit_word(origin++, b);
				}
			} while (!feof(ptr));
		}
    }

    return r;
}

t_stat fprint_sym(FILE *ofile, t_addr addr, t_value *val, UNIT *uptr, int32 sw)
{
    return SCPE_NOFNC;
}

t_stat parse_sym(CONST char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
    return SCPE_NOFNC;
}

/* reset routine */
static t_stat cpu_reset(DEVICE *dptr)
{
    t_stat result = SCPE_OK;
    sim_brk_types = sim_brk_dflt = SWMASK('E');
    cpu_reset_state();
    if (sim_switches & SWMASK('T'))
    {
        TESTCONTEXT testContext;
        mu5_selftest_start(&testContext);
        cpu_selftest(&testContext);
        sac_selftest(&testContext);
        console_selftest(&testContext);
        result = mu5_selftest_end(&testContext);
    }

    return result;
}

/* memory examine */
static t_stat cpu_ex(t_value *vptr, t_addr addr, UNIT *uptr, int32 sw)
{
    t_stat result = SCPE_OK;
    if (vptr == NULL)
    {
        result = SCPE_ARG;
    }
    else if (addr < MAXMEMORY)
    {
        *vptr = sac_read_32_bit_word(addr);
    }
    else
    {
        result = SCPE_NXM;
    }

    return result;
}

/* memory deposit */
static t_stat cpu_dep(t_value val, t_addr addr, UNIT *uptr, int32 sw)
{
    return SCPE_AFAIL;
}

/* used for self test purposes only, so register setting includes calling the callbacks if appropriate, to make interrupt testing easier */
void cpu_set_register(REG *reg, t_uint64 value)
{
    switch (reg->width)
    {
        case 16:
        {
            cpu_set_register_16(reg, value & 0xFFFF);
            break;
        }
        case 32:
        {
            cpu_set_register_32(reg, value & 0xFFFFFFFF);
            break;
        }
        case 64:
        {
            cpu_set_register_64(reg, value & 0xFFFFFFFFFFFFFFFF);
            break;
        }
    }
}

static SIM_INLINE void cpu_set_register_16(REG *reg, uint16 value)
{
	uint16 old_value;
	uint16_register_backing *backing;
	assert(reg->width == 16);
	backing = reg->loc;
	old_value = backing->backing_value;
	backing->backing_value = value;
	if (reg->flags & REG_CALLBACK)
	{
		backing->callback(old_value, backing->backing_value);
	}
}

static SIM_INLINE void cpu_set_register_32(REG *reg, uint32 value)
{
	uint32 old_value;
	uint32_register_backing *backing;
    assert(reg->width == 32);
	backing = reg->loc;
	old_value = backing->backing_value;
	backing->backing_value = value;
    if (reg->flags & REG_CALLBACK)
    {
		backing->callback(old_value, backing->backing_value);
	}
}

static SIM_INLINE void cpu_set_register_64(REG *reg, t_uint64 value)
{
	t_uint64 old_value;
	t_uint64_register_backing *backing;
	assert(reg->width == 64);
    *(t_uint64 *)(reg->loc) = value;
	backing = reg->loc;
	old_value = backing->backing_value;
	backing->backing_value = value;
    if (reg->flags & REG_CALLBACK)
    {
		backing->callback(old_value, backing->backing_value);
	}
}

static SIM_INLINE void cpu_set_register_bit_16(REG *reg, uint16 mask, int value)
{
	uint16 old_value;
	uint16 new_value;
	assert(reg->width == 16);
	old_value = *(uint16 *)(reg->loc);
	if (value)
	{
		new_value = old_value | mask;
	}
	else
	{
		new_value = old_value & ~mask;
	}
	cpu_set_register_16(reg, new_value);
}

static SIM_INLINE void cpu_set_register_bit_32(REG *reg, uint32 mask, int value)
{
	uint32 old_value;
	uint32 new_value;
    assert(reg->width == 32);
	old_value = *(uint32 *)(reg->loc);
    if (value)
    {
        new_value = old_value | mask;
    }
    else
    {
		new_value = old_value & ~mask;
    }
	cpu_set_register_32(reg, new_value);
}

static SIM_INLINE void cpu_set_register_bit_64(REG *reg, t_uint64 mask, int value)
{
	t_uint64 old_value;
	t_uint64 new_value;
	assert(reg->width == 64);
	old_value = *(t_uint64 *)(reg->loc);
	if (value)
	{
		new_value = old_value | mask;
	}
	else
	{
		new_value = old_value & ~mask;
	}
	cpu_set_register_64(reg, new_value);
}

static SIM_INLINE uint16 cpu_get_register_16(REG *reg)
{
    uint16 result;
    assert(reg->width == 16);
    result = *(uint16 *)(reg->loc);
    return result;
}

static SIM_INLINE uint32 cpu_get_register_32(REG *reg)
{
    uint32 result;
    assert(reg->width == 32);
    result = *(uint32 *)(reg->loc);
    return result;
}

static SIM_INLINE t_uint64 cpu_get_register_64(REG *reg)
{
    t_uint64 result;
    assert(reg->width == 64);
    result = *(t_uint64 *)(reg->loc);
    return result;
}

static SIM_INLINE int cpu_get_register_bit_16(REG *reg, uint16 mask)
{
    int result;
    assert(reg->width == 16);
    if (*(uint16 *)(reg->loc) & mask)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

static SIM_INLINE int cpu_get_register_bit_32(REG *reg, uint32 mask)
{
    int result;
    assert(reg->width == 32);
    if (*(uint32 *)(reg->loc) & mask)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

static SIM_INLINE int cpu_get_register_bit_64(REG *reg, t_uint64 mask)
{
    int result;
    assert(reg->width == 64);
    if (*(t_uint64 *)(reg->loc) & mask)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

static void cpu_set_ms(uint16 value)
{
    uint16 msMask = (cpu_is_executive_mode()) ? 0xFFFF : 0xFF00;
    uint16 ms = cpu_get_register_16(reg_ms);
    uint16 newMs = (value & msMask) | (ms & ~msMask);
    cpu_set_register_16(reg_ms, newMs);
}

static void cpu_set_nb(uint16 value)
{
    cpu_set_register_16(reg_nb, value & 0xFFFE);
}

static void cpu_set_xnb(uint32 value)
{
    cpu_set_register_32(reg_xnb, value & 0xFFFFFFFE);
}

static void cpu_set_sf(uint16 value)
{
    cpu_set_register_16(reg_sf, value & 0xFFFE);
}

static void cpu_set_co(uint32 value)
{
    cpu_set_register_32(reg_co, value & 0x7FFFFFFF);
}

static void cpu_set_ms_bn(int value)
{
    cpu_set_register_bit_16(reg_ms, mask_ms_bn, value);
}

static void cpu_aod_callback(t_uint64 old_value, t_uint64 new_value)
{
    cpu_evaluate_interrupts();
}

static void cpu_bod_callback(uint32 old_value, uint32 new_value)
{
    cpu_evaluate_interrupts();
}

static void cpu_dod_callback(uint32 old_value, uint32 new_value)
{
    cpu_evaluate_interrupts();
}

static uint32 cpu_co_add(uint32 co, t_int64 operand)
{
    /* TODO: I think this is wrong because CO is the address of a 16-bit word and segments are 64K 32-bit words (I think) */
    uint16 originalSegment = co >> 16;
    uint32 newCo = ((t_uint64)co + operand) & MASK_32;
    uint16 newSegment = newCo >> 16;
    if (originalSegment != newSegment)
    {
        cpu_set_control_adder_overflow_interrupt();
    }

    return newCo;
}

static t_uint64 cpu_get_link(void)
{
    t_uint64 link = ((t_uint64)cpu_get_register_16(reg_ms) << 48) | ((t_uint64)cpu_get_register_16(reg_nb) << 32) | cpu_get_register_32(reg_co);
    return link;
}

static void cpu_set_link(t_uint64 link)
{
    cpu_set_ms((link >> 48) & MASK_16);
    cpu_set_nb((link >> 32) & MASK_16);
    cpu_set_co(link & MASK_32);
}

static uint16 cpu_calculate_base_offset_from_addr(t_addr base, t_int64 offset, uint8 scale)
{
    t_int64 result = base + (offset * scale);
    if (result < 0 || result > 65535)
    {
        cpu_set_name_adder_overflow_interrupt();
    }

    return (uint16)(result & 0xFFFF);
}

static t_addr cpu_get_name_segment_address_from_addr(t_addr base, int16 offset, uint8 scale)
{
    t_addr result = (cpu_get_register_16(reg_sn) << 16) | cpu_calculate_base_offset_from_addr(base, offset, scale);
    return result;
}

static uint16 cpu_calculate_base_offset_from_reg_16(REG *reg, t_int64 offset, uint8 scale)
{
    uint16 result = cpu_calculate_base_offset_from_addr(cpu_get_register_16(reg), offset, scale);

    return result;
}

static uint32 cpu_calculate_base_offset_from_reg_32(REG *reg, t_int64 offset, uint8 scale)
{
    uint32 result;
    uint32 fullAddress = cpu_get_register_32(reg);
    uint16 segmentOffset = fullAddress & MASK_16;
    uint16 newOffset = cpu_calculate_base_offset_from_addr(segmentOffset, offset, scale);
    result = (fullAddress & 0xFFFF0000) | (newOffset & MASK_16);
    return result;
}

static t_addr cpu_get_name_segment_address_from_reg(REG *reg, int16 offset, uint8 scale)
{
    t_addr result = cpu_get_name_segment_address_from_addr(cpu_get_register_16(reg), offset, scale);
    return result;
}

static int cpu_is_executive_mode(void)
{
    uint16 ms = cpu_get_register_16(reg_ms);
    int result = (ms & mask_ms_exec) != 0;
    return result;
}

void cpu_reset_state(void)
{
    reg_aod_backing_value.callback = cpu_aod_callback;
    reg_bod_backing_value.callback = cpu_bod_callback;
    reg_dod_backing_value.callback = cpu_dod_callback;


    PROPProcessNumber = 0;
    PROPInstructionCounter = 0;
    cpu_clear_all_interrupts();
    cpu_set_register_32(reg_b, 0x0);
    cpu_set_register_32(reg_bod, 0x0);
    cpu_set_register_64(reg_a, 0x0);
    cpu_set_register_64(reg_aod, 0x0);
    cpu_set_register_64(reg_aex, 0x0);
    cpu_set_register_32(reg_x, 0x0);
    cpu_set_register_16(reg_ms, mask_ms_bcpr | mask_ms_exec); /* initialise in executive mode using real addresses */
    cpu_set_register_16(reg_nb, 0x0);
    cpu_set_register_32(reg_xnb, 0x0);
    cpu_set_register_16(reg_sn, 0x0);
    cpu_set_register_16(reg_sf, 0x0);
    cpu_set_register_32(reg_co, 0x0); /* TODO: probably needs to be reset to start of OS (upper half of memory) */
    cpu_set_register_64(reg_d, 0x0);
    cpu_set_register_64(reg_xd, 0x0);
    cpu_set_register_32(reg_dod, 0x0);
    cpu_set_register_32(reg_dt, 0x0);
    cpu_set_register_32(reg_xdt, 0x0);
    cpu_set_register_32(reg_dl, 0x0);

    sac_setup_v_store_location(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, prop_read_process_number_callback, prop_write_process_number_callback);
    sac_setup_v_store_location(PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, prop_read_program_fault_status_callback, prop_write_program_fault_status_callback);
	sac_setup_v_store_location(PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, prop_read_system_error_status_callback, prop_write_system_error_status_callback);
	sac_setup_v_store_location(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, prop_read_instruction_counter, prop_write_instruction_counter);
}

void cpu_set_interrupt(uint8 number)
{
    interrupt |= 1u << number;
}

static void cpu_clear_interrupt(uint8 number)
{
    interrupt &= ~(1u << number);
}

static void cpu_clear_all_interrupts(void)
{
    PROPProgramFaultStatus = 0;
    PROPSystemErrorStatus = 0;
    interrupt = 0;
}

static int cpu_check_condition_with_inhibit(REG *reg, uint32 condition_mask, uint32 inhibit_mask)
{
    int result;
    uint32 value = cpu_get_register_32(reg);
    result = (value & condition_mask) && !(value & inhibit_mask);
    return result;
}

static void cpu_evaluate_interrupts(void)
{
    int b_overflow = cpu_check_condition_with_inhibit(reg_bod, mask_bod_bovf, mask_bod_ibovf);
    int b_or_d_error = b_overflow;
    /* OR in the bits until all conditions are processed */
    if (cpu_ms_is_all(MS_MASK_EXEC))
    {
        if (cpu_ms_is_all(MS_MASK_B_D_SYS_ERR_EXEC))
        {
            PROPSystemErrorStatus = b_or_d_error << 7; /* TODO: check if really EXEC bit*/
        }
    }
    else
    {
        PROPProgramFaultStatus = b_overflow << 7; /* TODO: make some masks */
    }

    if (PROPSystemErrorStatus != 0)
    {
        cpu_set_interrupt(INT_SYSTEM_ERROR);
    }
    else
    {
        cpu_clear_interrupt(INT_SYSTEM_ERROR);
    }

    if (PROPProgramFaultStatus != 0 && !cpu_ms_is_all(MS_MASK_INH_PROG_FLT))
    {
        cpu_set_interrupt(INT_PROGRAM_FAULTS);
    }
    else
    {
        cpu_clear_interrupt(INT_PROGRAM_FAULTS);
    }
}

static void cpu_set_system_error_interrupt(uint16 reason)
{
    if ((cpu_ms_is_all(MS_MASK_B_D_SYS_ERR_EXEC) && (reason == SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR))
        ||
        (cpu_ms_is_all(MS_MASK_A_SYS_ERR_EXEC) && (reason == SYSTEM_ERROR_STATUS_MASK_ACC_ERROR))
        ||
        (reason != SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR
         &&
         reason != SYSTEM_ERROR_STATUS_MASK_ACC_ERROR
        )
       )
    {
        cpu_set_interrupt(INT_SYSTEM_ERROR);
        PROPSystemErrorStatus |= reason;
    }
}

static void cpu_set_program_fault_interrupt(uint16 reason)
{
    if (!cpu_ms_is_all(MS_MASK_INH_PROG_FLT))
    {
        cpu_set_interrupt(INT_PROGRAM_FAULTS);
        PROPProgramFaultStatus |= reason;
    }
}

static void cpu_set_illegal_order_interrupt(uint16 reason)
{
    cpu_set_interrupt(INT_ILLEGAL_ORDERS);
    PROPProgramFaultStatus |= reason;
}

static void cpu_set_A_interrupt(void)
{
    if (cpu_ms_is_all(MS_MASK_EXEC))
    {
        cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_ACC_ERROR);
    }
    else
    {
        cpu_set_program_fault_interrupt(PROGRAM_FAULT_STATUS_MASK_ACC_ERROR);
    }
}

static void cpu_set_B_interrupt(void)
{
    if (cpu_ms_is_all(MS_MASK_EXEC))
    {
        cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR);
    }
    else
    {
        cpu_set_program_fault_interrupt(PROGRAM_FAULT_STATUS_MASK_B_ERROR);
    }
}

static void cpu_set_D_interrupt(void)
{
    if (cpu_ms_is_all(MS_MASK_EXEC))
    {
        cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR);
    }
    else
    {
        cpu_set_program_fault_interrupt(PROGRAM_FAULT_STATUS_MASK_D_ERROR);
    }
}

static void cpu_set_its_interrupt(void)
{
    cpu_set_register_bit_32(reg_dod, mask_dod_its, 1);
    cpu_set_D_interrupt();
}

static void cpu_set_sss_interrupt(void)
{
    cpu_set_register_bit_32(reg_dod, mask_dod_sss, 1);
    if (!cpu_get_register_bit_32(reg_dod, mask_dod_sssi))
    {
        cpu_set_D_interrupt();
    }
}

static void cpu_set_bounds_check_interrupt(void)
{
    cpu_set_register_bit_32(reg_dod, mask_dod_bch, 1);
    if (!cpu_get_register_bit_32(reg_dod, mask_dod_bchi))
    {
        cpu_set_D_interrupt();
    }
}

static void cpu_set_name_adder_overflow_interrupt()
{
    if (cpu_ms_is_any(MS_MASK_LEVEL0 | MS_MASK_LEVEL1 | MS_MASK_EXEC))
    {
        cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_NAME_ADDER_OVF_ERROR);
    }
    else
    {
        cpu_set_illegal_order_interrupt(PROGRAM_FAULT_STATUS_MASK_NAME_ADDER_OVF_ERROR);
    }
}

static void cpu_set_control_adder_overflow_interrupt()
{
    if (cpu_ms_is_any(MS_MASK_LEVEL0 | MS_MASK_LEVEL1 | MS_MASK_EXEC))
    {
        cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_CONTROL_ADDER_OVF_ERROR);
    }
    else
    {
        cpu_set_illegal_order_interrupt(PROGRAM_FAULT_STATUS_MASK_CONTROL_ADDER_OVF_ERROR);
    }
}

static void cpu_set_illegal_function_interrupt(void)
{
	if (cpu_ms_is_any(MS_MASK_LEVEL0 | MS_MASK_LEVEL1 | MS_MASK_EXEC))
	{
		cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_ILLEGAL_FUNCTION_ERROR);
	}
	else
	{
		cpu_set_illegal_order_interrupt(PROGRAM_FAULT_STATUS_MASK_ILLEGAL_FUNCTION_ERROR);
	}
}

void cpu_set_access_violation_interrupt()
{
    if (cpu_ms_is_all(MS_MASK_EXEC))
    {
        cpu_set_system_error_interrupt(SYSTEM_ERROR_STATUS_MASK_CPR_ERROR);
    }
    else
    {
        cpu_set_illegal_order_interrupt(PROGRAM_FAULT_STATUS_MASK_CPR_ERROR);
    }
}

uint8 cpu_get_interrupt_number(void)
{
    uint8 i;
    uint8 result = 255;

    for (i = 0; i < 7; i++)
    {
        if (interrupt & (1u << i))
        {
            result = i;
            break;
        }
    }

    return result;
}

uint16 cpu_get_ms(void)
{
    return cpu_get_register_16(reg_ms);
}

int cpu_ms_is_all(uint16 bits)
{
    return (cpu_get_ms() & bits) == bits;
}

int cpu_ms_is_any(uint16 bits)
{
    return cpu_get_ms() & bits;
}

static uint8 cpu_get_cr(uint16 order)
{
    uint8 cr = (order >> 13) & 0x7;
    return cr;
}

static uint8 cpu_get_f(uint16 order)
{
    uint8 f;
    uint16 cr = cpu_get_cr(order);
    if (cr == 0)
    {
        f = (order >> 7) & 0x3F;
    }
    else
    {
        f = (order >> 9) & 0xF;
    }

    return f;
}

/* returns operand kind k, for organisational orders it returns 7 for extended operands, so k is the same across all order types */
static uint16 cpu_get_k(uint16 order)
{
    uint16 k;
    uint16 cr = cpu_get_cr(order);
    if (cr == 0)
    {
        k = (order >> 6) & 0x1;
        if (k == 1)
        {
            k = 7;
        }
    }
    else
    {
        k = (order >> 6) & 0x7;
    }

    return k;
}

static uint16 cpu_get_n(uint16 order)
{
    uint16 result = order & 0x3F;
    return result;
}

static uint16 cpu_get_extended_k(uint16 order)
{
    uint16 result = (order >> 3) & 0x7;
    return result;
}

static uint16 cpu_get_extended_n(uint16 order)
{
    uint16 result = order & 0x7;
    return result;
}

static int cpu_operand_is_secondary(uint16 order)
{
    int result = 0;
    uint16 k = cpu_get_k(order);
    uint16 kp = cpu_get_extended_k(order);

    result = (k == 4) || (k == 5) || (k == 6) || ((k == 7) && ((kp == 4) || (kp == 5) || (kp == 6)));

    return result;
}

static t_uint64 cpu_get_operand_6_bit_literal(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    result = cpu_get_n(order);
    result = cpu_sign_extend_6_bit(result);
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "%lld\n", result);
    return result;
}

static t_uint64 cpu_get_operand_extended_literal(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    uint16 word;
    int negative;
    int i;
    uint16 nprime = cpu_get_extended_n(order) & 0x3;
    uint8 unsignedLiteral = (order >> 2) & 0x1;

    int words = (nprime == 2) ? 4 : nprime + 1;
    for (i = 0; i < words; i++)
    {
        word = sac_read_16_bit_word_for_obey(instructionAddress + 1 + i);
        if (i == 0)
        {
            negative = (word & 0x8000);
        }
        result = result << 16;
        result |= word;
    }

    if (!unsignedLiteral && negative)
    {
        for (i = 0; i < 4 - words; i++)
        {
            result |= (t_uint64)0xFFFF << (3 - i) * 16;
        }
    }

    *instructionLength += words;

    if (unsignedLiteral)
    {
        sim_debug(LOG_CPU_DECODE, &cpu_dev, "%llu\n", result);
    }
    else
    {
        sim_debug(LOG_CPU_DECODE, &cpu_dev, "%lld\n", result);
    }

    return result;
}

static t_addr cpu_get_operand_extended_variable_address(uint16 order, uint32 instructionAddress, int *instructionLength, uint8 scale, int *is64bit)
{
    t_addr result = 0;
    uint16 np = cpu_get_extended_n(order);

    *is64bit = FALSE;

    switch (np)
    {
        case 0:
        {
            uint16 offset = sac_read_16_bit_word_for_obey(instructionAddress + 1);
            result = cpu_get_name_segment_address_from_reg(reg_sf, offset, scale);
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "SF %u\n", offset);
            break;
        }
        case 1:
        {
            uint16 offset = sac_read_16_bit_word_for_obey(instructionAddress + 1);
            result = cpu_get_name_segment_address_from_addr(0, offset, scale);
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "Z %u\n", offset);
            break;
        }
        case 2:
        {
            uint16 offset = sac_read_16_bit_word_for_obey(instructionAddress + 1);
            result = cpu_get_name_segment_address_from_reg(reg_nb, offset, scale);
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB %u\n", offset);
            break;
        }
        case 3:
        {
            t_addr addr = cpu_get_register_32(reg_xnb) & 0xFFFF;
            uint16 offset = sac_read_16_bit_word_for_obey(instructionAddress + 1);
            result = cpu_get_name_segment_address_from_addr(addr, offset, scale);
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "XNB %u\n", offset);
            break;
        }
        case 4:
        {
            result = cpu_pop_address();
            *is64bit = TRUE;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "STACK Z 0\n");
            break;
        }
        case 5:
        {
            result = 0;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "DR\n");
            break;
        }
        case 6:
        {
            t_addr addr = cpu_get_register_16(reg_nb);
            result = addr;
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "V32 NB 0\n");
            break;
        }
        case 7:
        {
            t_addr addr = cpu_get_register_32(reg_xnb) & 0xFFFF;
            result = addr;
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "XNB 0\n");
            break;
        }
    }

    return result;
}

static t_addr cpu_get_operand_address_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_addr result;
    uint16 n = cpu_get_n(order);
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V32 %hu\n", n);
    result = cpu_get_name_segment_address_from_reg(reg_nb, n, SCALE_32);

    return result;
}

static t_addr cpu_get_operand_address_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_addr result;
    uint16 n = cpu_get_n(order);
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V64 %hu\n", n);
    result = cpu_get_name_segment_address_from_reg(reg_nb, n, SCALE_64);

    return result;
}

static t_uint64 cpu_get_operand_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    t_addr addr;

    addr = cpu_get_operand_address_variable_32(order, instructionAddress, instructionLength);
    result = sac_read_32_bit_word(addr);

    return result;
}

static t_uint64 cpu_get_operand_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    t_addr addr;

    addr = cpu_get_operand_address_variable_64(order, instructionAddress, instructionLength);
    result = sac_read_64_bit_word(addr);

    return result;
}

static int32 cpu_scale_descriptor_modifier(t_uint64 descriptor, uint32 modifier)
{
    int32 result = modifier;
    uint8 type = cpu_get_descriptor_type(descriptor);
    if ((type == DESCRIPTOR_TYPE_GENERAL_VECTOR || type == DESCRIPTOR_TYPE_ADDRESS_VECTOR) && !cpu_get_descriptor_unscaled(descriptor))
    {
        switch (cpu_get_descriptor_size(descriptor))
        {
            case DESCRIPTOR_SIZE_1_BIT:
            {
                result = modifier >> 3;
                break;
            }
            case DESCRIPTOR_SIZE_4_BIT:
            {
                result = modifier >> 1;
                break;
            }
            case DESCRIPTOR_SIZE_8_BIT:
            {
                result = modifier;
                break;
            }
            case DESCRIPTOR_SIZE_16_BIT:
            {
                result = modifier << 1;
                break;
            }
            case DESCRIPTOR_SIZE_32_BIT:
            {
                result = modifier << 2;
                break;
            }
            case DESCRIPTOR_SIZE_64_BIT:
            {
                result = modifier << 3;
                break;
            }
            default:
            {
                cpu_set_its_interrupt();
                break;
            }
        }
    }

    return result;
}

/* see p 90 of Morris and Ibbett and section 2.10 of programming manual */
static t_addr cpu_get_operand_byte_address_by_descriptor_vector_access(t_uint64 descriptor, uint32 modifier)
{
    t_addr result;
    uint32 origin = cpu_get_descriptor_origin(descriptor);
    result = origin + cpu_scale_descriptor_modifier(descriptor, modifier);

    return result;
}

static t_uint64 cpu_get_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier)
{
    t_addr addr;
    t_uint64 result = 0;

	if (cpu_check_bound(descriptor, modifier))
	{
		addr = cpu_get_operand_byte_address_by_descriptor_vector_access(descriptor, modifier);

		switch (cpu_get_descriptor_size(descriptor))
		{
			case DESCRIPTOR_SIZE_1_BIT:
			{
				int bit = 7 - (modifier & 7);
				result = sac_read_8_bit_word(addr);
				result = (result >> bit) & 1;
				break;
			}
			case DESCRIPTOR_SIZE_4_BIT:
			{
				int nibble = 1 - (modifier & 1);
				result = sac_read_8_bit_word(addr);
				result = (result >> (4 * nibble)) & 0xF;
				break;
			}
			case DESCRIPTOR_SIZE_8_BIT:
			{
				result = sac_read_8_bit_word(addr);
				break;
			}
			case DESCRIPTOR_SIZE_16_BIT:
			{
				result = sac_read_16_bit_word(addr >> 1); /* TODO: 16-bit word routine takes a 16-bit word address, not a byte address. Need to make memory access consistent */
				break;
			}
			case DESCRIPTOR_SIZE_32_BIT:
			{
				result = sac_read_32_bit_word(addr >> 2); /* TODO: 32-bit word routine takes a 32-bit word address, not a byte address. Need to make memory access consistent */
				break;
			}
			case DESCRIPTOR_SIZE_64_BIT:
			{
				result = sac_read_64_bit_word(addr >> 2); /* TODO: 64-bit word routine takes a 32-bit word address, not a byte address. Need to make memory access consistent */
				break;
			}
			default:
			{
				result = 0;
				cpu_set_its_interrupt();
				break;
			}
		}
	}

    return result;
}

static void cpu_set_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier, t_uint64 value)
{
    t_addr addr;

	if (cpu_check_bound(descriptor, modifier))
	{
		addr = cpu_get_operand_byte_address_by_descriptor_vector_access(descriptor, modifier);

		switch (cpu_get_descriptor_size(descriptor))
		{
			case DESCRIPTOR_SIZE_1_BIT:
			{
				uint8 byte = sac_read_8_bit_word(addr);
				uint8 shift = 7 - (modifier & 0x7);
				uint8 bitMask = (uint8)0x1 << shift;
				uint8 bit = (value & 0x1) << shift;
				byte = (byte & ~bitMask) | bit;
				sac_write_8_bit_word(addr, byte);
				break;
			}
			case DESCRIPTOR_SIZE_4_BIT:
			{
				uint8 byte = sac_read_8_bit_word(addr);
				uint8 shift = 4 * (1 - (modifier & 0x1));
				uint8 nibbleMask = (uint8)0xF << shift;
				uint8 nibble = (value & 0xF) << shift;
				byte = (byte & ~nibbleMask) | nibble;
				sac_write_8_bit_word(addr, byte);
				break;
			}
			case DESCRIPTOR_SIZE_8_BIT:
			{
				sac_write_8_bit_word(addr, value & 0xFF);
				break;
			}
			case DESCRIPTOR_SIZE_16_BIT:
			{
				sac_write_16_bit_word(addr >> 1, value & 0xFFFF);  /* TODO: 16-bit word routine takes a 16-bit word address, not a byte address. Need to make memory access consistent */
				break;
			}
			case DESCRIPTOR_SIZE_32_BIT:
			{
				sac_write_32_bit_word(addr >> 2, value & MASK_32); /* TODO: 32-bit word routine takes a 32-bit word address, not a byte address. Need to make memory access consistent */
				break;
			}
			case DESCRIPTOR_SIZE_64_BIT:
			{
				sac_write_64_bit_word(addr >> 2, value); /* TODO: 64-bit word routine takes a 32-bit word address, not a byte address. Need to make memory access consistent */
				break;
			}
			default:
			{
				cpu_set_its_interrupt();
				break;
			}
		}
	}
}

static void cpu_process_source_to_destination_descriptor_vector(t_uint64 operand, t_uint64(*func)(t_uint64 source, t_uint64 destination, t_uint64 operand))
{
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 xdb = cpu_get_descriptor_bound(xd);

    if (db != 0)
    {
        int i;
        int n = (xdb < db) ? xdb : db;
        for (i = 0; i < n; i++)
        {
            t_uint64 source = cpu_get_operand_by_descriptor_vector(xd, i);
            t_uint64 destination = cpu_get_operand_by_descriptor_vector(d, i);
            cpu_set_operand_by_descriptor_vector(d, i, func(source, destination, operand));
        }
        cpu_descriptor_modify(reg_d, n, FALSE);
        cpu_descriptor_modify(reg_xd, n, FALSE);
        if (xdb < db)
        {
            cpu_set_sss_interrupt();
        }
    }
}

static t_uint64 cpu_get_operand_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    uint16 n = cpu_get_n(order);

    result = cpu_get_operand_from_descriptor(order, instructionAddress, instructionLength, cpu_get_register_32(reg_b));

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SB NB %hu\n", n);

    return result;
}

static void cpu_set_operand_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, t_uint64 value)
{
    t_uint64 result = 0;
    uint16 n = cpu_get_n(order);

    cpu_set_operand_from_descriptor(order, instructionAddress, instructionLength, cpu_get_register_32(reg_b), value);

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SB NB %hu\n", n);
}

static t_uint64 cpu_get_operand_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    uint16 n = cpu_get_n(order);

    result = cpu_get_operand_from_descriptor(order, instructionAddress, instructionLength, 0);

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S0 %hu\n", n);

    return result;
}

static void cpu_set_operand_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, t_uint64 value)
{
    t_uint64 result = 0;
    uint16 n = cpu_get_n(order);

    cpu_set_operand_from_descriptor(order, instructionAddress, instructionLength, 0, value);

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S0 NB %hu\n", n);
}


static t_uint64 cpu_get_operand_from_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, uint32 modifier)
{
    t_uint64 result = 0;
    t_uint64 d;
    t_addr daddr;
    uint16 n = cpu_get_n(order);

    daddr = cpu_get_name_segment_address_from_reg(reg_nb, n, SCALE_64);
    d = sac_read_64_bit_word(daddr);
    cpu_set_register_64(reg_d, d);

    result = cpu_get_operand_by_descriptor_vector(d, modifier);

    return result;
}

static void cpu_set_operand_from_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, uint32 modifier, t_uint64 value)
{
    t_uint64 result = 0;
    t_uint64 d;
    t_addr daddr;
    uint16 n = cpu_get_n(order);

    daddr = cpu_get_name_segment_address_from_reg(reg_nb, n, SCALE_64);
    d = sac_read_64_bit_word(daddr);
    cpu_set_register_64(reg_d, d);

    cpu_set_operand_by_descriptor_vector(d, modifier, value);
}

static t_uint64 cpu_get_operand_internal_register(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    uint8 n = order & 0x3F;

    switch (n)
    {
        case 0:
        {
            result = ((t_uint64)cpu_get_register_16(reg_ms) << 48) | ((t_uint64)cpu_get_register_16(reg_nb) << 32) | cpu_get_register_32(reg_co);
            break;
        }
        case 1:
        {
            result = cpu_get_register_32(reg_xnb);
            break;
        }
        case 2:
        {
            result = ((cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_nb)) & MASK_32;
            break;
        }
        case 3:
        {
            result = ((cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_sf)) & MASK_32;
            break;
        }
        case 4:
        {
            result = cpu_get_register_bit_16(reg_ms, mask_ms_bn);
            break;
        }
        case 16:
        {
            result = cpu_get_register_64(reg_d);
            break;
        }
        case 17:
        {
            result = cpu_get_register_64(reg_xd);
            break;
        }
        case 18:
        {
            result = cpu_get_register_32(reg_dt);
            break;
        }
        case 19:
        {
            result = cpu_get_register_32(reg_xdt);
            break;
        }
        case 20:
        {
            result = cpu_get_register_32(reg_dod);
            break;
        }
        case 32:
        {
			/* RNI comment April 2017 in relation to BOD for IR 32: The 1978 manual shows IR32 as just B, so either the 1972 version was wrong or the hardware
			was altered - I have a vague memory that this did happen. The book agrees with the 1978 version, so I'm sure it's
			correct and I've listed the B IR's this way in my reconstruction.
			*/
            result = cpu_get_register_32(reg_b);
            break;
        }
        case 33:
        {
            result = cpu_get_register_32(reg_bod);
            break;
        }
        case 34:
        {
            result = 0; /* cpu_get_register_32(reg_z); */ /* Z is an "imaginary" register, see p31 of Morris & Ibbett book. Z not implemented for now. */
            break;
        }
        case 36:
        {
            /* This one exists in the Morris and Ibbett book, but is not present in the MU5 Programming Manual */
            result = ((t_uint64)cpu_get_register_32(reg_bod) << 32) | cpu_get_register_32(reg_b);
            break;
        }
        case 48:
        {
            result = cpu_get_register_64(reg_aex);
            break;
        }
        default:
        {
            result = 0;
            break;
        }
    }

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "R%hu\n", (unsigned short)n);

    return result;
}

static t_uint64 cpu_set_operand_internal_register(uint16 order, t_uint64 value)
{
    t_uint64 result = 0;
    uint8 n = order & 0x3F;

    switch (n)
    {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		{
			cpu_set_illegal_function_interrupt();
			break;
		}
        case 16:
        {
            cpu_set_register_64(reg_d, value);
            break;
        }
        case 17:
        {
            cpu_set_register_64(reg_xd, value);
            break;
        }
        case 18:
        {
            cpu_set_register_32(reg_dt, value & MASK_32);
            break;
        }
        case 19:
        {
            cpu_set_register_32(reg_xdt, value & MASK_32);
            break;
        }
        case 20:
        {
            cpu_set_register_32(reg_dod, value & MASK_32);
            break;
        }
        case 32:
        {
			/* RNI comment April 2017 in relation to BOD for IR 32: The 1978 manual shows IR32 as just B, so either the 1972 version was wrong or the hardware
			was altered - I have a vague memory that this did happen. The book agrees with the 1978 version, so I'm sure it's
			correct and I've listed the B IR's this way in my reconstruction.
			*/
            cpu_set_register_32(reg_b, value & MASK_32);
            break;
        }
        case 33:
        {
            cpu_set_register_32(reg_bod, value & MASK_32);
            break;
        }
        case 34:
        {
            /* Z is an "imaginary" register, see p31 of Morris & Ibbett book. Programming Manual section 2.5 describes it used as an overlap suppression mechanism */
            break;
        }
        case 36:
        {
            /* This one exists in the Morris and Ibbett book, but is not present in the MU5 Programming Manual */
            cpu_set_register_32(reg_bod, (value >> 32) & MASK_32);
            cpu_set_register_32(reg_b, value & MASK_32);
            break;
        }
        case 48:
        {
            cpu_set_register_64(reg_aex, value);
            break;
        }
        default:
        {
            break;
        }
    }

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "R%hu\n", (unsigned short)n);

    return result;
}

static t_uint64 cpu_get_operand(uint16 order)
{
    t_uint64 result = 0;
    uint32 addr;
    int instructionLength = 1;
    uint16 k = cpu_get_k(order);
    uint32 instructionAddress = cpu_get_register_32(reg_co);
    int is64bit;

    switch (k)
    {
        case 0:
        {
            result = cpu_get_operand_6_bit_literal(order, instructionAddress, &instructionLength);
            break;
        }
        case 1:
        {
            result = cpu_get_operand_internal_register(order, instructionAddress, &instructionLength);
            break;
        }
        case 2:
        {
            result = cpu_get_operand_variable_32(order, instructionAddress, &instructionLength);
            break;
        }
        case 3:
        {
            result = cpu_get_operand_variable_64(order, instructionAddress, &instructionLength);
            break;
        }
        case 4:
        case 5: /* RNI - From a programmer's perspective, k=5 was a spare, so in the hardware it was treated as being identical to k=4.*/
        {
            result = cpu_get_operand_b_relative_descriptor(order, instructionAddress, &instructionLength);
            break;
        }
        case 6:
        {
            result = cpu_get_operand_zero_relative_descriptor(order, instructionAddress, &instructionLength);
            break;
        }
        case 7:
        {
            switch (cpu_get_extended_k(order))
            {
                case 0:
                case 1:
                {
                    result = cpu_get_operand_extended_literal(order, instructionAddress, &instructionLength);
                    break;
                }
                case 2:
                {
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V32 ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_32, &is64bit);
                    if (is64bit)
                    {
                        result = sac_read_64_bit_word(addr);
                    }
                    else
                    {
                        result = sac_read_32_bit_word(addr);
                    }

                    break;
                }
                case 3:
                {
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V64 ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    result = sac_read_64_bit_word(addr);
                    break;
                }
                case 4:
                case 5:
                {
                    t_uint64 d;
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S[B] ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    if (addr == 0)
                    {
                        d = cpu_get_register_64(reg_d);
                    }
                    else
                    {
                        d = sac_read_64_bit_word(addr);
                    }
                    result = cpu_get_operand_by_descriptor_vector(d, cpu_get_register_32(reg_b));
                    break;
                }
                case 6:
                {
                    t_uint64 d;
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S[0] ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    if (addr == 0)
                    {
                        d = cpu_get_register_64(reg_d);
                    }
                    else
                    {
                        d = sac_read_64_bit_word(addr);
                    }
                    result = cpu_get_operand_by_descriptor_vector(d, 0);
                    break;
                }
                case 7:
                {
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V-S ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    addr = addr >> 1; /* considered a 64-bit address? */
                    uint8 block = (addr >> 8) & MASK_8;
                    uint8 line = addr & MASK_8;
                    if (cpu_is_executive_mode())
                    {
                        result = sac_read_v_store(block, line);
                    }
                    else
                    {
                        cpu_set_illegal_order_interrupt(PROGRAM_FAULT_STATUS_ILLEGAL_V_STORE_ACCESS_ERROR);
                    }
                    break;
                }
            }

            break;
        }
    }

    cpu_set_co(cpu_co_add(instructionAddress, instructionLength));

    return result;
}

static void cpu_set_operand(uint16 order, t_uint64 value)
{
    t_addr addr;
    int instructionLength = 1;
    uint16 k = cpu_get_k(order);
    uint32 instructionAddress = cpu_get_register_32(reg_co);
    int is64bit;

    switch (k)
    {
        case 1:
        {
            cpu_set_operand_internal_register(order, value);
        }
        case 2:
        {
            addr = cpu_get_operand_address_variable_32(order, instructionAddress, &instructionLength);
            sac_write_32_bit_word(addr, value & MASK_32);
            break;
        }
        case 3:
        {
            addr = cpu_get_operand_address_variable_64(order, instructionAddress, &instructionLength);
            sac_write_64_bit_word(addr, value);
            break;
        }
        case 4:
        case 5: /* RNI - From a programmer's perspective, k=5 was a spare, so in the hardware it was treated as being identical to k=4.*/
        {
            cpu_set_operand_b_relative_descriptor(order, instructionAddress, &instructionLength, value);
            break;
        }
        case 6:
        {
            cpu_set_operand_zero_relative_descriptor(order, instructionAddress, &instructionLength, value);
            break;
        }
        case 7:
        {
            switch (cpu_get_extended_k(order))
            {
                case 2:
                {
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V32 ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_32, &is64bit);
                    if (is64bit)
                    {
                        sac_write_64_bit_word(addr, value & MASK_32);
                    }
                    else
                    {
                        sac_write_32_bit_word(addr, value & MASK_32);
                    }
                    break;
                }
                case 3:
                {
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V64 ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    sac_write_64_bit_word(addr, value);
                    break;
                }
                case 4:
                case 5:
                {
                    t_uint64 d;
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S[B] ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    if (addr == 0)
                    {
                        d = cpu_get_register_64(reg_d);
                    }
                    else
                    {
                        d = sac_read_64_bit_word(addr);
                    }
                    cpu_set_operand_by_descriptor_vector(d, cpu_get_register_32(reg_b), value);
                    break;
                }
                case 6:
                {
                    t_uint64 d;
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S[0] ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    if (addr == 0)
                    {
                        d = cpu_get_register_64(reg_d);
                    }
                    else
                    {
                        d = sac_read_64_bit_word(addr);
                    }
                    cpu_set_operand_by_descriptor_vector(d, 0, value);
                    break;
                case 7:
                {
                    uint8 block;
                    uint8 line;
                    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V-S ");
                    addr = cpu_get_operand_extended_variable_address(order, instructionAddress, &instructionLength, SCALE_64, &is64bit);
                    addr = addr >> 1; /* 64-bit address? */
                    block = (addr >> 8) & MASK_8;
                    line = addr & MASK_8;
                    if (cpu_is_executive_mode())
                    {
                        sac_write_v_store(block, line, value);
                    }
                    else
                    {
                        cpu_set_illegal_order_interrupt(PROGRAM_FAULT_STATUS_ILLEGAL_V_STORE_ACCESS_ERROR);
                    }
                    break;
                }
                }
                default:
                {
					cpu_set_illegal_function_interrupt();
                    break;
                }
            }

            break;
        }
        default:
        {
			cpu_set_illegal_function_interrupt();
        }
    }

    cpu_set_co(cpu_co_add(instructionAddress, instructionLength));
}

static t_uint64 cpu_sign_extend(t_uint64 value, int8 bits)
{
    t_uint64 mask = (t_uint64)(~0) >> (64 - bits);
    t_uint64 result = value & mask;
    result |= (value & ((t_uint64)1 << (bits -1))) ? ~mask : 0;
    return result;
}

static t_uint64 cpu_sign_extend_6_bit(t_uint64 value)
{
    t_uint64 result = cpu_sign_extend(value, 6);
    return result;
}

static t_uint64 cpu_sign_extend_32_bit(t_uint64 value)
{
    t_uint64 result = cpu_sign_extend(value, 32);
    return result;
}

static void cpu_push_value(t_uint64 value)
{
    uint16 newSF = cpu_get_register_16(reg_sf) + 2;
    cpu_set_sf(newSF);
    sac_write_64_bit_word(cpu_get_name_segment_address_from_reg(reg_sf, 0, SCALE_64), value);
}

static t_addr cpu_pop_address(void)
{
    t_addr result;
    uint16 sf = cpu_get_register_16(reg_sf);
    result = cpu_get_name_segment_address_from_reg(reg_sf, 0, SCALE_64);
    cpu_set_sf(sf - 2);
    return result;
}

static t_uint64 cpu_pop_value(void)
{
    t_uint64 result;
    result = sac_read_64_bit_word(cpu_pop_address());
    return result;
}

static void cpu_test_value(t_int64 value)
{
    cpu_set_register_bit_16(reg_ms, mask_ms_t1, value != 0);
    cpu_set_register_bit_16(reg_ms, mask_ms_t2, value < 0);
}

void cpu_execute_next_order(void)
{
    uint16 order;
    uint16 cr;

    if (interrupt == 0)
    {
        order = sac_read_16_bit_word_for_obey(cpu_get_register_32(reg_co));
        if (interrupt == 0)
        {
            cr = cpu_get_cr(order);
            if ((cpu_get_ms() & MS_MASK_INH_INS_COUNT) == 0 && PROPInstructionCounter > 0)
            {
                PROPInstructionCounter--;
                if (PROPInstructionCounter == 0)
                {
                    cpu_set_interrupt(INT_INSTRUCTION_COUNT_ZERO);
                }
            }

            crDispatchTable[cr].execute(order, crDispatchTable[cr].innerTable);
        }
    }
    else
    {
        cpu_start_interrupt_processing();
    }
}

static t_uint64 prop_read_process_number_callback(uint8 line)
{
    return PROPProcessNumber & 0xF;
}

static void prop_write_process_number_callback(uint8 line, t_uint64 value)
{
    PROPProcessNumber = value & 0xF;
}

static t_uint64 prop_read_program_fault_status_callback(uint8 line)
{
    return PROPProgramFaultStatus & 0xFFE0;
}

static void prop_write_program_fault_status_callback(uint8 line, t_uint64 value)
{
    PROPProgramFaultStatus = 0;
}

static t_uint64 prop_read_system_error_status_callback(uint8 line)
{
    return PROPSystemErrorStatus;
}

static t_uint64 prop_read_instruction_counter(uint8 line)
{
	return PROPInstructionCounter;
}

static void prop_write_instruction_counter(uint8 line, t_uint64 value)
{
	PROPInstructionCounter = value & 0xFFFF;
}

static void prop_write_system_error_status_callback(uint8 line, t_uint64 value)
{
    PROPSystemErrorStatus = 0;
}

static void cpu_execute_dummy_order(uint16 order, DISPATCH_ENTRY *innerTable)
{
    /* Dummy orders are legal, they just have no effect. */
}

static void cpu_start_interrupt_processing(void)
{
    uint8 interruptNumber = cpu_get_interrupt_number();
    t_uint64 link = cpu_get_link();
    t_uint64 newLink = sac_read_v_store(SYSTEM_V_STORE_BLOCK, 16 + (interruptNumber * 2) + 1);
    sac_write_v_store(SYSTEM_V_STORE_BLOCK, 16 + (interruptNumber * 2), link);
	cpu_set_register_bit_16(reg_ms, MS_MASK_EXEC, 1);
    cpu_set_link(newLink);
    printf("Interrupt %hu detected\n", (unsigned short)interruptNumber);
	cpu_clear_interrupt(interruptNumber); /* TODO: this is a hack for now until I do the cascaded logic and implement L0IF and L1IF to inhibit */
    //cpu_stopped = 1; /* TODO: temporary halt CPU until implement interrupt processing */
}

static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable)
{
    uint16 f = cpu_get_f(order);
    innerTable[f].execute(order, innerTable[f].innerTable);
}

static void cpu_jump_relative(uint16 order, int performJump)
{
    uint32 relativeTo = cpu_get_register_32(reg_co); /* Get CO before it is advanced by getting operand as we jump relative to instruction */
    t_int64 relative = (t_int64)cpu_get_operand(order) & MASK_32;
    uint32 newCo = cpu_co_add(relativeTo, relative);

    if (performJump)
    {
        cpu_set_co(newCo);

        /* The real MU5 did not have a STOP instruction, see comment above the declaration of cpu_stopped */
        if (relative == 0)
        {
            cpu_stopped = 1;
        }
    }
}

static int cpu_calculate_condition(uint8 conditionCode)
{
    int r;

    switch (conditionCode & 0x7)
    {
        case 0:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "= 0 ");
            r = !cpu_get_register_bit_16(reg_ms, mask_ms_t1);
            break;
        }
        case 1:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "/= 0 ");
            r = cpu_get_register_bit_16(reg_ms, mask_ms_t1);
            break;
        }
        case 2:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, ">= 0 ");
            r = !cpu_get_register_bit_16(reg_ms, mask_ms_t1) || !cpu_get_register_bit_16(reg_ms, mask_ms_t2);
            break;
        }
        case 3:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "< 0 ");
            r = cpu_get_register_bit_16(reg_ms, mask_ms_t2);
            break;
        }
        case 4:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "<= 0 ");
            r = !cpu_get_register_bit_16(reg_ms, mask_ms_t1) || cpu_get_register_bit_16(reg_ms, mask_ms_t2);
            break;
        }
        case 5:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "> 0 ");
            r = cpu_get_register_bit_16(reg_ms, mask_ms_t1) && !cpu_get_register_bit_16(reg_ms, mask_ms_t2);
            break;
        }
        case 6:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "OVERFLOW ");
            r = cpu_get_register_bit_16(reg_ms, mask_ms_t0);
            break;
        }
        case 7:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN ");
            r = cpu_get_register_bit_16(reg_ms, mask_ms_bn);
            break;
        }
    }

    return r;
}

static void cpu_process_condition(int condition, int8 function)
{
    int bn = cpu_get_register_bit_16(reg_ms, mask_ms_bn);
    int newBn;

    switch (function & 0xF)
    {
        case 0:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN = 0 ");
            newBn = 0;
            break;
        }
        case 1:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN & T ");
            newBn = bn && condition;
            break;
        }
        case 2:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN/ & T ");
            newBn = !bn && condition;
            break;
        }
        case 3:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN = T ");
            newBn = condition;
            break;
        }
        case 4:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN &/ T ");
            newBn = bn && !condition;
            break;
        }
        case 5:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "DUMMY ");
            newBn = bn;
            break;
        }
        case 6:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN /== T ");
            newBn = bn ^ condition;
            break;
        }
        case 7:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN V T ");
            newBn = bn || condition;
            break;
        }
        case 8:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN/ &/ T ");
            newBn = !bn && !condition;
            break;
        }
        case 9:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN == T ");
            newBn = !(bn ^ condition);
            break;
        }
        case 10:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN/ ");
            newBn = !bn;
            break;
        }
        case 11:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN /V T ");
            newBn = !bn || condition;
            break;
        }
        case 12:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN =/ T ");
            newBn = !condition;
            break;
        }
        case 13:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN V /T ");
            newBn = bn || !condition;
            break;
        }
        case 14:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN/ V /T ");
            newBn = !bn || !condition;
            break;
        }
        case 15:
        {
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "BN = 1 ");
            newBn = 1;
            break;
        }
    }

    cpu_set_ms_bn(newBn);
}

static void cpu_execute_organisational_relative_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "-> ");
    cpu_jump_relative(order, 1);
}

static void cpu_execute_organisational_exit(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "EXIT ");
    t_uint64 operand = cpu_get_operand(order);
    cpu_set_link(operand);
}

static void cpu_execute_organisational_absolute_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "JUMP ");
    cpu_set_co(cpu_get_operand(order) & MASK_32);
}

static void cpu_execute_organisational_return(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "RETURN ");
    cpu_set_sf(cpu_get_register_16(reg_nb));
    t_uint64 operand = cpu_get_operand(order);
    cpu_set_ms((operand >> 48) & MASK_16);
    cpu_set_nb((operand >> 32) & MASK_16);
    cpu_set_co(operand & MASK_32);
}

static void cpu_execute_organisational_xcn(uint16 order, DISPATCH_ENTRY *innerTable)
{
    uint8 n = cpu_get_f(order) - 8;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "XC%d ", n);
    cpu_push_value(cpu_get_operand(order));
    cpu_set_co(0x20010000 | n);
    cpu_set_register_16(reg_ms, cpu_get_ms() | MS_MASK_EXEC);
}

static void cpu_execute_organisational_stacklink(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STACKLINK ");
    uint32 co = cpu_get_register_32(reg_co); /* Get CO before it is advanced by getting operand as we jump relative to instruction */
    t_uint64 operand = cpu_get_operand(order);
    t_uint64 link = ((t_uint64)cpu_get_register_16(reg_ms) << 48) | ((t_uint64)cpu_get_register_16(reg_nb) << 32) | cpu_co_add(co, operand);
    cpu_push_value(link);
}

static void cpu_execute_organisational_MS_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "MS= ");
    uint16 ms = cpu_get_register_16(reg_ms);
    t_uint64 operand = cpu_get_operand(order);
    uint16 setMs = ((operand >> 16) & (MASK_8 << 8)) | ((operand >> 8) & MASK_8);
    uint16 mask = ((operand >> 8) & (MASK_8 << 8)) | (operand & MASK_8);
    uint16 newMs = (ms & ~mask) | (setMs & mask);
    cpu_set_ms(newMs);
}

static void cpu_execute_organisational_DL_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "DL= ");
    t_uint64 operand = cpu_get_operand(order);
    cpu_set_register_32(reg_dl, operand & MASK_32);
}

static void cpu_execute_organisational_spm(uint16 order, DISPATCH_ENTRY *innerTable)
{
    /* RNI when asked if there was any information regarding this order said:
    
    Only in my head I imagine. All it did was cause a signal to be sent to the System Performance Monitor (SPM). The SPM was a separate physical entity.
    You can see it in the right foreground of the picture in http://www.cs.manchester.ac.uk/about-us/history/mu5/

    The SPM could be set up so as to count these signals. I can't remember whether it sent an operand or just a binary signal. It was a way of monitoring
    and analysing programs. I think you should just treat the SPM order as a dummy.
    */

    /* so this is a dummy implementation that just consumes the operand */
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SPM ");
    cpu_get_operand(order);
}

static void cpu_execute_organisational_setlink(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SETLINK ");
    t_uint64 link = cpu_get_link();
    cpu_set_operand(order, link);
}

static void cpu_execute_organisational_load_XNB(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "XNB= ");
    t_uint64 newBase = cpu_get_operand(order);
    cpu_set_xnb(newBase & MASK_32);
}

static void cpu_execute_organisational_load_SN(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SN= ");
    t_uint64 newSn = cpu_get_operand(order);
    if (cpu_is_executive_mode())
    {
        cpu_set_register_16(reg_sn, (newSn >> 16) & MASK_16);
    }
}

static void cpu_execute_organisational_XNB_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "XNB+ ");
    int16 operand = cpu_get_operand(order) & MASK_16;
    uint32 xnb = cpu_calculate_base_offset_from_reg_32(reg_xnb, operand, SCALE_32);
    cpu_set_xnb(xnb);
}

static void cpu_execute_organisational_XNB_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "XNB=> ");
	if (!cpu_operand_is_secondary(order))
	{
		t_uint64 xnb = cpu_get_register_32(reg_xnb);
		cpu_set_operand(order, xnb);
	}
	else
	{
		cpu_set_illegal_function_interrupt();
	}
}

static void cpu_execute_organisational_SF_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SF= ");
    t_uint64 newSf = cpu_get_operand(order);
    cpu_set_sf(newSf & MASK_16);
}

static void cpu_execute_organisational_SF_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SF+ ");
    int16 operand = cpu_get_operand(order) & MASK_16;
    uint16 sf = cpu_calculate_base_offset_from_reg_16(reg_sf, operand, SCALE_32);
    cpu_set_sf(sf);
}

static void cpu_execute_organisational_SF_load_NB_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SF=NB+ ");
    uint16 newSF = cpu_calculate_base_offset_from_reg_16(reg_nb, cpu_get_operand(order), SCALE_32);
    cpu_set_sf(newSF);
}

static void cpu_execute_organisational_SF_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SF=> ");
	if (!cpu_operand_is_secondary(order))
	{
		t_uint64 sf = cpu_get_register_16(reg_sf);
        cpu_set_operand(order, sf);
	}
	else
	{
		cpu_set_illegal_function_interrupt();
	}
}

static void cpu_execute_organisational_NB_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB= ");
    t_uint64 newBase = cpu_get_operand(order);
    cpu_set_nb(newBase & MASK_16);
}

static void cpu_execute_organisational_NB_load_SF_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB=SF+ ");
    uint16 newNB = cpu_calculate_base_offset_from_reg_16(reg_sf, cpu_get_operand(order), SCALE_32);
    cpu_set_nb(newNB);
}

static void cpu_execute_organisational_NB_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB+ ");
    int16 operand = cpu_get_operand(order) & MASK_16;
    uint16 nb = cpu_calculate_base_offset_from_reg_16(reg_nb, operand, SCALE_32);
    cpu_set_nb(nb);
}

static void cpu_execute_organisational_NB_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB=> ");
	if (!cpu_operand_is_secondary(order))
	{
		t_uint64 nb = ((t_uint64)cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_nb);
		cpu_set_operand(order, nb);
	}
	else
	{
		cpu_set_illegal_function_interrupt();
	}
}

static void cpu_execute_organisational_branch_eq(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_ne(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_ge(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_lt(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_le(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_gt(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_ovf(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_branch_bn(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "->");
    cpu_jump_relative(order, cpu_calculate_condition(cpu_get_f(order)));
}

static void cpu_execute_organisational_bn_by_operand(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "");
    uint8 f = cpu_get_f(order);
    uint8 code = cpu_get_operand(order) & 0xF;
    cpu_process_condition(cpu_calculate_condition(f), code);
}

static void cpu_execute_organisational_bn_by_order(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "");
    uint8 f = cpu_get_f(order);
    uint8 operand = cpu_get_operand(order) & 0x1;
    cpu_process_condition(operand, f & 0xF);
}

static uint8 cpu_get_descriptor_type(t_uint64 descriptor)
{
    uint8 result = descriptor >> 62;
    return result;
}

static uint8 cpu_get_descriptor_subtype(t_uint64 descriptor)
{
    uint8 result = (descriptor >> 56) & 0x3F;
    return result;
}

static uint8 cpu_get_descriptor_size(t_uint64 descriptor)
{
    uint8 result = (descriptor >> 59) & 0x7;
    return result;
}

static uint8 cpu_get_descriptor_unscaled(t_uint64 descriptor)
{
    uint8 result = (descriptor >> 57) & 0x1;
    return result;
}

static uint8 cpu_get_descriptor_bound_check_inhibit(t_uint64 descriptor)
{
    uint8 result = (descriptor >> 56) & 0x1;
    return result;
}

static uint32 cpu_get_descriptor_bound(t_uint64 descriptor)
{
    uint32 result = (descriptor >> 32) & 0x00FFFFFF;
    return result;
}

static uint32 cpu_get_descriptor_origin(t_uint64 descriptor)
{
    uint32 result = descriptor & MASK_32;
    return result;
}

static void cpu_set_descriptor_bound(t_uint64 *descriptor, uint32 bound)
{
    *descriptor = (*descriptor & 0xFF000000FFFFFFFF) | ((t_uint64)(bound & 0x00FFFFFF) << 32);
}

static void cpu_set_descriptor_origin(t_uint64 *descriptor, uint32 origin)
{
    *descriptor = (*descriptor & 0xFFFFFFFF00000000) | origin;
}

static void cpu_descriptor_modify(REG *descriptorReg, int32 modifier, int originOnly)
{
    t_uint64 d = cpu_get_register_64(descriptorReg);
    uint32 bound = cpu_get_descriptor_bound(d);
    uint32 origin = cpu_get_descriptor_origin(d);
    int32 scaledModifier = modifier;
    scaledModifier = cpu_scale_descriptor_modifier(d, modifier);
    /* TODO: indirect descriptor processing not implemented, p49 */
    /* TODO: procedure call processing not implemented, p49 */
    if (!originOnly)
    {
        cpu_set_descriptor_bound(&d, bound - modifier);
    }

    cpu_set_descriptor_origin(&d, origin + scaledModifier);
    cpu_set_register_64(descriptorReg, d);
}

static int cpu_check_bound(t_uint64 descriptor, int32 modifier)
{
	int result = 1;
	uint8 type = cpu_get_descriptor_type(descriptor);
	uint8 subtype = cpu_get_descriptor_subtype(descriptor);
	uint32 bound = cpu_get_descriptor_bound(descriptor);
	if (!cpu_get_descriptor_bound_check_inhibit(descriptor) && (type == DESCRIPTOR_TYPE_GENERAL_VECTOR || type == DESCRIPTOR_TYPE_GENERAL_STRING || type == DESCRIPTOR_TYPE_ADDRESS_VECTOR || (type == DESCRIPTOR_TYPE_MISCELLANEOUS && subtype == 0) || (type == DESCRIPTOR_TYPE_MISCELLANEOUS && subtype == 1) || (type == DESCRIPTOR_TYPE_MISCELLANEOUS && subtype == 2)))
	{
		if (modifier < 0 || (uint32)modifier >= bound)
		{
			cpu_set_bounds_check_interrupt();
			result = 0;
		}
	}

	return result;
}

static void cpu_execute_descriptor_modify(uint16 order, REG *descriptorReg)
{
    t_uint64 d = cpu_get_register_64(descriptorReg);
    int32 modifier = cpu_get_operand(order) & MASK_32;
	if (cpu_check_bound(d, modifier))
	{
		cpu_descriptor_modify(descriptorReg, modifier, FALSE);
	}
}

static void cpu_parse_sts_string_to_string_operand(t_uint64 operand, uint8 *mask, uint8 *filler)
{
    if (mask != NULL)
    {
        *mask = (operand >> 8) & 0xFF;
    }

    if (filler != NULL)
    {
        *filler = operand & 0xFF;
    }
}

static void cpu_parse_sts_string_to_string_operand_from_order(uint16 order, uint8 *mask, uint8 *filler)
{
    t_uint64 operand = cpu_get_operand(order);

    cpu_parse_sts_string_to_string_operand(operand, mask, filler);
}

static int cpu_check_string_descriptor(t_uint64 descriptor)
{
    int result = 1;
    uint8 type = cpu_get_descriptor_type(descriptor);
    uint8 size = cpu_get_descriptor_size(descriptor);
    if (type > DESCRIPTOR_TYPE_ADDRESS_VECTOR || size != DESCRIPTOR_SIZE_8_BIT)
    {
        cpu_set_its_interrupt();
        result = 0;
    }

    return result;
}

static int cpu_check_32bit_descriptor(t_uint64 descriptor)
{
    int result;
    uint8 dt = cpu_get_descriptor_type(descriptor);
    uint8 ds = cpu_get_descriptor_size(descriptor);
    result = (dt == DESCRIPTOR_TYPE_GENERAL_VECTOR || dt == DESCRIPTOR_TYPE_ADDRESS_VECTOR) && ds == DESCRIPTOR_SIZE_32_BIT;
    if (!result)
    {
        cpu_set_its_interrupt();
    }

    return result;
}

/* operand is the mask to apply to the source to get the destination */
static t_uint64 cpu_sts1_smve_element_operation(t_uint64 source, t_uint64 destination, t_uint64 operand)
{
    return source & operand;
}

static t_uint64 cpu_sts1_slgc_element_operation(t_uint64 source, t_uint64 destination, t_uint64 operand)
{
    t_uint64 mask;
    t_uint64 result = 0;
    t_uint64 lnValue;

    /* L0 bits */
    mask = ~source & ~destination;
    lnValue = (operand & 0x80000) ? 0xFFFFFFFFFFFFFFFF : 0;
    result |= lnValue & mask;

    /* L1 bits */
    mask = ~source & destination;
    lnValue = (operand & 0x40000) ? 0xFFFFFFFFFFFFFFFF : 0;
    result |= lnValue & mask;

    /* L2 bits */
    mask = source & ~destination;
    lnValue = (operand & 0x20000) ? 0xFFFFFFFFFFFFFFFF : 0;
    result |= lnValue & mask;

    /* L3 bits */
    mask = source & destination;
    lnValue = (operand & 0x10000) ? 0xFFFFFFFFFFFFFFFF : 0;
    result |= lnValue & mask;

    return result;
}

static void cpu_execute_sts1_xdo_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 xd;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XDO= ");
    xd = cpu_get_register_64(reg_xd) & 0xFFFFFFFF00000000;
    xd = xd | (cpu_get_operand(order) & MASK_32);
    cpu_set_register_64(reg_xd, xd);
}

static void cpu_execute_sts1_xd_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XD= ");
    cpu_set_register_64(reg_xd, cpu_get_operand(order));
}

static void cpu_execute_sts1_stack(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS STACK ");
    cpu_push_value(cpu_get_operand(order));
}

static void cpu_execute_sts1_xd_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XD=> ");
    if (!cpu_operand_is_secondary(order))
    {
        cpu_set_operand(order, cpu_get_register_64(reg_xd));
    }
    else
    {
		cpu_set_illegal_function_interrupt();
    }
}

static void cpu_execute_sts1_xdb_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 xd;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XDB= ");
    xd = cpu_get_register_64(reg_xd) & 0xFF000000FFFFFFFF;
    xd = xd | ((cpu_get_operand(order) & 0xFFFFFF) << 32);
    cpu_set_register_64(reg_xd, xd);
}

static void cpu_execute_sts1_xchk(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XCHK ");
    uint32 check = cpu_get_operand(order) & MASK_32;
    uint32 xdb = cpu_get_descriptor_bound(cpu_get_register_64(reg_xd));
    int result = (int32)check >= 0 && check < xdb;
    cpu_set_register_bit_32(reg_dod, mask_dod_xch, result);
}

static void cpu_execute_sts1_smod(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SMOD ");
    int32 modifier = cpu_get_operand(order) & MASK_32;
    cpu_descriptor_modify(reg_d, modifier, TRUE);
}

static void cpu_execute_sts1_xmod(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XMOD ");
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint8 type = cpu_get_descriptor_type(xd);
    uint8 subtype = cpu_get_descriptor_subtype(xd);
    if (type == DESCRIPTOR_TYPE_MISCELLANEOUS && (subtype >= DESCRIPTOR_TYPE_MISCELLANEOUS && subtype <= 31))
    {
        cpu_set_its_interrupt();
    }

    cpu_execute_descriptor_modify(order, reg_xd);
}

static void cpu_execute_sts1_slgc(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    t_uint64 operand;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SLGC ");
    operand = cpu_get_operand(order);
    if (cpu_check_string_descriptor(d) && cpu_check_string_descriptor(xd))
    {
        cpu_process_source_to_destination_descriptor_vector(operand, cpu_sts1_slgc_element_operation);
    }
}

static void cpu_execute_sts1_smvb(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SMVB ");
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint8 mask;
    uint8 filler;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 xdb = cpu_get_descriptor_bound(xd);
    uint32 dorig = cpu_get_descriptor_origin(d);
    uint32 xdorig = cpu_get_descriptor_origin(xd);
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &filler);

    if (cpu_check_string_descriptor(d) && cpu_check_string_descriptor(xd))
    {
        if (db == 0)
        {
            cpu_set_bounds_check_interrupt();
        }
        else if (xdb == 0)
        {
            cpu_set_operand_by_descriptor_vector(d, 0, filler & ~mask);
            cpu_descriptor_modify(reg_d, 1, FALSE);
        }
        else
        {
            cpu_set_operand_by_descriptor_vector(d, 0, cpu_get_operand_by_descriptor_vector(xd, 0) & ~mask);
            cpu_descriptor_modify(reg_d, 1, FALSE);
            cpu_descriptor_modify(reg_xd, 1, FALSE);
        }
    }
}

static void cpu_execute_sts1_smve(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SMVE ");
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint8 mask;
    uint8 filler;
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &filler);

    if (cpu_check_string_descriptor(d) && cpu_check_string_descriptor(xd))
    {
        cpu_process_source_to_destination_descriptor_vector(~mask, cpu_sts1_smve_element_operation);
    }
}

static void cpu_execute_sts1_smvf(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SMVF ");
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint8 mask;
    uint8 filler;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 xdb = cpu_get_descriptor_bound(xd);
    uint32 dorig = cpu_get_descriptor_origin(d);
    uint32 xdorig = cpu_get_descriptor_origin(xd);
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &filler);

    if (cpu_check_string_descriptor(d) && cpu_check_string_descriptor(xd))
    {
        if (db != 0)
        {
            unsigned int i;
            unsigned int n = (xdb < db) ? db : xdb;
            for (i = 0; i < n; i++)
            {
                t_uint64 value = (i < xdb) ? cpu_get_operand_by_descriptor_vector(xd, i) : filler;
                cpu_set_operand_by_descriptor_vector(d, i, value & ~mask);
            }
            cpu_descriptor_modify(reg_d, n, FALSE);
            cpu_descriptor_modify(reg_xd, (xdb < db) ? xdb : db, FALSE);
        }
    }
}

static void cpu_execute_sts1_talu(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS TALU ");
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 dorig = cpu_get_descriptor_origin(d);
    unsigned int i;
    int found = FALSE;
    uint32 mask = cpu_get_register_64(reg_xd) >> 32;
    uint32 comparand = ~mask & (cpu_get_operand(order) & MASK_32);

    if (cpu_check_32bit_descriptor(d))
    {
        for (i = 0; i < db && !found; i++)
        {
            uint32 source = ~mask & (cpu_get_operand_by_descriptor_vector(d, i) & MASK_32);
            if (source == comparand)
            {
                found = TRUE;
            }
            else
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
            }
        }

        cpu_test_value(found ? 0 : 1);
    }
}

static void cpu_execute_sts1_scmp(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SCMP ");
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint8 mask;
    uint8 filler;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 xdb = cpu_get_descriptor_bound(xd);
    uint32 dorig = cpu_get_descriptor_origin(d);
    uint32 xdorig = cpu_get_descriptor_origin(xd);
    unsigned int i;
    int compareResult = 0;
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &filler);

    if (cpu_check_string_descriptor(d) && cpu_check_string_descriptor(xd))
    {
        for (i = 0; i < db && compareResult == 0; i++)
        {
            uint8 sourceByte = ((i < xdb) ? cpu_get_operand_by_descriptor_vector(xd, i) & 0xFF : filler) & ~mask;
            uint8 destByte = (uint8)cpu_get_operand_by_descriptor_vector(d, i) & ~mask;
            if (destByte == sourceByte)
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
                if (i < xdb)
                {
                    cpu_descriptor_modify(reg_xd, 1, FALSE);
                }
            }
            else if (sourceByte > destByte)
            {
                compareResult = 1;
            }
            else
            {
                compareResult = -1;
            }
        }

        cpu_test_value(compareResult);
    }
}

static void cpu_execute_sts1_sub1(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SUB1 ");
    t_uint64 xd = cpu_get_operand(order);
    t_uint64 d = 0;
    uint32 b = cpu_get_register_32(reg_b);

    if (cpu_check_32bit_descriptor(xd))
    {
        cpu_set_register_64(reg_xd, xd);
        b -= cpu_get_operand_by_descriptor_vector(xd, 0) & MASK_32;
        b *= cpu_get_operand_by_descriptor_vector(xd, 1) & MASK_32;
        cpu_set_register_32(reg_b, b);
        cpu_set_descriptor_bound(&d, cpu_get_operand_by_descriptor_vector(xd, 2) & MASK_32);
        cpu_set_register_64(reg_d, d);
        cpu_descriptor_modify(reg_d, b, FALSE);
        cpu_descriptor_modify(reg_xd, 3, FALSE);
    }
}

static void cpu_execute_sts2_do_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 d;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS DO= ");
    d = cpu_get_register_64(reg_d) & 0xFFFFFFFF00000000;
    d = d | (cpu_get_operand(order) & MASK_32);
    cpu_set_register_64(reg_d, d);
}

static void cpu_execute_sts2_d_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS D= ");
    cpu_set_register_64(reg_d, cpu_get_operand(order));
}

static void cpu_execute_sts2_d_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS D*= ");
    cpu_push_value(cpu_get_register_64(reg_d));
    cpu_set_register_64(reg_d, cpu_get_operand(order));
}

static void cpu_execute_sts2_d_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS D=> ");
    if (!cpu_operand_is_secondary(order))
    {
        cpu_set_operand(order, cpu_get_register_64(reg_d));
    }
    else
    {
		cpu_set_illegal_function_interrupt();
	}
}

static void cpu_execute_sts2_db_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 d;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS DB= ");
    d = cpu_get_register_64(reg_d) & 0xFF000000FFFFFFFF;
    d = d | ((cpu_get_operand(order) & 0xFFFFFF) << 32);
    cpu_set_register_64(reg_d, d);
}

static void cpu_execute_sts2_mdr(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 d;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS MDR ");
    cpu_execute_descriptor_modify(order, reg_d);

    /* RNI - I don't remember too much about MDR, just that it did what it
       says in the manual. So I don't remember if there were any checks
       (though I think not) but clearly it would have to have a Size field
       of 64. Sometimes the engineers just left the programmers to their own
       devices and it was their tough luck if they got it wrong. But it would
       have just been the OS or compiler writers who needed to get things
       right, not the users.
    */
    d = cpu_get_register_64(reg_d);
    cpu_set_register_64(reg_d, cpu_get_operand_by_descriptor_vector(d, 0));
}

static void cpu_execute_sts2_mod(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS MOD ");
    cpu_execute_descriptor_modify(order, reg_d);
}

static void cpu_execute_sts2_rmod(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 operand;
    t_uint64 d;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS RMOD ");
    operand = cpu_get_operand(order);
    d = cpu_get_register_64(reg_d);
    d = (operand & 0xFFFFFFFF00000000) | ((d & MASK_32) + (operand & MASK_32));
    cpu_set_register_64(reg_d, d);
}

static void cpu_execute_sts2_blgc(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 d = cpu_get_register_64(reg_d);
    t_uint64 operand;
    uint8 byte;
    int i;
    int n = cpu_get_descriptor_bound(d);
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS BLGC ");
    operand = cpu_get_operand(order);
    cpu_parse_sts_string_to_string_operand(operand, NULL, &byte);
    if (cpu_check_string_descriptor(d))
    {
        for (i = 0; i < n; i++)
        {
            t_uint64 destination = cpu_get_operand_by_descriptor_vector(d, i);
            cpu_set_operand_by_descriptor_vector(d, i, cpu_sts1_slgc_element_operation((t_uint64)byte, destination, operand));
        }

        cpu_descriptor_modify(reg_d, n, FALSE);
    }
}

static void cpu_execute_sts2_bmvb(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS BMVB ");
    t_uint64 d = cpu_get_register_64(reg_d);
    uint8 mask;
    uint8 byte;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 dorig = cpu_get_descriptor_origin(d);
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &byte);

    if (cpu_check_string_descriptor(d))
    {
        if (db != 0)
        {
            cpu_set_operand_by_descriptor_vector(d, 0, byte & ~mask);
            cpu_descriptor_modify(reg_d, 1, FALSE);
        }
        else
        {
            cpu_set_bounds_check_interrupt();
        }
    }
}

static void cpu_execute_sts2_bmve(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS BMVE ");
    t_uint64 d = cpu_get_register_64(reg_d);
    uint8 mask;
    uint8 byte;
    uint32 db = cpu_get_descriptor_bound(d);
    unsigned int i;
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &byte);

    if (cpu_check_string_descriptor(d))
    {
        if (db != 0)
        {
            for (i = 0; i < db; i++)
            {
                cpu_set_operand_by_descriptor_vector(d, i, byte & ~mask);
            }
            cpu_descriptor_modify(reg_d, db, FALSE);
        }
    }
}

static void cpu_execute_sts2_bscn(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS BSCN ");
    t_uint64 d = cpu_get_register_64(reg_d);
    uint8 mask;
    uint8 byte;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 dorig = cpu_get_descriptor_origin(d);
    unsigned int i;
    int found = 0;
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &byte);

    if (cpu_check_string_descriptor(d))
    {
        for (i = 0; i < db && !found; i++)
        {
            found = (byte & ~mask) == ((uint8)cpu_get_operand_by_descriptor_vector(d, i) & ~mask);
            if (!found)
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
            }
        }

        cpu_test_value(found ? 0 : -1);
    }
}

static void cpu_execute_sts2_bcmp(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS BCMP ");
    t_uint64 d = cpu_get_register_64(reg_d);
    uint8 mask;
    uint8 byte;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 dorig = cpu_get_descriptor_origin(d);
    unsigned int i;
    int compareResult = 0;
    cpu_parse_sts_string_to_string_operand_from_order(order, &mask, &byte);
    byte = byte & ~mask;

    if (cpu_check_string_descriptor(d))
    {
        for (i = 0; i < db && compareResult == 0; i++)
        {
            uint8 destinationByte = (uint8)cpu_get_operand_by_descriptor_vector(d, i) & ~mask;
            if (byte == destinationByte)
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
            }
            else if (byte > destinationByte)
            {
                compareResult = 1;
            }
            else
            {
                compareResult = -1;
            }
        }

        cpu_test_value(compareResult);
    }
}

static void cpu_execute_sts2_sub2(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SUB2\n");
    t_uint64 xd = cpu_get_register_64(reg_xd);
    t_uint64 d = cpu_get_register_64(reg_d);
    uint32 b = cpu_get_register_32(reg_b);

    if (cpu_check_32bit_descriptor(xd))
    {
        b -= cpu_get_operand_by_descriptor_vector(xd, 0) & MASK_32;
        b *= cpu_get_operand_by_descriptor_vector(xd, 1) & MASK_32;
        cpu_set_register_32(reg_b, b);
        cpu_set_descriptor_bound(&d, cpu_get_operand_by_descriptor_vector(xd, 2) & MASK_32);
        cpu_set_register_64(reg_d, d);
        cpu_descriptor_modify(reg_d, b, FALSE);
        cpu_descriptor_modify(reg_xd, 3, FALSE);
    }
}

static int cpu_test_b_overflow(t_uint64 value)
{
    int result = 0;
    uint32 ms = value >> 32;
    uint8 sign = value >> 31 & 1;
    if (!(sign == 0 && ms == 0) && !(sign == 1 && ms == ~0))
    {
        result = 1;
    }

    return result;
}

static void cpu_check_b_overflow(t_uint64 value)
{
    if (cpu_test_b_overflow(value))
    {
        cpu_set_register_bit_32(reg_bod, mask_bod_bovf, 1);

        if (!cpu_get_register_bit_32(reg_bod, mask_bod_ibovf))
        {
            cpu_set_B_interrupt();
        }
    }
}

static void cpu_test_b_value(t_int64 value)
{
    int overflow;
    cpu_test_value(value);
    overflow = cpu_test_b_overflow(value);
    cpu_set_register_bit_16(reg_ms, mask_ms_t0, overflow);
}

static void cpu_execute_b_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B= ");
    cpu_set_register_32(reg_b, cpu_get_operand(order) & MASK_32);
}

static void cpu_execute_b_load_and_decrement(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B=' ");
    t_int64 b = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    b--;
    cpu_check_b_overflow(b);
    cpu_set_register_32(reg_b, b & MASK_32);
}

static void cpu_execute_b_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B*= ");
    cpu_push_value(cpu_get_register_32(reg_b));
    cpu_set_register_32(reg_b, cpu_get_operand(order) & MASK_32);
}

static void cpu_execute_b_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B=> ");
    cpu_set_operand(order, cpu_get_register_32(reg_b));
}

static void cpu_execute_b_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B+ ");
    t_int64 augend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 addend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = augend + addend;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    cpu_check_b_overflow(result);
}

static void cpu_execute_b_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B- ");
    t_int64 minuend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 subtrahend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    cpu_check_b_overflow(result);
}

static void cpu_execute_b_mul(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B* ");
    t_int64 multiplicand = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 multiplier = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = multiplicand * multiplier;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    cpu_check_b_overflow(result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_b_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B DIV ");
    t_int64 dividend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 divisor = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    if (divisor == 0)
    {
        /* Since this order is not in the programming manual, there are no BOD status bits that can be used to report divide by zero, so just set a B interrupt. It is likely
           to confuse any interrupt handler though, but real software would never execute this instruction in any case.
        */
        cpu_set_B_interrupt();
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    }
}

static void cpu_execute_b_xor(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B XOR ");
    t_uint64 xorend = cpu_get_register_32(reg_b);
    t_uint64 xorand = cpu_get_operand(order) & MASK_32;
    t_uint64 result = xorend ^ xorand;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
}

static void cpu_execute_b_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B OR ");
    t_uint64 orend = cpu_get_register_32(reg_b);
    t_uint64 orand = cpu_get_operand(order) & MASK_32;
    t_uint64 result = orend | orand;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
}

static void cpu_execute_b_shift_left(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B <- ");
    t_int64 value = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b)); /* signed, for arithmetic shift */
    t_int64 shift = cpu_sign_extend_6_bit(cpu_get_operand(order));
    t_int64 result = (shift >= 0) ? value << shift : value >> -shift;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    cpu_check_b_overflow(result);
}

static void cpu_execute_b_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B AND ");
    t_uint64 andend = cpu_get_register_32(reg_b);
    t_uint64 andand = cpu_get_operand(order) & MASK_32;
    t_uint64 result = andend & andand;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
}

static void cpu_execute_b_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B RSUB ");
    t_int64 subtrahend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 minuend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    cpu_check_b_overflow(result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_b_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B RDIV ");
    t_int64 divisor = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 dividend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    if (divisor == 0)
    {
        /* Since this order is not in the programming manual, there are no BOD status bits that can be used to report divide by zero, so just set a B interrupt. It is likely
           to confuse any interrupt handler though, but real software would never execute this instruction in any case.
        */
        cpu_set_B_interrupt();
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_b, (uint32)(result & MASK_32));
    }
}

/* The programming manual says "no overflow may occur", the interpretation from RNI of this statement is "COMP won't generate an overflow interrupt if overflow occurs because
   B isn't altered so B hasn't overflowed. If the result overflows, Test bit 0 is set." In a follow up with RNI he said: "I think the manual is misleading, I don't think
   the bit got set in BOD at all, just sent to Test 0" */
static void cpu_execute_b_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B COMP ");
    t_uint64 b = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 comparand = cpu_sign_extend_32_bit(cpu_get_operand(order));
    cpu_test_b_value(b - comparand);
}

static void cpu_execute_b_compare_and_increment(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B CINC ");
    t_uint64 b = cpu_sign_extend_32_bit(cpu_get_register_32(reg_b));
    t_int64 comparand = cpu_sign_extend_32_bit(cpu_get_operand(order));
    cpu_test_b_value(b - comparand);
    b++;
    cpu_check_b_overflow(b);
    cpu_set_register_32(reg_b, b & MASK_32);
}

static void cpu_set_divide_by_zero(void)
{
    cpu_set_register_bit_64(reg_aod, mask_aod_zdiv, 1);
    if (!cpu_get_register_bit_64(reg_aod, mask_aod_izdiv))
    {
        cpu_set_A_interrupt();
    }
}

static void cpu_check_x_overflow(t_uint64 result)
{
    uint32 ms = result >> 32;
    uint8 sign = result >> 31 & 1;
    if (!(sign == 0 && ms == 0) && !(sign == 1 && ms == ~0))
    {
        cpu_set_register_bit_64(reg_aod, mask_aod_fxpovf, 1);
        if (!cpu_get_register_bit_64(reg_aod, mask_aod_ifxpovf))
        {
            cpu_set_A_interrupt();
        }
    }
    else
    {
        cpu_set_register_bit_64(reg_aod, mask_aod_fxpovf, 0);
    }
}

static void cpu_execute_fp_signed_load_single(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X=(32) ");
    cpu_set_register_32(reg_x, cpu_get_operand(order) & MASK_32);
}

static void cpu_execute_fp_signed_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X*= ");
    cpu_push_value(cpu_get_register_32(reg_x));
    cpu_set_register_32(reg_x, cpu_get_operand(order) & MASK_32);
}

static void cpu_execute_fp_signed_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X=> ");
    cpu_set_operand(order, cpu_get_register_32(reg_x));
}

static void cpu_execute_fp_signed_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X+ ");
    t_int64 augend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x) & MASK_32);
    t_int64 addend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = augend + addend;
    cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X- ");
    t_int64 minuend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x));
    t_int64 subtrahend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_mul(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X* ");
    t_int64 multiplicand = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x));
    t_int64 multiplier = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = multiplicand * multiplier;
    cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X/ ");
    t_int64 dividend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x));
    t_int64 divisor = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    if (divisor == 0)
    {
        cpu_set_divide_by_zero();
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    }
}

static void cpu_execute_fp_signed_xor(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X XOR ");
    uint32 xorend = cpu_get_register_32(reg_x);
    uint32 xorand = cpu_get_operand(order) & MASK_32;
    uint32 result = xorend ^ xorand;
    cpu_set_register_32(reg_x, result);
}

static void cpu_execute_fp_signed_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X OR ");
    uint32 orend = cpu_get_register_32(reg_x);
    uint32 orand = cpu_get_operand(order) & MASK_32;
    uint32 result = orend | orand;
    cpu_set_register_32(reg_x, result);
}

static void cpu_execute_fp_signed_shift_left(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X <- ");
    t_int64 value = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x)); /* signed for arithmetic shift */
    t_int64 shift = cpu_sign_extend_6_bit(cpu_get_operand(order));
    t_int64 result = (shift >= 0) ? value << shift : value >> -shift;
    cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X AND ");
    uint32 andend = cpu_get_register_32(reg_x);
    uint32 andand = cpu_get_operand(order) & MASK_32;
    uint32 result = andend & andand;
    cpu_set_register_32(reg_x, result);
}

static void cpu_execute_fp_signed_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X RSUB ");
    t_int64 subtrahend = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x));
    t_int64 minuend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    int t0;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X COMP ");
    t_uint64 x = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x));
    t_int64 comparand = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    t_int64 result = x - comparand;
    cpu_test_value(result);
    cpu_check_x_overflow(result);
    cpu_clear_interrupt(INT_PROGRAM_FAULTS); /* TODO: supposed to ignore A overflow interrupts, just copy the overflow bit, is there a better way to do this now? */
    t0 = cpu_get_register_bit_64(reg_aod, mask_aod_fxpovf) || cpu_get_register_bit_64(reg_aod, mask_aod_zdiv);
    cpu_set_register_bit_16(reg_ms, mask_ms_t0, t0);
}

static void cpu_execute_fp_signed_convert(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X CONV ");
    /* TODO: Implement convert to floating point */
    cpu_set_illegal_order_interrupt(0);
}

static void cpu_execute_fp_signed_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X RDIV ");
    t_int64 divisor = cpu_sign_extend_32_bit(cpu_get_register_32(reg_x));
    t_int64 dividend = cpu_sign_extend_32_bit(cpu_get_operand(order) & MASK_32);
    if (divisor == 0)
    {
        cpu_set_divide_by_zero();
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_x, (uint32)(result & MASK_32));
    }
}

static void cpu_execute_fp_unsigned_load_single(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AOD=(32) ");
    cpu_set_register_64(reg_aod, cpu_get_operand(order) & mask_aod);
}

static void cpu_execute_fp_unsigned_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AOD*= ");
    cpu_push_value(cpu_get_register_64(reg_aod) & mask_aod);
    cpu_set_register_64(reg_aod, cpu_get_operand(order) & mask_aod);
}

static void cpu_execute_fp_unsigned_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AOD=> ");
    cpu_set_operand(order, cpu_get_register_64(reg_aod) & mask_aod);
}

static void cpu_execute_fp_unsigned_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A+ ");
    t_int64 augend = cpu_get_register_64(reg_a) & MASK_32;
    t_int64 addend = cpu_get_operand(order) & MASK_32;
    t_int64 result = augend + addend;
    cpu_set_register_64(reg_a, (t_uint64)result);
}

static void cpu_execute_fp_unsigned_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A- ");
    t_int64 minuend = cpu_get_register_64(reg_a) & MASK_32;
    t_int64 subtrahend = cpu_get_operand(order) & MASK_32;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_64(reg_a, (t_uint64)result);
}

static void cpu_execute_fp_unsigned_mul(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A* ");
    t_uint64 multiplicand = cpu_get_register_64(reg_a) & MASK_32;
    t_uint64 multiplier = cpu_get_operand(order) & MASK_32;
    t_uint64 result = multiplicand * multiplier;
    cpu_set_register_64(reg_a, result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_fp_unsigned_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A/ ");
    uint32 dividend = cpu_get_register_64(reg_a) & MASK_32;
    uint32 divisor = cpu_get_operand(order) & MASK_32;
    if (divisor == 0)
    {
        cpu_set_divide_by_zero();
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_64(reg_a, (t_uint64)result);
    }
}

static void cpu_execute_fp_unsigned_xor(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A XOR ");
    t_uint64 xorend = cpu_get_register_64(reg_a);
    t_uint64 xorand = cpu_get_operand(order);
    t_uint64 result = (xorend ^ xorand) & MASK_32;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A OR ");
    t_uint64 orend = cpu_get_register_64(reg_a);
    t_uint64 orand = cpu_get_operand(order);
    t_uint64 result = (orend | orand) & MASK_32;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_shift_left(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A <- ");
    t_uint64 value = cpu_get_register_64(reg_a); /* unsigned for logical shift */
    t_int64 shift = cpu_sign_extend(cpu_get_operand(order), 7);
    t_int64 result = (shift >= 0) ? value << shift : value >> -shift;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A AND ");
    t_uint64 andend = cpu_get_register_64(reg_a);
    t_uint64 andand = cpu_get_operand(order) & MASK_32;
    t_uint64 result = (andend & andand) & MASK_32;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A RSUB ");
    t_int64 subtrahend = cpu_get_register_64(reg_a) & MASK_32;
    t_int64 minuend = cpu_get_operand(order) & MASK_32;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_64(reg_a, (t_uint64)result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_fp_unsigned_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A RDIV ");
    uint32 divisor = cpu_get_register_64(reg_a) & MASK_32;
    uint32 dividend = cpu_get_operand(order) & MASK_32;
    if (divisor == 0)
    {
        cpu_set_divide_by_zero();
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_64(reg_a, (t_uint64)result);
    }
}

static void cpu_execute_fp_unsigned_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A COMP ");
    t_uint64 a = cpu_get_register_64(reg_a) & MASK_32;
    t_uint64 comparand = cpu_get_operand(order) & MASK_32;
    cpu_test_value((t_int64)a - (t_int64)comparand);
    cpu_set_register_bit_16(reg_ms, mask_ms_t0, 0);
}

static t_uint64 cpu_get_acc_value()
{
    t_uint64 result = (cpu_get_register_bit_64(reg_aod, mask_aod_opsiz64)) ? cpu_get_register_64(reg_a) : cpu_get_register_64(reg_a) >> 32;
    return result;
}

static void cpu_execute_fp_decimal_load_double(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AEX=(64) ");
    cpu_set_register_64(reg_aex, cpu_get_operand(order));
}

static void cpu_execute_fp_decimal_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AEX*= ");
    cpu_push_value(cpu_get_register_64(reg_aex));
    cpu_set_register_64(reg_aex, cpu_get_operand(order));
}

static void cpu_execute_fp_decimal_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AEX=> ");
    cpu_set_operand(order, cpu_get_register_64(reg_aex));
}

static void cpu_execute_flp_unsigned_xor(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "XOR ");
    t_uint64 xorend = cpu_get_register_64(reg_a);
    t_uint64 xorand = cpu_get_operand(order);
    t_uint64 result = xorend ^ xorand;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_flp_unsigned_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "OR ");
    t_uint64 xorend = cpu_get_register_64(reg_a);
    t_uint64 xorand = cpu_get_operand(order);
    t_uint64 result = xorend | xorand;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_flp_shift_circ(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SHIFT CIRC ");
    t_uint64 a = cpu_get_register_64(reg_a);
    t_int64 shift = cpu_sign_extend(cpu_get_operand(order), 6);
    t_uint64 savedPart;
    if (shift > 0)
    {
        savedPart = a >> (64 - shift);
        a = (a << shift) | savedPart;
    }
    else if (shift < 0)
    {
        savedPart = a << (64 - -shift);
        a = (a >> -shift) | savedPart;
    }

    cpu_set_register_64(reg_a, a);
}

static void cpu_execute_flp_unsigned_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AND ");
    t_uint64 xorend = cpu_get_register_64(reg_a);
    t_uint64 xorand = cpu_get_operand(order);
    t_uint64 result = xorend & xorand;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_decimal_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AODCOMP ");
    t_uint64 aod = cpu_get_register_64(reg_aod) & mask_aod;
    t_uint64 comparand = cpu_get_operand(order) & mask_aod;
    int result = (aod & comparand) != 0;
    cpu_set_register_bit_16(reg_ms, mask_ms_t0, result);
}

static void cpu_execute_flp_load_single(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A=(32) ");
    cpu_set_register_bit_64(reg_aod, mask_aod_opsiz64, 0);
    cpu_set_register_64(reg_a, (cpu_get_operand(order) << 32) & 0xFFFFFFFF00000000);
}

static void cpu_execute_flp_load_double(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A=(64) ");
    cpu_set_register_bit_64(reg_aod, mask_aod_opsiz64, 1);
    cpu_set_register_64(reg_a, cpu_get_operand(order));
}

static void cpu_execute_flp_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 newA;

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A*= ");
    cpu_push_value(cpu_get_acc_value());
    if (cpu_get_register_bit_64(reg_aod, mask_aod_opsiz64))
    {
        newA = cpu_get_operand(order);
    }
    else
    {
        newA = (cpu_get_operand(order) << 32) & 0xFFFFFFFF00000000;
    }

    cpu_set_register_64(reg_a, newA);
}

static void cpu_execute_flp_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A=> ");
    cpu_set_operand(order, cpu_get_acc_value());
}
