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

*/

#include <assert.h>
#include "mu5_defs.h"
#include "mu5_sac.h"

/* Debug flags */
#define LOG_CPU_PERF          (1 << 0)
#define LOG_CPU_DECODE        (1 << 1)

/* This structure is used to allow instruction execution to be table driven. It allows for tables to be nested. At each level the
   execute() function does anything it needs to do before invoking the function on the inner table. The leaf tables have a NULL for
   the inner table.
*/
typedef struct DISPATCH_ENTRY
{
    void (*execute)(uint16 order, struct DISPATCH_ENTRY *innerTable);
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
static int cpu_stopped = 0;

int32 sim_emax;

UNIT cpu_unit =
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

static t_uint64 mask_aod = 0x000000000000EFFF;
static t_uint64 mask_aod_zdiv    = 0xFFFFFFFFFFFFFFFB;
static t_uint64 mask_aod_fxpovf  = 0xFFFFFFFFFFFFFFFE;
static t_uint64 mask_aod_izdiv   = 0xFFFFFFFFFFFFFF7F;
static t_uint64 mask_aod_ifxpovf = 0xFFFFFFFFFFFFF7FE;
static t_uint64 mask_aod_opsiz64 = 0xFFFFFFFFFFFFEFFF;

BITFIELD bod_bits[] = {
    BIT(IBOVF),  /* Inhibit B overflow interrupt */
    BITNCF(4),
    BIT(BOVF),   /* B Overflow */
    BITNCF(26),
    ENDBITS
};

static uint32 mask_bod_bovf = 0xFFFFFFDF;
static uint32 mask_bod_ibovf = 0xFFFFFFFE;

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

static uint16 mask_ms_bn = 0xFEFF;
static uint16 mask_ms_t2 = 0xFDFF;
static uint16 mask_ms_t1 = 0xF9FF;
static uint16 mask_ms_t0 = 0xF7FF;

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

static uint32 mask_dod_xch = 0xFFFFFFFE;
static uint32 mask_dod_sss = 0xFFFFFFF7;
static uint32 mask_dod_bch = 0xFFFFFFDF;
static uint32 mask_dod_sssi = 0xFFFFFFBF;
static uint32 mask_dod_bchi = 0xFFFFFEFF;


/* Register backing values */
static uint32 reg_b_backing_value;     /* B register */
static uint32 reg_bod_backing_value;   /* BOD register */
static t_uint64 reg_a_backing_value;   /* A register */
static t_uint64 reg_aod_backing_value; /* AOD register */
static t_uint64 reg_aex_backing_value; /* AEX register */
static uint32 reg_x_backing_value;     /* X register */
static uint16 reg_ms_backing_value;    /* MS register */
static uint16 reg_nb_backing_value;    /* NB register */
static uint32 reg_xnb_backing_value;   /* XNB register */
static uint16 reg_sn_backing_value;    /* SN register */
static uint16 reg_sf_backing_value;    /* SF register */
static uint32 reg_co_backing_value;    /* CO Register */
static t_uint64 reg_d_backing_value;   /* D Register */
static t_uint64 reg_xd_backing_value;  /* XD Register */
static uint32 reg_dod_backing_value;   /* DOD Register */
static uint32 reg_dt_backing_value;    /* DT Register */
static uint32 reg_xdt_backing_value;   /* XDT Register */

static REG cpu_reg[] =
{
    { HRDATAD(B,    reg_b_backing_value,       32,    "B register") },
    { GRDATADF(BOD, reg_bod_backing_value, 16, 32, 0, "BOD register", bod_bits) },
    { HRDATAD(A,    reg_a_backing_value,       64,    "Accumulator") },
    { GRDATADF(AOD, reg_aod_backing_value, 16, 64, 0, "AOD register", aod_bits) },
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
    { GRDATADF(DOD, reg_dod_backing_value, 16, 32, 0, "DOD register", dod_bits) },
    { HRDATAD(DT,   reg_dt_backing_value,      32,    "DT register") },
    { HRDATAD(XDT,  reg_xdt_backing_value,     32,    "XDT register") },
    { NULL }
};

REG *reg_b    = &cpu_reg[0];
REG *reg_bod  = &cpu_reg[1];
REG *reg_a	  = &cpu_reg[2];
REG *reg_aod  = &cpu_reg[3];
REG *reg_aex  = &cpu_reg[4];
REG *reg_x	  = &cpu_reg[5];
REG *reg_ms	  = &cpu_reg[6];
REG *reg_nb	  = &cpu_reg[7];
REG *reg_xnb  = &cpu_reg[8];
REG *reg_sn	  = &cpu_reg[9];
REG *reg_sf	  = &cpu_reg[10];
REG *reg_co	  = &cpu_reg[11];
REG *reg_d	  = &cpu_reg[12];
REG *reg_xd	  = &cpu_reg[13];
REG *reg_dod  = &cpu_reg[14];
REG *reg_dt	  = &cpu_reg[15];
REG *reg_xdt  = &cpu_reg[16];

static uint8 interrupt;
REG *sim_PC = &cpu_reg[11];

static MTAB cpu_mod[] =
{
    { 0 }
};

/* Debug Flags */
static DEBTAB cpu_debtab[] =
{
    { "PERF",    LOG_CPU_PERF,      "CPU performance" },
    { "EVENT",   SIM_DBG_EVENT,     "event dispatch activities" },
    { "DECODE",  LOG_CPU_DECODE,    "decode instructions" },
    { NULL,         0 }
};

static const char* cpu_description(DEVICE *dptr) {
    return "Central Processing Unit";
}

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
static uint16 cpu_calculate_base_offset(REG *reg, t_int64 offset);
static t_addr cpu_get_name_segment_address(REG *reg, int16 offset);

static void cpu_set_interrupt(uint8 number);
static void cpu_clear_interrupt(uint8 number);
static uint8 cpu_get_interrupt_number(void);
static uint16 cpu_get_cr(uint16 order);
static uint16 cpu_get_f(uint16 order);
static uint16 cpu_get_k(uint16 order);
static uint16 cpu_get_n(uint16 order);
static uint16 cpu_get_extended_k(uint16 order);
static uint16 cpu_get_extended_n(uint16 order);
static t_uint64 cpu_get_operand_6_bit_literal(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_extended_literal(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_extended_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_extended_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_extended_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_internal_register(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_addr cpu_get_operand_address_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_addr cpu_get_operand_address_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_addr cpu_get_operand_byte_address_by_descriptor_vector_access(t_uint64 descriptor, uint32 modifier);
static t_uint64 cpu_get_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier);
static void cpu_set_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier, t_uint64 value);
static void cpu_process_source_to_destination_descriptor_vector(t_uint64 operand, t_uint64 (*func)(t_uint64 source, t_uint64 destination, t_uint64 operand));
static t_uint64 cpu_get_operand_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_from_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, uint32 modifier);
static t_uint64 cpu_get_operand(uint16 order);
static void cpu_set_operand(uint16 order, t_uint64 value);
static t_uint64 cpu_sign_extend_6_bit(t_uint64 value);
static void cpu_push_value(t_uint64 value);
static t_uint64 cpu_pop_value(void);
static void cpu_test_value(t_int64 value);

static void cpu_execute_next_order(void);
static void cpu_execute_illegal_order(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_start_interrupt_processing(void);

/* cr functions */
static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable);

/* organisational order functions */
static void cpu_jump_relative(uint16 order, int performJump);
static void cpu_execute_organisational_relative_jump(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_absolute_jump(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_SF_load_NB_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_load_SF_plus(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_eq(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_ne(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_ge(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_lt(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_le(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_gt(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_ovf(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_branch_bn(uint16 order, DISPATCH_ENTRY *innerTable);

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
static void cpu_execute_descriptor_modify(uint16 order, REG *descriptorReg);

static void cpu_parse_sts_string_to_string_operand(uint16 order, uint8 *mask, uint8 *filler);
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
static void cpu_execute_sts2_bmvb(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_bscn(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_bcmp(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_sts2_sub2(uint16 order, DISPATCH_ENTRY *innerTable);

/* B order functions */
static void cpu_check_b_overflow(t_uint64 result);
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
static t_uint64 cpu_get_acc_value();
static void cpu_execute_flp_load_single(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_load_double(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_store(uint16 order, DISPATCH_ENTRY *innerTable);

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
    { cpu_execute_organisational_relative_jump,   NULL }, /* 0 */
    { cpu_execute_illegal_order,                  NULL }, /* 1 */
    { cpu_execute_illegal_order,                  NULL }, /* 2 */
    { cpu_execute_illegal_order,                  NULL }, /* 3 */
    { cpu_execute_organisational_absolute_jump,   NULL }, /* 4 */
    { cpu_execute_illegal_order,                  NULL }, /* 5 */
    { cpu_execute_illegal_order,                  NULL }, /* 6 */
    { cpu_execute_illegal_order,                  NULL }, /* 7 */
    { cpu_execute_illegal_order,                  NULL }, /* 8 */
    { cpu_execute_illegal_order,                  NULL }, /* 9 */
    { cpu_execute_illegal_order,                  NULL }, /* 10 */
    { cpu_execute_illegal_order,                  NULL }, /* 11*/
    { cpu_execute_illegal_order,                  NULL }, /* 12 */
    { cpu_execute_illegal_order,                  NULL }, /* 13 */
    { cpu_execute_illegal_order,                  NULL }, /* 14 */
    { cpu_execute_illegal_order,                  NULL }, /* 15 */
    { cpu_execute_illegal_order,                  NULL }, /* 16 */
    { cpu_execute_illegal_order,                  NULL }, /* 17 */
    { cpu_execute_illegal_order,                  NULL }, /* 18 */
    { cpu_execute_illegal_order,                  NULL }, /* 19 */
    { cpu_execute_illegal_order,                  NULL }, /* 20 */
    { cpu_execute_illegal_order,                  NULL }, /* 21 */
    { cpu_execute_illegal_order,                  NULL }, /* 22 */
    { cpu_execute_illegal_order,                  NULL }, /* 23 */
    { cpu_execute_illegal_order,                  NULL }, /* 24 */
    { cpu_execute_illegal_order,                  NULL }, /* 25 */
    { cpu_execute_organisational_SF_load_NB_plus, NULL }, /* 26 */
    { cpu_execute_illegal_order,                  NULL }, /* 27 */
    { cpu_execute_organisational_NB_load,         NULL }, /* 28 */
    { cpu_execute_organisational_NB_load_SF_plus, NULL }, /* 29 */
    { cpu_execute_illegal_order,                  NULL }, /* 30 */
    { cpu_execute_illegal_order,                  NULL }, /* 31 */
    { cpu_execute_organisational_branch_eq,       NULL }, /* 32 */
    { cpu_execute_organisational_branch_ne,       NULL }, /* 33 */
    { cpu_execute_organisational_branch_ge,       NULL }, /* 34 */
    { cpu_execute_organisational_branch_lt,       NULL }, /* 35 */
    { cpu_execute_organisational_branch_le,       NULL }, /* 36 */
    { cpu_execute_organisational_branch_gt,       NULL }, /* 37 */
    { cpu_execute_organisational_branch_ovf,      NULL }, /* 38 */
    { cpu_execute_organisational_branch_bn,       NULL }, /* 39 */
    { cpu_execute_organisational_branch_eq,       NULL }, /* 40 */
    { cpu_execute_organisational_branch_ne,       NULL }, /* 41 */
    { cpu_execute_organisational_branch_ge,       NULL }, /* 42 */
    { cpu_execute_organisational_branch_lt,       NULL }, /* 43 */
    { cpu_execute_organisational_branch_le,       NULL }, /* 44 */
    { cpu_execute_organisational_branch_gt,       NULL }, /* 45 */
    { cpu_execute_organisational_branch_ovf,      NULL }, /* 46 */
    { cpu_execute_organisational_branch_bn,       NULL }, /* 47 */
    { cpu_execute_illegal_order,                  NULL }, /* 48 */
    { cpu_execute_illegal_order,                  NULL }, /* 49 */
    { cpu_execute_illegal_order,                  NULL }, /* 50 */
    { cpu_execute_illegal_order,                  NULL }, /* 51 */
    { cpu_execute_illegal_order,                  NULL }, /* 52 */
    { cpu_execute_illegal_order,                  NULL }, /* 53 */
    { cpu_execute_illegal_order,                  NULL }, /* 54 */
    { cpu_execute_illegal_order,                  NULL }, /* 55 */
    { cpu_execute_illegal_order,                  NULL }, /* 56 */
    { cpu_execute_illegal_order,                  NULL }, /* 57 */
    { cpu_execute_illegal_order,                  NULL }, /* 58 */
    { cpu_execute_illegal_order,                  NULL }, /* 59 */
    { cpu_execute_illegal_order,                  NULL }, /* 60 */
    { cpu_execute_illegal_order,                  NULL }, /* 61 */
    { cpu_execute_illegal_order,                  NULL }, /* 62 */
    { cpu_execute_illegal_order,                  NULL }  /* 63 */
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
    { cpu_execute_illegal_order, NULL },   /* 13 */
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
    { cpu_execute_illegal_order,         NULL },   /* 7 */
    { cpu_execute_illegal_order,         NULL },   /* 8 */
    { cpu_execute_sts2_bmvb,             NULL },   /* 9 */
    { cpu_execute_illegal_order,         NULL },   /* 10 */
    { cpu_execute_sts1_smvf,             NULL },   /* 11*/
    { cpu_execute_illegal_order,         NULL },   /* 12 */
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
    { cpu_execute_illegal_order,            NULL }, /* 1 */
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
    { cpu_execute_illegal_order,              NULL }, /* 1 */
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
    { cpu_execute_illegal_order,              NULL }, /* 14 */
    { cpu_execute_fp_unsigned_reverse_div,    NULL }, /* 15 */ /* Remove when don't need to compare to HASE simulator, was added there by mistake, never implemented in MU5 */
};

static DISPATCH_ENTRY accFPDecimalDispatchTable[] =
{
    { cpu_execute_illegal_order,             NULL }, /* 0 */
    { cpu_execute_fp_decimal_load_double,    NULL }, /* 1 */
    { cpu_execute_fp_decimal_stack_and_load, NULL }, /* 2 */
    { cpu_execute_fp_decimal_store,          NULL }, /* 3 */
    {  cpu_execute_illegal_order,            NULL }, /* 4 */
    {  cpu_execute_illegal_order,            NULL }, /* 5 */
    {  cpu_execute_illegal_order,            NULL }, /* 6 */
    {  cpu_execute_illegal_order,            NULL }, /* 7 */
    {  cpu_execute_illegal_order,            NULL }, /* 8 */
    {  cpu_execute_illegal_order,            NULL }, /* 9 */
    {  cpu_execute_illegal_order,            NULL }, /* 10 */
    {  cpu_execute_illegal_order,            NULL }, /* 11*/
    { cpu_execute_fp_decimal_compare,        NULL }, /* 12 */
    {  cpu_execute_illegal_order,            NULL }, /* 13 */
    {  cpu_execute_illegal_order,            NULL }, /* 14 */
    {  cpu_execute_illegal_order,            NULL }, /* 15 */
};

static DISPATCH_ENTRY floatingPointDispatchTable[] =
{
    { cpu_execute_flp_load_single,    NULL }, /* 0 */
    { cpu_execute_flp_load_double,    NULL }, /* 1 */
    { cpu_execute_flp_stack_and_load, NULL }, /* 2 */
    { cpu_execute_flp_store,          NULL }, /* 3 */
    { cpu_execute_illegal_order,      NULL }, /* 4 */
    { cpu_execute_illegal_order,      NULL }, /* 5 */
    { cpu_execute_illegal_order,      NULL }, /* 6 */
    { cpu_execute_illegal_order,      NULL }, /* 7 */
    { cpu_execute_illegal_order,      NULL }, /* 8 */
    { cpu_execute_illegal_order,      NULL }, /* 9 */
    { cpu_execute_illegal_order,      NULL }, /* 10 */
    { cpu_execute_illegal_order,      NULL }, /* 11*/
    { cpu_execute_illegal_order,      NULL }, /* 12 */
    { cpu_execute_illegal_order,      NULL }, /* 13 */
    { cpu_execute_illegal_order,      NULL }, /* 14 */
    { cpu_execute_illegal_order,      NULL }, /* 15 */
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

/* file loaded is a sequence of 16-bit words */
t_stat sim_load(FILE *ptr, CONST char *cptr, CONST char *fnam, int flag)
{
    t_stat r = SCPE_OK;
    int b;
    t_addr origin, limit;

    if (flag) /* dump? */
    {
        r = sim_messagef(SCPE_NOFNC, "Command Not Implemented\n");
    }
    else
    {
        origin = 0;
        limit = (t_addr)cpu_unit.capac * 4; /* Capacity is in 32-bit words, we need bytes here */
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
        while ((b = Fgetc(ptr)) != EOF)
        {
            if (origin >= limit)
            {
                r = SCPE_NXM;
                break;
            }

            sac_write_8_bit_word(origin, b & 0xFF);
            origin = origin + 1;
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
    //cpu_set_register_32(reg_co, 0); /* TODO: probably needs to be reset to start of OS (upper half of memory) */
    return SCPE_OK;
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

static SIM_INLINE void cpu_set_register_16(REG *reg, uint16 value)
{
    assert(reg->width == 16);
    *(uint16 *)(reg->loc) = value;
}

static SIM_INLINE void cpu_set_register_32(REG *reg, uint32 value)
{
    assert(reg->width == 32);
    *(uint32 *)(reg->loc) = value;
}

static SIM_INLINE void cpu_set_register_64(REG *reg, t_uint64 value)
{
    assert(reg->width == 64);
    *(t_uint64 *)(reg->loc) = value;
}

static SIM_INLINE void cpu_set_register_bit_16(REG *reg, uint16 mask, int value)
{
    assert(reg->width == 16);
    if (value)
    {
        *(uint16 *)(reg->loc) = *(uint16 *)(reg->loc) | ~mask;
    }
    else
    {
        *(uint16 *)(reg->loc) = *(uint16 *)(reg->loc) & mask;
    }
}

static SIM_INLINE void cpu_set_register_bit_32(REG *reg, uint32 mask, int value)
{
    assert(reg->width == 32);
    if (value)
    {
        *(uint32 *)(reg->loc) = *(uint32 *)(reg->loc) | ~mask;
    }
    else
    {
        *(uint32 *)(reg->loc) = *(uint32 *)(reg->loc) & mask;
    }
}

static SIM_INLINE void cpu_set_register_bit_64(REG *reg, t_uint64 mask, int value)
{
    assert(reg->width == 64);
    if (value)
    {
        *(t_uint64 *)(reg->loc) = *(t_uint64 *)(reg->loc) | ~mask;
    }
    else
    {
        *(t_uint64 *)(reg->loc) = *(t_uint64 *)(reg->loc) & mask;
    }
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
    if (*(uint16 *)(reg->loc) & ~mask)
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
    if (*(uint32 *)(reg->loc) & ~mask)
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
    if (*(t_uint64 *)(reg->loc) & ~mask)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

static uint16 cpu_calculate_base_offset(REG *reg, t_int64 offset)
{
    t_int64 result = cpu_get_register_16(reg) + offset;
    if (result < 0 || result > 65535)
    {
        cpu_set_interrupt(INT_SOFTWARE_INTERRUPT); /* TODO: must be segment overflow interrupt */
    }

    return (uint16)(result & 0xFFFF);
}

static t_addr cpu_get_name_segment_address(REG *reg, int16 offset)
{
    t_addr result = (cpu_get_register_16(reg_sn) << 16) | cpu_calculate_base_offset(reg, offset);
    return result;
}

static void cpu_set_interrupt(uint8 number)
{
    interrupt |= 1u << number;
}

static void cpu_clear_interrupt(uint8 number)
{
    interrupt &= ~(1u << number);
}

static uint8 cpu_get_interrupt_number(void)
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

static uint16 cpu_get_cr(uint16 order)
{
    uint16 cr = (order >> 13) & 0x7;
    return cr;
}

static uint16 cpu_get_f(uint16 order)
{
    uint16 f;
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
    int i;
    uint16 nprime = cpu_get_extended_n(order);
    uint8 unsignedLiteral = (order >> 2) & 0x1;

    if (nprime > 2)
    {
        /* The MU5 Basic Programming Manual does not list n'==3 as valid, but the Roland and Ibbett book lists it as another 64-bit option */
        cpu_set_interrupt(INT_ILLEGAL_ORDERS);
    }
    else
    {
        int words = (nprime == 2) ? 4 : nprime + 1;
        uint16 lastWord;
        for (i = 0; i < words; i++)
        {
            lastWord = sac_read_16_bit_word(instructionAddress + 1 + i);
            result |= (t_uint64)lastWord << (i * 16);
        }

        if (!unsignedLiteral)
        {
            result |= (lastWord & 0x8000) ? -1 : 0; /* sign extend */
        }

        *instructionLength += words;
    }

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

/* TODO: These extended routines repeat the same n' code and should be written only once if possible */
static t_uint64 cpu_get_operand_extended_variable_32(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;

    switch (cpu_get_extended_n(order))
    {
        case 1:
        {
            t_addr addr = sac_read_16_bit_word(instructionAddress + 1);
            result = sac_read_32_bit_word(addr);
            *instructionLength += 1;
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "V32 Z %u\n", addr);
            break;
        }
        case 4:
        {
            result = cpu_pop_value();
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "STACK Z 0\n");
            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS);
            break;
        }
    }

    return result;
}

/* TODO: These extended routines repeat the same n' code and should be written only once if possible */
static t_uint64 cpu_get_operand_extended_b_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;

    switch (cpu_get_extended_n(order))
    {
        case 5:
        {
            result = cpu_get_operand_by_descriptor_vector(cpu_get_register_64(reg_d), 0); /* TODO: Not sure what sense this mode has, B doesn't appear to come into it at all */
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "B D 0\n");
            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS);
            break;
        }
    }

    return result;
}

/* TODO: These extended routines repeat the same n' code and should be written only once if possible */
static t_uint64 cpu_get_operand_extended_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;

    switch (cpu_get_extended_n(order))
    {
        case 5:
        {
            result = result = cpu_get_operand_by_descriptor_vector(cpu_get_register_64(reg_d), 0);
            sim_debug(LOG_CPU_DECODE, &cpu_dev, "S0 D 0\n");
            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS);
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
    result = cpu_get_name_segment_address(reg_nb, n);

    return result;
}

static t_addr cpu_get_operand_address_variable_64(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_addr result;
    uint16 n = cpu_get_n(order);
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "V64 %hu\n", n);
    result = cpu_get_name_segment_address(reg_nb, n << 1); /* address in 64-bit units for 64-bit access */

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

/* see p 90 of Morris and Ibbett and p23 of programming manual */
static t_addr cpu_get_operand_byte_address_by_descriptor_vector_access(t_uint64 descriptor, uint32 modifier)
{
    t_addr result;
    uint8 unscaled = cpu_get_descriptor_unscaled(descriptor);
    uint32 origin = cpu_get_descriptor_origin(descriptor);
    if (unscaled)
    {
        result = origin + modifier;
    }
    else
    {
        switch (cpu_get_descriptor_size(descriptor))
        {
            case DESCRIPTOR_SIZE_1_BIT:
            {
                result = origin + (modifier >> 3);
                break;
            }
            case DESCRIPTOR_SIZE_4_BIT:
            {
                result = origin + (modifier >> 1);
                break;
            }
            case DESCRIPTOR_SIZE_8_BIT:
            {
                result = origin + modifier;
                break;
            }
            case DESCRIPTOR_SIZE_16_BIT:
            {
                result = origin + (modifier << 1);
                break;
            }
            case DESCRIPTOR_SIZE_32_BIT:
            {
                result = origin + (modifier << 2);
                break;
            }
            case DESCRIPTOR_SIZE_64_BIT:
            {
                result = origin + (modifier << 3);
                break;
            }
            default:
            {
                cpu_set_interrupt(INT_ILLEGAL_ORDERS); /* TODO: needs to be an interrupt about a bad descriptor */
                break;
            }
        }
    }

    return result;
}

static t_uint64 cpu_get_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier)
{
    t_addr addr;
    t_uint64 result;

    addr = cpu_get_operand_byte_address_by_descriptor_vector_access(descriptor, modifier);
    
    switch (cpu_get_descriptor_size(descriptor))
    {
        case DESCRIPTOR_SIZE_1_BIT:
        {
            assert(0); /* TODO: Implement bit access */
            result = 0;
            break;
        }
        case DESCRIPTOR_SIZE_4_BIT:
        {
            assert(0); /* TODO: Implement nibble access */
            result = 0;
            break;
        }
        case DESCRIPTOR_SIZE_8_BIT:
        {
            result = sac_read_8_bit_word(addr);
            break;
        }
        case DESCRIPTOR_SIZE_16_BIT:
        {
            result = sac_read_16_bit_word(addr);
            break;
        }
        case DESCRIPTOR_SIZE_32_BIT:
        {
            result = sac_read_32_bit_word(addr >> 2); /* TODO: 32-bit word routine takes a word address, not a byte address. Need to make memory access consistent */
            break;
        }
        case DESCRIPTOR_SIZE_64_BIT:
        {
            result = sac_read_64_bit_word(addr >> 2); /* TODO: 64-bit word routine takes a word address, not a byte address. Need to make memory access consistent */
            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS); /* TODO: needs to be an interrupt about a bad descriptor */
            break;
        }
    }

    return result;
}

static void cpu_set_operand_by_descriptor_vector(t_uint64 descriptor, uint32 modifier, t_uint64 value)
{
    t_addr addr;

    addr = cpu_get_operand_byte_address_by_descriptor_vector_access(descriptor, modifier);

    switch (cpu_get_descriptor_size(descriptor))
    {
        case DESCRIPTOR_SIZE_1_BIT:
        {
            assert(0); /* TODO: Implement bit access */
            break;
        }
        case DESCRIPTOR_SIZE_4_BIT:
        {
            assert(0); /* TODO: Implement nibble access */
            break;
        }
        case DESCRIPTOR_SIZE_8_BIT:
        {
            sac_write_8_bit_word(addr, value & 0xFF);
            break;
        }
        case DESCRIPTOR_SIZE_16_BIT:
        {
            sac_write_16_bit_word(addr, value & 0xFFFF);
            break;
        }
        case DESCRIPTOR_SIZE_32_BIT:
        {
            sac_write_32_bit_word(addr >> 2, value & 0xFFFFFFFF); /* TODO: 32-bit word routine takes a word address, not a byte address. Need to make memory access consistent */
            break;
        }
        case DESCRIPTOR_SIZE_64_BIT:
        {
            sac_write_64_bit_word(addr >> 2, value); /* TODO: 64-bit word routine takes a word address, not a byte address. Need to make memory access consistent */
            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS); /* TODO: needs to be an interrupt about a bad descriptor */
            break;
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
        if (xdb < db && !cpu_get_register_bit_32(reg_dod, mask_dod_sssi))
        {
            cpu_set_register_bit_32(reg_dod, mask_dod_sss, 1);
            cpu_set_interrupt(INT_ILLEGAL_ORDERS);
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

static t_uint64 cpu_get_operand_zero_relative_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    uint16 n = cpu_get_n(order);

    result = cpu_get_operand_from_descriptor(order, instructionAddress, instructionLength, 0);

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "S0 %hu\n", n);

    return result;
}

static t_uint64 cpu_get_operand_from_descriptor(uint16 order, uint32 instructionAddress, int *instructionLength, uint32 modifier)
{
    t_uint64 result = 0;
    t_uint64 d;
    t_addr daddr;
    uint16 n = cpu_get_n(order);

    daddr = cpu_get_name_segment_address(reg_nb, n << 1); /* address in 64-bit units for 64-bit access */
    /* TODO: refactor 64-bit address calculation */
    d = sac_read_64_bit_word(daddr);
    cpu_set_register_64(reg_d, d);

    result = cpu_get_operand_by_descriptor_vector(d, modifier);

    return result;
}

static t_uint64 cpu_get_operand_internal_register(uint16 order, uint32 instructionAddress, int *instructionLength)
{
    t_uint64 result = 0;
    uint8 n = order & 0x7;

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
            result = (cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_nb);
            break;
        }
        case 3:
        {
            result = (cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_sf);
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

    sim_debug(LOG_CPU_DECODE, &cpu_dev, "R%hu\n", n);

    return result;
}

static t_uint64 cpu_get_operand(uint16 order)
{
    t_uint64 result = 0;
    int instructionLength = 1;
    uint16 k = cpu_get_k(order);
    uint32 instructionAddress = cpu_get_register_32(reg_co);

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
                {
                    result = cpu_get_operand_extended_literal(order, instructionAddress, &instructionLength);
                    break;
                }
                case 2:
                {
                    result = cpu_get_operand_extended_variable_32(order, instructionAddress, &instructionLength);
                    break;
                }
                case 4:
                case 5:
                {
                    result = cpu_get_operand_extended_b_relative_descriptor(order, instructionAddress, &instructionLength);
                    break;
                }
                case 6:
                {
                    result = cpu_get_operand_extended_zero_relative_descriptor(order, instructionAddress, &instructionLength);
                    break;
                }
                default:
                {
                    cpu_set_interrupt(INT_ILLEGAL_ORDERS);
                    break;
                }
            }

            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS);
        }
    }

    cpu_set_register_32(reg_co, instructionAddress + instructionLength);

    return result;
}

static void cpu_set_operand(uint16 order, t_uint64 value)
{
    t_addr addr;
    int instructionLength = 1;
    uint16 k = cpu_get_k(order);
    uint32 instructionAddress = cpu_get_register_32(reg_co);

    switch (k)
    {
        case 2:
        {
            addr = cpu_get_operand_address_variable_32(order, instructionAddress, &instructionLength);
            sac_write_32_bit_word(addr, value & 0xFFFFFFFF);
            break;
        }
        case 3:
        {
            addr = cpu_get_operand_address_variable_64(order, instructionAddress, &instructionLength);
            sac_write_64_bit_word(addr, value);
            break;
        }
        default:
        {
            cpu_set_interrupt(INT_ILLEGAL_ORDERS);
        }
    }

    cpu_set_register_32(reg_co, instructionAddress + instructionLength);
}

static t_uint64 cpu_sign_extend_6_bit(t_uint64 value)
{
    t_uint64 result = value & 0x3F;
    result |= (value & 0x20) ? 0xFFFFFFFFFFFFFFC0 : 0;
    return result;
}

static void cpu_push_value(t_uint64 value)
{
    uint16 newSF = cpu_get_register_16(reg_sf) + 2;
    cpu_set_register_16(reg_sf, newSF);
    sac_write_64_bit_word(cpu_get_name_segment_address(reg_sf, 0), value);
}

static t_uint64 cpu_pop_value(void)
{
    t_uint64 result;
    uint16 sf = cpu_get_register_16(reg_sf);
    result = sac_read_64_bit_word(cpu_get_name_segment_address(reg_sf, 0));
    cpu_set_register_16(reg_sf, sf - 2);
    return result;
}

static void cpu_test_value(t_int64 value)
{
    cpu_set_register_bit_16(reg_ms, mask_ms_t1, value != 0);
    cpu_set_register_bit_16(reg_ms, mask_ms_t2, value < 0);
}

static void cpu_execute_next_order(void)
{
    uint16 order;
    uint16 cr;

    if (interrupt == 0)
    {
        order = sac_read_16_bit_word(cpu_get_register_32(reg_co));
        cr = cpu_get_cr(order);

        crDispatchTable[cr].execute(order, crDispatchTable[cr].innerTable);
    }
    else
    {
        cpu_start_interrupt_processing();
    }
}

static void cpu_execute_illegal_order(uint16 order, DISPATCH_ENTRY *innerTable)
{
    cpu_set_interrupt(INT_ILLEGAL_ORDERS);
}

static void cpu_start_interrupt_processing(void)
{
    printf("Interrupt %hu detected - processing TBD\n", cpu_get_interrupt_number());
    cpu_stopped = 1; /* TODO: temporary halt CPU until implement interrupt processing */
}

static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable)
{
    uint16 f = cpu_get_f(order);
    innerTable[f].execute(order, innerTable[f].innerTable);
}

static void cpu_jump_relative(uint16 order, int performJump)
{
    int32 relativeTo = (int32)cpu_get_register_32(reg_co); /* Get CO before it is advanced by getting operand as we jump relative to instruction */
    int32 relative = (int32)(cpu_get_operand(order) & 0xFFFFFFFF);
    uint32 newCo = (uint32)(relativeTo + relative);

    if (performJump)
    {
        cpu_set_register_32(reg_co, newCo);
        // TODO: cross-segment generates interrupt

        /* The real MU5 did not have a STOP instruction, see comment above the declaration of cpu_stopped */
        if (relative == 0)
        {
            cpu_stopped = 1;
        }
    }
}

static void cpu_execute_organisational_relative_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "-> ");
    cpu_jump_relative(order, 1);
}

static void cpu_execute_organisational_absolute_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "JUMP ");
    cpu_set_register_32(reg_co, cpu_get_operand(order) & 0x7FFFFFFF);
}

static void cpu_execute_organisational_SF_load_NB_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "SF=NB+ ");
    uint16 newSF = cpu_calculate_base_offset(reg_nb, cpu_get_operand(order));
    cpu_set_register_16(reg_sf, newSF);
}

static void cpu_execute_organisational_NB_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB= ");
    t_uint64 newBase = cpu_get_operand(order); /* TODO: the operand may not be a secondary operand - see p59 */
    cpu_set_register_16(reg_sn, (newBase >> 48) & 0xFFFF);
    cpu_set_register_16(reg_nb, newBase & 0xFFFE); /* LS bit of NB is always zero */
}

static void cpu_execute_organisational_NB_load_SF_plus(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB=SF+ ");
    uint16 newNB = cpu_calculate_base_offset(reg_sf, cpu_get_operand(order));
    cpu_set_register_16(reg_nb, newNB & 0xFFFE); /* LS bit of NB is always zero */
}

static void cpu_execute_organisational_branch_eq(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "= 0 ");
    cpu_jump_relative(order, !cpu_get_register_bit_16(reg_ms, mask_ms_t1));
}

static void cpu_execute_organisational_branch_ne(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "/= 0 ");
    cpu_jump_relative(order, cpu_get_register_bit_16(reg_ms, mask_ms_t1));
}

static void cpu_execute_organisational_branch_ge(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, ">= 0 ");
    cpu_jump_relative(order, !cpu_get_register_bit_16(reg_ms, mask_ms_t1) || !cpu_get_register_bit_16(reg_ms, mask_ms_t2));
}

static void cpu_execute_organisational_branch_lt(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "< 0 ");
    cpu_jump_relative(order, cpu_get_register_bit_16(reg_ms, mask_ms_t2));
}

static void cpu_execute_organisational_branch_le(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "<= 0 ");
    cpu_jump_relative(order, !cpu_get_register_bit_16(reg_ms, mask_ms_t1) || cpu_get_register_bit_16(reg_ms, mask_ms_t2));
}

static void cpu_execute_organisational_branch_gt(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "> 0 ");
    cpu_jump_relative(order, cpu_get_register_bit_16(reg_ms, mask_ms_t1) && !cpu_get_register_bit_16(reg_ms, mask_ms_t2));
}

static void cpu_execute_organisational_branch_ovf(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "OVERFLOW ");
    cpu_jump_relative(order, cpu_get_register_bit_16(reg_ms, mask_ms_t0));
}

static void cpu_execute_organisational_branch_bn(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "Bn ");
    cpu_jump_relative(order, cpu_get_register_bit_16(reg_ms, mask_ms_bn));
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
    uint8 result = (descriptor >> 58) & 0x1;
    return result;
}

static uint8 cpu_get_descriptor_bound_check_inhibit(t_uint64 descriptor)
{
    uint8 result = (descriptor >> 57) & 0x1;
    return result;
}

static uint32 cpu_get_descriptor_bound(t_uint64 descriptor)
{
    uint32 result = (descriptor >> 32) & 0x00FFFFFF;
    return result;
}

static uint32 cpu_get_descriptor_origin(t_uint64 descriptor)
{
    uint32 result = descriptor & 0xFFFFFFFF;
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
    /* TODO: scaling not implemented, p49 */
    /* TODO: indirect descriptor processing not implemented, p49 */
    /* TODO: procedure call processing not implemented, p49 */
    if (!originOnly)
    {
        cpu_set_descriptor_bound(&d, bound - modifier);
    }

    cpu_set_descriptor_origin(&d, origin + modifier);
    cpu_set_register_64(descriptorReg, d);
}

static void cpu_execute_descriptor_modify(uint16 order, REG *descriptorReg)
{
    t_uint64 d = cpu_get_register_64(descriptorReg);
    uint8 type = cpu_get_descriptor_type(d);
    uint8 subtype = cpu_get_descriptor_subtype(d);
    uint32 bound = cpu_get_descriptor_bound(d);
    int32 modifier = cpu_get_operand(order) & 0xFFFFFFFF;
    if (!cpu_get_descriptor_bound_check_inhibit(d) && (type == 0 || type == 1 || type == 2 || (type == 3 && subtype == 0) || (type == 3 && subtype == 1) || (type == 3 && subtype == 2)))
    {
        if (modifier < 0 || (uint32)modifier >= bound)
        {
            cpu_set_interrupt(INT_PROGRAM_FAULTS); /* TODO: bound check interrupt */
        }
    }

    cpu_descriptor_modify(descriptorReg, modifier, FALSE);
}

static void cpu_parse_sts_string_to_string_operand(uint16 order, uint8 *mask, uint8 *filler)
{
    t_uint64 operand = cpu_get_operand(order);

    if (mask != NULL)
    {
        *mask = (operand >> 8) & 0xFF;
    }

    if (filler != NULL)
    {
        *filler = operand & 0xFF;
    }
}

static int cpu_check_string_descriptor(t_uint64 descriptor)
{
    int result = 1;
    uint8 type = cpu_get_descriptor_type(descriptor);
    uint8 size = cpu_get_descriptor_size(descriptor);
    if (type > 2 || size != DESCRIPTOR_SIZE_8_BIT)
    {
        cpu_set_interrupt(INT_ILLEGAL_ORDERS);
        result = 0;
    }

    return result;
}

static int cpu_check_32bit_descriptor(t_uint64 descriptor)
{
    uint8 dt = cpu_get_descriptor_type(descriptor);
    uint8 ds = cpu_get_descriptor_size(descriptor);
    return (dt == 0 || dt == 2) && ds == DESCRIPTOR_SIZE_32_BIT;
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
    xd = xd | (cpu_get_operand(order) & 0xFFFFFFFF);
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
    cpu_push_value(cpu_get_operand(order) & 0xFFFFFFFF);
}

static void cpu_execute_sts1_xd_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XD=> ");
    cpu_set_operand(order, cpu_get_register_64(reg_xd));
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
    uint32 check = cpu_get_operand(order) & 0xFFFFFFFF;
    uint32 xdb = cpu_get_descriptor_bound(cpu_get_register_64(reg_xd));
    int result = check >= 0 && check < xdb;
    cpu_set_register_bit_32(reg_dod, mask_dod_xch, result);
}

static void cpu_execute_sts1_smod(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SMOD ");
    int32 modifier = cpu_get_operand(order) & 0xFFFFFFFF;
    cpu_descriptor_modify(reg_d, modifier, TRUE);
}

static void cpu_execute_sts1_xmod(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS XMOD ");
    t_uint64 xd = cpu_get_register_64(reg_xd);
    uint8 type = cpu_get_descriptor_type(xd);
    uint8 subtype = cpu_get_descriptor_subtype(xd);
    if (type == 3 && (subtype >= 3 && subtype <= 31))
    {
        cpu_set_interrupt(INT_ILLEGAL_ORDERS); /* TODO: better interrupt handling */
    }

    cpu_execute_descriptor_modify(order, reg_xd);
}

static void cpu_execute_sts1_slgc(uint16 order, DISPATCH_ENTRY *innerTable)
{
    t_uint64 operand;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS SLGC ");
    operand = cpu_get_operand(order);
    cpu_process_source_to_destination_descriptor_vector(operand, cpu_sts1_slgc_element_operation);
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
    cpu_parse_sts_string_to_string_operand(order, &mask, &filler);

    if (cpu_check_string_descriptor(d) && cpu_check_string_descriptor(xd))
    {
        if (db == 0)
        {
            if (!cpu_get_register_bit_32(reg_dod, mask_dod_bchi))
            {
                cpu_set_register_bit_32(reg_dod, mask_dod_bch, 1);
                cpu_set_interrupt(INT_ILLEGAL_ORDERS);
            }
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
    cpu_parse_sts_string_to_string_operand(order, &mask, &filler);

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
    cpu_parse_sts_string_to_string_operand(order, &mask, &filler);

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
    uint8 dt = cpu_get_descriptor_type(d);
    uint8 ds = cpu_get_descriptor_size(d);
    unsigned int i;
    int found = FALSE;
    uint32 comparand = cpu_get_operand(order) & 0xFFFFFFFF;
    // TODO: get mask from XD? see p57 of manual
    //cpu_parse_sts_string_to_string_operand(order, &mask, &filler);

    if (cpu_check_32bit_descriptor(d))
    {
        for (i = 0; i < db && !found; i++)
        {
            uint32 source = cpu_get_operand_by_descriptor_vector(d, i) & 0xFFFFFFFF;
            if (source == comparand)
            {
                found = TRUE;
            }
            else
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
            }
        }

        cpu_test_value(found? 0 : 1);
    }
    else
    {
        cpu_set_interrupt(INT_ILLEGAL_ORDERS);
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
    unsigned int n = (xdb < db) ? db : xdb;
    int compareResult = 0;
    cpu_parse_sts_string_to_string_operand(order, &mask, &filler);

    if (cpu_check_string_descriptor(d))
    {
        for (i = 0; i < n && compareResult == 0; i++)
        {
            uint8 sourceByte = (i < xdb) ? cpu_get_operand_by_descriptor_vector(xd, i) & 0xFF : filler;
            uint8 destByte = (uint8)cpu_get_operand_by_descriptor_vector(d, i);
            if (destByte == sourceByte)
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
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
        b -= cpu_get_operand_by_descriptor_vector(xd, 0) & 0xFFFFFFFF;
        b *= cpu_get_operand_by_descriptor_vector(xd, 1) & 0xFFFFFFFF;
        cpu_set_register_32(reg_b, b);
        cpu_set_descriptor_bound(&d, cpu_get_operand_by_descriptor_vector(xd, 2) & 0xFFFFFFFF);
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
    d = d | (cpu_get_operand(order) & 0xFFFFFFFF);
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
    cpu_set_operand(order, cpu_get_register_64(reg_d));
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
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS MOD ");
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

static void cpu_execute_sts2_bmvb(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "STS BMVB ");
    t_uint64 d = cpu_get_register_64(reg_d);
    uint8 mask;
    uint8 byte;
    uint32 db = cpu_get_descriptor_bound(d);
    uint32 dorig = cpu_get_descriptor_origin(d);
    cpu_parse_sts_string_to_string_operand(order, &mask, &byte);

    if (cpu_check_string_descriptor(d))
    {
        if (db != 0)
        {
            cpu_set_operand_by_descriptor_vector(d, 0, byte & ~mask);
            cpu_descriptor_modify(reg_d, 1, FALSE);
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
    cpu_parse_sts_string_to_string_operand(order, &mask, &byte);

    if (cpu_check_string_descriptor(d))
    {
        for (i = 0; i < db && !found; i++)
        {
            found = byte == (uint8)cpu_get_operand_by_descriptor_vector(d, i);
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
    cpu_parse_sts_string_to_string_operand(order, &mask, &byte);

    if (cpu_check_string_descriptor(d))
    {
        for (i = 0; i < db && compareResult == 0; i++)
        {
            uint8 sourceByte = (uint8)cpu_get_operand_by_descriptor_vector(d, i);
            if (byte == sourceByte)
            {
                cpu_descriptor_modify(reg_d, 1, FALSE);
            }
            else if (sourceByte > byte)
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
        b -= cpu_get_operand_by_descriptor_vector(xd, 0) & 0xFFFFFFFF;
        b *= cpu_get_operand_by_descriptor_vector(xd, 1) & 0xFFFFFFFF;
        cpu_set_register_32(reg_b, b);
        cpu_set_descriptor_bound(&d, cpu_get_operand_by_descriptor_vector(xd, 2) & 0xFFFFFFFF);
        cpu_set_register_64(reg_d, d);
        cpu_descriptor_modify(reg_d, b, FALSE);
        cpu_descriptor_modify(reg_xd, 3, FALSE);
    }
}

static void cpu_check_b_overflow(t_uint64 result)
{
    uint32 ms = result >> 32;
    uint8 sign = result >> 31 & 1;
    if (!(sign == 0 && ms == 0) && !(sign == 1 && ms == ~0))
    {
        cpu_set_register_bit_32(reg_bod, mask_bod_bovf, 1);
        if (!cpu_get_register_bit_32(reg_bod, mask_bod_ibovf))
        {
            cpu_set_interrupt(INT_PROGRAM_FAULTS);
        }
    }
    else
    {
        cpu_set_register_bit_32(reg_bod, mask_bod_bovf, 0);
    }
}

static void cpu_test_b_value(t_int64 value)
{
    cpu_set_register_bit_16(reg_ms, mask_ms_t0, cpu_get_register_bit_32(reg_bod, mask_bod_bovf));
    cpu_test_value(value);
}

static void cpu_execute_b_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B= ");
    cpu_set_register_32(reg_b, cpu_get_operand(order) & 0xFFFFFFFF);
}

static void cpu_execute_b_load_and_decrement(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B=' ");
    t_int64 b = cpu_get_operand(order) & 0xFFFFFFFF;
    b--;
    cpu_check_b_overflow(b);
    cpu_set_register_32(reg_b, b & 0xFFFFFFFF);
}

static void cpu_execute_b_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B*= ");
    cpu_push_value(cpu_get_register_32(reg_b));
    cpu_set_register_32(reg_b, cpu_get_operand(order) & 0xFFFFFFFF);
}

static void cpu_execute_b_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B=> ");
    cpu_set_operand(order, cpu_get_register_32(reg_b));
}

static void cpu_execute_b_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B+ ");
    t_int64 augend = cpu_get_register_32(reg_b);
    t_int64 addend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = augend + addend;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    cpu_check_b_overflow(result);
}

static void cpu_execute_b_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B- ");
    t_int64 minuend = cpu_get_register_32(reg_b);
    t_int64 subtrahend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    cpu_check_b_overflow(result);
}

static void cpu_execute_b_mul(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B* ");
    t_int64 multiplicand = cpu_get_register_32(reg_b);
    t_int64 multiplier = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = multiplicand * multiplier;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    cpu_check_b_overflow(result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_b_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B DIV ");
    t_int64 dividend = cpu_get_register_32(reg_b);
    t_int64 divisor = cpu_get_operand(order) & 0xFFFFFFFF;
    if (divisor == 0)
    {
        cpu_set_interrupt(INT_PROGRAM_FAULTS); /* TODO: more to do here? */
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    }
}

static void cpu_execute_b_xor(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B XOR ");
    t_uint64 xorend = cpu_get_register_32(reg_b);
    t_uint64 xorand = cpu_get_operand(order) & 0xFFFFFFFF;
    t_uint64 result = xorend ^ xorand;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
}

static void cpu_execute_b_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B OR ");
    t_uint64 orend = cpu_get_register_32(reg_b);
    t_uint64 orand = cpu_get_operand(order) & 0xFFFFFFFF;
    t_uint64 result = orend | orand;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
}

static void cpu_execute_b_shift_left(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B <- ");
    t_int64 value = cpu_get_register_32(reg_b); /* signed, for arithmetic shift */
    t_int64 shift = cpu_sign_extend_6_bit(cpu_get_operand(order));
    t_int64 result = value << shift;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    cpu_check_b_overflow(result);
}

static void cpu_execute_b_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B AND ");
    t_uint64 andend = cpu_get_register_32(reg_b);
    t_uint64 andand = cpu_get_operand(order) & 0xFFFFFFFF;
    t_uint64 result = andend & andand;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
}

static void cpu_execute_b_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B RSUB ");
    t_int64 subtrahend = cpu_get_register_32(reg_b);
    t_int64 minuend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    cpu_check_b_overflow(result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_b_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B RDIV ");
    t_int64 divisor = cpu_get_register_32(reg_b);
    t_int64 dividend = cpu_get_operand(order) & 0xFFFFFFFF;
    if (divisor == 0)
    {
        cpu_set_interrupt(INT_PROGRAM_FAULTS); /* TODO: more to do here? */
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_b, (uint32)(result & 0xFFFFFFFF));
    }
}

static void cpu_execute_b_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B COMP ");
    t_uint64 b = cpu_get_register_32(reg_b);
    t_int64 comparand = cpu_get_operand(order);
    cpu_test_b_value(b - comparand);
}

static void cpu_execute_b_compare_and_increment(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "B CINC ");
    t_uint64 b = cpu_get_register_32(reg_b);
    t_int64 comparand = cpu_get_operand(order);
    cpu_test_b_value(b - comparand);
    b++;
    cpu_check_b_overflow(b);
    cpu_set_register_32(reg_b, b & 0xFFFFFFFF);
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
            cpu_set_interrupt(INT_PROGRAM_FAULTS);
        }
    }
    else
    {
        cpu_set_register_bit_32(reg_bod, mask_bod_bovf, 0);
    }
}


static void cpu_execute_fp_signed_load_single(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X=(32) ");
    cpu_set_register_32(reg_x, cpu_get_operand(order) & 0xFFFFFFFF);
}

static void cpu_execute_fp_signed_stack_and_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X*= ");
    cpu_push_value(cpu_get_register_32(reg_x));
    cpu_set_register_32(reg_a, cpu_get_operand(order) & 0xFFFFFFFF);
}

static void cpu_execute_fp_signed_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X=> ");
    cpu_set_operand(order, cpu_get_register_32(reg_x));
}

static void cpu_execute_fp_signed_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X+ ");
    t_int64 augend = cpu_get_register_32(reg_x) & 0xFFFFFFFF;
    t_int64 addend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = augend + addend;
    cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X- ");
    t_int64 minuend = cpu_get_register_32(reg_x);
    t_int64 subtrahend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_mul(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X* ");
    t_int64 multiplicand = cpu_get_register_64(reg_x);
    t_int64 multiplier = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = multiplicand * multiplier;
    cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X/ ");
    t_int64 dividend = cpu_get_register_32(reg_x);
    t_int64 divisor = cpu_get_operand(order) & 0xFFFFFFFF;
    if (divisor == 0)
    {
        cpu_set_register_bit_64(reg_aod, mask_aod_zdiv, 1);
        if (!cpu_get_register_bit_64(reg_aod, mask_aod_izdiv))
        {
            cpu_set_interrupt(INT_PROGRAM_FAULTS);
        }
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
    }
}

static void cpu_execute_fp_signed_xor(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X XOR ");
    uint32 xorend = cpu_get_register_32(reg_x);
    uint32 xorand = cpu_get_operand(order) & 0xFFFFFFFF;
    uint32 result = xorend ^ xorand;
    cpu_set_register_32(reg_x, result);
}

static void cpu_execute_fp_signed_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X OR ");
    uint32 orend = cpu_get_register_32(reg_x);
    uint32 orand = cpu_get_operand(order) & 0xFFFFFFFF;
    uint32 result = orend | orand;
    cpu_set_register_32(reg_x, result);
}

static void cpu_execute_fp_signed_shift_left(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X <- ");
    t_int64 value = cpu_get_register_32(reg_x); /* signed for arithmetic shift */
    t_int64 shift = cpu_sign_extend_6_bit(cpu_get_operand(order));
    t_int64 result = value << shift;
    cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X AND ");
    uint32 andend = cpu_get_register_32(reg_x);
    uint32 andand = cpu_get_operand(order) & 0xFFFFFFFF;
    uint32 result = andend & andand;
    cpu_set_register_32(reg_x, result);
}

static void cpu_execute_fp_signed_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X RSUB ");
    t_int64 subtrahend = cpu_get_register_32(reg_x);
    t_int64 minuend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
    cpu_check_x_overflow(result);
}

static void cpu_execute_fp_signed_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    int t0;
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X COMP ");
    t_uint64 x = cpu_get_register_32(reg_x);
    t_int64 comparand = cpu_get_operand(order) & 0xFFFFFFFF;
    cpu_test_value(x - comparand);
    t0 = cpu_get_register_bit_64(reg_aod, mask_aod_fxpovf) | cpu_get_register_bit_64(reg_aod, mask_aod_zdiv);
    cpu_set_register_bit_16(reg_ms, mask_ms_t0, t0);
}

static void cpu_execute_fp_signed_convert(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X CONV ");
    /* TODO: Implement convert to floating point */
    cpu_set_interrupt(INT_ILLEGAL_ORDERS);
}

static void cpu_execute_fp_signed_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "X RDIV ");
    t_int64 divisor = cpu_get_register_32(reg_x);
    t_int64 dividend = cpu_get_operand(order) & 0xFFFFFFFF;
    if (divisor == 0)
    {
        cpu_set_register_bit_64(reg_aod, mask_aod_zdiv, 1);
        if (!cpu_get_register_bit_64(reg_aod, mask_aod_izdiv))
        {
            cpu_set_interrupt(INT_PROGRAM_FAULTS);
        }
    }
    else
    {
        t_int64 result = dividend / divisor;
        cpu_set_register_32(reg_x, (uint32)(result & 0xFFFFFFFF));
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
    cpu_push_value(cpu_get_register_32(reg_aod) & mask_aod);
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
    t_int64 augend = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    t_int64 addend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = augend + addend;
    cpu_set_register_64(reg_a, (t_uint64)result);
}

static void cpu_execute_fp_unsigned_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A- ");
    t_int64 minuend = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    t_int64 subtrahend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_64(reg_a, (t_uint64)result);
}

static void cpu_execute_fp_unsigned_mul(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A* ");
    uint32 multiplicand = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    uint32 multiplier = cpu_get_operand(order) & 0xFFFFFFFF;
    t_uint64 result = multiplicand * multiplier;
    cpu_set_register_64(reg_a, result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_fp_unsigned_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A/ ");
    uint32 dividend = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    uint32 divisor = cpu_get_operand(order) & 0xFFFFFFFF;
    if (divisor == 0)
    {
        cpu_set_interrupt(INT_PROGRAM_FAULTS); /* TODO: more to do here? */
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
    t_uint64 result = (xorend ^ xorand) & 0xFFFFFFFF;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_or(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A OR ");
    t_uint64 orend = cpu_get_register_64(reg_a);
    t_uint64 orand = cpu_get_operand(order);
    t_uint64 result = (orend | orand) & 0xFFFFFFFF;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_shift_left(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A <- ");
    t_uint64 value = cpu_get_register_64(reg_a); /* unsigned for logical shift */
    t_int64 shift = cpu_sign_extend_6_bit(cpu_get_operand(order));
    t_int64 result = value << shift;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_and(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A AND ");
    t_uint64 andend = cpu_get_register_64(reg_a);
    t_uint64 andand = cpu_get_operand(order) & 0xFFFFFFFF;
    t_uint64 result = (andend & andand) & 0xFFFFFFFF;
    cpu_set_register_64(reg_a, result);
}

static void cpu_execute_fp_unsigned_reverse_sub(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A RSUB ");
    t_int64 subtrahend = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    t_int64 minuend = cpu_get_operand(order) & 0xFFFFFFFF;
    t_int64 result = minuend - subtrahend;
    cpu_set_register_64(reg_a, (t_uint64)result);
}

/* MU5 Basic Programming Manual lists this as a dummy order so implementation is a guess */
static void cpu_execute_fp_unsigned_reverse_div(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "A RDIV ");
    uint32 divisor = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    uint32 dividend = cpu_get_operand(order) & 0xFFFFFFFF;
    if (divisor == 0)
    {
        cpu_set_interrupt(INT_PROGRAM_FAULTS); /* TODO: more to do here? */
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
    t_uint64 a = cpu_get_register_64(reg_a) & 0xFFFFFFFF;
    t_uint64 comparand = cpu_get_operand(order) & 0xFFFFFFFF;
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
    cpu_set_register_64(reg_a, cpu_get_operand(order));
}

static void cpu_execute_fp_decimal_store(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AEX=> ");
    cpu_set_operand(order, cpu_get_register_64(reg_aex));
}

static void cpu_execute_fp_decimal_compare(uint16 order, DISPATCH_ENTRY *innerTable)
{
    sim_debug(LOG_CPU_DECODE, &cpu_dev, "AODCOMP ");
    t_uint64 aod = cpu_get_register_64(reg_aod) & mask_aod;
    int result = aod == cpu_get_operand(order);
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
