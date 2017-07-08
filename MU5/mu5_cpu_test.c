/* mu5_cpu_test.c: MU5 CPU self tests

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
#include "mu5_test.h"
#include "mu5_cpu_test.h"
#include "mu5_sac.h"

#define CR_ORG 0
#define CR_B 1
#define CR_STS1 2
#define CR_STS2 3
#define CR_XS 4
#define CR_AU 5
#define CR_ADC 6
#define CR_FLOAT 7

#define F_LOAD_B 0
#define F_LOAD_DEC_B 1
#define F_STACK_LOAD_B 2
#define F_STORE_B 3
#define F_ADD_B 4
#define F_SUB_B 5
#define F_MUL_B 6
#define F_XOR_B 8
#define F_OR_B 9
#define F_SHIFT_L_B 10
#define F_AND_B 11
#define F_RSUB_B 12
#define F_COMP_B 13
#define F_CINC_B 14

#define F_LOAD_X 0
#define F_STACK_LOAD_X 2
#define F_STORE_X 3
#define F_ADD_X 4
#define F_SUB_X 5
#define F_MUL_X 6
#define F_DIV_X 7
#define F_XOR_X 8
#define F_OR_X 9
#define F_SHIFT_L_X 10
#define F_AND_X 11
#define F_RSUB_X 12
#define F_COMP_X 13
#define F_CONV_X 14
#define F_RDIV_X 15

#define F_LOAD_AOD 0
#define F_STACK_LOAD_AOD 2
#define F_STORE_AOD 3
#define F_ADD_A 4
#define F_SUB_A 5
#define F_MUL_A 6
#define F_DIV_A 7
#define F_XOR_A 8
#define F_OR_A 9
#define F_SHIFT_L_A 10
#define F_AND_A 11
#define F_RSUB_A 12
#define F_COMP_A 13

#define F_LOAD_AEX 1
#define F_STACK_LOAD_AEX 2
#define F_STORE_AEX 3
#define F_COMP_AOD 12

#define F_LOAD_XDO 0
#define F_LOAD_XD 1
#define F_STACK 2
#define F_STORE_XD 3
#define F_LOAD_XDB 4
#define F_XCHK 5
#define F_SMOD 6
#define F_XMOD 7
#define F_SLGC 8
#define F_SMVB 9
#define F_SMVF 11
#define F_TALU 12
#define F_SCMP 14
#define F_SUB1 15

#define F_LOAD_DO 0
#define F_LOAD_D 1
#define F_STACK_LOAD_D 2
#define F_STORE_D 3
#define F_LOAD_DB 4
#define F_MDR 5
#define F_MOD 6
#define F_RMOD 7
#define F_BLGC 8
#define F_BMVB 9
#define F_BMVE 10
#define F_BSCN 13
#define F_BCMP 14
#define F_SUB2 15

#define F_LOAD_32 0
#define F_LOAD_64 1
#define F_STACK_LOAD 2
#define F_STORE 3
#define F_XOR 8
#define F_OR 9
#define F_SHIFT_CIRC 10
#define F_AND 11

#define F_RELJUMP 0
#define F_EXIT 1
#define F_ABSJUMP 4
#define F_RETURN 5
#define F_XC0 8
#define F_XC1 9
#define F_XC2 10
#define F_XC3 11
#define F_XC4 12
#define F_XC5 13
#define F_XC6 14
#define F_STACKLINK 15
#define F_MS_LOAD 16
#define F_DL_LOAD 17
#define F_SPM 18
#define F_SETLINK 19
#define F_XNB_LOAD 20
#define F_SN_LOAD 21
#define F_XNB_PLUS 22
#define F_XNB_STORE 23
#define F_SF_LOAD 24
#define F_SF_PLUS 25
#define F_SF_LOAD_NB_PLUS 26
#define F_SF_STORE 27
#define F_NB_LOAD 28
#define F_NB_LOAD_SF_PLUS 29
#define F_NB_PLUS 30
#define F_NB_STORE 31
#define F_BRANCH_EQ 32
#define F_BRANCH_NE 33
#define F_BRANCH_GE 34
#define F_BRANCH_LT 35
#define F_BRANCH_LE 36
#define F_BRANCH_GT 37
#define F_BRANCH_OVF 38
#define F_BRANCH_BN 39
#define F_BN_EQ 40
#define F_BN_NE 41
#define F_BN_GE 42
#define F_BN_LT 43
#define F_BN_LE 44
#define F_BN_GT 45
#define F_BN_OVF 46
#define F_BN_BN 47
#define F_BN_0 48
#define F_BN_BN_AND_X 49
#define F_BN_NOT_BN_AND_X 50
#define F_BN_X 51
#define F_BN_BN_AND_NOT_X 52
#define F_BN_COPY_BN 53
#define F_BNBN_NEQV_X 54
#define F_BN_BN_OR_X 55
#define F_BN_NOT_BN_AND_NOT_X 56
#define F_BN_BN_EQV_X 57
#define F_BN_NOT_BN 58
#define F_BN_NOT_BN_OR_X 59
#define F_BN_NOT_X 60
#define F_BN_BN_OR_NOT_X 61
#define F_BN_NOT_BN_OR_NOT_X 62
#define F_BN_1 63

#define K_LITERAL 0
#define K_IR 1
#define K_V32 2
#define K_V64 3
#define K_SB 4
#define K_SB_5 5
#define K_S0 6
#define K_PRIVILEGED 7

#define KP_LITERAL 0
#define KP_LITERAL_1 1

#define NP_16_BIT_SIGNED_LITERAL 0
#define NP_32_BIT_SIGNED_LITERAL 1
#define NP_64_BIT_LITERAL 2
#define NP_64_BIT_LITERAL_3 3
#define NP_16_BIT_UNSIGNED_LITERAL 4
#define NP_32_BIT_UNSIGNED_LITERAL 5
#define NP_64_BIT_LITERAL_6 6
#define NP_64_BIT_LITERAL_7 7

#define NP_SF 0
#define NP_0 1
#define NP_NB 2
#define NP_XNB 3
#define NP_STACK 4
#define NP_DR 5
#define NP_NB_REF 6
#define NP_XNB_REF 7

#define DESCRIPTOR_US_MASK 0x0200000000000000
#define DESCRIPTOR_BC_MASK 0x0100000000000000

#define AOD_OPSIZ_MASK   0x00001000
#define AOD_IFLPOVF_MASK 0x00000800
#define AOD_IFLPUDF_MASK 0x00000400
#define AOD_IFXPOVF_MASK 0x00000200
#define AOD_IDECOVF_MASK 0x00000100
#define AOD_IZDIV_MASK   0x00000080
#define AOD_FLPOVF_MASK  0x00000040
#define AOD_FLPUDF_MASK  0x00000020
#define AOD_FXPOVF_MASK  0x00000010
#define AOD_DECOVF_MASK  0x00000008
#define AOD_ZDIV_MASK    0x00000004

#define BOD_IBOVF_MASK 0x80000000
#define BOD_BOVF_MASK 0x04000000

#define DOD_XCHK_MASK 0x00000001
#define DOD_ITS_MASK  0x00000002
#define DOD_EMS_MASK  0x00000004
#define DOD_SSS_MASK  0x00000008
#define DOD_NZT_MASK  0x00000010
#define DOD_BCH_MASK  0x00000020
#define DOD_SSSI_MASK 0x00000040
#define DOD_NZTI_MASK 0x00000080
#define DOD_BCHI_MASK 0x00000100
#define DOD_WRO_MASK  0x00000200

#define MS_TEST_MASK 0x0F00
#define MS_OVERFLOW_MASK 0x0800
#define TEST_EQUALS 0x0000
#define TEST_GREATER_THAN 0x0400
#define TEST_LESS_THAN 0x0600
#define TEST_OVERFLOW 0x0800

#define SN_DEFAULT          0x0001
#define SN_BASE            0x10000
#define NB_DEFAULT          0x00F0
#define SF_DEFAULT          0x00F8
#define XNB_DEFAULT        (SN_BASE + NB_DEFAULT)
#define VEC_ORIGIN_DEFAULT  0x01F0

#define NAME_SEGMENT_DEFAULT_BASE (SN_BASE + NB_DEFAULT)
#define NAME_SEGMENT_DEFAULT_STACK_BASE (SN_BASE + SF_DEFAULT)
#define ZERO_OFFSET_32(n) (SN_BASE + n)
#define ZERO_OFFSET_64(n) (SN_BASE + (2 * n))
#define NAME_SEGMENT_OFFSET_32(n) (NAME_SEGMENT_DEFAULT_BASE + n)
#define NAME_SEGMENT_OFFSET_64(n) (NAME_SEGMENT_DEFAULT_BASE + (2 * n))
#define EXTRA_NAME_BASE_OFFSET_32(n) (XNB_DEFAULT + n)
#define EXTRA_NAME_BASE_OFFSET_64(n) (XNB_DEFAULT + (2 * n))
#define NAME_SEGMENT_STACK_OFFSET_32(n) (NAME_SEGMENT_DEFAULT_STACK_BASE + n)
#define NAME_SEGMENT_STACK_OFFSET_64(n) (NAME_SEGMENT_DEFAULT_STACK_BASE + (2 * n))

typedef struct
{
    uint8 func;
    int8 bn;
    int8 r;
    int8 newBn;
} CONDITIONTABLE;

static TESTCONTEXT *localTestContext;
static uint32 currentLoadLocation;
extern DEVICE cpu_dev;

static void cpu_selftest_reset(UNITTEST *test);
static void cpu_selftest_setup_test_virtual_pages(uint8 access);
static void cpu_selftest_set_load_location(uint32 location);
static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n);
static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 kp, uint8 np);
static void cpu_selftest_load_organisational_order_literal(uint8 f,uint8 n);
static void cpu_selftest_load_organisational_order_extended(uint8 f, uint8 kp, uint8 np);
static void cpu_selftest_load_16_bit_literal(uint16 value);
static void cpu_selftest_load_32_bit_literal(uint32 value);
static void cpu_selftest_load_64_bit_literal(t_uint64 value);
static t_uint64 cpu_selftest_create_descriptor(uint8 type, uint8 size, uint32 bound, uint32 origin);
static t_uint64 cpu_selftest_create_miscellaneous_descriptor(uint8 type, uint8 subtype, uint32 bound, uint32 origin);
static t_addr cpu_selftest_get_64_bit_vector_element_address(uint32 origin, uint32 offset);
static t_addr cpu_selftest_get_32_bit_vector_element_address(uint32 origin, uint32 offset);
static t_addr cpu_selftest_get_16_bit_vector_element_address(uint32 origin, uint32 offset);
static t_addr cpu_selftest_get_8_bit_vector_element_address(uint32 origin, uint32 offset);
static t_addr cpu_selftest_get_4_bit_vector_element_address(uint32 origin, uint32 offset);
static t_addr cpu_selftest_get_1_bit_vector_element_address(uint32 origin, uint32 offset);
static void cpu_selftest_load_64_bit_value_to_descriptor_location(uint32 origin, uint32 offset, t_uint64 value);
static void cpu_selftest_load_32_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint32 value);
static void cpu_selftest_load_16_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint16 value);
static void cpu_selftest_load_8_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint8 value);
static void cpu_selftest_load_4_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint8 value);
static void cpu_selftest_load_1_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint8 value);
static uint32 cpu_selftest_byte_address_from_word_address(uint32 address);
static void cpu_selftest_set_aod_operand_32_bit(void);
static void cpu_selftest_set_aod_operand_64_bit(void);
static void cpu_selftest_set_level0_mode(void);
static void cpu_selftest_set_level1_mode(void);
static void cpu_selftest_set_executive_mode(void);
static void cpu_selftest_set_user_mode(void);
static void cpu_selftest_clear_bcpr(void);
static void cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode(void);
static void cpu_selftest_set_acc_faults_to_system_error_in_exec_mode(void);
static void cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode(void);
static void cpu_selftest_set_b_and_d_faults_to_system_error_in_exec_mode(void);
static void cpu_selftest_clear_inhibit_program_fault_interrupts(void);
static void cpu_selftest_set_inhibit_program_fault_interrupts(void);
static void cpu_selftest_set_inhibit_instruction_counter(void);
static void cpu_selftest_set_bn(int8 bn);
static void cpu_selftest_set_test_is_zero(int8 isZero);
static void cpu_selftest_run_code(void);
static void cpu_selftest_run_code_from_location(uint32 location);
static void cpu_selftest_run_continue(void);
static REG *cpu_selftest_find_register(char *name);
static t_uint64 cpu_selftest_get_register(char *name);
static void cpu_selftest_set_register(char *name, t_uint64 value);
static void cpu_selftest_setup_default_segment(void);
static void cpu_selftest_setup_name_base(uint16 base);
static void cpu_selftest_setup_default_name_base(void);
static void cpu_selftest_setup_default_extra_name_base(void);
static void cpu_selftest_setup_stack_base(uint16 base);
static void cpu_selftest_setup_default_stack_base(void);
static void cpu_selftest_setup_vstore_test_location(void);
static void cpu_selftest_setup_interrupt_vector(int interruptNumber, uint16 ms, uint16 nb, uint32 co);
static void cpu_selftest_setup_illegal_function_error(void);
static void cpu_selftest_setup_name_adder_overflow_error(void);
static void cpu_selftest_setup_control_adder_overflow_error(void);
static void cpu_selftest_setup_interrupt_entry_link(int interruptNumber);

static void cpu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void cpu_selftest_assert_reg_equals_mask(char *name, t_uint64 expectedValue, t_uint64 mask);
static void cpu_selftest_assert_memory_contents_32_bit(t_addr address, uint32 expectedValue);
static void cpu_selftest_assert_memory_contents_64_bit(t_addr address, t_uint64 expectedValue);
static void cpu_selftest_assert_vector_content_64_bit(t_addr origin, uint32 offset, t_uint64 expectedValue);
static void cpu_selftest_assert_vector_content_32_bit(t_addr origin, uint32 offset, uint32 expectedValue);
static void cpu_selftest_assert_vector_content_16_bit(t_addr origin, uint32 offset, uint16 expectedValue);
static void cpu_selftest_assert_vector_content_8_bit(t_addr origin, uint32 offset, uint8 expectedValue);
static void cpu_selftest_assert_vector_content_4_bit(t_addr origin, uint32 offset, uint8 expectedValue);
static void cpu_selftest_assert_vector_content_1_bit(t_addr origin, uint32 offset, uint8 expectedValue);

static void cpu_selftest_assert_interrupt_return_address(int interruptNumber, uint32 expectedAddress);
static void cpu_selftest_assert_inhibited_program_fault_interrupt(uint16 expected_program_fault_status);
static void cpu_selftest_assert_no_system_error(void);
static void cpu_selftest_assert_no_program_fault(void);

static void cpu_selftest_assert_no_b_overflow(void);
static void cpu_selftest_assert_no_b_overflow_interrupt(void);
static void cpu_selftest_assert_b_overflow(void);
static void cpu_selftest_assert_b_overflow_interrupt_as_system_error(void);
static void cpu_selftest_assert_no_acc_overflow(void);
static void cpu_selftest_assert_no_acc_overflow_interrupt(void);
static void cpu_selftest_assert_acc_fixed_point_overflow(void);
static void cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error(void);
static void cpu_selftest_assert_no_a_zero_divide(void);
static void cpu_selftest_assert_no_a_zero_divide_interrupt(void);
static void cpu_selftest_assert_a_zero_divide(void);
static void cpu_selftest_assert_a_zero_divide_interrupt(void);
static void cpu_selftest_assert_interrupt(int interruptNumber);
static void cpu_selftest_assert_interrupt_inhibited(void);
static void cpu_selftest_assert_no_interrupt(void);
static void cpu_selftest_assert_dod_interrupt_as_system_error(char *name, uint32 mask);
static void cpu_selftest_assert_d_error_no_interrupt(char *name, uint32 mask);
static void cpu_selftest_assert_no_d_interrupt(char *name, uint32 mask);
static void cpu_selftest_assert_bound_check_interrupt_as_system_error(void);
static void cpu_selftest_assert_its_interrupt_as_system_error(void);
static void cpu_selftest_assert_sss_interrupt(void);
static void cpu_selftest_assert_bounds_check_no_interrupt(void);
static void cpu_selftest_assert_no_bounds_check_interrupt(void);
static void cpu_selftest_assert_no_its_interrupt(void);
static void cpu_selftest_assert_sss_no_interrupt(void);
static void cpu_selftest_assert_no_sss_interrupt(void);
static void cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error(void);
static void cpu_selftest_assert_name_adder_overflow_interrupt_as_illegal_order(void);
static void cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error(void);
static void cpu_selftest_assert_control_adder_overflow_interrupt_as_illegal_order(void);
static void cpu_selftest_assert_spm_program_fault_interrupt(void);
static void cpu_selftest_assert_acc_interrupt_as_system_error(void);
static void cpu_selftest_assert_acc_interrupt_as_program_fault(void);
static void cpu_selftest_assert_B_or_D_system_error(void);
static void cpu_selftest_assert_B_or_D_interrupt_as_system_error(void);
static void cpu_selftest_assert_B_program_fault(void);
static void cpu_selftest_assert_B_interrupt_as_program_fault(void);
static void cpu_selftest_assert_D_interrupt_as_program_fault(void);
static void cpu_selftest_assert_illegal_v_store_access_interrupt();
static void cpu_selftest_assert_illegal_function_as_system_error(void);
static void cpu_selftest_assert_illegal_function_as_illegal_order(void);
static void cpu_selftest_assert_cpr_not_equivalence_system_error_interrupt(void);
static void cpu_selftest_assert_test_equals(void);
static void cpu_selftest_assert_test_greater_than(void);
static void cpu_selftest_assert_test_less_than(void);
static void cpu_selftest_assert_test_no_overflow(void);
static void cpu_selftest_assert_test_overflow(void);
static void cpu_selftest_assert_operand_size_32(void);
static void cpu_selftest_assert_operand_size_64(void);
static void cpu_selftest_assert_bn(int expectedValue);
static void cpu_selftest_assert_boolean_order_condition(CONDITIONTABLE *entry);
static void cpu_selftest_assert_v_store_contents(uint8 block, uint8 line, t_uint64 expectedValue);
static void cpu_selftest_assert_fail(void);
static void cpu_selftest_set_failure(void);

static void cpu_selftest_16_bit_instruction_fetches_using_obey_access(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_not_obeyed_if_access_violation_when_fetching_instruction(TESTCONTEXT *testContext);
static void cpu_selftest_16_bit_instruction_generates_instruction_access_violation_when_fetching_from_non_executable_segment(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_fetches_extended_literal_using_obey_access(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_sf_using_obey_access(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_zero_using_obey_access(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_nb_using_obey_access(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_xnb_using_obey_access(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_access_violation_when_fetching_extended_literal_from_non_executable_segment(TESTCONTEXT *testContext);

static void cpu_selftest_16_bit_instruction_advances_co_by_1(TESTCONTEXT *testContext);
static void cpu_selftest_32_bit_instruction_advances_co_by_2(TESTCONTEXT *testContext);
static void cpu_selftest_48_bit_instruction_advances_co_by_3(TESTCONTEXT *testContext);
static void cpu_selftest_80_bit_instruction_advances_co_by_5(TESTCONTEXT *testContext);
static void cpu_selftest_dummy_order_advances_co_over_operand(TESTCONTEXT *testContext);
static void cpu_selftest_advancing_co_across_segment_boundary_with_get_operand_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_advancing_co_across_segment_boundary_with_set_operand_generates_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_6_bit_positive_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_6_bit_negative_literal(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_internal_register_0(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_1(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_2(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_3(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_4(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_16(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_17(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_18(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_19(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_20(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_32(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_33(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_34(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_internal_register_48(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_non_existent_internal_register(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_32_bit_variable(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_64_bit_variable(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_loads_D(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_with_negative_modifier_generates_bounds_check(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_b_relative_descriptor_modifier_greater_than_bound_generates_bounds_check(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_zero_relative_descriptor_loads_D(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_16_bit_signed_positive_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_16_bit_signed_negative_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_32_bit_signed_positive_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_32_bit_signed_negative_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_64_bit_literal_np_2(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_64_bit_literal_np_3(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_16_bit_unsigned_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_32_bit_unsigned_literal(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_64_bit_literal_np_6(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_64_bit_literal_np_7(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_literal_kp_1(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_sf(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_sf(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext);

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_with_invalid_descriptor_element_size_generates_its_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_privileged_reads_v_store_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_load_operand_privileged_generates_interrupt_in_user_mode(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_6_bit_literal_generates_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt_as_illegal_order_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_0_generates_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_1_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_2_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_3_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_4_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_non_existent_prop_internal_register_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_16(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_17(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_18(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_19(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_20(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_32(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_33(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_34(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_internal_register_48(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_non_existent_internal_register(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_32_bit_variable(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_64_bit_variable(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_loads_D(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_zero_relative_descriptor_loads_D(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_extended_literal_kp_0_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_literal_kp_1_generates_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext);

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_with_invalid_descriptor_element_size_generates_its_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_privileged_stores_v_store_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_store_operand_privileged_generates_interrupt_in_user_mode(TESTCONTEXT *testContext);

static void cpu_selftest_any_descriptor_modify_generates_its_interrupt_if_descriptor_has_invalid_size(TESTCONTEXT *testContext);

static void cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xd_load_loads_whole_of_XD(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_stack_stacks_operand(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xd_store_stores_xd_to_operand(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xd_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xdb_load_loads_bound_in_XD(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xchk_operand_negative_clears_DOD_XCH_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xchk_operand_ge_XDB_clears_DOD_XCH_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xchk_operand_within_XDB_sets_DOD_XCH_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_adds_signed_operand_to_D_origin(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_scales_modifier_for_type_0(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_scales_modifier_for_type_1(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_scales_modifier_for_type_2(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_0_when_US_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_2_when_US_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smod_does_not_check_bounds(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_adds_signed_operand_to_XD_origin_subtracts_from_bound(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_64_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_32_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_16_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_8_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_within_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_crossing_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_within_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_crossing_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_checks_bounds_for_type_0(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_checks_bounds_for_type_1(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_checks_bounds_for_type_2(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_0_when_BC_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_1_when_BC_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_2_when_BC_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_xmod_generates_its_interrupt_if_illegal_descriptor_type_used(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_is_type_3(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_type_0_descriptors(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_type_2_descriptors(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_long_vector(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_L0(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_L1(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_L2(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_slgc_processes_L3(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvb_generates_checks_bound_on_destination_not_0(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvb_copies_byte_with_mask(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvb_uses_filler_when_source_empty_with_mask(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvf_copies_to_zero_length_destination(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_smvf_copies_bytes_and_fills_with_mask(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_talu_returns_test_register_greater_than_if_not_found(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_0(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_2(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_talu_generates_its_interrupt_if_descriptor_is_not_32_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical_with_filler(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_destination_is_shorter_and_subset_is_equal(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_returns_test_register_greater_than_if_source_byte_greater_than_destination_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_scmp_returns_test_register_less_than_if_source_byte_less_than_destination_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_sub1_loads_XD_and_modifies_it(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_sub1_calculates_B(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_sub1_modifies_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_sub1_generates_its_interrupt_if_descriptor_is_not_valid(TESTCONTEXT *testContext);

static void cpu_selftest_sts2_do_load_loads_ls_half_of_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_d_load_loads_whole_of_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_d_stack_load_stacks_D_loads_new_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_d_store_stores_d_to_operand(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_d_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_db_load_loads_bound_in_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mdr_advances_location_and_loads_D_with_operand_pointed_to_by_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_adds_signed_operand_to_D_origin_subtracts_from_bound(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_64_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_32_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_16_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_8_bit_value(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_within_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_crossing_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_within_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_crossing_byte(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_checks_bounds_for_type_0(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_checks_bounds_for_type_1(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_checks_bounds_for_type_2(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_0_when_BC_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_1_when_BC_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_2_when_BC_set(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_rmod_loads_least_significant_half_of_D_and_adds_to_origin(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L0(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_blgc_processes_type_1_descriptor_L1(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_blgc_processes_type_2_descriptor_L2(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L3(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bmvb_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bmvb_generates_checks_bound_on_destination_not_0(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bmvb_copies_byte_with_mask(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bmve_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bmve_copies_byte_with_mask_to_whole_destination(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_smvf_copies_bytes_and_fills_with_mask(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bscn_sets_less_than_if_byte_not_found(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bscn_finds_byte_in_type_0_descriptor(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bscn_finds_byte_in_type_1_descriptor(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bscn_finds_byte_in_type_2_descriptor(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_sets_equals_if_byte_found_in_all_elements(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_smaller(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_larger(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_finds_byte_in_type_0_descriptor(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_finds_byte_in_type_1_descriptor(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_bcmp_finds_byte_in_type_2_descriptor(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_sub2_modifies_XD(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_sub2_calculates_B(TESTCONTEXT *testContext);
static void cpu_selftest_sts2_sub2_modifies_D_existing_D(TESTCONTEXT *testContext);
static void cpu_selftest_sts1_sub2_generates_its_interrupt_if_descriptor_is_not_valid(TESTCONTEXT *testContext);

static void cpu_selftest_b_load_loads_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_load_and_decrement_loads_B_and_subtracts_1(TESTCONTEXT *testContext);
static void cpu_selftest_b_load_and_decrement_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_b_stack_and_load_stacks_B_and_loads_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_store_stores_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_add_adds_operand_to_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_add_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_b_sub_subtracts_operand_from_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_sub_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_b_mul_multiplies_operand_by_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_mul_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_b_xor(TESTCONTEXT *testContext);
static void cpu_selftest_b_or(TESTCONTEXT *testContext);
static void cpu_selftest_b_shift_shifts_left_for_positive_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_shift_shifts_right_for_negative_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_shift_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_b_and(TESTCONTEXT *testContext);
static void cpu_selftest_b_rsub_subtracts_B_from_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_rsub_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_b_comp_sets_less_than_when_B_less_than_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_comp_sets_equals_when_B_equals_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_comp_sets_greater_than_when_B_greater_than_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_comp_sets_overflow_in_t0_only(TESTCONTEXT *testContext);
static void cpu_selftest_b_cinc_compares_B_with_operand(TESTCONTEXT *testContext);
static void cpu_selftest_b_cinc_increments_B(TESTCONTEXT *testContext);
static void cpu_selftest_b_cinc_flags_overflow(TESTCONTEXT *testContext);

static void cpu_selftest_x_load_loads_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_stack_and_load_stacks_X_and_loads_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_store_stores_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_add_adds_operand_to_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_add_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_x_sub_subtracts_operand_from_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_sub_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_x_mul_multiplies_operand_by_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_mul_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_x_div_divides_X_by_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_div_flags_divide_by_zero(TESTCONTEXT *testContext);
static void cpu_selftest_x_xor(TESTCONTEXT *testContext);
static void cpu_selftest_x_or(TESTCONTEXT *testContext);
static void cpu_selftest_x_shift_shifts_left_for_positive_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_shift_shifts_right_for_negative_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_shift_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_x_and(TESTCONTEXT *testContext);
static void cpu_selftest_x_rsub_subtracts_X_from_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_rsub_flags_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_x_comp_sets_less_than_when_X_less_than_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_comp_sets_equals_when_X_equals_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_comp_sets_greater_than_when_X_greater_than_operand(TESTCONTEXT *testContext);
static void cpu_selftest_x_comp_sets_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_x_comp_overflow_does_not_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_x_rdiv_divides_operand_by_X(TESTCONTEXT *testContext);
static void cpu_selftest_x_rdiv_flags_divide_by_zero(TESTCONTEXT *testContext);

static void cpu_selftest_a_load_loads_AOD(TESTCONTEXT *testContext);
static void cpu_selftest_a_stack_and_load_stacks_AOD_and_loads_AOD(TESTCONTEXT *testContext);
static void cpu_selftest_a_store_stores_AOD(TESTCONTEXT *testContext);
static void cpu_selftest_a_add_adds_operand_to_A(TESTCONTEXT *testContext);
static void cpu_selftest_a_sub_subtracts_operand_from_A(TESTCONTEXT *testContext);
static void cpu_selftest_a_mul_multiplies_operand_by_A(TESTCONTEXT *testContext);
static void cpu_selftest_a_xor(TESTCONTEXT *testContext);
static void cpu_selftest_a_or(TESTCONTEXT *testContext);
static void cpu_selftest_a_shift_shifts_left_for_positive_operand(TESTCONTEXT *testContext);
static void cpu_selftest_a_shift_shifts_right_for_negative_operand(TESTCONTEXT *testContext);
static void cpu_selftest_a_and(TESTCONTEXT *testContext);
static void cpu_selftest_a_rsub_subtracts_A_from_operand(TESTCONTEXT *testContext);
static void cpu_selftest_a_comp_sets_less_than_when_A_less_than_operand(TESTCONTEXT *testContext);
static void cpu_selftest_a_comp_sets_equals_when_A_equals_operand(TESTCONTEXT *testContext);
static void cpu_selftest_a_comp_sets_greater_than_when_A_greater_than_operand(TESTCONTEXT *testContext);

static void cpu_selftest_dec_load_loads_AEX(TESTCONTEXT *testContext);
static void cpu_selftest_dec_stack_and_load_stacks_AEX_and_loads_AEX(TESTCONTEXT *testContext);
static void cpu_selftest_dec_store_stores_AEX(TESTCONTEXT *testContext);
static void cpu_selftest_dec_comp_sets_overflow_when_AOD_and_operand_non_zero(TESTCONTEXT *testContext);
static void cpu_selftest_dec_comp_clears_overflow_when_AOD_and_operand_is_zero(TESTCONTEXT *testContext);

static void cpu_selftest_flt_load_single_loads_32_bits_into_A(TESTCONTEXT *testContext);
static void cpu_selftest_flt_load_double_loads_64_bits_into_A(TESTCONTEXT *testContext);
static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_32_bits(TESTCONTEXT *testContext);
static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_64_bits(TESTCONTEXT *testContext);
static void cpu_selftest_flt_store_stores_A_32_bits(TESTCONTEXT *testContext);
static void cpu_selftest_flt_store_stores_A_64_bits(TESTCONTEXT *testContext);
static void cpu_selftest_flt_xor(TESTCONTEXT *testContext);
static void cpu_selftest_flt_or(TESTCONTEXT *testContext);
static void cpu_selftest_flt_shift_shifts_left_circular_for_positive_operand(TESTCONTEXT *testContext);
static void cpu_selftest_flt_shift_shifts_right_circular_for_negative_operand(TESTCONTEXT *testContext);
static void cpu_selftest_flt_and(TESTCONTEXT *testContext);

static void cpu_selftest_org_relative_jump_jumps_forward(TESTCONTEXT *testContext);
static void cpu_selftest_org_relative_jump_jumps_backward(TESTCONTEXT *testContext);
static void cpu_selftest_org_relative_jump_across_segement_boundary_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_org_exit_resets_link_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_exit_resets_link_except_privileged_ms_bits_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_absolute_jump(TESTCONTEXT *testContext);
static void cpu_selftest_org_return_sets_SF_and_unstacks_link(TESTCONTEXT *testContext);
static void cpu_selftest_org_return_resets_link_except_privileged_ms_bits_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_return_does_not_pop_stack_if_operand_is_not_stack_but_sets_NB(TESTCONTEXT *testContext);
static void cpu_selftest_org_XCn_stacks_operand_and_jumps_to_offset_n(TESTCONTEXT *testContext);
static void cpu_selftest_org_XCn_sets_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_puts_link_on_stack_adding_operand_to_stacked_CO(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_treats_operand_as_signed(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_overflows_segment_boundary(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_underflows_segment_boundary(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_generates_illegal_order_interrupt_if_segment_overflow_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_level0_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_level1_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_ms_load_sets_unmasked_bits_only_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_ms_load_does_not_set_masked_bits_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_ms_load_does_not_set_privileged_unmasked_bits_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_dl_load_sets_dl_pseudo_register(TESTCONTEXT *testContext);
static void cpu_selftest_org_spm_dummy(TESTCONTEXT *testContext);
static void cpu_selftest_org_setlink_stores_link(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_load_loads_XNB(TESTCONTEXT *testContext);
static void cpu_selftest_org_sn_load_in_user_mode_does_not_load_SN(TESTCONTEXT *testContext);
static void cpu_selftest_org_sn_load_in_executive_mode_loads_SN(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_adds_operand_to_XNB(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_generates_interrupt_if_segment_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_generates_interrupt_if_segment_underflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_generates_illegal_order_interrupt_if_segment_overflow_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_level0_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_level1_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_store_stores_XNB(TESTCONTEXT *testContext);
static void cpu_selftest_org_xnb_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_load_loads_SF(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_plus_adds_operand_to_SF(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_load_nb_plus_adds_NB_to_signed_operand_and_stores_to_SF(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_store_stores_SF(TESTCONTEXT *testContext);
static void cpu_selftest_org_sf_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_load_loads_NB(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_load_sf_plus_adds_SF_to_signed_operand_and_stores_to_NB(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_plus_adds_signed_operand_to_NB(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_store_stores_SN_and_NB(TESTCONTEXT *testContext);
static void cpu_selftest_org_nb_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext);
static uint16 cpu_selftest_calculate_ms_from_t_bits(uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_branch_test_branch_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_branch_test_branch_not_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_bn_test_true_result(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_bn_test_false_result(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_br_eq_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_eq_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_ne_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_ne_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_ge_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_ge_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_lt_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_lt_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_le_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_le_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_gt_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_gt_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_ovf_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_ovf_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_bn_does_not_branch_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_br_bn_does_branch_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_function_tests(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_eq_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_eq_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_ne_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_ne_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_ge_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_ge_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_lt_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_lt_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_le_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_le_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_gt_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_gt_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_ovf_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_ovf_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_bn_on_false(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_bn_on_true(TESTCONTEXT *testContext);
static void cpu_selftest_org_bn_order_tests(TESTCONTEXT *testContext);

static void cpu_selftest_setting_b_or_d_fault_in_executive_mode_generates_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_b_or_d_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_b_fault_in_user_mode_generates_program_fault_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_b_fault_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_bod_b_overflow_in_executive_mode_generates_b_or_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_bod_b_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_bod_b_overflow_in_user_mode_generates_b_program_fault_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_bod_b_overflow_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_clearing_bod_b_overflow_in_executive_mode_does_not_clear_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_clearing_bod_b_overflow_in_user_mode_does_not_clear_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_setting_acc_fault_in_executive_mode_generates_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_acc_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_acc_fault_in_user_mode_generates_program_fault_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_acc_fault_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_floating_point_overflow_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_floating_point_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_floating_point_underflow_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_floating_point_underflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_decimal_overflow_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_fixed_point_overflow_in_executive_generates_acc_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_fixed_point_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_decimal_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_zero_divide_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_aod_zero_divide_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_clearing_acc_fault_in_executive_mode_does_not_clear_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_clearing_acc_fault_in_user_mode_does_not_clear_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_operand_size_in_aod_does_not_trigger_interrupt_evaluation(TESTCONTEXT *testContext);

static void cpu_selftest_setting_d_fault_in_executive_mode_generates_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_d_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_d_fault_in_user_mode_generates_program_fault_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_d_fault_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_xchk_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_illegal_type_size_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_executive_mode_subtype_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_short_source_string_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_short_source_string_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_non_zero_truncation_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_non_zero_truncation_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_bounds_check_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_bounds_check_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_setting_dod_write_to_read_only_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_clearing_d_fault_in_executive_mode_does_not_clear_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_clearing_d_fault_in_user_mode_does_not_clear_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_switch_from_executive_mode_to_user_mode_when_system_error_does_not_create_program_fault(TESTCONTEXT *testContext);
static void cpu_selftest_switch_from_user_mode_to_executive_mode_program_fault_does_not_create_system_error(TESTCONTEXT *testContext);
static void cpu_selftest_return_from_interrupt_handler_without_clearing_system_error_status_v_line_masks_system_error_interrupts(TESTCONTEXT *testContext);

static void cpu_selftest_illegal_function_generates_system_error_interrupt_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_illegal_function_generates_system_error_interrupt_if_L1IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_illegal_function_generates_system_error_interrupt_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_illegal_function_generates_illegal_order_interrupt_in_user_mode(TESTCONTEXT *testContext);

static void cpu_selftest_name_adder_overflow_generates_system_error_interrupt_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_name_adder_overflow_generates_system_error_interrupt_if_L1IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_name_adder_overflow_generates_system_error_interrupt_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_name_adder_overflow_generates_illegal_order_interrupt_in_user_mode(TESTCONTEXT *testContext);

static void cpu_selftest_control_adder_overflow_generates_system_error_interrupt_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_control_adder_overflow_generates_system_error_interrupt_if_L1IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_control_adder_overflow_generates_system_error_interrupt_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_control_adder_overflow_generates_illegal_order_interrupt_in_user_mode(TESTCONTEXT *testContext);

static void cpu_selftest_cpr_not_equivalence_interrupt_in_executive_mode_if_L0IF_is_clear(TESTCONTEXT *testContext);
static void cpu_selftest_cpr_not_equivalence_generates_system_error_interrupt_in_executive_mode_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_cpr_not_equivalence_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_cpr_not_equivalence_generates_system_error_interrupt_in_user_mode_if_L0IF_is_set(TESTCONTEXT *testContext);

static void cpu_selftest_spm_bit_in_program_fault_interrupt_status_causes_program_fault_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_system_error_interrupt_not_inhibited_even_if_L0IF_or_L1IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_cpr_not_equivalence_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_exchange_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_peripheral_window_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_level_0_interrupt_not_inhibited_if_L1IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_level_1_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext);
static void cpu_selftest_level_1_interrupt_inhibited_if_L1IF_is_set(TESTCONTEXT *testContext);

static void cpu_selftest_cpr_not_equivalance_interrupt_on_order_fetch_stores_link_that_re_executes_failed_order(TESTCONTEXT *testContext);
static void cpu_selftest_cpr_not_equivalance_interrupt_on_primary_operand_stores_link_that_re_executes_failed_order(TESTCONTEXT *testContext);
static void cpu_selftest_cpr_not_equivalance_interrupt_on_secondary_operand_stores_link_that_re_executes_failed_order(TESTCONTEXT *testContext);

static void cpu_selftest_no_b_overflow_interrupt_if_b_overflow_is_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_no_acc_zero_divide_interrupt_if_acc_zero_divide_is_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_no_sss_interrupt_if_sss_is_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_D_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_no_D_interrupt_if_inhibited_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_D_interrupt_as_program_fault_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_no_D_interrupt_if_inhibited_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_B_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_no_B_interrupt_if_inhibited_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_B_interrupt_as_program_fault_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_acc_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_no_acc_interrupt_if_inhibited_in_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_acc_interrupt_as_program_fault_in_user_mode(TESTCONTEXT *testContext);
static void cpu_selftest_no_acc_interrupt_if_inhibited_in_user_mode(TESTCONTEXT *testContext);

static void cpu_selftest_interrupt_stacks_link_in_system_v_store(TESTCONTEXT *testContext);
static void cpu_selftest_interrupt_calls_handler_using_link_in_system_v_store(TESTCONTEXT *testContext);
static void cpu_selftest_interrupt_sets_executive_mode(TESTCONTEXT *testContext);
static void cpu_selftest_interrupt_sequence_clears_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_interrupt_sequence_does_not_clear_interrupt_when_handling_another_interrupt(TESTCONTEXT *testContext);

static void cpu_selftest_write_to_prop_program_fault_status_resets_it(TESTCONTEXT *testContext);
static void cpu_selftest_write_to_prop_system_error_status_resets_it(TESTCONTEXT *testContext);

static void cpu_selftest_read_and_write_instruction_counter(TESTCONTEXT *testContext);
static void cpu_selftest_executing_instruction_decrements_instruction_counter(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_counter_not_decremented_if_inhibited(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_counter_not_decremented_if_already_zero(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_counter_zero_generates_interrupt(TESTCONTEXT *testContext);
static void cpu_selftest_instruction_counter_already_zero_does_not_generate_new_interrupt(TESTCONTEXT *testContext);

CONDITIONTABLE conditionalFuncsTable[] =
{
    { 0x0, 0, 0, 0 },
    { 0x0, 0, 1, 0 },
    { 0x0, 1, 0, 0 },
    { 0x0, 1, 1, 0 },

    { 0x1, 0, 0, 0 },
    { 0x1, 0, 1, 0 },
    { 0x1, 1, 0, 0 },
    { 0x1, 1, 1, 1 },

    { 0x2, 0, 0, 0 },
    { 0x2, 0, 1, 1 },
    { 0x2, 1, 0, 0 },
    { 0x2, 1, 1, 0 },

    { 0x3, 0, 0, 0 },
    { 0x3, 0, 1, 1 },
    { 0x3, 1, 0, 0 },
    { 0x3, 1, 1, 1 },

    { 0x4, 0, 0, 0 },
    { 0x4, 0, 1, 0 },
    { 0x4, 1, 0, 1 },
    { 0x4, 1, 1, 0 },

    { 0x5, 0, 0, 0 },
    { 0x5, 0, 1, 0 },
    { 0x5, 1, 0, 1 },
    { 0x5, 1, 1, 1 },

    { 0x6, 0, 0, 0 },
    { 0x6, 0, 1, 1 },
    { 0x6, 1, 0, 1 },
    { 0x6, 1, 1, 0 },

    { 0x7, 0, 0, 0 },
    { 0x7, 0, 1, 1 },
    { 0x7, 1, 0, 1 },
    { 0x7, 1, 1, 1 },

    { 0x8, 0, 0, 1 },
    { 0x8, 0, 1, 0 },
    { 0x8, 1, 0, 0 },
    { 0x8, 1, 1, 0 },

    { 0x9, 0, 0, 1 },
    { 0x9, 0, 1, 0 },
    { 0x9, 1, 0, 0 },
    { 0x9, 1, 1, 1 },

    { 0xA, 0, 0, 1 },
    { 0xA, 0, 1, 1 },
    { 0xA, 1, 0, 0 },
    { 0xA, 1, 1, 0 },

    { 0xB, 0, 0, 1 },
    { 0xB, 0, 1, 1 },
    { 0xB, 1, 0, 0 },
    { 0xB, 1, 1, 1 },

    { 0xC, 0, 0, 1 },
    { 0xC, 0, 1, 0 },
    { 0xC, 1, 0, 1 },
    { 0xC, 1, 1, 0 },

    { 0xD, 0, 0, 1 },
    { 0xD, 0, 1, 0 },
    { 0xD, 1, 0, 1 },
    { 0xD, 1, 1, 1 },

    { 0xE, 0, 0, 1 },
    { 0xE, 0, 1, 1 },
    { 0xE, 1, 0, 1 },
    { 0xE, 1, 1, 0 },

    { 0xF, 0, 0, 1 },
    { 0xF, 0, 1, 1 },
    { 0xF, 1, 0, 1 },
    { 0xF, 1, 1, 1 }
};

static UNITTEST tests[] =
{
    { "16-bit instruction fetches using obey access", cpu_selftest_16_bit_instruction_fetches_using_obey_access },
    { "16-bit instruction generates instruction access violation when fetching from a non-executable segment", cpu_selftest_16_bit_instruction_generates_instruction_access_violation_when_fetching_from_non_executable_segment },
    { "Instruction not obeyed if there is an access violation fetching the instruction", cpu_selftest_instruction_not_obeyed_if_access_violation_when_fetching_instruction },
    { "Instruction fetches extended literal using obey access", cpu_selftest_instruction_fetches_extended_literal_using_obey_access },
    { "Instruction fetches extended operand offset from SF using obey access", cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_sf_using_obey_access },
    { "Instruction fetches extended operand offset from Zero using obey access", cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_zero_using_obey_access },
    { "Instruction fetches extended operand offset from NB using obey access", cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_nb_using_obey_access },
    { "Instruction fetches extended operand offset from XNB using obey access", cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_xnb_using_obey_access },
    { "Instruction generates instruction access violation when fetching extended literal from a non-executable segment", cpu_selftest_instruction_access_violation_when_fetching_extended_literal_from_non_executable_segment },

    { "16-bit instruction advances CO by 1", cpu_selftest_16_bit_instruction_advances_co_by_1 },
    { "32-bit instruction advances CO by 2", cpu_selftest_32_bit_instruction_advances_co_by_2 },
    { "48-bit instruction advances CO by 3", cpu_selftest_48_bit_instruction_advances_co_by_3 },
    { "80-bit instruction advances CO by 5", cpu_selftest_80_bit_instruction_advances_co_by_5 },
    { "Dummy order advances CO over operand", cpu_selftest_dummy_order_advances_co_over_operand },
    { "Instruction with get operand that advances CO across a segment boundary generates an interrupt", cpu_selftest_advancing_co_across_segment_boundary_with_get_operand_generates_interrupt },
    { "Instruction with set operand that advances CO across a segment boundary generates an interrupt", cpu_selftest_advancing_co_across_segment_boundary_with_set_operand_generates_interrupt },

    { "Load operand 6-bit positive literal", cpu_selftest_load_operand_6_bit_positive_literal },
    { "Load operand 6-bit negative literal", cpu_selftest_load_operand_6_bit_negative_literal },

    { "Load operand internal register 0", cpu_selftest_load_operand_internal_register_0 },
    { "Load operand internal register 1", cpu_selftest_load_operand_internal_register_1 },
    { "Load operand internal register 2", cpu_selftest_load_operand_internal_register_2 },
    { "Load operand internal register 3", cpu_selftest_load_operand_internal_register_3 },
    { "Load operand internal register 4", cpu_selftest_load_operand_internal_register_4 },
    { "Load operand internal register 16", cpu_selftest_load_operand_internal_register_16 },
    { "Load operand internal register 17", cpu_selftest_load_operand_internal_register_17 },
    { "Load operand internal register 18", cpu_selftest_load_operand_internal_register_18 },
    { "Load operand internal register 19", cpu_selftest_load_operand_internal_register_19 },
    { "Load operand internal register 20", cpu_selftest_load_operand_internal_register_20 },
    { "Load operand internal register 32", cpu_selftest_load_operand_internal_register_32 },
    { "Load operand internal register 33", cpu_selftest_load_operand_internal_register_33 },
    { "Load operand internal register 34", cpu_selftest_load_operand_internal_register_34 },
    { "Load operand internal register 48", cpu_selftest_load_operand_internal_register_48 },
    { "Load operand from non-existent internal register", cpu_selftest_load_operand_non_existent_internal_register },

    { "Load operand 32-bit variable", cpu_selftest_load_operand_32_bit_variable },
    { "Load operand 32-bit variable 6-bit offset is unsigned", cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned },
    { "Load operand 64-bit variable", cpu_selftest_load_operand_64_bit_variable },
    { "Load operand via B-relative descriptor loads D", cpu_selftest_load_operand_b_relative_descriptor_loads_D },
    { "Load operand 32-bit via B-relative descriptor at 6-bit offset for k=4", cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4 },
    { "Load operand 32-bit via B-relative descriptor at 6-bit offset for k=5", cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5 },
    { "Load operand 64-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset },
    { "Load operand 16-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset },
    { "Load operand 8-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset },
    { "Load operand 4-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset },
    { "Load operand 1-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset },
    { "Load operand via B-relative descriptor with a negative modifier generates a bounds check", cpu_selftest_load_operand_b_relative_descriptor_with_negative_modifier_generates_bounds_check },
    { "Load operand via B-relative descriptor with a modifier greater than the bound generates a bounds check", cpu_selftest_load_operand_b_relative_descriptor_modifier_greater_than_bound_generates_bounds_check },
    { "Load operand via 0-relative descriptor loads D", cpu_selftest_load_operand_zero_relative_descriptor_loads_D },
    { "Load operand 64-bit via 0-relative descriptor at 6-bit offset", cpu_selftest_load_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset },

    { "Load operand 16-bit signed positive literal", cpu_selftest_load_operand_16_bit_signed_positive_literal },
    { "Load operand 16-bit signed negative literal", cpu_selftest_load_operand_16_bit_signed_negative_literal },
    { "Load operand 32-bit signed positive literal", cpu_selftest_load_operand_32_bit_signed_positive_literal },
    { "Load operand 32-bit signed negative literal", cpu_selftest_load_operand_32_bit_signed_negative_literal },
    { "Load operand 64-bit literal n'=2", cpu_selftest_load_operand_64_bit_literal_np_2 },
    { "Load operand 64-bit literal n'=3", cpu_selftest_load_operand_64_bit_literal_np_3 },
    { "Load operand 16-bit unsigned literal", cpu_selftest_load_operand_16_bit_unsigned_literal },
    { "Load operand 32-bit unsigned literal", cpu_selftest_load_operand_32_bit_unsigned_literal },
    { "Load operand 64-bit literal n'=6", cpu_selftest_load_operand_64_bit_literal_np_6 },
    { "Load operand 64-bit literal n'=7", cpu_selftest_load_operand_64_bit_literal_np_7 },
    { "Load operand extended literal k'=1", cpu_selftest_load_operand_extended_literal_kp_1 },

    { "Load operand 32-bit variable extended offset from stack", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_sf },
    { "Load operand 32-bit variable extended offset from zero", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_zero },
    { "Load operand 32-bit variable extended offset from NB", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb },
    { "Load operand 32-bit variable extended offset from XNB", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb },
    { "Load operand 32-bit variable extended from stack", cpu_selftest_load_operand_extended_32_bit_variable_from_stack },
    { "Load operand 32-bit variable extended from D generates interrupt", cpu_selftest_load_operand_extended_32_bit_variable_from_d_generates_interrupt },
    { "Load operand 32-bit variable extended from (NB)", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref },
    { "Load operand 32-bit variable extended from (XNB)", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref },

    { "Load operand 64-bit variable extended offset from stack", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_sf },
    { "Load operand 64-bit variable extended offset from zero", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero },
    { "Load operand 64-bit variable extended offset from NB", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb },
    { "Load operand 64-bit variable extended offset from XNB", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb },
    { "Load operand 64-bit variable extended from stack", cpu_selftest_load_operand_extended_64_bit_variable_from_stack },
    { "Load operand 64-bit variable extended from D generates interrupt", cpu_selftest_load_operand_extended_64_bit_variable_from_d_generates_interrupt },
    { "Load operand 64-bit variable extended from (NB)", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref },
    { "Load operand 64-bit variable extended from (XNB)", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref },

    { "Load operand 32-bit extended from b-relative descriptor from SF for kp=4", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4 },
    { "Load operand 32-bit extended from b-relative descriptor from SF for kp=5", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5 },
    { "Load operand 32-bit extended from b-relative descriptor from zero", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_zero },
    { "Load operand 32-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb },
    { "Load operand 32-bit extended from b-relative descriptor from XNB", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb },
    { "Load operand 32-bit extended from b-relative descriptor from stack", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_stack },
    { "Load operand 32-bit extended from b-relative descriptor from D", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_dr },
    { "Load operand extended from b-relative descriptor from D does not load D", cpu_selftest_load_operand_extended_b_relative_descriptor_from_dr_does_not_load_D },
    { "Load operand 32-bit extended from b-relative descriptor from (NB)", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref },
    { "Load operand 32-bit extended from b-relative descriptor from (XNB)", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref },
    { "Load operand 64-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_64_bit_value_from_nb },
    { "Load operand 16-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_16_bit_value_from_nb },
    { "Load operand 8-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_8_bit_value_from_nb },
    { "Load operand 4-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_4_bit_value_from_nb },
    { "Load operand 1-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_1_bit_value_from_nb },

    { "Load operand 32-bit extended from 0-relative descriptor from SF", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_sf },
    { "Load operand 32-bit extended from 0-relative descriptor from zero", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_zero },
    { "Load operand 32-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb },
    { "Load operand 32-bit extended from 0-relative descriptor from XNB", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb },
    { "Load operand 32-bit extended from 0-relative descriptor from stack", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_stack },
    { "Load operand 32-bit extended from 0-relative descriptor from D", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_dr },
    { "Load operand extended from 0-relative descriptor from D", cpu_selftest_load_operand_extended_zero_relative_descriptor_from_dr_does_not_load_D },
    { "Load operand 32-bit extended from 0-relative descriptor from (NB)", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref },
    { "Load operand 32-bit extended from 0-relative descriptor from (XNB)", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref },
    { "Load operand 64-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_64_bit_value_from_nb },
    { "Load operand 16-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_16_bit_value_from_nb },
    { "Load operand 8-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_8_bit_value_from_nb },
    { "Load operand 4-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_4_bit_value_from_nb },
    { "Load operand 1-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_1_bit_value_from_nb },
    { "Load operand with invalid descriptor element size generates ITS interrupt", cpu_selftest_load_operand_with_invalid_descriptor_element_size_generates_its_interrupt },
    { "Load privileged operand reads the V-Store in executive mode", cpu_selftest_load_operand_privileged_reads_v_store_in_executive_mode },
    { "Load privileged operand generates interrupt in user mode", cpu_selftest_load_operand_privileged_generates_interrupt_in_user_mode },

    { "Store operand 6-bit literal generates interrupt", cpu_selftest_store_operand_6_bit_literal_generates_interrupt },

    { "Store operand internal register 0 generates an illegal order interrupt in user mode", cpu_selftest_store_operand_internal_register_0_generates_interrupt_as_illegal_order_in_user_mode },
    { "Store operand internal register 0 generates a system error interrupt in executive mode", cpu_selftest_store_operand_internal_register_0_generates_interrupt_as_system_error_in_executive_mode },
    { "Store operand internal register 1 generates an interrupt", cpu_selftest_store_operand_internal_register_1_generates_interrupt },
    { "Store operand internal register 2 generates an interrupt", cpu_selftest_store_operand_internal_register_2_generates_interrupt },
    { "Store operand internal register 3 generates an interrupt", cpu_selftest_store_operand_internal_register_3_generates_interrupt },
    { "Store operand internal register 4 generates an interrupt", cpu_selftest_store_operand_internal_register_4_generates_interrupt },
    { "Store operand to non-existent internal register in prop generates an interrupt", cpu_selftest_store_operand_non_existent_prop_internal_register_generates_interrupt },
    { "Store operand internal register 16", cpu_selftest_store_operand_internal_register_16 },
    { "Store operand internal register 17", cpu_selftest_store_operand_internal_register_17 },
    { "Store operand internal register 18", cpu_selftest_store_operand_internal_register_18 },
    { "Store operand internal register 19", cpu_selftest_store_operand_internal_register_19 },
    { "Store operand internal register 20", cpu_selftest_store_operand_internal_register_20 },
    { "Store operand internal register 32", cpu_selftest_store_operand_internal_register_32 },
    { "Store operand internal register 33", cpu_selftest_store_operand_internal_register_33 },
    { "Store operand internal register 34", cpu_selftest_store_operand_internal_register_34 },
    { "Store operand internal register 48", cpu_selftest_store_operand_internal_register_48 },
    { "Store operand to non-existent internal register", cpu_selftest_store_operand_non_existent_internal_register },

    { "Store operand 32-bit variable", cpu_selftest_store_operand_32_bit_variable },
    { "Store operand 64-bit variable", cpu_selftest_store_operand_64_bit_variable },
    { "Store operand 32-bit via B-relative descriptor loads D", cpu_selftest_store_operand_b_relative_descriptor_loads_D },
    { "Store operand 32-bit via B-relative descriptor at 6-bit offset for k=4", cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4 },
    { "Store operand 32-bit via B-relative descriptor at 6-bit offset for k=5", cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5 },
    { "Store operand 64-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset },
    { "Store operand 16-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset },
    { "Store operand 8-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset },
    { "Store operand 4-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset },
    { "Store operand 1-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset },
    { "Store operand 64-bit via 0-relative descriptor loads D", cpu_selftest_store_operand_zero_relative_descriptor_loads_D },
    { "Store operand 64-bit via 0-relative descriptor at 6-bit offset", cpu_selftest_store_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset },

    { "Store operand to extended literal generates interrupt, k'=0", cpu_selftest_store_operand_extended_literal_kp_0_generates_interrupt },
    { "Store operand to extended literal generates interrupt, k'=1", cpu_selftest_store_operand_extended_literal_kp_1_generates_interrupt },

    { "Store operand 32-bit variable extended offset from stack", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf },
    { "Store operand 32-bit variable extended offset from zero", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero },
    { "Store operand 32-bit variable extended offset from NB", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb },
    { "Store operand 32-bit variable extended offset from XNB", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb },
    { "Store operand 32-bit variable extended from stack", cpu_selftest_store_operand_extended_32_bit_variable_from_stack },
    { "Store operand 32-bit variable extended from D generates interrupt", cpu_selftest_store_operand_extended_32_bit_variable_from_d_generates_interrupt },
    { "Store operand 32-bit variable extended from (NB)", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb_ref },
    { "Store operand 32-bit variable extended from (XNB)", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref },

    { "Store operand 64-bit variable extended offset from stack", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf },
    { "Store operand 64-bit variable extended offset from zero", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero },
    { "Store operand 64-bit variable extended offset from NB", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb },
    { "Store operand 64-bit variable extended offset from XNB", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb },
    { "Store operand 64-bit variable extended from stack", cpu_selftest_store_operand_extended_64_bit_variable_from_stack },
    { "Store operand 64-bit variable extended from D generates interrupt", cpu_selftest_store_operand_extended_64_bit_variable_from_d_generates_interrupt },
    { "Store operand 64-bit variable extended from (NB)", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb_ref },
    { "Store operand 64-bit variable extended from (XNB)", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref },

    { "Store operand 32-bit extended from b-relative descriptor from SF for kp=4", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4 },
    { "Store operand 32-bit extended from b-relative descriptor from SF for kp=5", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5 },
    { "Store operand 32-bit extended from b-relative descriptor from zero", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_zero },
    { "Store operand 32-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb },
    { "Store operand 32-bit extended from b-relative descriptor from XNB", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb },
    { "Store operand 32-bit extended from b-relative descriptor from stack", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_stack },
    { "Store operand 32-bit extended from b-relative descriptor from D", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_dr },
    { "Store operand extended from b-relative descriptor from D does not load D", cpu_selftest_store_operand_extended_b_relative_descriptor_from_dr_does_not_load_D },
    { "Store operand 32-bit extended from b-relative descriptor from (NB)", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref },
    { "Store operand 32-bit extended from b-relative descriptor from (XNB)", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref },
    { "Store operand 64-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_64_bit_value_from_nb },
    { "Store operand 16-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_16_bit_value_from_nb },
    { "Store operand 8-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_8_bit_value_from_nb },
    { "Store operand 4-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_4_bit_value_from_nb },
    { "Store operand 1-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_1_bit_value_from_nb },

    { "Store operand 32-bit extended from 0-relative descriptor from SF", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_sf },
    { "Store operand 32-bit extended from 0-relative descriptor from zero", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_zero },
    { "Store operand 32-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb },
    { "Store operand 32-bit extended from 0-relative descriptor from XNB", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb },
    { "Store operand 32-bit extended from 0-relative descriptor from stack", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_stack },
    { "Store operand 32-bit extended from 0-relative descriptor from D", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_dr },
    { "Store operand extended from 0-relative descriptor from D does not load D", cpu_selftest_store_operand_extended_zero_relative_descriptor_from_dr_does_not_load_D },
    { "Store operand 32-bit extended from 0-relative descriptor from (NB)", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref },
    { "Store operand 32-bit extended from 0-relative descriptor from (XNB)", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref },
    { "Store operand 64-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_64_bit_value_from_nb },
    { "Store operand 16-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_16_bit_value_from_nb },
    { "Store operand 8-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_8_bit_value_from_nb },
    { "Store operand 4-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_4_bit_value_from_nb },
    { "Store operand 1-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_1_bit_value_from_nb },
    { "Store operand with invalid descriptor element size generates ITS interrupt", cpu_selftest_store_operand_with_invalid_descriptor_element_size_generates_its_interrupt },
    { "Store privileged operand storess the V-Store in executive mode", cpu_selftest_store_operand_privileged_stores_v_store_in_executive_mode },
    { "Store privileged operand generates interrupt in user mode", cpu_selftest_store_operand_privileged_generates_interrupt_in_user_mode },

    { "Any descriptor modify generates an ITS interrupt if the descriptor has an invalid size", cpu_selftest_any_descriptor_modify_generates_its_interrupt_if_descriptor_has_invalid_size },

    { "STS1 XDO Load Loads LS half of XD", cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD },
    { "STS1 XD Load Loads whole of XD", cpu_selftest_sts1_xd_load_loads_whole_of_XD },
    { "STS1 STACK stacks operand", cpu_selftest_sts1_stack_stacks_operand },
    { "STS1 XD Store stores XD to operand", cpu_selftest_sts1_xd_store_stores_xd_to_operand },
    { "STS1 XD Store to secondary operand generates interrupt", cpu_selftest_sts1_xd_store_to_secondary_operand_generates_interrupt },
    { "STS1 XDB Load loads the bound in XD", cpu_selftest_sts1_xdb_load_loads_bound_in_XD },
    { "STS1 XCHK clears DOD XCH bit if operand is negative", cpu_selftest_sts1_xchk_operand_negative_clears_DOD_XCH_bit },
    { "STS1 XCHK clears DOD XCH bit if operand is >= bound", cpu_selftest_sts1_xchk_operand_ge_XDB_clears_DOD_XCH_bit },
    { "STS1 XCHK sets DOD XCH bit if operand is within XD bound", cpu_selftest_sts1_xchk_operand_within_XDB_sets_DOD_XCH_bit },
    { "SMOD adds signed operand to D origin", cpu_selftest_sts1_smod_adds_signed_operand_to_D_origin },
    { "SMOD scales the modifier for type 0 descriptors", cpu_selftest_sts1_smod_scales_modifier_for_type_0 },
    { "SMOD scales the modifier for type 1 descriptors", cpu_selftest_sts1_smod_scales_modifier_for_type_1 },
    { "SMOD scales the modifier for type 2 descriptors", cpu_selftest_sts1_smod_scales_modifier_for_type_2 },
        /* TODO: add tests for type 3 descriptors */
    { "SMOD does not scale the modifier for type 0 descriptors when US set", cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_0_when_US_set },
    { "SMOD does not scale the modifier for type 2 descriptors when US set", cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_2_when_US_set },
    { "SMOD does not check bounds", cpu_selftest_sts1_smod_does_not_check_bounds },
    { "XMOD adds signed operand to XD origin and subtracts it from the bound", cpu_selftest_sts1_xmod_adds_signed_operand_to_XD_origin_subtracts_from_bound },
    { "XMOD scales modifier for 64-bit value", cpu_selftest_sts1_xmod_scales_modifier_for_64_bit_value },
    { "XMOD scales modifier for 32-bit value", cpu_selftest_sts1_xmod_scales_modifier_for_32_bit_value },
    { "XMOD scales modifier for 16-bit value", cpu_selftest_sts1_xmod_scales_modifier_for_16_bit_value },
    { "XMOD scales modifier for 8-bit value", cpu_selftest_sts1_xmod_scales_modifier_for_8_bit_value },
    { "XMOD scales modifier for 4-bit value within a byte", cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_within_byte },
    { "XMOD scales modifier for 4-bit value crossing a byte", cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_crossing_byte },
    { "XMOD scales modifier for 1-bit value within a byte", cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_within_byte },
    { "XMOD scales modifier for 1-bit value crossing a byte", cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_crossing_byte },
    { "XMOD checks bounds for type 0 descriptor", cpu_selftest_sts1_xmod_checks_bounds_for_type_0 },
    { "XMOD checks bounds for type 1 descriptor", cpu_selftest_sts1_xmod_checks_bounds_for_type_1 },
    { "XMOD checks bounds for type 2 descriptor", cpu_selftest_sts1_xmod_checks_bounds_for_type_2 },
    { "XMOD does not check bounds for type 0 descriptor when BC is set", cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_0_when_BC_set },
    { "XMOD does not check bounds for type 1 descriptor when BC is set", cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_1_when_BC_set },
    { "XMOD does not check bounds for type 2 descriptor when BC is set", cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_2_when_BC_set },
    { "XMOD generates ITS interrupt if illegal descriptor type is used", cpu_selftest_sts1_xmod_generates_its_interrupt_if_illegal_descriptor_type_used },
    { "SLGC generates ITS interrupt if source is not 8-bit", cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_not_8_bit },
    { "SLGC generates ITS interrupt if destination is not 8-bit", cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_not_8_bit },
    { "SLGC generates ITS interrupt if source is type 3", cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_is_type_3 },
    { "SLGC generates ITS interrupt if destination is type 3", cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_is_type_3 },
    { "SLGC processes type 0 descriptors", cpu_selftest_sts1_slgc_processes_type_0_descriptors },
    { "SLGC processes type 2 descriptors", cpu_selftest_sts1_slgc_processes_type_2_descriptors },
    { "SLGC processes long vector", cpu_selftest_sts1_slgc_processes_long_vector },
    { "SLGC generates SSS interrupt if source runs out", cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out },
    { "SLGC processes L0", cpu_selftest_sts1_slgc_processes_L0 },
    { "SLGC processes L1", cpu_selftest_sts1_slgc_processes_L1 },
    { "SLGC processes L2", cpu_selftest_sts1_slgc_processes_L2 },
    { "SLGC processes L3", cpu_selftest_sts1_slgc_processes_L3 },
    { "SMVB generates ITS interrupt if source is not 8-bit", cpu_selftest_sts1_smvb_generates_its_interrupt_if_source_not_8_bit },
    { "SMVB generates ITS interrupt if destination is not 8-bit", cpu_selftest_sts1_smvb_generates_its_interrupt_if_destination_not_8_bit },
    { "SMVB generates bounds check if destination bound is zero", cpu_selftest_sts1_smvb_generates_checks_bound_on_destination_not_0 },
    { "SMVB copies byte from source, uses mask", cpu_selftest_sts1_smvb_copies_byte_with_mask },
    { "SMVB uses filler when source is empty, uses mask", cpu_selftest_sts1_smvb_uses_filler_when_source_empty_with_mask },
    { "SMVF generates ITS interrupt if source is not 8-bit", cpu_selftest_sts1_smvf_generates_its_interrupt_if_source_not_8_bit },
    { "SMVF generates ITS interrupt if destination is not 8-bit", cpu_selftest_sts1_smvf_generates_its_interrupt_if_destination_not_8_bit },
    { "SMVFcopies to zero length destination", cpu_selftest_sts1_smvf_copies_to_zero_length_destination },
    { "SMVF copies byte from source, uses mask", cpu_selftest_sts1_smvf_copies_bytes_and_fills_with_mask },
    { "TALU returns test register > if entry not found", cpu_selftest_sts1_talu_returns_test_register_greater_than_if_not_found },
    { "TALU returns test register = if entry found in type 0 vector", cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_0 },
    { "TALU returns test register = if entry found in type 2 vector", cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_2 },
    { "TALU generates an ITS interrupt if the descriptor is not 32-bit", cpu_selftest_sts1_talu_generates_its_interrupt_if_descriptor_is_not_32_bit },
    { "SCMP generates ITS interrupt if source is not 8-bit", cpu_selftest_sts1_scmp_generates_its_interrupt_if_source_not_8_bit },
    { "SCMP generates ITS interrupt if destination is not 8-bit", cpu_selftest_sts1_scmp_generates_its_interrupt_if_destination_not_8_bit },
    { "SCMP returns test register = if strings identical", cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical },
    { "SCMP returns test register = if strings identical with filler for source", cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical_with_filler },
    { "SCMP returns test register = if strings identical when destination runs out", cpu_selftest_sts1_scmp_returns_test_register_equals_if_destination_is_shorter_and_subset_is_equal },
    { "SCMP returns test register > if source > destination", cpu_selftest_sts1_scmp_returns_test_register_greater_than_if_source_byte_greater_than_destination_byte },
    { "SCMP  returns test register < if source < destination", cpu_selftest_sts1_scmp_returns_test_register_less_than_if_source_byte_less_than_destination_byte },
    { "SUB1 loads XD and modifies it", cpu_selftest_sts1_sub1_loads_XD_and_modifies_it },
    { "SUB1 calculates B", cpu_selftest_sts1_sub1_calculates_B },
    { "SUB1 modifies D", cpu_selftest_sts1_sub1_modifies_D },
    { "SUB1 generates ITS interrupt if the descriptor is not valid", cpu_selftest_sts1_sub1_generates_its_interrupt_if_descriptor_is_not_valid },

    { "STS2 DO Load Loads LS half of D", cpu_selftest_sts2_do_load_loads_ls_half_of_D },
    { "STS2 D Load Loads whole of D", cpu_selftest_sts2_d_load_loads_whole_of_D },
    { "STS2 D stack load stacks D and then loads a new value for D", cpu_selftest_sts2_d_stack_load_stacks_D_loads_new_D },
    { "STS2 D Store stores D to operand", cpu_selftest_sts2_d_store_stores_d_to_operand },
    { "STS2 D Store to secondary operand generates interrupt", cpu_selftest_sts2_d_store_to_secondary_operand_generates_interrupt },
    { "STS2 DB Load loads the bound in D", cpu_selftest_sts2_db_load_loads_bound_in_D },
    { "STS2 MDR advances location and loads D with operand pointed to by D", cpu_selftest_sts2_mdr_advances_location_and_loads_D_with_operand_pointed_to_by_D },
    { "MOD adds signed operand to D origin and subtracts it from the bound", cpu_selftest_sts2_mod_adds_signed_operand_to_D_origin_subtracts_from_bound },
    { "MOD scales modifier for 64-bit value", cpu_selftest_sts2_mod_scales_modifier_for_64_bit_value },
    { "MOD scales modifier for 32-bit value", cpu_selftest_sts2_mod_scales_modifier_for_32_bit_value },
    { "MOD scales modifier for 16-bit value", cpu_selftest_sts2_mod_scales_modifier_for_16_bit_value },
    { "MOD scales modifier for 8-bit value", cpu_selftest_sts2_mod_scales_modifier_for_8_bit_value },
    { "MOD scales modifier for 4-bit value within a byte", cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_within_byte },
    { "MOD scales modifier for 4-bit value crossing a byte", cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_crossing_byte },
    { "MOD scales modifier for 1-bit value within a byte", cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_within_byte },
    { "MOD scales modifier for 1-bit value crossing a byte", cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_crossing_byte },
    { "MOD checks bounds for type 0 descriptor", cpu_selftest_sts2_mod_checks_bounds_for_type_0 },
    { "MOD checks bounds for type 1 descriptor", cpu_selftest_sts2_mod_checks_bounds_for_type_1 },
    { "MOD checks bounds for type 2 descriptor", cpu_selftest_sts2_mod_checks_bounds_for_type_2 },
    { "MOD does not check bounds for type 0 descriptor when BC is set", cpu_selftest_sts2_mod_does_not_check_bounds_for_type_0_when_BC_set },
    { "MOD does not check bounds for type 1 descriptor when BC is set", cpu_selftest_sts2_mod_does_not_check_bounds_for_type_1_when_BC_set },
    { "MOD does not check bounds for type 2 descriptor when BC is set", cpu_selftest_sts2_mod_does_not_check_bounds_for_type_2_when_BC_set },
    { "RMOD loads least significant half of D and adds to origin", cpu_selftest_sts2_rmod_loads_least_significant_half_of_D_and_adds_to_origin },
    { "BLGC generates interrupt if destination is not 8 bit", cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_not_8_bit },
    { "BLGC generates interrupt if destination is type 3", cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_is_type_3 },
    { "BLGC processes L0 for type 0", cpu_selftest_sts2_blgc_processes_type_0_descriptor_L0 },
    { "BLGC processes L1 for type 1", cpu_selftest_sts2_blgc_processes_type_1_descriptor_L1 },
    { "BLGC processes L2 for type 2", cpu_selftest_sts2_blgc_processes_type_2_descriptor_L2 },
    { "BLGC processes L3 for type 0", cpu_selftest_sts2_blgc_processes_type_0_descriptor_L3 },
    { "BMVB generates ITS interrupt if destination is not 8-bit", cpu_selftest_sts2_bmvb_generates_its_interrupt_if_destination_not_8_bit },
    { "BMVB generates bounds check if destination bound is zero", cpu_selftest_sts2_bmvb_generates_checks_bound_on_destination_not_0 },
    { "BMVB copies byte from source, uses mask", cpu_selftest_sts2_bmvb_copies_byte_with_mask },
    { "BMVE generates ITS interrupt if destination is not 8-bit", cpu_selftest_sts2_bmve_generates_its_interrupt_if_destination_not_8_bit },
    { "BMVE copies byte, using mask, to the whole destination", cpu_selftest_sts2_bmve_copies_byte_with_mask_to_whole_destination },
    { "STS2 SMVF copies byte from source, uses mask", cpu_selftest_sts2_smvf_copies_bytes_and_fills_with_mask },
    { "BSCN generates interrupt if destination is not 8 bit", cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_not_8_bit },
    { "BSCN generates interrupt if destination is type 3", cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_is_type_3 },
    { "BSCN sets <0 if byte not found", cpu_selftest_sts2_bscn_sets_less_than_if_byte_not_found },
    { "BSCN finds byte in type 0 descriptor", cpu_selftest_sts2_bscn_finds_byte_in_type_0_descriptor },
    { "BSCN finds byte in type 0 descriptor", cpu_selftest_sts2_bscn_finds_byte_in_type_1_descriptor },
    { "BSCN finds byte in type 0 descriptor", cpu_selftest_sts2_bscn_finds_byte_in_type_2_descriptor },
    { "BCMP generates interrupt if destination is not 8 bit", cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_not_8_bit },
    { "BCMP generates interrupt if destination is type 3", cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_is_type_3 },
    { "BCMP sets =0 if byte is found in all elements", cpu_selftest_sts2_bcmp_sets_equals_if_byte_found_in_all_elements },
    { "BCMP sets <0 if byte differs and is smaller", cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_smaller },
    { "BCMP sets <0 if byte differs and is larger", cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_larger },
    { "BCMP finds byte in type 0 descriptor", cpu_selftest_sts2_bcmp_finds_byte_in_type_0_descriptor },
    { "BCMP finds byte in type 1 descriptor", cpu_selftest_sts2_bcmp_finds_byte_in_type_1_descriptor },
    { "BCMP finds byte in type 2 descriptor", cpu_selftest_sts2_bcmp_finds_byte_in_type_2_descriptor },
    { "SUB2 modifies existing descriptor in XD", cpu_selftest_sts2_sub2_modifies_XD },
    { "SUB2 calculates B", cpu_selftest_sts2_sub2_calculates_B },
    { "SUB2 modifies existing descriptor in D", cpu_selftest_sts2_sub2_modifies_D_existing_D },
    { "SUB2 generates ITS interrupt if the descriptor is not valid", cpu_selftest_sts1_sub2_generates_its_interrupt_if_descriptor_is_not_valid },

    { "B Load loads B", cpu_selftest_b_load_loads_B },
    { "B Load & Decrement loads B and subtracts 1", cpu_selftest_b_load_and_decrement_loads_B_and_subtracts_1 },
    { "B Load  & Decrement flags overflow", cpu_selftest_b_load_and_decrement_flags_overflow },
    { "B stack and load stacks B and then loads it", cpu_selftest_b_stack_and_load_stacks_B_and_loads_B },
    { "B store stores B", cpu_selftest_b_store_stores_B },
    { "B ADD adds operand to B", cpu_selftest_b_add_adds_operand_to_B },
    { "B ADD flags overflow", cpu_selftest_b_add_flags_overflow },
    { "B SUB subtracts operand from B", cpu_selftest_b_sub_subtracts_operand_from_B },
    { "B SUB flags overflow", cpu_selftest_b_sub_flags_overflow },
    { "B MUL multiplies operand by B", cpu_selftest_b_mul_multiplies_operand_by_B },
    { "B MUL flags overflow", cpu_selftest_b_mul_flags_overflow },
    { "B XOR", cpu_selftest_b_xor },
    { "B OR", cpu_selftest_b_or },
    { "B Shift shifts left for positive operand", cpu_selftest_b_shift_shifts_left_for_positive_operand },
    { "B Shift shifts right for negative operand", cpu_selftest_b_shift_shifts_right_for_negative_operand },
    { "B Shift flags overflow", cpu_selftest_b_shift_flags_overflow },
    { "B AND", cpu_selftest_b_and },
    { "B reverse SUB subtracts B from operand", cpu_selftest_b_rsub_subtracts_B_from_operand },
    { "B reverse SUB flags overflow", cpu_selftest_b_rsub_flags_overflow },
    { "B COMP sets < when B is less than operand", cpu_selftest_b_comp_sets_less_than_when_B_less_than_operand },
    { "B COMP sets = when B equals operand", cpu_selftest_b_comp_sets_equals_when_B_equals_operand },
    { "B COMP sets > when B is greater than operand", cpu_selftest_b_comp_sets_greater_than_when_B_greater_than_operand },
    { "B COMP sets overflow in T0 only", cpu_selftest_b_comp_sets_overflow_in_t0_only },
    { "B CINC compares B with operand", cpu_selftest_b_cinc_compares_B_with_operand },
    { "B CINC increments B", cpu_selftest_b_cinc_increments_B },
    { "B CINC flags overflow", cpu_selftest_b_cinc_flags_overflow },

    { "X Load loads X", cpu_selftest_x_load_loads_X },
    { "X Stack and Load stacks X and then loads it", cpu_selftest_x_stack_and_load_stacks_X_and_loads_X },
    { "X Store stores X", cpu_selftest_x_store_stores_X },
    { "X ADD adds operand to X", cpu_selftest_x_add_adds_operand_to_X },
    { "X ADD flags overflow", cpu_selftest_x_add_flags_overflow },
    { "X SUB subtracts operand from X", cpu_selftest_x_sub_subtracts_operand_from_X },
    { "X SUB flags overflow", cpu_selftest_x_sub_flags_overflow },
    { "X MUL multiplies operand by X", cpu_selftest_x_mul_multiplies_operand_by_X },
    { "X MUL flags overflow", cpu_selftest_x_mul_flags_overflow },
    { "X DIV divides X by operand", cpu_selftest_x_div_divides_X_by_operand },
    { "X DIV flags divide by zero", cpu_selftest_x_div_flags_divide_by_zero },
    { "X XOR", cpu_selftest_b_xor },
    { "X OR", cpu_selftest_b_or },
    { "X Shift shifts left for positive operand", cpu_selftest_x_shift_shifts_left_for_positive_operand },
    { "X Shift shifts right for negative operand", cpu_selftest_x_shift_shifts_right_for_negative_operand },
    { "X Shift flags overflow", cpu_selftest_x_shift_flags_overflow },
    { "X AND", cpu_selftest_x_and },
    { "X reverse SUB subtracts X from operand", cpu_selftest_x_rsub_subtracts_X_from_operand },
    { "X reverse SUB flags overflow", cpu_selftest_x_rsub_flags_overflow },
    { "X COMP sets < when X is less than operand", cpu_selftest_x_comp_sets_less_than_when_X_less_than_operand },
    { "X COMP sets = when X equals operand", cpu_selftest_x_comp_sets_equals_when_X_equals_operand },
    { "X COMP sets > when X is greater than operand", cpu_selftest_x_comp_sets_greater_than_when_X_greater_than_operand },
    { "X COMP sets overflow", cpu_selftest_x_comp_sets_overflow },
    { "X COMP overflow does not interrupt", cpu_selftest_x_comp_overflow_does_not_interrupt },
    { "X Reverse DIV divides the operand by X", cpu_selftest_x_rdiv_divides_operand_by_X },
    { "X Reverse DIV flags divide by zero", cpu_selftest_x_rdiv_flags_divide_by_zero },

    { "A Load loads AOD", cpu_selftest_a_load_loads_AOD },
    { "A Stack and Load stacks AOD and then loads it", cpu_selftest_a_stack_and_load_stacks_AOD_and_loads_AOD },
    { "A Store stores AOD", cpu_selftest_a_store_stores_AOD },
    { "A X ADD adds operand to A", cpu_selftest_a_add_adds_operand_to_A },
    { "A SUB subtracts operand from A", cpu_selftest_a_sub_subtracts_operand_from_A },
    { "A MUL multiplies operand by A", cpu_selftest_a_mul_multiplies_operand_by_A },
    { "A XOR", cpu_selftest_a_xor },
    { "A OR", cpu_selftest_a_or },
    { "A Shift shifts left for positive operand", cpu_selftest_a_shift_shifts_left_for_positive_operand },
    { "A Shift shifts right for negative operand", cpu_selftest_a_shift_shifts_right_for_negative_operand },
    { "A AND", cpu_selftest_a_and },
    { "A reverse SUB subtracts A from operand", cpu_selftest_a_rsub_subtracts_A_from_operand },
    { "A COMP sets < when A is less than operand", cpu_selftest_a_comp_sets_less_than_when_A_less_than_operand },
    { "A COMP sets = when A equals operand", cpu_selftest_a_comp_sets_equals_when_A_equals_operand },
    { "A COMP sets > when A is greater than operand", cpu_selftest_a_comp_sets_greater_than_when_A_greater_than_operand },

    { "DEC Load loads AEX", cpu_selftest_dec_load_loads_AEX },
    { "DEC Stack and Load stacks AEX and loads it", cpu_selftest_dec_stack_and_load_stacks_AEX_and_loads_AEX },
    { "DEC Store stores AEX", cpu_selftest_dec_store_stores_AEX },
    { "DEC COMP sets overflow when AND of AOD with operand is non-zero", cpu_selftest_dec_comp_sets_overflow_when_AOD_and_operand_non_zero },
    { "DEC COMP clears overlow when AND of AOD with operand is zero", cpu_selftest_dec_comp_clears_overflow_when_AOD_and_operand_is_zero },

    { "FLT Load Single loads 32 bits into A", cpu_selftest_flt_load_single_loads_32_bits_into_A },
    { "FLT Load Double loads 64 bits into A", cpu_selftest_flt_load_double_loads_64_bits_into_A },
    { "FLT Stack and Load stacks A and loads it (32 bits)", cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_32_bits },
    { "FLT Stack and Load stacks A and loads it (64 bits)", cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_64_bits },
    { "FLT Store stores A (32 bits)", cpu_selftest_flt_store_stores_A_32_bits },
    { "FLT Store stores A (64 bits)", cpu_selftest_flt_store_stores_A_64_bits },
    { "FLT XOR", cpu_selftest_flt_xor },
    { "FLT OR", cpu_selftest_flt_or },
    { "FLT Shift does a circular shift left for a positive operand", cpu_selftest_flt_shift_shifts_left_circular_for_positive_operand },
    { "FLT Shift does a circular shift rigt for a negative operand", cpu_selftest_flt_shift_shifts_right_circular_for_negative_operand },
    { "FLT AND", cpu_selftest_flt_and },

    { "Relative Jump jumps forward", cpu_selftest_org_relative_jump_jumps_forward },
    { "Relative Jump jumps backward", cpu_selftest_org_relative_jump_jumps_backward },
    { "Relative jump across segment boundary generates interrupt", cpu_selftest_org_relative_jump_across_segement_boundary_generates_interrupt },
    { "EXIT resets the link in executive mode", cpu_selftest_org_exit_resets_link_in_executive_mode },
    { "EXIT resets the link except the privileged MS bits is user mode", cpu_selftest_org_exit_resets_link_except_privileged_ms_bits_in_user_mode },
    { "Absolute jump jumps to new location", cpu_selftest_org_absolute_jump },
    { "RETURN sets SF and unstacks link", cpu_selftest_org_return_sets_SF_and_unstacks_link },
    { "RETURN resets the link except privileged MS bits in user mode", cpu_selftest_org_return_resets_link_except_privileged_ms_bits_in_user_mode },
    { "RETURN does not pop stack if operand is not stack but does set SF to NB", cpu_selftest_org_return_does_not_pop_stack_if_operand_is_not_stack_but_sets_NB },
    { "XC0-6 orders stack operand and jump to offset n in segment 8193", cpu_selftest_org_XCn_stacks_operand_and_jumps_to_offset_n },
    { "XC0-6 orders set executive mode", cpu_selftest_org_XCn_sets_executive_mode },
    { "STACK LINK puts link on the stack and adds the operand to the stacked value of CO", cpu_selftest_org_stacklink_puts_link_on_stack_adding_operand_to_stacked_CO },
    { "STACK LINK treats operand as signed", cpu_selftest_org_stacklink_treats_operand_as_signed },
    { "STACK LINK generates an interrupt when adding the operand to CO overflows a segment boundary", cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_overflows_segment_boundary },
    { "STACK LINK generates an interrupt when adding the operand to CO underlows a segment boundary", cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_underflows_segment_boundary },
    { "STACK LINK generates an illegal order interrupt when segment overflow occurs in user mode", cpu_selftest_org_stacklink_generates_illegal_order_interrupt_if_segment_overflow_in_user_mode },
    { "STACK LINK generates a system error interrupt when when segment overflow occurs in level 0 mode", cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_level0_mode },
    { "STACK LINK generates a system error interrupt when when segment overflow occurs in level 1 mode", cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_level1_mode },
    { "STACK LINK generates a system error interrupt when when segment overflow occurs in executive mode", cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_executive_mode },
    { "MS= sets unmasked bits only when in executive mode", cpu_selftest_org_ms_load_sets_unmasked_bits_only_in_executive_mode },
    { "MS= does not set masked bits in executive mode", cpu_selftest_org_ms_load_does_not_set_masked_bits_in_executive_mode },
    { "MS= does not set privileged bits even if unmasked when in user mode", cpu_selftest_org_ms_load_does_not_set_privileged_unmasked_bits_in_user_mode },
    { "DL= loads pseudo register for the display lamps", cpu_selftest_org_dl_load_sets_dl_pseudo_register },
    { "SPM dummy order", cpu_selftest_org_spm_dummy },
    { "SETLINK stores the link", cpu_selftest_org_setlink_stores_link },
    { "XNB= loads XNB", cpu_selftest_org_xnb_load_loads_XNB },
    { "SN= does not load SN in user mode", cpu_selftest_org_sn_load_in_user_mode_does_not_load_SN },
    { "SN= loads SN in executive mode", cpu_selftest_org_sn_load_in_executive_mode_loads_SN },
    { "XNB+ adds operand to XNB", cpu_selftest_org_xnb_plus_adds_operand_to_XNB },
    { "XNB+ generates an interrupt if there is a segment overflow", cpu_selftest_org_xnb_plus_generates_interrupt_if_segment_overflow },
    { "XNB+ generates an interrupt if there is a segment underflow", cpu_selftest_org_xnb_plus_generates_interrupt_if_segment_underflow },
    { "XNB+ generates an illegal order interrupt if there is a segment overflow in user mode", cpu_selftest_org_xnb_plus_generates_illegal_order_interrupt_if_segment_overflow_in_user_mode },
    { "XNB+ generates a system error interrupt if there is a segment overflow in level 0 mode", cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_level0_mode },
    { "XNB+ generates a system error interrupt if there is a segment overflow in level 1 mode", cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_level1_mode },
    { "XNB+ generates a system error interrupt if there is a segment overflow in executive mode", cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_executive_mode },
    { "XNB=> stores XNB", cpu_selftest_org_xnb_store_stores_XNB },
    { "XNB=> to secondary operand generates interrupt", cpu_selftest_org_xnb_store_to_secondary_operand_generates_interrupt },
    { "SF= loads SF", cpu_selftest_org_sf_load_loads_SF },
    { "SF+ adds operand to SF", cpu_selftest_org_sf_plus_adds_operand_to_SF },
    { "SF+ generates interrupt on segment overflow", cpu_selftest_org_sf_plus_generates_interrupt_on_segment_overflow },
    { "SF+ generates interrupt on segment underflow", cpu_selftest_org_sf_plus_generates_interrupt_on_segment_underflow },
    { "SF=NB+ adds NB to signed operand and stores result to SF", cpu_selftest_org_sf_load_nb_plus_adds_NB_to_signed_operand_and_stores_to_SF },
    { "SF=NB+ generates interrupt on segment overflow", cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_overflow },
    { "SF=NB+ generates interrupt on segment underflow", cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_underflow },
    { "SF=> stores SF", cpu_selftest_org_sf_store_stores_SF },
    { "SF=> to secondary operand generates interrupt", cpu_selftest_org_sf_store_to_secondary_operand_generates_interrupt },
    { "NB= loads NB", cpu_selftest_org_nb_load_loads_NB },
    { "NB=SF+ adds SF to signed operand and stores result to NB", cpu_selftest_org_nb_load_sf_plus_adds_SF_to_signed_operand_and_stores_to_NB },
    { "NB=SF+ generates interrupt on segment overflow", cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_overflow },
    { "NB=SF+ generates interrupt on segment underflow", cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_underflow },
    { "NB+ adds signed operand to NB", cpu_selftest_org_nb_plus_adds_signed_operand_to_NB },
    { "NB+ generates interrupt on segment overflow", cpu_selftest_org_nb_plus_generates_interrupt_on_segment_overflow },
    { "NB+ generates interrupt on segment underflow", cpu_selftest_org_nb_plus_generates_interrupt_on_segment_underflow },
    { "NB=> stores SN and NB", cpu_selftest_org_nb_store_stores_SN_and_NB },
    { "NB=> to secondary operand generates interrupt", cpu_selftest_org_nb_store_to_secondary_operand_generates_interrupt },
    { "Branch on eq does not branch on false", cpu_selftest_org_br_eq_does_not_branch_on_false },
    { "Branch on eq branches on true", cpu_selftest_org_br_eq_does_branch_on_true },
    { "Branch on ne does not branch on false", cpu_selftest_org_br_ne_does_not_branch_on_false },
    { "Branch on ne branches on true", cpu_selftest_org_br_ne_does_branch_on_true },
    { "Branch on ge does not branch on false", cpu_selftest_org_br_ge_does_not_branch_on_false },
    { "Branch on ge branches on true", cpu_selftest_org_br_ge_does_branch_on_true },
    { "Branch on lt does not branch on false", cpu_selftest_org_br_lt_does_not_branch_on_false },
    { "Branch on lt branches on true", cpu_selftest_org_br_lt_does_branch_on_true },
    { "Branch on le does not branch on false", cpu_selftest_org_br_le_does_not_branch_on_false },
    { "Branch on le branches on true", cpu_selftest_org_br_le_does_branch_on_true },
    { "Branch on gt does not branch on false", cpu_selftest_org_br_gt_does_not_branch_on_false },
    { "Branch on gt branches on true", cpu_selftest_org_br_gt_does_branch_on_true },
    { "Branch on ovf does not branch on false", cpu_selftest_org_br_ovf_does_not_branch_on_false },
    { "Branch on ovf branches on true", cpu_selftest_org_br_ovf_does_branch_on_true },
    { "Branch on bn does not branch on false", cpu_selftest_org_br_bn_does_not_branch_on_false },
    { "Branch on bn branches on true", cpu_selftest_org_br_bn_does_branch_on_true },
    { "Boolean order with function from operand, tests all function combinations", cpu_selftest_org_bn_function_tests },
    { "Boolean order with function from operand on eq test is false", cpu_selftest_org_bn_eq_on_false },
    { "Boolean order with function from operand on eq test is true", cpu_selftest_org_bn_eq_on_true },
    { "Boolean order with function from operand on ne test is false", cpu_selftest_org_bn_ne_on_false },
    { "Boolean order with function from operand on ne test is true", cpu_selftest_org_bn_ne_on_true },
    { "Boolean order with function from operand on ge test is false", cpu_selftest_org_bn_ge_on_false },
    { "Boolean order with function from operand on ge test is true", cpu_selftest_org_bn_ge_on_true },
    { "Boolean order with function from operand on lt test is false", cpu_selftest_org_bn_lt_on_false },
    { "Boolean order with function from operand on lt test is true", cpu_selftest_org_bn_lt_on_true },
    { "Boolean order with function from operand on le test is false", cpu_selftest_org_bn_le_on_false },
    { "Boolean order with function from operand on le test is true", cpu_selftest_org_bn_le_on_true },
    { "Boolean order with function from operand on gt test is false", cpu_selftest_org_bn_gt_on_false },
    { "Boolean order with function from operand on gt test is true", cpu_selftest_org_bn_gt_on_true },
    { "Boolean order with function from operand on ovf test is false", cpu_selftest_org_bn_ovf_on_false },
    { "Boolean order with function from operand on ovf test is true", cpu_selftest_org_bn_ovf_on_true },
    { "Boolean order with function from operand on bn test is false", cpu_selftest_org_bn_bn_on_false },
    { "Boolean order with function from operand on bn test is ", cpu_selftest_org_bn_bn_on_true },
    { "Boolean order with function from order, tests all functions", cpu_selftest_org_bn_order_tests },

	{ "Setting a B or D fault in executive mode generates system error interrupt", cpu_selftest_setting_b_or_d_fault_in_executive_mode_generates_system_error_interrupt },
	{ "Setting a B or D fault in executive mode does not generate system error interrupt if inhibited", cpu_selftest_setting_b_or_d_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting a B fault in user mode generates program fault interrupt", cpu_selftest_setting_b_fault_in_user_mode_generates_program_fault_interrupt },
    { "Setting a B fault in user mode does not generate program fault interrupt if inhibited", cpu_selftest_setting_b_fault_in_user_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting BOD B overflow in executive mode generates B or D system error interrupt", cpu_selftest_setting_bod_b_overflow_in_executive_mode_generates_b_or_d_system_error_interrupt },
    { "Setting BOD B overflow in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_bod_b_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting BOD B overflow in user mode generates B program fault interrupt", cpu_selftest_setting_bod_b_overflow_in_user_mode_generates_b_program_fault_interrupt },
    { "Setting BOD B overflow in user mode does not generate an interrupt if inhibited", cpu_selftest_setting_bod_b_overflow_in_user_mode_does_not_generate_interrupt_if_inhibited },
    { "Clearing BOD B overflow in executive mode does not clear interrupt", cpu_selftest_clearing_bod_b_overflow_in_executive_mode_does_not_clear_interrupt },
    { "Clearing BOD B overflow in user mode does not clear interrupt", cpu_selftest_clearing_bod_b_overflow_in_user_mode_does_not_clear_interrupt },

	{ "Setting an A fault in executive mode generates system error interrupt", cpu_selftest_setting_acc_fault_in_executive_mode_generates_system_error_interrupt },
	{ "Setting an A fault in executive mode does not generate system error interrupt if inhibited", cpu_selftest_setting_acc_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting an A fault in user mode generates program fault interrupt", cpu_selftest_setting_acc_fault_in_user_mode_generates_program_fault_interrupt },
    { "Setting an A fault in user mode does not generate program fault interrupt if inhibited", cpu_selftest_setting_acc_fault_in_user_mode_does_not_generate_interrupt_if_inhibited },
	{ "Setting AOD floating point overflow in executive mode generates acc error interrupt", cpu_selftest_setting_aod_floating_point_overflow_in_executive_mode_generates_acc_system_error_interrupt },
	{ "Setting AOD floating point overflow in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_aod_floating_point_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited },
	{ "Setting AOD floating point underflow in executive mode generates acc error interrupt", cpu_selftest_setting_aod_floating_point_underflow_in_executive_mode_generates_acc_system_error_interrupt },
	{ "Setting AOD floating point underflow in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_aod_floating_point_underflow_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting AOD fixed point overflow in executive mode generates acc error interrupt", cpu_selftest_setting_aod_fixed_point_overflow_in_executive_generates_acc_system_error_interrupt },
    { "Setting AOD fixed point overflow in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_aod_fixed_point_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting AOD decimal overflow in executive mode generates acc error interrupt", cpu_selftest_setting_aod_decimal_overflow_in_executive_mode_generates_acc_system_error_interrupt },
	{ "Setting AOD decimal overflow in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_aod_decimal_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited },
	{ "Setting AOD zero divide in executive mode generates acc error interrupt", cpu_selftest_setting_aod_zero_divide_in_executive_mode_generates_acc_system_error_interrupt },
	{ "Setting AOD zero divide in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_aod_zero_divide_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Clearing an A fault in executive mode does not clear interrupt", cpu_selftest_clearing_acc_fault_in_executive_mode_does_not_clear_interrupt },
    { "Clearing an A fault in user mode does not clear interrupt", cpu_selftest_clearing_acc_fault_in_user_mode_does_not_clear_interrupt },
	{ "Setting operand size in AOD does not trigger interrupt evaluation", cpu_selftest_setting_operand_size_in_aod_does_not_trigger_interrupt_evaluation },

    { "Setting a D fault in executive mode generates system error interrupt", cpu_selftest_setting_d_fault_in_executive_mode_generates_system_error_interrupt },
    { "Setting a D fault in executive mode does not generate system error interrupt if inhibited", cpu_selftest_setting_d_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting a D fault in user mode generates program fault interrupt", cpu_selftest_setting_d_fault_in_user_mode_generates_program_fault_interrupt },
    { "Setting a D fault in user mode does not generate program fault interrupt if inhibited", cpu_selftest_setting_d_fault_in_user_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting DOD XCHK in executive mode generates an interrupt", cpu_selftest_setting_dod_xchk_in_executive_mode_generates_d_system_error_interrupt },
    { "Setting DOD ITS in executive mode generates an interrupt", cpu_selftest_setting_dod_illegal_type_size_in_executive_mode_generates_d_system_error_interrupt },
    { "Setting DOD EMS in executive mode generates an interrupt", cpu_selftest_setting_dod_executive_mode_subtype_in_executive_mode_generates_d_system_error_interrupt },
    { "Setting DOD SSS in executive mode generates an interrupt", cpu_selftest_setting_dod_short_source_string_in_executive_mode_generates_d_system_error_interrupt },
    { "Setting DOD SSS in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_dod_short_source_string_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting DOD NZT in executive mode generates an interrupt", cpu_selftest_setting_dod_non_zero_truncation_in_executive_mode_generates_d_system_error_interrupt },
    { "Setting DOD NZT in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_dod_non_zero_truncation_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting DOD BCH in executive mode generates an interrupt", cpu_selftest_setting_dod_bounds_check_in_executive_mode_generates_d_system_error_interrupt },
    { "Setting DOD BCH in executive mode does not generate an interrupt if inhibited", cpu_selftest_setting_dod_bounds_check_in_executive_mode_does_not_generate_interrupt_if_inhibited },
    { "Setting DOD write to read only in executive mode generates an interrupt", cpu_selftest_setting_dod_write_to_read_only_in_executive_mode_generates_d_system_error_interrupt },
    { "Clearing a D fault in executive mode does not clear interrupt", cpu_selftest_clearing_d_fault_in_executive_mode_does_not_clear_interrupt },
    { "Clearing a D fault in user mode does not clear interrupt", cpu_selftest_clearing_d_fault_in_user_mode_does_not_clear_interrupt },

    { "Switch from executive mode to user mode when there is a system error does not create a program fault", cpu_selftest_switch_from_executive_mode_to_user_mode_when_system_error_does_not_create_program_fault },
    { "Switch from user mode to executive mode when there is a program fault does not create a system error", cpu_selftest_switch_from_user_mode_to_executive_mode_program_fault_does_not_create_system_error },
    { "Return from interrupt handler without clearing System Error Status V-line masks System Error interrupts", cpu_selftest_return_from_interrupt_handler_without_clearing_system_error_status_v_line_masks_system_error_interrupts },

    { "Illegal function generates System Error interrupt in executive mode", cpu_selftest_illegal_function_generates_system_error_interrupt_in_executive_mode },
    { "Illegal function generates System Error interrupt if L1IF is set", cpu_selftest_illegal_function_generates_system_error_interrupt_if_L1IF_is_set },
    { "Illegal function generates System Error interrupt if L0IF is set", cpu_selftest_illegal_function_generates_system_error_interrupt_if_L0IF_is_set },
    { "Illegal function generates Illegal Order interrupt in user mode", cpu_selftest_illegal_function_generates_illegal_order_interrupt_in_user_mode },

    { "Name adder overflow generates System Error interrupt in executive mode", cpu_selftest_name_adder_overflow_generates_system_error_interrupt_in_executive_mode },
    { "Name adder overflow generates System Error interrupt if L1IF is set", cpu_selftest_name_adder_overflow_generates_system_error_interrupt_if_L1IF_is_set },
    { "Name adder overflow generates System Error interrupt if L0IF is set", cpu_selftest_name_adder_overflow_generates_system_error_interrupt_if_L0IF_is_set },
    { "Name adder overflow generates Illegal Order interrupt in user mode", cpu_selftest_name_adder_overflow_generates_illegal_order_interrupt_in_user_mode },

    { "Control adder overflow generates System Error interrupt in executive mode", cpu_selftest_control_adder_overflow_generates_system_error_interrupt_in_executive_mode },
    { "Control adder overflow generates System Error interrupt if L1IF is set", cpu_selftest_control_adder_overflow_generates_system_error_interrupt_if_L1IF_is_set },
    { "Control adder overflow generates System Error interrupt if L0IF is set", cpu_selftest_control_adder_overflow_generates_system_error_interrupt_if_L0IF_is_set },
    { "Control adder overflow generates Illegal Order interrupt in user mode", cpu_selftest_control_adder_overflow_generates_illegal_order_interrupt_in_user_mode },

    { "CPR Not Equivalence generates CPR Not Equivalence interrupt in executive mode if L0IF is clear", cpu_selftest_cpr_not_equivalence_interrupt_in_executive_mode_if_L0IF_is_clear },
    { "CPR Not Equivalence generates System Error interrupt if L0IF is set", cpu_selftest_cpr_not_equivalence_generates_system_error_interrupt_in_executive_mode_if_L0IF_is_set },
    { "CPR Not Equivalence generates CPR Not Equivalence interrupt in user mode", cpu_selftest_cpr_not_equivalence_in_user_mode },
    { "CPR Not Equivalence generates System Error interrupt in user mode if L0IF is set (unlikely scenario)", cpu_selftest_cpr_not_equivalence_generates_system_error_interrupt_in_user_mode_if_L0IF_is_set },

    { "SPM bit in Program Fault Interrupt Status V-Line Causes a Program Fault Interrupt", cpu_selftest_spm_bit_in_program_fault_interrupt_status_causes_program_fault_interrupt },

    { "System Error interrupt is not inhibited even if L0IF or L1IF is set", cpu_selftest_system_error_interrupt_not_inhibited_even_if_L0IF_or_L1IF_is_set },
    { "CPR Not Equivalence (Level 0) interrupt inhibited if L0IF is set", cpu_selftest_cpr_not_equivalence_interrupt_inhibited_if_L0IF_is_set },
    { "Exchange interrupt (Level 0) interrupt inhibited if L0IF is set",  cpu_selftest_exchange_interrupt_inhibited_if_L0IF_is_set },
    { "Peripheral Window (Level 0) interrupt inhibited if L0IF is set", cpu_selftest_peripheral_window_interrupt_inhibited_if_L0IF_is_set },
    { "Level 0 interrupt not inhibited if L1IF is set", cpu_selftest_level_0_interrupt_not_inhibited_if_L1IF_is_set },
	{ "Level 1 interrupt inhibited if L0IF is set", cpu_selftest_level_1_interrupt_inhibited_if_L0IF_is_set },
	{ "Level 1 interrupt inhibited if L1IF is set", cpu_selftest_level_1_interrupt_inhibited_if_L1IF_is_set },

    { "CPR Not Equivalence interrupt on order fetch stores link that re-executes failed order", cpu_selftest_cpr_not_equivalance_interrupt_on_order_fetch_stores_link_that_re_executes_failed_order },
    { "CPR Not Equivalence interrupt on primary operand stores link that re-executes failed order", cpu_selftest_cpr_not_equivalance_interrupt_on_primary_operand_stores_link_that_re_executes_failed_order },
    { "CPR Not Equivalence interrupt on secondary operand stores link that re-executes failed order", cpu_selftest_cpr_not_equivalance_interrupt_on_secondary_operand_stores_link_that_re_executes_failed_order },

    /* The next block of tests overlaps with previous tests but they run by actually executing orders and setting inhibit flags beforehand */
    { "No B overflow interrupt if B overflow is inhibited", cpu_selftest_no_b_overflow_interrupt_if_b_overflow_is_inhibited },
    { "No Acc zero divide interrupt if Acc zero divide is inhibited", cpu_selftest_no_acc_zero_divide_interrupt_if_acc_zero_divide_is_inhibited },
    { "No bounds check interrupt if bounds check is inhibited", cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited },
    { "No SSS interrupt if SSS interrupt is inhibited", cpu_selftest_no_sss_interrupt_if_sss_is_inhibited },
    { "D interrupt as system error in executive mode", cpu_selftest_D_interrupt_as_system_error_in_executive_mode },
    { "No D interrupt if inhibited in executive mode", cpu_selftest_no_D_interrupt_if_inhibited_in_executive_mode },
    { "D interrupt as program fault in user mode", cpu_selftest_D_interrupt_as_program_fault_in_user_mode },
    { "No D interrupt if inhibited in user mode", cpu_selftest_no_D_interrupt_if_inhibited_in_user_mode },
    { "B interrupt as system error in executive mode", cpu_selftest_B_interrupt_as_system_error_in_executive_mode },
    { "B interrupt as program fault in user mode", cpu_selftest_B_interrupt_as_program_fault_in_user_mode },
    { "Acc interrupt as system error in executive mode", cpu_selftest_acc_interrupt_as_system_error_in_executive_mode },
    { "No Acc interrupt if inhibited in executive mode", cpu_selftest_no_acc_interrupt_if_inhibited_in_executive_mode },
    { "Acc interrupt as program fault in user mode", cpu_selftest_acc_interrupt_as_program_fault_in_user_mode },
    { "No Acc interrupt if inhibited in user mode", cpu_selftest_no_acc_interrupt_if_inhibited_in_user_mode },

    { "Interrupt stacks link in System V-Store", cpu_selftest_interrupt_stacks_link_in_system_v_store },
    { "Interrupt calls handler using link in System V-Store", cpu_selftest_interrupt_calls_handler_using_link_in_system_v_store },
    { "Interrupt sets executive mode", cpu_selftest_interrupt_sets_executive_mode },
    { "Interrupt sequence clears interrupt so it is not called again", cpu_selftest_interrupt_sequence_clears_interrupt },
    { "Interrupt sequence does not clear interrupt when handling a different interrupt", cpu_selftest_interrupt_sequence_does_not_clear_interrupt_when_handling_another_interrupt },

    { "Write to PROP PROGRAM FAULT STATUS V-Line resets it", cpu_selftest_write_to_prop_program_fault_status_resets_it },
    { "Write to PROP SYSTEM ERROR STATUS V-Line resets it", cpu_selftest_write_to_prop_system_error_status_resets_it },

    { "Read and write instruction counter ", cpu_selftest_read_and_write_instruction_counter },
    { "Executing instruction decrements instruction counter ", cpu_selftest_executing_instruction_decrements_instruction_counter },
    { "Instruction counter not decremented if inhibited", cpu_selftest_instruction_counter_not_decremented_if_inhibited },
    { "Instruction counter not decremented if already zero", cpu_selftest_instruction_counter_not_decremented_if_already_zero },
    { "Instruction counter zero generates interrupt", cpu_selftest_instruction_counter_zero_generates_interrupt },
    { "Instruction counter already zero does not generate new interrupt", cpu_selftest_instruction_counter_already_zero_does_not_generate_new_interrupt },

};

static void cpu_selftest_reset(UNITTEST *test)
{
    sac_reset_state(); /* reset SAC first because it clears the V-Store callbacks which may be set by other devices */
    cpu_reset_state();
    cpu_selftest_setup_test_virtual_pages(SAC_ALL_ACCESS);
    cpu_selftest_clear_bcpr();
    currentLoadLocation = 0;
    cpu_selftest_set_acc_faults_to_system_error_in_exec_mode(); // TODO: remove this line and test explicitly.
    cpu_selftest_set_b_and_d_faults_to_system_error_in_exec_mode(); // TODO: remove this line and test explicitly.
}

static void cpu_selftest_setup_test_virtual_pages(uint8 access)
{
	mu5_selftest_setup_cpr(0, CPR_VA(0, 0x0000, 0x000), CPR_RA_LOCAL(access, 0x0000, 0xA)); /* 16K-word page at the bottom of segment 0 */
	mu5_selftest_setup_cpr(1, CPR_VA(0, 0x0001, 0x000), CPR_RA_LOCAL(access, 0x1000, 0xA)); /* 16K-word page at the top of segment 0, used for tests that go off the end of the segment */
	mu5_selftest_setup_cpr(2, CPR_VA(0, 0x0000, 0x400), CPR_RA_LOCAL(access, 0x1000, 0xA)); /* 16K-word page at the bottom of segment 1, overlaps with CPR1, but is only used for some XNB tests */
}

static void cpu_selftest_set_load_location(uint32 location)
{
    currentLoadLocation = location;
}

static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n)
{
    uint16 order;

    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= (k & 0x7) << 6;
    order |= n & 0x3F;
    sac_write_16_bit_word(currentLoadLocation, order);
    currentLoadLocation += 1;
}

static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 kp, uint8 np)
{
    uint16 order;

    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= 0x7 << 6;
    order |= (kp & 0x7) << 3;
    order |= np & 0x7;
    sac_write_16_bit_word(currentLoadLocation, order);
    currentLoadLocation += 1;
}

static void cpu_selftest_load_organisational_order_literal(uint8 f, uint8 n)
{
    uint16 order;

    order = 0;
    order |= (f & 0x3F) << 7;
    order |= n & 0x3F;
    sac_write_16_bit_word(currentLoadLocation, order);
    currentLoadLocation += 1;
}

static void cpu_selftest_load_organisational_order_extended(uint8 f, uint8 kp, uint8 np)
{
    uint16 order;

    order = 0;
    order |= (f & 0x3F) << 7;
    order |= 0x1 << 6;
    order |= (kp & 0x7) << 3;
    order |= np & 0x7;
    sac_write_16_bit_word(currentLoadLocation, order);
    currentLoadLocation += 1;
}

static void cpu_selftest_load_16_bit_literal(uint16 value)
{
    sac_write_16_bit_word(currentLoadLocation, value);
    currentLoadLocation += 1;
}

static void cpu_selftest_load_32_bit_literal(uint32 value)
{
    sac_write_16_bit_word(currentLoadLocation, (value >> 16) & 0xFFFF);
    sac_write_16_bit_word(currentLoadLocation + 1, value & 0xFFFF);
    currentLoadLocation += 2;
}

static void cpu_selftest_load_64_bit_literal(t_uint64 value)
{
    sac_write_16_bit_word(currentLoadLocation, (value >> 48) & 0xFFFF);
    sac_write_16_bit_word(currentLoadLocation + 1, (value >> 32) & 0xFFFF);
    sac_write_16_bit_word(currentLoadLocation + 2, (value >> 16) & 0xFFFF);
    sac_write_16_bit_word(currentLoadLocation + 3, value & 0xFFFF);
    currentLoadLocation += 4;
}

static t_uint64 cpu_selftest_create_descriptor(uint8 type, uint8 size, uint32 bound, uint32 origin)
{
    t_uint64 result = 0;
    result |= (t_uint64)(type & 0x3) << 62;
    result |= (t_uint64)(size & 0x7) << 59;
    result |= (t_uint64)bound << 32;
    result |= origin;

    return result;
}

static t_uint64 cpu_selftest_create_miscellaneous_descriptor(uint8 type, uint8 subtype, uint32 bound, uint32 origin)
{
    t_uint64 result = 0;
    result |= (t_uint64)(type & 0x3) << 62;
    result |= (t_uint64)(subtype & 0x3F) << 56;
    result |= (t_uint64)(bound & 0x0FFF) << 32;
    result |= origin;

    return result;
}

static t_addr cpu_selftest_get_64_bit_vector_element_address(uint32 origin, uint32 offset)
{
    return (origin + (offset << 3)) >> 2;
}

static t_addr cpu_selftest_get_32_bit_vector_element_address(uint32 origin, uint32 offset)
{
    return (origin + (offset << 2)) >> 2;
}

static t_addr cpu_selftest_get_16_bit_vector_element_address(uint32 origin, uint32 offset)
{
    return (origin + (offset << 1)) >> 1;
}

static t_addr cpu_selftest_get_8_bit_vector_element_address(uint32 origin, uint32 offset)
{
    return origin + offset;
}

static t_addr cpu_selftest_get_4_bit_vector_element_address(uint32 origin, uint32 offset)
{
    return origin + (offset >> 1);
}

static t_addr cpu_selftest_get_1_bit_vector_element_address(uint32 origin, uint32 offset)
{
    return origin + (offset >> 3);
}

static void cpu_selftest_load_64_bit_value_to_descriptor_location(uint32 origin, uint32 offset, t_uint64 value)
{
    sac_write_64_bit_word(cpu_selftest_get_64_bit_vector_element_address(origin, offset), value);
}

static void cpu_selftest_load_32_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint32 value)
{
    sac_write_32_bit_word(cpu_selftest_get_32_bit_vector_element_address(origin, offset), value);
}

static void cpu_selftest_load_16_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint16 value)
{
    sac_write_16_bit_word(cpu_selftest_get_16_bit_vector_element_address(origin, offset), value);
}

static void cpu_selftest_load_8_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint8 value)
{
    sac_write_8_bit_word(cpu_selftest_get_8_bit_vector_element_address(origin, offset), value);
}

static void cpu_selftest_load_4_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint8 value)
{
    t_addr addr = cpu_selftest_get_4_bit_vector_element_address(origin, offset);
    uint8 shift = 4 * (1 - (offset & 0x1));
    uint8 nibbleMask = (uint8)0xF << shift;
    uint8 nibble = (value & 0xF) << shift;
    uint8 byte = sac_read_8_bit_word(addr);
    byte = (byte & ~nibbleMask) | nibble;
    sac_write_8_bit_word(addr, byte);
}

static void cpu_selftest_load_1_bit_value_to_descriptor_location(uint32 origin, uint32 offset, uint8 value)
{
    t_addr addr = cpu_selftest_get_1_bit_vector_element_address(origin, offset);
    uint8 shift = 7 - (offset & 0x7);
    uint8 bitMask = (uint8)0x1 << shift;
    uint8 bit = (value & 0x1) << shift;
    uint8 byte = sac_read_8_bit_word(addr);
    byte = (byte & ~bitMask) | bit;
    sac_write_8_bit_word(addr, byte);
}

static uint32 cpu_selftest_byte_address_from_word_address(uint32 address)
{
    return address << 2;
}

static void cpu_selftest_set_aod_operand_32_bit()
{
    cpu_selftest_set_register(REG_AOD, 0);
}

static void cpu_selftest_set_aod_operand_64_bit()
{
    cpu_selftest_set_register(REG_AOD, AOD_OPSIZ_MASK);
}

static void cpu_selftest_set_level0_mode(void)
{
    mu5_selftest_set_level0_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_level1_mode(void)
{
    mu5_selftest_set_level1_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_executive_mode(void)
{
    mu5_selftest_set_executive_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_user_mode(void)
{
    mu5_selftest_set_user_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_clear_bcpr(void)
{
    mu5_selftest_clear_bcpr(localTestContext, &cpu_dev);
}

static void cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode(void)
{
    mu5_selftest_clear_acc_faults_to_system_error_in_exec_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_acc_faults_to_system_error_in_exec_mode(void)
{
    mu5_selftest_set_acc_faults_to_system_error_in_exec_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode(void)
{
    mu5_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_b_and_d_faults_to_system_error_in_exec_mode(void)
{
    mu5_selftest_set_b_and_d_faults_to_system_error_in_exec_mode(localTestContext, &cpu_dev);
}

static void cpu_selftest_clear_inhibit_program_fault_interrupts(void)
{
    mu5_selftest_clear_inhibit_program_fault_interrupts(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_inhibit_program_fault_interrupts(void)
{
    mu5_selftest_set_inhibit_program_fault_interrupts(localTestContext, &cpu_dev);
}

static void cpu_selftest_set_inhibit_instruction_counter(void)
{
    uint16 ms = cpu_get_ms() | MS_MASK_INH_INS_COUNT;
    mu5_selftest_set_register(localTestContext, &cpu_dev, REG_MS, ms);
}

static void cpu_selftest_set_bn(int8 bn)
{
    uint16 ms = cpu_selftest_get_register(REG_MS) & 0xFFFF;
    ms = (ms & 0xFEFF) | ((bn & 1) << 8);
    cpu_selftest_set_register(REG_MS, ms);
}

static void cpu_selftest_set_test_is_zero(int8 isZero)
{
    uint16 ms = cpu_selftest_get_register(REG_MS) & 0xFFFF;
    ms = (ms & 0xFBFF) | ((~isZero & 1) << 10);
    cpu_selftest_set_register(REG_MS, ms);
}

static void cpu_selftest_run_code(void)
{
    cpu_selftest_run_code_from_location(0);
}

static void cpu_selftest_run_code_from_location(uint32 location)
{
    cpu_selftest_set_register("CO", location);
    cpu_execute_next_order();
}

static void cpu_selftest_run_continue(void)
{
    cpu_execute_next_order();
}

static REG *cpu_selftest_find_register(char *name)
{
    return mu5_selftest_find_register(localTestContext, &cpu_dev, name);
}

static t_uint64 cpu_selftest_get_register(char *name)
{
    return mu5_selftest_get_register(localTestContext, &cpu_dev, name);
}

static void cpu_selftest_set_register(char *name, t_uint64 value)
{
    mu5_selftest_set_register(localTestContext, &cpu_dev, name, value);
}

static void cpu_selftest_setup_default_segment(void)
{
	cpu_selftest_set_register(REG_SN, SN_DEFAULT);
}

static void cpu_selftest_setup_name_base(uint16 base)
{
	cpu_selftest_setup_default_segment();
	cpu_selftest_set_register(REG_NB, base);
}

static void cpu_selftest_setup_default_name_base(void)
{
	cpu_selftest_setup_name_base(NB_DEFAULT);
}

static void cpu_selftest_setup_default_extra_name_base(void)
{
	cpu_selftest_set_register(REG_XNB, XNB_DEFAULT);
}

static void cpu_selftest_setup_stack_base(uint16 base)
{
	cpu_selftest_setup_default_segment();
	cpu_selftest_set_register(REG_SF, base);
}

static void cpu_selftest_setup_default_stack_base(void)
{
	cpu_selftest_setup_stack_base(SF_DEFAULT);
}

static void cpu_selftest_setup_vstore_test_location(void)
{
    sac_setup_v_store_location(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, mu5_selftest_read_callback_for_static_64_bit_location, mu5_selftest_write_callback_for_static_64_bit_location);
}

static void cpu_selftest_setup_interrupt_vector(int interruptNumber, uint16 ms, uint16 nb, uint32 co)
{
    t_uint64 link = ((t_uint64)ms << 48) | ((t_uint64)nb << 32) | co;
    sac_write_v_store(SYSTEM_V_STORE_BLOCK, 16 + (interruptNumber * 2) + 1, link);
}

static void cpu_selftest_setup_illegal_function_error(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_LITERAL, 0x1F);
}

static void cpu_selftest_setup_name_adder_overflow_error(void)
{
    cpu_selftest_load_organisational_order_extended(F_SF_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_setup_stack_base(0xFFFE);
}

static void cpu_selftest_setup_control_adder_overflow_error(void)
{
    cpu_selftest_set_load_location(0xFFFF);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
}

static void cpu_selftest_setup_interrupt_entry_link(int interruptNumber)
{
	sac_write_v_store(SYSTEM_V_STORE_BLOCK, 16 + (interruptNumber * 2) + 1, 0x0004000000000000); /* Ensure Executive mode is retained after processing interrupt entry sequence */
}

static void cpu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_equals(localTestContext, &cpu_dev, name, expectedValue);
}

static void cpu_selftest_assert_reg_equals_mask(char *name, t_uint64 expectedValue, t_uint64 mask)
{
    t_uint64 actualValue = cpu_selftest_get_register(name);

    if ((mask & actualValue) != (mask & expectedValue))
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value in register %s to be %llX, but was %llX for mask %llX\n", name, mask & expectedValue, mask & actualValue, mask);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_memory_contents_32_bit(t_addr address, uint32 expectedValue)
{
    uint32 actualValue = sac_read_32_bit_word(address);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at address %08X to be %08X, but was %08X\n", address, expectedValue, actualValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_memory_contents_64_bit(t_addr address, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_64_bit_word(address);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at address %08X to be %016llX, but was %016llX\n", address, expectedValue, actualValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_vector_content_64_bit(t_addr origin, uint32 offset, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_64_bit_word(cpu_selftest_get_64_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %016llX, but was %016llX\n", offset, origin, expectedValue, actualValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_vector_content_32_bit(t_addr origin, uint32 offset, uint32 expectedValue)
{
    uint32 actualValue = sac_read_32_bit_word(cpu_selftest_get_32_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %08X, but was %08X\n", offset, origin, expectedValue, actualValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_vector_content_16_bit(t_addr origin, uint32 offset, uint16 expectedValue)
{
    uint16 actualValue = sac_read_16_bit_word(cpu_selftest_get_16_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %04X, but was %04X\n", offset, origin, expectedValue, actualValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_vector_content_8_bit(t_addr origin, uint32 offset, uint8 expectedValue)
{
    uint8 actualValue = sac_read_8_bit_word(cpu_selftest_get_8_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %02X, but was %02X\n", offset, origin, expectedValue, actualValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_vector_content_4_bit(t_addr origin, uint32 offset, uint8 expectedValue)
{
    uint8 byte = sac_read_8_bit_word(cpu_selftest_get_4_bit_vector_element_address(origin, offset));
    uint8 shift = 4 * (1 - (offset & 0x1));
    uint8 actualNibble = (byte >> shift) & 0xF;
    if (actualNibble != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %01X, but was %01X\n", offset, origin, expectedValue, actualNibble);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_vector_content_1_bit(t_addr origin, uint32 offset, uint8 expectedValue)
{
    uint8 byte = sac_read_8_bit_word(cpu_selftest_get_1_bit_vector_element_address(origin, offset));
    uint8 shift = 7 - (offset & 0x7);
    uint8 actualBit = (byte >> shift) & 0x1;
    if (actualBit != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %01X, but was %01X\n", offset, origin, expectedValue, actualBit);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_interrupt_return_address(int interruptNumber, uint32 expectedAddress)
{
    t_uint64 stored_link = sac_read_v_store(SYSTEM_V_STORE_BLOCK, 16 + (interruptNumber * 2));
    uint32 actual = stored_link & 0x00000000FFFFFFFF;
    if (actual != expectedAddress)
    {
        sim_debug(LOG_SELFTEST_FAIL, localTestContext->dev, "Interrupt return address was %08X, expected %08X\n", actual, expectedAddress);
        mu5_selftest_set_failure(localTestContext);
    }
}

static void cpu_selftest_assert_inhibited_program_fault_interrupt(uint16 expected_program_fault_status)
{
    cpu_selftest_assert_v_store_contents(PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, expected_program_fault_status);
    cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_assert_no_system_error(void)
{
    mu5_selftest_assert_no_system_error(localTestContext);
}

static void cpu_selftest_assert_no_program_fault(void)
{
    mu5_selftest_assert_no_program_fault(localTestContext);
}

static void cpu_selftest_assert_no_b_overflow(void)
{
    t_uint64 bod = cpu_selftest_get_register(REG_BOD);
    if (bod & BOD_BOVF_MASK)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Unexpected B overflow\n");
        cpu_selftest_set_failure();
    }
}
static void cpu_selftest_assert_no_b_overflow_interrupt(void)
{
    cpu_selftest_assert_no_interrupt();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_assert_b_overflow(void)
{
    t_uint64 bod = cpu_selftest_get_register(REG_BOD);
    if (!(bod & BOD_BOVF_MASK))
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected B overflow\n");
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_b_overflow_interrupt_as_system_error(void)
{
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
    cpu_selftest_assert_b_overflow();
}

static void cpu_selftest_assert_no_acc_overflow(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (aod & AOD_FLPOVF_MASK)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Unexpected A overflow\n");
        cpu_selftest_set_failure();
    }
}
static void cpu_selftest_assert_no_acc_overflow_interrupt(void)
{
    cpu_selftest_assert_no_interrupt();
    cpu_selftest_assert_no_acc_overflow();
}

static void cpu_selftest_assert_acc_fixed_point_overflow(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (!(aod & AOD_FXPOVF_MASK))
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected A fixed point overflow\n");
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error(void)
{
    cpu_selftest_assert_acc_interrupt_as_system_error();
    cpu_selftest_assert_acc_fixed_point_overflow();
}

static void cpu_selftest_assert_no_a_zero_divide(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (aod &AOD_ZDIV_MASK)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Unexpected A zero divide\n");
        cpu_selftest_set_failure();
    }
}
static void cpu_selftest_assert_no_a_zero_divide_interrupt(void)
{
    cpu_selftest_assert_no_interrupt();
    cpu_selftest_assert_no_a_zero_divide();
}

static void cpu_selftest_assert_a_zero_divide(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (!(aod & AOD_ZDIV_MASK))
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected A zero divide\n");
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_a_zero_divide_interrupt(void)
{
    cpu_selftest_assert_acc_interrupt_as_system_error();
    cpu_selftest_assert_a_zero_divide();
}

static void cpu_selftest_assert_interrupt(int interruptNumber)
{
    if (cpu_get_interrupt_number() != interruptNumber)
    {
        switch (interruptNumber)
        {
            case INT_SYSTEM_ERROR:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected System Error interrupt to have occurred\n");
                break;
            }
            case INT_CPR_NOT_EQUIVALENCE:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected CPR Not Equivalence interrupt to have occurred\n");
                break;
            }
            case INT_EXCHANGE:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected Exchange interrupt to have occurred\n");
                break;
            }
            case INT_PERIPHERAL_WINDOW:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected Peripheral Window interrupt to have occurred\n");
                break;
            }
            case INT_INSTRUCTION_COUNT_ZERO:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected Instruction Count Zero interrupt to have occurred\n");
                break;
            }
            case INT_ILLEGAL_ORDERS:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected Illegal Orders interrupt to have occurred\n");
                break;
            }
            case INT_PROGRAM_FAULTS:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected Program Faults interrupt to have occurred\n");
                break;
            }
            case INT_SOFTWARE_INTERRUPT:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected Software interrupt to have occurred\n");
                break;
            }
            default:
            {
                sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Error in test, expecting invalid interrupt number %d\n", interruptNumber);
                break;
            }
        }
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_interrupt_inhibited(void)
{
    mu5_selftest_assert_interrupt_inhibited(localTestContext);
}

static void cpu_selftest_assert_no_interrupt(void)
{
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void cpu_selftest_assert_dod_interrupt_as_system_error(char *name, uint32 mask)
{
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();

    uint32 dod = cpu_selftest_get_register(REG_DOD) & 0xFFFFFFFF;
    if (!(dod & mask))
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected %s bit to be set in DOD\n", name);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_d_error_no_interrupt(char *name, uint32 mask)
{
    cpu_selftest_assert_no_interrupt();

    uint32 dod = cpu_selftest_get_register(REG_DOD) & 0xFFFFFFFF;
    if (!(dod & mask))
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected %s bit to be set in DOD\n", name);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_no_d_interrupt(char *name, uint32 mask)
{
    cpu_selftest_assert_no_interrupt();

    uint32 dod = cpu_selftest_get_register(REG_DOD) & 0xFFFFFFFF;
    if (dod & mask)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected %s bit to be clear in DOD\n", name);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_bound_check_interrupt_as_system_error(void)
{
    cpu_selftest_assert_dod_interrupt_as_system_error("BCH", DOD_BCH_MASK);
}

static void cpu_selftest_assert_its_interrupt_as_system_error(void)
{
    cpu_selftest_assert_dod_interrupt_as_system_error("ITS", DOD_ITS_MASK);
}

static void cpu_selftest_assert_sss_interrupt(void)
{
    cpu_selftest_assert_dod_interrupt_as_system_error("SSS", DOD_SSS_MASK);
}

static void cpu_selftest_assert_bounds_check_no_interrupt(void)
{
    cpu_selftest_assert_d_error_no_interrupt("BCH", DOD_BCH_MASK);
}


static void cpu_selftest_assert_no_bounds_check_interrupt(void)
{
    cpu_selftest_assert_no_d_interrupt("BCH", DOD_BCH_MASK);
}

static void cpu_selftest_assert_no_its_interrupt(void)
{
    cpu_selftest_assert_no_d_interrupt("ITS", DOD_ITS_MASK);
}

static void cpu_selftest_assert_sss_no_interrupt(void)
{
    cpu_selftest_assert_d_error_no_interrupt("SSS", DOD_SSS_MASK);
}

static void cpu_selftest_assert_no_sss_interrupt(void)
{
    cpu_selftest_assert_no_d_interrupt("SSS", DOD_SSS_MASK);
}

static void cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error(void)
{
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0x0010);
}

static void cpu_selftest_assert_name_adder_overflow_interrupt_as_illegal_order(void)
{
    cpu_selftest_assert_interrupt(INT_ILLEGAL_ORDERS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x4000);
}

static void cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error(void)
{
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0x0008);
}

static void cpu_selftest_assert_control_adder_overflow_interrupt_as_illegal_order(void)
{
    cpu_selftest_assert_interrupt(INT_ILLEGAL_ORDERS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x2000);
}

static void cpu_selftest_assert_spm_program_fault_interrupt(void)
{
    cpu_selftest_assert_interrupt(INT_PROGRAM_FAULTS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x0200);
}

static void cpu_selftest_assert_acc_interrupt_as_system_error(void)
{
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0x0040);
}

static void cpu_selftest_assert_acc_interrupt_as_program_fault(void)
{
    cpu_selftest_assert_interrupt(INT_PROGRAM_FAULTS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x0020);
}

static void cpu_selftest_assert_B_or_D_system_error(void)
{
	mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, SYSTEM_ERROR_STATUS_MASK_B_OR_D_ERROR);
}

static void cpu_selftest_assert_B_or_D_interrupt_as_system_error(void)
{
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
    cpu_selftest_assert_B_or_D_system_error();
}

static void cpu_selftest_assert_B_program_fault(void)
{
	mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, PROGRAM_FAULT_STATUS_MASK_B_ERROR);
}

static void cpu_selftest_assert_B_interrupt_as_program_fault(void)
{
    cpu_selftest_assert_interrupt(INT_PROGRAM_FAULTS);
    cpu_selftest_assert_B_program_fault();
}

static void cpu_selftest_assert_D_interrupt_as_program_fault(void)
{
    cpu_selftest_assert_interrupt(INT_PROGRAM_FAULTS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x0040);
}

static void cpu_selftest_assert_illegal_v_store_access_interrupt()
{
    cpu_selftest_assert_interrupt(INT_ILLEGAL_ORDERS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x1000);
}

static void cpu_selftest_assert_illegal_function_as_system_error(void)
{
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0x0020);
}
static void cpu_selftest_assert_illegal_function_as_illegal_order(void)
{
    cpu_selftest_assert_interrupt(INT_ILLEGAL_ORDERS);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x8000);
}

static void cpu_selftest_assert_cpr_not_equivalence_system_error_interrupt(void)
{
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, SYSTEM_ERROR_STATUS_MASK_CPR_NEQV);
}

static void cpu_selftest_assert_test_equals(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_MS, TEST_EQUALS, MS_TEST_MASK);
}

static void cpu_selftest_assert_test_greater_than(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_MS, TEST_GREATER_THAN, MS_TEST_MASK);
}

static void cpu_selftest_assert_test_less_than(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_MS, TEST_LESS_THAN, MS_TEST_MASK);
}

static void cpu_selftest_assert_test_no_overflow(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_MS, 0, MS_OVERFLOW_MASK);
}

static void cpu_selftest_assert_test_overflow(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_MS, TEST_OVERFLOW, MS_OVERFLOW_MASK);
}

static void cpu_selftest_assert_operand_size_32(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_AOD, 0x000, AOD_OPSIZ_MASK);
}

static void cpu_selftest_assert_operand_size_64(void)
{
    cpu_selftest_assert_reg_equals_mask(REG_AOD, AOD_OPSIZ_MASK, AOD_OPSIZ_MASK);
}

static void cpu_selftest_assert_bn(int expectedValue)
{
    uint16 ms = cpu_selftest_get_register(REG_MS) & 0xFFFF;
    uint8 bn = (ms >> 8) & 1;
    if (bn != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "Expected BN to be %d\n", expectedValue);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_boolean_order_condition(CONDITIONTABLE *entry)
{
    uint16 ms = cpu_selftest_get_register(REG_MS) & 0xFFFF;
    uint8 bn = (ms >> 8) & 1;
    if (bn != entry->newBn)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "For BN=%d and R=%d, function %X expected BN to become %d\n", entry->bn, entry->r, entry->func, entry->newBn);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_v_store_contents(uint8 block, uint8 line, t_uint64 expectedValue)
{
    t_uint64 actual = sac_read_v_store(block, line);
    if (actual != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "V-Store block=%u, line=%u, expected 0x%llX, but was x%llX\n", block, line, expectedValue, actual);
        cpu_selftest_set_failure();
    }
}

static void cpu_selftest_assert_fail(void)
{
    mu5_selftest_assert_fail(localTestContext);
}

static void cpu_selftest_set_failure(void)
{
    mu5_selftest_set_failure(localTestContext);
}

void cpu_selftest(TESTCONTEXT *testContext)
{
    int n;

    n = sizeof(tests) / sizeof(UNITTEST);

    localTestContext = testContext;
    localTestContext->dev = &cpu_dev;
    mu5_selftest_run_suite(testContext, tests, n, cpu_selftest_reset);
}

static void cpu_selftest_16_bit_instruction_fetches_using_obey_access(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_setup_test_virtual_pages(SAC_OBEY_ACCESS);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_16_bit_instruction_generates_instruction_access_violation_when_fetching_from_non_executable_segment(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_setup_test_virtual_pages(SAC_READ_ACCESS);
    cpu_selftest_run_code();
    mu5_selftest_assert_instruction_access_violation_as_system_error_interrupt(localTestContext);
}

static void cpu_selftest_instruction_not_obeyed_if_access_violation_when_fetching_instruction(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_setup_test_virtual_pages(SAC_READ_ACCESS);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0);
}

static void cpu_selftest_instruction_fetches_extended_literal_using_obey_access(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_setup_test_virtual_pages(SAC_OBEY_ACCESS);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_sf_using_obey_access(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
    mu5_selftest_setup_cpr(0, CPR_VA(0, SN_DEFAULT, 0), CPR_RA_LOCAL(SAC_READ_ACCESS, 0x1000, 0xC));
    mu5_selftest_setup_cpr(1, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0xC));
    cpu_selftest_run_code_from_location(0x40200000); /* expressed as 16-bit word address */
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_zero_using_obey_access(TESTCONTEXT *testContext)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    mu5_selftest_setup_cpr(0, CPR_VA(0, 0x0000, 0), CPR_RA_LOCAL(SAC_READ_ACCESS, 0x1000, 0xC));
    mu5_selftest_setup_cpr(1, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0xC));
    cpu_selftest_run_code_from_location(0x40200000); /* expressed as 16-bit word address */
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_nb_using_obey_access(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    mu5_selftest_setup_cpr(0, CPR_VA(0, SN_DEFAULT, 0), CPR_RA_LOCAL(SAC_READ_ACCESS, 0x1000, 0xC));
    mu5_selftest_setup_cpr(1, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0xC));
    cpu_selftest_run_code_from_location(0x40200000); /* expressed as 16-bit word address */
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_instruction_fetches_extended_32_bit_variable_offset_from_xnb_using_obey_access(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_setup_default_extra_name_base();
    mu5_selftest_setup_cpr(0, CPR_VA(0, SN_DEFAULT, 0), CPR_RA_LOCAL(SAC_READ_ACCESS, 0x1000, 0xC));
    mu5_selftest_setup_cpr(1, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0xC));
    cpu_selftest_run_code_from_location(0x40200000); /* expressed as 16-bit word address */
    cpu_selftest_assert_no_interrupt();
}


static void cpu_selftest_instruction_access_violation_when_fetching_extended_literal_from_non_executable_segment(TESTCONTEXT *testContext)
{
    /* not sure how valid this test is, with the order crossing a segment boundary*/
    mu5_selftest_setup_cpr(1, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0x0));
    mu5_selftest_setup_cpr(2, CPR_VA(0, 0x2010, 1), CPR_RA_LOCAL(SAC_READ_ACCESS, 0x10, 0x0));
    cpu_selftest_set_load_location(0x1F);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_clear_bcpr();
    cpu_selftest_run_code_from_location(0x4020001F); /* expressed as 16-bit word virtual address */
    mu5_selftest_assert_instruction_access_violation_as_system_error_interrupt(localTestContext);
}

static void cpu_selftest_16_bit_instruction_advances_co_by_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_32_bit_instruction_advances_co_by_2(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_48_bit_instruction_advances_co_by_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 3);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_80_bit_instruction_advances_co_by_5(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x7FFFFFFFFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 5);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dummy_order_advances_co_over_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, 1, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 5);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_advancing_co_across_segment_boundary_with_get_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_load_location(0xFFFF);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code_from_location(0xFFFF);
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_advancing_co_across_segment_boundary_with_set_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_load_location(0xFFFF);
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, 0);
    cpu_selftest_run_code_from_location(0xFFFF);
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_load_operand_6_bit_positive_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000001F);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_6_bit_negative_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x3F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_0(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 0);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 0);
    cpu_selftest_set_register(REG_MS, 0xAA24);
	cpu_selftest_set_register(REG_NB, 0xBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAA24BBBB00000000);
    cpu_execute_next_order();
    cpu_selftest_assert_reg_equals(REG_A, 0xAA24BBBB00000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 1);
    cpu_selftest_set_register(REG_XNB, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_2(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 2);
    cpu_selftest_set_register(REG_SN, 0xAAAA);
    cpu_selftest_set_register(REG_NB, 0xBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 3);
    cpu_selftest_set_register(REG_SN, 0xAAAA);
    cpu_selftest_set_register(REG_SF, 0xBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_4(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 4);
    uint16 initMs = (uint16)cpu_selftest_get_register(REG_MS);
    cpu_selftest_set_register(REG_MS, 0x0100 | (initMs & 0x00FF));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_16(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 16);
    cpu_selftest_set_register(REG_D, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_17(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 17);
    cpu_selftest_set_register(REG_XD, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_18(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 18);
    cpu_selftest_set_register(REG_DT, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_19(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 19);
    cpu_selftest_set_register(REG_XDT, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_20(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 20);
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_DOD, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

/* RNI comment April 2017 in relation to BOD for IR 32: The 1978 manual shows IR32 as just B, so either the 1972 version was wrong or the hardware
   was altered - I have a vague memory that this did happen. The book agrees with the 1978 version, so I'm sure it's
   correct and I've listed the B IR's this way in my reconstruction.
*/
static void cpu_selftest_load_operand_internal_register_32(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 32);
    cpu_selftest_set_register(REG_BOD, 0xAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xBBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000BBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_33(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 33);
    cpu_selftest_set_register(REG_BOD, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_34(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 34);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_48(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 48);
    cpu_selftest_set_register(REG_AEX, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_non_existent_internal_register(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 49);
    cpu_selftest_set_register(REG_AEX, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_variable(TESTCONTEXT *testContext)
{
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V32, n);
	cpu_selftest_setup_default_name_base();
    sac_write_32_bit_word(NAME_SEGMENT_OFFSET_32(n), 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned(TESTCONTEXT *testContext)
{
    int8 n = 0x3F;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V32, n);
	cpu_selftest_setup_default_name_base();
	sac_write_32_bit_word(NAME_SEGMENT_OFFSET_32(n), 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_variable(TESTCONTEXT *testContext)
{
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V64, n);
	cpu_selftest_setup_default_name_base();
	sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), 0xBBBBBBBBAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xBBBBBBBBAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

// TODO: p17 SN interrupt on overflow
// TODO: p17 long instruction offset is signed (?)

static void cpu_selftest_load_operand_b_relative_descriptor_loads_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), d);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB_5, n);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_load_16_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000AAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000000000AA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, 0, 0xAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000000B);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, 0, 0x40);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_with_negative_modifier_generates_bounds_check(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = -1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_load_operand_b_relative_descriptor_modifier_greater_than_bound_generates_bounds_check(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 2;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_load_operand_zero_relative_descriptor_loads_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x1;
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_S0, n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), d);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_S0, n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_16_bit_signed_positive_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0x7FFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000007FFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_16_bit_signed_negative_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_signed_positive_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000007FFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_signed_negative_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_2(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL_3);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_16_bit_unsigned_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000FFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_unsigned_literal(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_6(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL_6);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_7(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL_7);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_literal_kp_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL_1, NP_16_BIT_UNSIGNED_LITERAL);
    sac_write_16_bit_word(1, 0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000FFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_sf(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(NAME_SEGMENT_STACK_OFFSET_32(n), 0xAAAABBBB);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_zero(TESTCONTEXT *testContext)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    sac_write_32_bit_word(ZERO_OFFSET_32(n), 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(NAME_SEGMENT_OFFSET_32(n), 0xAAAABBBB);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(EXTRA_NAME_BASE_OFFSET_32(n), 0xAAAABBBB);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_from_stack(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_STACK);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(0), 0xAAAABBBB);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_DR);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext)
{
	cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_NB_REF);
	sac_write_32_bit_word(NAME_SEGMENT_OFFSET_32(0), 0xAAAABBBB);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_XNB);
    sac_write_32_bit_word(EXTRA_NAME_BASE_OFFSET_32(0), 0xAAAABBBB);
    cpu_selftest_setup_default_extra_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_sf(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero(TESTCONTEXT *testContext)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
	sac_write_64_bit_word(ZERO_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_from_stack(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_STACK);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(0), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_DR);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_NB_REF);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_XNB);
    sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(0), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_setup_default_extra_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB_5, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(ZERO_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_STACK);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_DR);
    cpu_selftest_set_register(REG_B, vecoffset);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_DR);
    cpu_selftest_set_register(REG_B, vecoffset);
    cpu_selftest_set_register(REG_D, d);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB_REF);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_XNB_REF);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_load_16_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000AABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000000000AB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_load_4_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000000C);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_load_1_bit_value_to_descriptor_location(vecorigin, vecoffset, 0x1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    sac_write_64_bit_word(ZERO_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_extra_name_base();
	sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_STACK);
	cpu_selftest_setup_default_stack_base();
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, d);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext)
{
	uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB_REF);
	cpu_selftest_setup_default_name_base();
	sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_XNB_REF);
	cpu_selftest_setup_default_extra_name_base();
	sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_load_16_bit_value_to_descriptor_location(vecorigin, 0, 0xAABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000AABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, 0, 0xAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000000000AB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_load_4_bit_value_to_descriptor_location(vecorigin, 0, 0xC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000000C);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_load_1_bit_value_to_descriptor_location(vecorigin, 0, 0x1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_with_invalid_descriptor_element_size_generates_its_interrupt(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT + 1, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_load_operand_privileged_reads_v_store_in_executive_mode(TESTCONTEXT *testContext)
{
    uint32 base = (TEST_V_STORE_LOCATION_BLOCK *256 + TEST_V_STORE_LOCATION_LINE) << 1; /* multiply by 2 because it is treated as 64-bit address and scaled like one */
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_PRIVILEGED, NP_NB);
    cpu_selftest_load_16_bit_literal(0);
    cpu_selftest_setup_vstore_test_location();
	cpu_selftest_setup_name_base(base);
    sac_write_v_store(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_privileged_generates_interrupt_in_user_mode(TESTCONTEXT *testContext)
{
    uint32 base = TEST_V_STORE_LOCATION_BLOCK * 256 + TEST_V_STORE_LOCATION_LINE;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_PRIVILEGED, NP_NB);
    cpu_selftest_load_16_bit_literal(0);
    cpu_selftest_setup_vstore_test_location();
	cpu_selftest_setup_name_base(base);
	sac_write_v_store(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_v_store_access_interrupt();
}

static void cpu_selftest_store_operand_6_bit_literal_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 0);
    uint16 initMs = (uint16)cpu_selftest_get_register(REG_MS);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, initMs);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt_as_illegal_order_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 1);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XNB, 0);
    cpu_selftest_assert_illegal_function_as_illegal_order();
}

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 1);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XNB, 0);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_1_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_2_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 2);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_3_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_SF, 0);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_4_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 4);
    uint16 initMs = (uint16)cpu_selftest_get_register(REG_MS);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, initMs);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_non_existent_prop_internal_register_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 15);
    uint16 initMs = (uint16)cpu_selftest_get_register(REG_MS);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, initMs);
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_internal_register_16(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 16);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_17(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 17);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_18(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 18);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DT, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_19(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 19);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XDT, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_20(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 20);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

/* RNI comment April 2017 in relation to BOD for IR 32: The 1978 manual shows IR32 as just B, so either the 1972 version was wrong or the hardware
was altered - I have a vague memory that this did happen. The book agrees with the 1978 version, so I'm sure it's
correct and I've listed the B IR's this way in my reconstruction.
*/
static void cpu_selftest_store_operand_internal_register_32(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 32);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_BOD, 0x00000000);
    cpu_selftest_assert_reg_equals(REG_B, 0xBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_33(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 33);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_BOD, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_34(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 34);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_48(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 48);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AEX, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_non_existent_internal_register(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 49);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AEX, 0x0);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_32_bit_variable(TESTCONTEXT *testContext)
{
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V32, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xBBBBBBBBAAAAAAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(NAME_SEGMENT_OFFSET_32(n), 0xAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_64_bit_variable(TESTCONTEXT *testContext)
{
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_loads_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin);
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), d);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAAAAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB_5, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAAAAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, vecoffset, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000AAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_16_bit(vecorigin, vecoffset, 0xAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000000000AA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_8_bit(vecorigin, vecoffset, 0xAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000000B);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_4_bit(vecorigin, vecoffset, 0xB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_1_bit(vecorigin, vecoffset, 0x1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_zero_relative_descriptor_loads_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x1;
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin);
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_S0, n);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), d);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_S0, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, 0, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_literal_kp_0_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, KP_LITERAL, NP_64_BIT_LITERAL_6);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_extended_literal_kp_1_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, KP_LITERAL_1, NP_64_BIT_LITERAL_6);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(NAME_SEGMENT_STACK_OFFSET_32(n), 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero(TESTCONTEXT *testContext)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(ZERO_OFFSET_32(n), 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(NAME_SEGMENT_OFFSET_32(n), 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(EXTRA_NAME_BASE_OFFSET_32(n), 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_from_stack(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_STACK);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(NAME_SEGMENT_STACK_OFFSET_32(1), 0xAAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_DR);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext)
{
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_NB_REF);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(NAME_SEGMENT_OFFSET_32(0), 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_XNB);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(EXTRA_NAME_BASE_OFFSET_32(0), 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero(TESTCONTEXT *testContext)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
	cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(ZERO_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(EXTRA_NAME_BASE_OFFSET_64(n), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}
static void cpu_selftest_store_operand_extended_64_bit_variable_from_stack(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_STACK);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_64(0), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_from_d_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_DR);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb_ref(TESTCONTEXT *testContext)
{
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_NB_REF);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(0), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_XNB);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(EXTRA_NAME_BASE_OFFSET_64(0), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB_5, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(ZERO_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_STACK);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_DR);
    cpu_selftest_set_register(REG_B, vecoffset);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_DR);
    cpu_selftest_set_register(REG_B, vecoffset);
    cpu_selftest_set_register(REG_D, d);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB_REF);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_XNB_REF);
	cpu_selftest_setup_default_extra_name_base();
	cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, vecoffset, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000AABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_16_bit(vecorigin, vecoffset, 0xAABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000000000AB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_8_bit(vecorigin, vecoffset, 0xAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000000C);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_4_bit(vecorigin, vecoffset, 0xC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_1_bit(vecorigin, vecoffset, 0x1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_stack_base();
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    sac_write_64_bit_word(ZERO_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin)); /* TODO: use offset macro, check all these calls */
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_extra_name_base();
	sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_STACK);
	cpu_selftest_setup_default_stack_base();
    sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_from_dr_does_not_load_D(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    t_uint64 d = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, d);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, d);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB_REF);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_XNB_REF);
	cpu_selftest_setup_default_extra_name_base();
	sac_write_64_bit_word(EXTRA_NAME_BASE_OFFSET_64(0), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, 0, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000AABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_16_bit(vecorigin, 0, 0xAABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000000000AB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_8_bit(vecorigin, 0, 0xAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000000C);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_4_bit(vecorigin, 0, 0xC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_1_bit(vecorigin, 0, 0x1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_with_invalid_descriptor_element_size_generates_its_interrupt(TESTCONTEXT *testContext)
{
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(VEC_ORIGIN_DEFAULT);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(n), cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT + 1, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_store_operand_privileged_stores_v_store_in_executive_mode(TESTCONTEXT *testContext)
{
    uint32 base = TEST_V_STORE_LOCATION_BLOCK * 256 + TEST_V_STORE_LOCATION_LINE;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_PRIVILEGED, NP_NB);
    cpu_selftest_load_16_bit_literal(0);
    cpu_selftest_setup_vstore_test_location();
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_name_base(base);
	cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
    cpu_selftest_assert_v_store_contents(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0xAAAABBBBCCCCDDDD);
}

static void cpu_selftest_store_operand_privileged_generates_interrupt_in_user_mode(TESTCONTEXT *testContext)
{
    uint32 base = TEST_V_STORE_LOCATION_BLOCK * 256 + TEST_V_STORE_LOCATION_LINE;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_PRIVILEGED, NP_NB);
    cpu_selftest_load_16_bit_literal(0);
    cpu_selftest_setup_vstore_test_location();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_setup_name_base(base);
	sac_write_v_store(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0x000000000000);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_v_store_access_interrupt();
    cpu_selftest_assert_v_store_contents(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0x000000000000);
}

static void cpu_selftest_any_descriptor_modify_generates_its_interrupt_if_descriptor_has_invalid_size(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT + 1, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XDO, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xd_load_loads_whole_of_XD(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_stack_stacks_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_STACK, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xd_store_stores_xd_to_operand(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS1, F_STORE_XD, K_V64, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xd_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS1, F_STORE_XD, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_sts1_xdb_load_loads_bound_in_XD(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XDB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFCCCCCC);
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xAACCCCCCBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xchk_operand_negative_clears_DOD_XCH_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XCHK, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_DOD, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0x00000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xchk_operand_ge_XDB_clears_DOD_XCH_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XCHK, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 0));
    cpu_selftest_set_register(REG_DOD, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0x00000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xchk_operand_within_XDB_sets_DOD_XCH_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XCHK, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 0));
    cpu_selftest_set_register(REG_DOD, 0x00000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0x00000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_adds_signed_operand_to_D_origin(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 3));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_scales_modifier_for_type_0(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_scales_modifier_for_type_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 2, 5));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_scales_modifier_for_type_2(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 12));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_0_when_US_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 4) | DESCRIPTOR_US_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 5) | DESCRIPTOR_US_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_2_when_US_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 4) | DESCRIPTOR_US_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 5) | DESCRIPTOR_US_MASK);
    cpu_selftest_assert_no_interrupt();
}


static void cpu_selftest_sts1_smod_does_not_check_bounds(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_set_register(REG_DOD, 0x00000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 12));
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_xmod_adds_signed_operand_to_XD_origin_subtracts_from_bound(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_64_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_32_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 1, 12));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_16_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 1, 10));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_8_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_within_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 1, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_crossing_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000003);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 5, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_within_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000004);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 8, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 4, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_crossing_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000009);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 16, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 7, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_checks_bounds_for_type_0(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts1_xmod_checks_bounds_for_type_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts1_xmod_checks_bounds_for_type_2(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_0_when_BC_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_1_when_BC_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_2_when_BC_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_xmod_generates_its_interrupt_if_illegal_descriptor_type_used(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_miscellaneous_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, 3, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_is_type_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_slgc_processes_type_0_descriptors(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_type_2_descriptors(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_long_vector(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000060000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 1, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 2, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x55);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 3));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 3));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0xFF);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 1, 0xFF);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 2, 0xFF);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_L0(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000080000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0x33);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x88);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_L1(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000040000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0x33);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x44);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_L2(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000020000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0x33);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x22);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_L3(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000010000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0x33);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x11);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_smvb_generates_checks_bound_on_destination_not_0(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts1_smvb_copies_byte_with_mask(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080BB);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x2A);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_smvb_uses_filler_when_source_empty_with_mask(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080BB);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x3B);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_smvf_copies_to_zero_length_destination(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080BB);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin));
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_smvf_copies_bytes_and_fills_with_mask(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080BB);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 3));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x2A);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 1, 0x3B);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 2, 0x3B);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_talu_returns_test_register_greater_than_if_not_found(TESTCONTEXT *testContext)
{
    /* TODO: the programming manual (p68) says the bound is in bytes, not clear what the bound in the result should be though */
    uint32 origin = 16;
    cpu_selftest_load_order_extended(CR_STS1, F_TALU, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_set_register(REG_XD, 0x8000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 0xAAAAAAAA);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 0xBBBBBBBB);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 0, origin + 12));
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_0(TESTCONTEXT *testContext)
{
    /* TODO: the programming manual (p68) says the bound is in bytes, not clear what the bound in the result should be though */
    uint32 origin = 16;
    cpu_selftest_load_order_extended(CR_STS1, F_TALU, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x6AAABBBB);
    cpu_selftest_set_register(REG_XD, 0xC000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 0xAAAAAAAA);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 0xAAAABBBB);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 0xBBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, origin + 4));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_2(TESTCONTEXT *testContext)
{
    /* TODO: the programming manual (p68) says the bound is in bytes, not clear what the bound in the result should be though */
    uint32 origin = 16;
    cpu_selftest_load_order_extended(CR_STS1, F_TALU, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x6AAABBBB);
    cpu_selftest_set_register(REG_XD, 0xC000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 0xAAAAAAAA);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 0xAAAABBBB);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 0xBBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, origin + 4));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_talu_generates_its_interrupt_if_descriptor_is_not_32_bit(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    cpu_selftest_load_order_extended(CR_STS1, F_TALU, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x6AAABBBB);
    cpu_selftest_set_register(REG_XD, 0xC000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, origin));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}


static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_source_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 1, 0xBB);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 2, 0xCC);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x2A);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x3B);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0x4C);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 3));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 3));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical_with_filler(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080FF);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x2A);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x7F);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0x7F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 3));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_destination_is_shorter_and_subset_is_equal(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 1, 0xBB);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 2, 0xCC);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x2A);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_greater_than_if_source_byte_greater_than_destination_byte(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 1, 0xBB);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 2, 0xCC);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x2A);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x3A);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0x4C);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin + 1));
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_less_than_if_source_byte_less_than_destination_byte(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 1, 0xBB);
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 2, 0xCC);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x2A);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x3C);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0x4C);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin + 1));
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts1_sub1_loads_XD_and_modifies_it(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    cpu_selftest_load_order_extended(CR_STS1, F_SUB1, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 4);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 2);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 16);
    cpu_selftest_set_register(REG_B, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 0, origin + (3 * 4)));
}

static void cpu_selftest_sts1_sub1_calculates_B(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    cpu_selftest_load_order_extended(CR_STS1, F_SUB1, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 4);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 2);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 16);
    cpu_selftest_set_register(REG_B, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, (8-4)*2);
}

static void cpu_selftest_sts1_sub1_modifies_D(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    uint32 expectedB = (8 - 4) * 2;
    cpu_selftest_load_order_extended(CR_STS1, F_SUB1, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 4);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 2);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 16);
    cpu_selftest_set_register(REG_B, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(0, 0, 16 - expectedB, 0 + (expectedB >> 3))); /* size 0 is 1-bit vector */
}

static void cpu_selftest_sts1_sub1_generates_its_interrupt_if_descriptor_is_not_valid(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    uint32 expectedB = (8 - 4) * 2;
    cpu_selftest_load_order_extended(CR_STS1, F_SUB1, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 3, origin));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_do_load_loads_ls_half_of_D(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_D, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS2, F_LOAD_DO, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_load_loads_whole_of_D(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_D, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS2, F_LOAD_D, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_stack_load_stacks_D_loads_new_D(TESTCONTEXT *testContext)
{
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_load_order_extended(CR_STS2, F_STACK_LOAD_D, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_D, 0xCCCCCCCCFFFFFFFF);
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_store_stores_d_to_operand(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS2, F_STORE_D, K_V64, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS2, F_STORE_D, K_SB, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_sts2_db_load_loads_bound_in_D(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_LOAD_DB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFCCCCCC);
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xAACCCCCCBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mdr_advances_location_and_loads_D_with_operand_pointed_to_by_D(TESTCONTEXT *testContext)
{
    uint32 origin = 0x20;
    t_uint64 newD = cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 3, 0x30);
    cpu_selftest_load_order_extended(CR_STS2, F_MDR, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, origin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(origin, 1, newD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, newD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_adds_signed_operand_to_D_origin_subtracts_from_bound(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_64_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_32_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 1, 12));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_16_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 1, 10));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_8_bit_value(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_within_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 1, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_crossing_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000003);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 5, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_within_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000004);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 8, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 4, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_crossing_byte(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000009);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 16, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 7, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_checks_bounds_for_type_0(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts2_mod_checks_bounds_for_type_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts2_mod_checks_bounds_for_type_2(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_0_when_BC_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_1_when_BC_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_2_when_BC_set(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts2_rmod_loads_least_significant_half_of_D_and_adds_to_origin(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_RMOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_D, 0xBBBBBBBB11111111);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xAAAAAAAACCCCCCCC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L0(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000080033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x88);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_blgc_processes_type_1_descriptor_L1(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000040033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x44);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_blgc_processes_type_2_descriptor_L2(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000020033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x22);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L3(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000010033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x55);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x11);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bmvb_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bmvb_generates_checks_bound_on_destination_not_0(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bmvb_copies_byte_with_mask(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080AA);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x2A);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts2_bmve_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BMVE, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bmve_copies_byte_with_mask_to_whole_destination(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BMVE, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080AA);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 3));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x2A);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 1, 0x2A);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 2, 0x2A);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts2_smvf_copies_bytes_and_fills_with_mask(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 16;
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080BB);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(sourceOrigin, 0, 0xAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, sourceOrigin + 1));
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 3));
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 0, 0x2A);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 1, 0x3B);
    cpu_selftest_assert_vector_content_8_bit(destinationOrigin, 2, 0x3B);
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
    cpu_selftest_assert_no_bounds_check_interrupt();
}

static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bscn_sets_less_than_if_byte_not_found(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000080033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x31);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x32);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 2));
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bscn_finds_byte_in_type_0_descriptor(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0xB2);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0xB3);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0xB3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bscn_finds_byte_in_type_1_descriptor(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0xB2);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0xB3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bscn_finds_byte_in_type_2_descriptor(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0xB2);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0xB3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_not_8_bit(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_is_type_3(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}

static void cpu_selftest_sts2_bcmp_sets_equals_if_byte_found_in_all_elements(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0xB3);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0xB3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 2));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_smaller(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000008033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0xB3);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0xB2); /* when considering mask, byte is bigger than this element */
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0xB2);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin + 1));
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_larger(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x00000000000080B3);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 3, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0xB3);
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 1, 0x34); /* when considering mask, byte is smaller than this element */
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 2, 0x34);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin + 1));
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bcmp_finds_byte_in_type_0_descriptor(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x33);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bcmp_finds_byte_in_type_1_descriptor(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x33);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_bcmp_finds_byte_in_type_2_descriptor(TESTCONTEXT *testContext)
{
    uint32 destinationOrigin = 32;
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000033);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, destinationOrigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(destinationOrigin, 0, 0x33);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 0, destinationOrigin + 1));
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_its_interrupt();
    cpu_selftest_assert_no_sss_interrupt();
}

static void cpu_selftest_sts2_sub2_modifies_XD(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    cpu_selftest_load_order(CR_STS2, F_SUB2, K_LITERAL, 0);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 4);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 2);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 16);
    cpu_selftest_set_register(REG_B, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 0, origin + (3 * 4)));
}

static void cpu_selftest_sts2_sub2_calculates_B(TESTCONTEXT *testContext)
{
    uint32 origin = 16;
    cpu_selftest_load_order(CR_STS2, F_SUB2, K_LITERAL, 0);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, origin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 0, 4);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 1, 2);
    cpu_selftest_load_32_bit_value_to_descriptor_location(origin, 2, 16);
    cpu_selftest_set_register(REG_B, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, (8 - 4) * 2);
}

static void cpu_selftest_sts2_sub2_modifies_D_existing_D(TESTCONTEXT *testContext)
{
    uint32 xdOrigin = 16;
    uint32 dOrigin = 32;
    uint32 expectedB = (8 - 4) * 2;
    cpu_selftest_load_order(CR_STS2, F_SUB2, K_LITERAL, 0);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 3, xdOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, dOrigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(xdOrigin, 0, 4);
    cpu_selftest_load_32_bit_value_to_descriptor_location(xdOrigin, 1, 2);
    cpu_selftest_load_32_bit_value_to_descriptor_location(xdOrigin, 2, 16);
    cpu_selftest_set_register(REG_B, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 16 - expectedB, dOrigin + expectedB)); /* size 0 is 1-bit vector */
}

static void cpu_selftest_sts1_sub2_generates_its_interrupt_if_descriptor_is_not_valid(TESTCONTEXT *testContext)
{
    uint32 xdOrigin = 16;
    uint32 dOrigin = 32;
    cpu_selftest_load_order(CR_STS2, F_SUB2, K_LITERAL, 0);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 3, xdOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, dOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt_as_system_error();
}


static void cpu_selftest_b_load_loads_B(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_B, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_load_and_decrement_loads_B_and_subtracts_1(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x0000000F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x0000000E);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_load_and_decrement_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_b_stack_and_load_stacks_B_and_loads_B(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_STACK_LOAD_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_B, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0x00000000AAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_store_stores_B(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_B, F_STORE_B, K_V64, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_B, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_add_adds_operand_to_B(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_ADD_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x0AAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x0AAAAAA9);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_add_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_ADD_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_b_sub_subtracts_operand_from_B(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_SUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xAAAAAAAB);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_sub_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_SUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_b_mul_multiplies_operand_by_B(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_MUL_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-5);
    cpu_selftest_set_register(REG_B, 6);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, -30 & 0xFFFFFFFF);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_mul_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_MUL_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_b_xor(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_XOR_B, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x66666666);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_or(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_OR_B, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xEEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_shift_shifts_left_for_positive_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_B, F_SHIFT_L_B, K_LITERAL, 0x02);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFF8);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_shift_shifts_right_for_negative_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_B, F_SHIFT_L_B, K_LITERAL, 0x3E);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xE0000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_shift_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_B, F_SHIFT_L_B, K_LITERAL, 0x01);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_b_and(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_AND_B, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x88888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_rsub_subtracts_B_from_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_RSUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xAAAAAAAB);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_rsub_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_RSUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_b_comp_sets_less_than_when_B_less_than_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_comp_sets_equals_when_B_equals_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_comp_sets_greater_than_when_B_greater_than_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x00000001);
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_comp_sets_overflow_in_t0_only(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x80000000);
    cpu_selftest_assert_test_overflow();
    cpu_selftest_assert_no_b_overflow();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_cinc_compares_B_with_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_CINC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFE);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_cinc_increments_B(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_CINC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x00000000);
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_cinc_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_CINC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt_as_system_error();
}

static void cpu_selftest_x_load_loads_X(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_LOAD_X, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_stack_and_load_stacks_X_and_loads_X(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_STACK_LOAD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_X, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0x00000000AAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_store_stores_X(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_XS, F_STORE_X, K_V64, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_X, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_add_adds_operand_to_X(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x0AAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x0AAAAAA9);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_x_add_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error();
}

static void cpu_selftest_x_sub_subtracts_operand_from_X(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_SUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xAAAAAAAB);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_x_sub_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_SUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error();
}

static void cpu_selftest_x_mul_multiplies_operand_by_X(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_MUL_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-5);
    cpu_selftest_set_register(REG_X, 6);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, -30 & 0xFFFFFFFF);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_x_mul_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_MUL_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_X, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error();
}

static void cpu_selftest_x_div_divides_X_by_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_DIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-2);
    cpu_selftest_set_register(REG_X, -4 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x00000002);
    cpu_selftest_assert_no_a_zero_divide_interrupt();
}

static void cpu_selftest_x_div_flags_divide_by_zero(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_DIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000000);
    cpu_selftest_set_register(REG_X, -4 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_zero_divide_interrupt();
}

static void cpu_selftest_x_xor(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_XOR_X, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x66666666);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_or(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_OR_X, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xEEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_shift_shifts_left_for_positive_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_XS, F_SHIFT_L_X, K_LITERAL, 0x02);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFF8);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_shift_shifts_right_for_negative_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_XS, F_SHIFT_L_X, K_LITERAL, 0x3E);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xE0000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_shift_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_XS, F_SHIFT_L_X, K_LITERAL, 0x01);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error();
}

static void cpu_selftest_x_rdiv_divides_operand_by_X(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_RDIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-4);
    cpu_selftest_set_register(REG_X, -2 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x00000002);
    cpu_selftest_assert_no_a_zero_divide_interrupt();
}

static void cpu_selftest_x_rdiv_flags_divide_by_zero(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_RDIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-4);
    cpu_selftest_set_register(REG_X, 0x00000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_zero_divide_interrupt();
}

static void cpu_selftest_x_and(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_AND_X, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x88888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_rsub_subtracts_X_from_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_RSUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xAAAAAAAB);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_x_rsub_flags_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_RSUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_fixed_point_overflow_interrupt_as_system_error();
}

static void cpu_selftest_x_comp_sets_less_than_when_X_less_than_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_acc_overflow();
}

static void cpu_selftest_x_comp_sets_equals_when_X_equals_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_acc_overflow();
}

static void cpu_selftest_x_comp_sets_greater_than_when_X_greater_than_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x00000001);
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_acc_overflow();
}

static void cpu_selftest_x_comp_sets_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x80000000);
    cpu_selftest_assert_test_overflow();
}

static void cpu_selftest_x_comp_overflow_does_not_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x80000000);
    cpu_selftest_assert_no_acc_overflow();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_load_loads_AOD(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_LOAD_AOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFEDC);
    cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AOD, 0x0000000000001EDC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_stack_and_load_stacks_AOD_and_loads_AOD(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_STACK_LOAD_AOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode();
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_AOD, 0xBBBBBBBBFFFFFEDC);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0x0000000000001EDC);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_AOD, 0x0000000000001FFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_store_stores_AOD(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_AU, F_STORE_AOD, K_V64, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_AOD, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0x0000000000001FFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_add_adds_operand_to_A(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_ADD_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000100000000);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_a_sub_subtracts_operand_from_A(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_SUB_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFE);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_a_mul_multiplies_operand_by_A(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_MUL_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_set_register(REG_A, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000100000000);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_a_xor(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_XOR_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000066666666);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_or(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_OR_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000EEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_shift_shifts_left_for_positive_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_SHIFT_L_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAA82);
    cpu_selftest_set_register(REG_A, 0x0000000180000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000600000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_shift_shifts_right_for_negative_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_SHIFT_L_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAFE);
    cpu_selftest_set_register(REG_A, 0x0000000180000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000060000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_and(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_AND_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000088888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_rsub_subtracts_A_from_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_RSUB_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_A, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFE);
    cpu_selftest_assert_no_acc_overflow_interrupt();
}

static void cpu_selftest_a_comp_sets_less_than_when_A_less_than_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_COMP_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFF80000000);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFF7FFFFFFF);
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_a_comp_sets_equals_when_A_equals_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_COMP_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFF80000000);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAA80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAA80000000);
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_a_comp_sets_greater_than_when_A_greater_than_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_AU, F_COMP_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFF7FFFFFFF);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFF80000000);
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_dec_load_loads_AEX(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_ADC, F_LOAD_AEX, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AEX, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dec_stack_and_load_stacks_AEX_and_loads_AEX(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_ADC, F_STACK_LOAD_AEX, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_AEX, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_AEX, 0xCCCCCCCCFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dec_store_stores_AEX(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_ADC, F_STORE_AEX, K_V64, n);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_AEX, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dec_comp_sets_overflow_when_AOD_and_operand_non_zero(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_ADC, F_COMP_AOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFFFFFFF);
	cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_AOD, 0x00000000000011F1);
	cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AOD, 0x00000000000011F1);
    cpu_selftest_assert_test_overflow();
}

static void cpu_selftest_dec_comp_clears_overflow_when_AOD_and_operand_is_zero(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_ADC, F_COMP_AOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFFFFFFF);
    cpu_selftest_set_register(REG_AOD, 0xFFFFFFFFFFFFE000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AOD, 0xFFFFFFFFFFFFE000);
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_flt_load_single_loads_32_bits_into_A(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_32, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xBBBBBBBB00000000);
    cpu_selftest_assert_operand_size_32();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_load_double_loads_64_bits_into_A(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_operand_size_64();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_32_bits(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STACK_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_set_aod_operand_32_bit();
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0x00000000AAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFF00000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_64_bits(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STACK_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_set_aod_operand_64_bit();
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_reg_equals(REG_A, 0xCCCCCCCCFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_store_stores_A_32_bits(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, n);
    cpu_selftest_set_aod_operand_32_bit();
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_store_stores_A_64_bits(TESTCONTEXT *testContext)
{
    int8 n = 0x2;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, n);
    cpu_selftest_set_aod_operand_64_bit();
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_OFFSET_64(n), 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_xor(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_XOR, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x6666666666666666);
    cpu_selftest_assert_no_interrupt();
}
static void cpu_selftest_flt_or(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_OR, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xEEEEEEEEEEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_shift_shifts_left_circular_for_positive_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_SHIFT_CIRC, K_LITERAL, 4);
    cpu_selftest_set_register(REG_A, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xBBBBBBBFFFFFFFFB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_shift_shifts_right_circular_for_negative_operand(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order(CR_FLOAT, F_SHIFT_CIRC, K_LITERAL, 0x3C);
    cpu_selftest_set_register(REG_A, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFBBBBBBBBFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_and(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_AND, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x8888888888888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_relative_jump_jumps_forward(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_literal(F_RELJUMP, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_relative_jump_jumps_backward(TESTCONTEXT *testContext)
{
    cpu_selftest_set_load_location(8);
    cpu_selftest_load_organisational_order_literal(F_RELJUMP, 0x3F);
    cpu_selftest_run_code_from_location(8);
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000007);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_relative_jump_across_segement_boundary_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_literal(F_RELJUMP, 0x3E);
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_exit_resets_link_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_EXIT, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBFFFFFFFF);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xAAAA);
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x7FFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_exit_resets_link_except_privileged_ms_bits_in_user_mode(TESTCONTEXT *testContext)
{
    uint16 initMs;
    cpu_selftest_load_organisational_order_extended(F_EXIT, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBAFFBBBBFFFFFFFF);
    cpu_selftest_set_user_mode();
    initMs = cpu_get_ms();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0x8A00 | (initMs & 0x30FF));
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x7FFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_absolute_jump(TESTCONTEXT *testContext)
{
    cpu_selftest_set_load_location(8);
    cpu_selftest_load_organisational_order_literal(F_ABSJUMP, 0x10);
    cpu_selftest_run_code_from_location(8);
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000010);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_return_sets_SF_and_unstacks_link(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_RETURN, K_V64, NP_STACK);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), 0xFFFFBBBBAAAAAAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, NB_DEFAULT - 2);
    cpu_selftest_assert_reg_equals(REG_MS, 0xFFFF);
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x2AAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_return_resets_link_except_privileged_ms_bits_in_user_mode(TESTCONTEXT *testContext)
{
    uint16 initMs;
    cpu_selftest_load_organisational_order_extended(F_RETURN, K_V64, NP_STACK);
    sac_write_64_bit_word(NAME_SEGMENT_OFFSET_64(0), 0xFFFFBBBBAAAAAAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_user_mode();
    initMs = cpu_get_ms();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xCF00 | (initMs & 0x30FF));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_return_does_not_pop_stack_if_operand_is_not_stack_but_sets_NB(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_RETURN, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFBBBBAAAAAAAA);
	cpu_selftest_setup_default_name_base();
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, NB_DEFAULT);
    cpu_selftest_assert_reg_equals(REG_MS, 0xFFFF);
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x2AAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_XCn_stacks_operand_and_jumps_to_offset_n(TESTCONTEXT *testContext)
{
    for (int i = 0; i < 7; i++)
    {
        cpu_selftest_set_load_location(0);
        cpu_selftest_load_organisational_order_extended(F_XC0 + i, KP_LITERAL, NP_64_BIT_LITERAL);
        cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
	    cpu_selftest_setup_default_stack_base();
        sac_write_64_bit_word(NAME_SEGMENT_STACK_OFFSET_32(2), 0x0000000000000000);
        cpu_selftest_run_code();
        cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0xAAAABBBBCCCCDDDD);
        cpu_selftest_assert_reg_equals(REG_CO, 0x20010000 | (uint8)i);
        cpu_selftest_assert_no_interrupt();
    }
}

static void cpu_selftest_org_XCn_sets_executive_mode(TESTCONTEXT *testContext)
{
    for (int i = 0; i < 7; i++)
    {
        cpu_selftest_set_load_location(0);
        cpu_selftest_load_organisational_order_extended(F_XC0 + i, KP_LITERAL, NP_64_BIT_LITERAL);
        cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
        cpu_selftest_set_user_mode();
        cpu_selftest_run_code();
        cpu_selftest_assert_reg_equals_mask(REG_MS, ~0, 0x0004); /* not mentioned in section 6.6 of manual, but is mentioned section 7.2. RNI confirmed this is needed 26/2/2017 */
    }
}

static void cpu_selftest_org_stacklink_puts_link_on_stack_adding_operand_to_stacked_CO(TESTCONTEXT *testContext)
{
    cpu_selftest_set_load_location(10);
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x000000000000000A);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_set_register(REG_MS, 0xAA24);
    cpu_selftest_set_register(REG_NB, 0xBBBA);
    cpu_selftest_run_code_from_location(10);
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), 0xAA24BBBA00000014);
    cpu_selftest_assert_reg_equals(REG_SF, SF_DEFAULT + 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_stacklink_treats_operand_as_signed(TESTCONTEXT *testContext)
{
    /* Comment from RNI:
    The adder used to add the operand to the value of CO for a STACKLINK operation is the same adder that executes control transfers.
    Since these can jump backwards or forwards, the adder just treats the operand as a signed value in every case. For STACKLINK it
    will normally be a small positive number, as you surmise, the value depending on the number of parameters being passed, as shown
    in the example on page 62.
    */
    t_uint64 initMs = cpu_selftest_get_register(REG_MS);
    cpu_selftest_set_load_location(10);
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFE);
	cpu_selftest_setup_default_stack_base();
    cpu_selftest_run_code_from_location(10);
    cpu_selftest_assert_memory_contents_64_bit(NAME_SEGMENT_STACK_OFFSET_32(2), (initMs << 48) | 0x0000000000000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_overflows_segment_boundary(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_underflows_segment_boundary(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_stacklink_generates_illegal_order_interrupt_if_segment_overflow_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_illegal_order();
}

static void cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_level0_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_level0_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_level1_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_level1_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_stacklink_generates_system_error_interrupt_if_segment_overflow_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_ms_load_sets_unmasked_bits_only_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_MS_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAFFBBFF);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xAABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_ms_load_does_not_set_masked_bits_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_MS_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xBBCCCF78);
    cpu_selftest_set_register(REG_MS, 0x0024);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0x884C);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_ms_load_does_not_set_privileged_unmasked_bits_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_MS_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xBAFFFBFF);
    cpu_selftest_set_register(REG_MS, 0x1048);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0x9A48);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_dl_load_sets_dl_pseudo_register(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_DL_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DL, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_spm_dummy(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SPM, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 3);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_setlink_stores_link(TESTCONTEXT *testContext)
{
	int8 n = 0x1;
	cpu_selftest_set_load_location(10);
    cpu_selftest_load_organisational_order_extended(F_SETLINK, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_MS, 0xAA24);
    cpu_selftest_setup_name_base(0xBBBB);
    cpu_selftest_run_code_from_location(10);
    cpu_selftest_assert_memory_contents_64_bit(ZERO_OFFSET_64(n), 0xAA24BBBB0000000A);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_xnb_load_loads_XNB(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xABABABABCFCFCFCF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XNB, 0xCFCFCFCE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sn_load_in_user_mode_does_not_load_SN(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SN_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xABABABABCFCF);
    cpu_selftest_set_register(REG_SN, 0xAAAA);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0xAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sn_load_in_executive_mode_loads_SN(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SN_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xABABABABCFCFABAB);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0xCFCF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_xnb_plus_adds_operand_to_XNB(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x3333FFFE);
    cpu_selftest_set_register(REG_XNB, 0xAAAA4444);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XNB, 0xAAAA4442);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_xnb_plus_generates_interrupt_if_segment_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_set_register(REG_XNB, 0xAAAAFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_xnb_plus_generates_interrupt_if_segment_underflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_xnb_plus_generates_illegal_order_interrupt_if_segment_overflow_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_illegal_order();
}

static void cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_level0_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_set_level0_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_level1_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_set_level1_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_xnb_plus_generates_system_error_interrupt_if_segment_overflow_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_xnb_store_stores_XNB(TESTCONTEXT *testContext)
{
    int8 n = 1;
    cpu_selftest_load_organisational_order_extended(F_XNB_STORE, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_segment();
    cpu_selftest_set_register(REG_XNB, 0xBBBBAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(ZERO_OFFSET_64(n), 0x00000000BBBBAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_xnb_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_STORE, K_SB, 0);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_org_sf_load_loads_SF(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SF_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xABABABABBCBCCFCF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, 0xCFCE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sf_plus_adds_operand_to_SF(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SF_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x3333FFFE);
    cpu_selftest_setup_stack_base(0x4444);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, 0x4442);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sf_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext) 
{
    cpu_selftest_load_organisational_order_extended(F_SF_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
	cpu_selftest_setup_stack_base(0xFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_sf_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SF_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
	cpu_selftest_setup_stack_base(0x0002);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_sf_load_nb_plus_adds_NB_to_signed_operand_and_stores_to_SF(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_literal(F_SF_LOAD_NB_PLUS, 0x3F);
    cpu_selftest_setup_name_base(10);
    cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_SF, 0x00000008);
	cpu_selftest_assert_reg_equals(REG_SN, SN_DEFAULT);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SF_LOAD_NB_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x0000FFFF);
	cpu_selftest_setup_name_base(10);
	cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SF_LOAD_NB_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFB);
    cpu_selftest_setup_name_base(2);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_sf_store_stores_SF(TESTCONTEXT *testContext)
{
	int8 n = 0x1;
	cpu_selftest_load_organisational_order_extended(F_SF_STORE, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_default_name_base();
	cpu_selftest_setup_stack_base(0xAAAA);
	cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(ZERO_OFFSET_64(n), 0x000000000000AAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sf_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_SF_STORE, K_SB, 0);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_org_nb_load_loads_NB(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_LOAD, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xABABCFCF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_NB, 0xCFCE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_nb_load_sf_plus_adds_SF_to_signed_operand_and_stores_to_NB(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_literal(F_NB_LOAD_SF_PLUS, 0x3F);
    cpu_selftest_setup_stack_base(10);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_NB, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_LOAD_SF_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x0000FFFF);
	cpu_selftest_setup_stack_base(10);
	cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_LOAD_SF_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFB);
	cpu_selftest_setup_stack_base(2);
	cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_nb_plus_adds_signed_operand_to_NB(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x3333FFFE);
	cpu_selftest_setup_name_base(0x4444);
    cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_NB, 0x4442);
	cpu_selftest_assert_reg_equals(REG_SN, SN_DEFAULT);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_nb_plus_generates_interrupt_on_segment_overflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
	cpu_selftest_setup_name_base(0xFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_nb_plus_generates_interrupt_on_segment_underflow(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
	cpu_selftest_setup_name_base(0x0002);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_org_nb_store_stores_SN_and_NB(TESTCONTEXT *testContext)
{
	int8 n = 0x1;
	cpu_selftest_load_organisational_order_extended(F_NB_STORE, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_setup_name_base(0xBBBA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(ZERO_OFFSET_64(n), 0x000000000001BBBA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_nb_store_to_secondary_operand_generates_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_NB_STORE, K_SB, 0);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static uint16 cpu_selftest_calculate_ms_from_t_bits(uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 initMs = (uint16)cpu_selftest_get_register(REG_MS);
    uint16 ms = ((t0 & 1) << 11) | ((t1 & 1) << 10) | ((t2 & 1) << 9) | ((bn & 1) << 8) | (initMs & 0x00FF);
    return ms;
}

static void cpu_selftest_org_branch_test_branch_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 ms = cpu_selftest_calculate_ms_from_t_bits(t0, t1, t2, bn);
    cpu_selftest_set_load_location(0); /* reset load location as we may call this helper more than once in a test to test different combinations */
    cpu_selftest_load_organisational_order_literal(f, 8);
    cpu_selftest_set_register(REG_MS, ms);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_branch_test_branch_not_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 ms = cpu_selftest_calculate_ms_from_t_bits(t0, t1, t2, bn);
    cpu_selftest_set_load_location(0); /* reset load location as we may call this helper more than once in a test to test different combinations */
    cpu_selftest_load_organisational_order_literal(f, 8);

    cpu_selftest_set_register(REG_MS, ms);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_bn_test_true_result(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 ms = cpu_selftest_calculate_ms_from_t_bits(t0, t1, t2, bn);
    cpu_selftest_set_load_location(0); /* reset load location as we may call this helper more than once in a test to test different combinations */
    cpu_selftest_load_organisational_order_literal(f, 3); /* 3 copies R to BN */
    cpu_selftest_set_register(REG_MS, ms);
    cpu_selftest_run_code();
    cpu_selftest_assert_bn(1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_bn_test_false_result(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 ms = cpu_selftest_calculate_ms_from_t_bits(t0, t1, t2, bn);
    cpu_selftest_set_load_location(0); /* reset load location as we may call this helper more than once in a test to test different combinations */
    cpu_selftest_load_organisational_order_literal(f, 3); /* 3 copies R to BN */
    cpu_selftest_set_register(REG_MS, ms);
    cpu_selftest_run_code();
    cpu_selftest_assert_bn(0);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_br_eq_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_EQ, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_EQ, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_eq_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_EQ, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_EQ, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_ne_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_NE, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_NE, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_ne_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_NE, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_NE, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_ge_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GE, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GE, 0, 1, 1, 0);
}

static void cpu_selftest_org_br_ge_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GE, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GE, 1, 1, 0, 1);
}

static void cpu_selftest_org_br_lt_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LT, 1, 1, 0, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LT, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_lt_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LT, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LT, 0, 0, 1, 0);
}

static void cpu_selftest_org_br_le_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LE, 1, 1, 0, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LE, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_le_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 1, 0, 0, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 0, 0, 0, 0);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 0, 1, 1, 0);
}

static void cpu_selftest_org_br_gt_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 1, 0, 0, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 0, 1, 1, 0);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 0, 0, 0, 0);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 0, 0, 1, 0);
}

static void cpu_selftest_org_br_gt_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GT, 1, 1, 0, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GT, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_ovf_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_OVF, 0, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_OVF, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_ovf_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_OVF, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_OVF, 1, 0, 0, 0);
}

static void cpu_selftest_org_br_bn_does_not_branch_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_BN, 1, 1, 1, 0);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_BN, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_bn_does_branch_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_BN, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_BN, 0, 0, 0, 1);
}

static void cpu_selftest_org_bn_function_tests(TESTCONTEXT *testContext)
{
    int n = sizeof(conditionalFuncsTable) / sizeof(CONDITIONTABLE);
    int i;
    for (i = 0; i < n; i++)
    {
        CONDITIONTABLE *entry = &conditionalFuncsTable[i];
        cpu_selftest_set_load_location(0);
        cpu_selftest_load_organisational_order_extended(F_BN_EQ, K_LITERAL, NP_64_BIT_LITERAL);
        cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFFFFFF0 | (entry->func & 0xF));
        cpu_selftest_set_bn(entry->bn);
        cpu_selftest_set_test_is_zero(entry->r);
        cpu_selftest_run_code();
        cpu_selftest_assert_boolean_order_condition(entry);
        cpu_selftest_assert_no_interrupt();
    }
}

static void cpu_selftest_org_bn_eq_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_EQ, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_eq_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_EQ, 1, 0, 1, 1);
}

static void cpu_selftest_org_bn_ne_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_NE, 1, 0, 1, 1);
}

static void cpu_selftest_org_bn_ne_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_NE, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_ge_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_GE, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_ge_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_GE, 1, 0, 1, 1);
}

static void cpu_selftest_org_bn_lt_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_LT, 1, 1, 0, 1);
}

static void cpu_selftest_org_bn_lt_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_LT, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_le_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_LE, 1, 1, 0, 1);
}

static void cpu_selftest_org_bn_le_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_LE, 1, 0, 0, 1);
}

static void cpu_selftest_org_bn_gt_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_GT, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_gt_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_GT, 1, 1, 0, 1);
}

static void cpu_selftest_org_bn_ovf_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_OVF, 0, 1, 1, 1);
}

static void cpu_selftest_org_bn_ovf_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_OVF, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_bn_on_false(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_false_result(F_BN_BN, 1, 1, 1, 0);
}

static void cpu_selftest_org_bn_bn_on_true(TESTCONTEXT *testContext)
{
    cpu_selftest_org_bn_test_true_result(F_BN_BN, 1, 1, 1, 1);
}

static void cpu_selftest_org_bn_order_tests(TESTCONTEXT *testContext)
{
    int n = sizeof(conditionalFuncsTable) / sizeof(CONDITIONTABLE);
    int i;
    for (i = 0; i < n; i++)
    {
        CONDITIONTABLE *entry = &conditionalFuncsTable[i];
        cpu_selftest_set_load_location(0);
        cpu_selftest_load_organisational_order_literal(F_BN_0 + entry->func, entry->r);
        cpu_selftest_set_bn(entry->bn);
        cpu_selftest_run_code();
        cpu_selftest_assert_boolean_order_condition(entry);
        cpu_selftest_assert_no_interrupt();
    }
}

static void cpu_selftest_setting_b_or_d_fault_in_executive_mode_generates_system_error_interrupt(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_b_and_d_faults_to_system_error_in_exec_mode();
	cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
	cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_b_or_d_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
	cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_b_fault_in_user_mode_generates_program_fault_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_clear_inhibit_program_fault_interrupts();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_assert_B_interrupt_as_program_fault();
}

static void cpu_selftest_setting_b_fault_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_inhibit_program_fault_interrupts();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_bod_b_overflow_in_executive_mode_generates_b_or_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_bod_b_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK | BOD_IBOVF_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_bod_b_overflow_in_user_mode_generates_b_program_fault_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_assert_B_interrupt_as_program_fault();
}

static void cpu_selftest_setting_bod_b_overflow_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK | BOD_IBOVF_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_clearing_bod_b_overflow_in_executive_mode_does_not_clear_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_set_register(REG_BOD, 0);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_clearing_bod_b_overflow_in_user_mode_does_not_clear_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_set_register(REG_BOD, 0);
    cpu_selftest_assert_B_interrupt_as_program_fault();
}

static void cpu_selftest_switch_from_executive_mode_to_user_mode_when_system_error_does_not_create_program_fault(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_set_user_mode();
    cpu_selftest_assert_no_program_fault();
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_switch_from_user_mode_to_executive_mode_program_fault_does_not_create_system_error(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_set_executive_mode();
    cpu_selftest_assert_no_system_error();
    cpu_selftest_assert_B_interrupt_as_program_fault();
}

static void cpu_selftest_return_from_interrupt_handler_without_clearing_system_error_status_v_line_masks_system_error_interrupts(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_interrupt_vector(INT_SYSTEM_ERROR, MS_MASK_EXEC | MS_MASK_LEVEL0 | MS_MASK_LEVEL1, 0, 0);
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_run_code();
    cpu_selftest_set_register(REG_BOD, 0);
    cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
    cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_setting_acc_fault_in_executive_mode_generates_system_error_interrupt(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_acc_faults_to_system_error_in_exec_mode();
	cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
	cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_setting_acc_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode();
	cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_acc_fault_in_user_mode_generates_program_fault_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_clear_inhibit_program_fault_interrupts();
    cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
    cpu_selftest_assert_acc_interrupt_as_program_fault();
}

static void cpu_selftest_setting_acc_fault_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_inhibit_program_fault_interrupts();
    cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_aod_floating_point_overflow_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
	cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_setting_aod_floating_point_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK | AOD_IFLPOVF_MASK);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_aod_floating_point_underflow_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_FLPUDF_MASK);
	cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_setting_aod_floating_point_underflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_FLPUDF_MASK | AOD_IFLPUDF_MASK);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_aod_fixed_point_overflow_in_executive_generates_acc_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_AOD, AOD_FXPOVF_MASK);
    cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_setting_aod_fixed_point_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_AOD, AOD_FXPOVF_MASK | AOD_IFXPOVF_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_aod_decimal_overflow_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_DECOVF_MASK);
	cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_setting_aod_decimal_overflow_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_DECOVF_MASK | AOD_IDECOVF_MASK);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_aod_zero_divide_in_executive_mode_generates_acc_system_error_interrupt(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_ZDIV_MASK);
	cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_setting_aod_zero_divide_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_ZDIV_MASK | AOD_IZDIV_MASK);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_clearing_acc_fault_in_executive_mode_does_not_clear_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
    cpu_selftest_set_register(REG_AOD, 0);
    cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_clearing_acc_fault_in_user_mode_does_not_clear_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_register(REG_AOD, AOD_FLPOVF_MASK);
    cpu_selftest_set_register(REG_AOD, 0);
    cpu_selftest_assert_acc_interrupt_as_program_fault();
}

static void cpu_selftest_setting_operand_size_in_aod_does_not_trigger_interrupt_evaluation(TESTCONTEXT *testContext)
{
	cpu_selftest_set_user_mode();
	cpu_selftest_set_register(REG_AOD, AOD_ZDIV_MASK);
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_AOD, AOD_OPSIZ_MASK | AOD_ZDIV_MASK);
	cpu_selftest_assert_acc_interrupt_as_program_fault();
}

static void cpu_selftest_setting_d_fault_in_executive_mode_generates_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_d_fault_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_d_fault_in_user_mode_generates_program_fault_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_clear_inhibit_program_fault_interrupts();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_assert_D_interrupt_as_program_fault();
}

static void cpu_selftest_setting_d_fault_in_user_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_inhibit_program_fault_interrupts();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_dod_xchk_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_XCHK_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_dod_illegal_type_size_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_ITS_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_dod_executive_mode_subtype_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_EMS_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_dod_short_source_string_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_SSS_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_dod_short_source_string_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_SSS_MASK | DOD_SSSI_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_dod_non_zero_truncation_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_NZT_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_dod_non_zero_truncation_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_NZT_MASK | DOD_NZTI_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_dod_bounds_check_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_setting_dod_bounds_check_in_executive_mode_does_not_generate_interrupt_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK | DOD_BCHI_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_setting_dod_write_to_read_only_in_executive_mode_generates_d_system_error_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_WRO_MASK);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_clearing_d_fault_in_executive_mode_does_not_clear_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_set_register(REG_DOD, 0);
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_clearing_d_fault_in_user_mode_does_not_clear_interrupt(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_selftest_set_register(REG_DOD, DOD_BCH_MASK);
    cpu_selftest_set_register(REG_DOD, 0);
    cpu_selftest_assert_D_interrupt_as_program_fault();
}

static void cpu_selftest_illegal_function_generates_system_error_interrupt_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_illegal_function_error();
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_illegal_function_generates_system_error_interrupt_if_L1IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_illegal_function_error();
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL1);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_illegal_function_generates_system_error_interrupt_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_illegal_function_error();
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL0);
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_system_error();
}

static void cpu_selftest_illegal_function_generates_illegal_order_interrupt_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_illegal_function_error();
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_illegal_function_as_illegal_order();
}

static void cpu_selftest_name_adder_overflow_generates_system_error_interrupt_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_name_adder_overflow_error();
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_name_adder_overflow_generates_system_error_interrupt_if_L1IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_name_adder_overflow_error();
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL1);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_name_adder_overflow_generates_system_error_interrupt_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_name_adder_overflow_error();
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL0);
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_name_adder_overflow_generates_illegal_order_interrupt_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_name_adder_overflow_error();
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_name_adder_overflow_interrupt_as_illegal_order();
}

static void cpu_selftest_control_adder_overflow_generates_system_error_interrupt_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_control_adder_overflow_error();
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code_from_location(0xFFFF);
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_control_adder_overflow_generates_system_error_interrupt_if_L1IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_control_adder_overflow_error();
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL1);
    cpu_selftest_run_code_from_location(0xFFFF);
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_control_adder_overflow_generates_system_error_interrupt_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_control_adder_overflow_error();
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL0);
    cpu_selftest_run_code_from_location(0xFFFF);
    cpu_selftest_assert_control_adder_overflow_interrupt_as_system_error();
}

static void cpu_selftest_control_adder_overflow_generates_illegal_order_interrupt_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_setup_control_adder_overflow_error();
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code_from_location(0xFFFF);
    cpu_selftest_assert_control_adder_overflow_interrupt_as_illegal_order();
}

static void cpu_selftest_cpr_not_equivalence_interrupt_in_executive_mode_if_L0IF_is_clear(TESTCONTEXT *testContext)
{
    cpu_selftest_set_executive_mode();
    cpu_set_cpr_non_equivalence_interrupt();
    mu5_selftest_assert_interrupt_number(testContext, INT_CPR_NOT_EQUIVALENCE);
}

static void cpu_selftest_cpr_not_equivalence_generates_system_error_interrupt_in_executive_mode_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL0);
    cpu_selftest_set_executive_mode();
    cpu_set_cpr_non_equivalence_interrupt();
    cpu_selftest_assert_cpr_not_equivalence_system_error_interrupt();
}

static void cpu_selftest_cpr_not_equivalence_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_set_user_mode();
    cpu_set_cpr_non_equivalence_interrupt();
    mu5_selftest_assert_interrupt_number(testContext, INT_CPR_NOT_EQUIVALENCE);
}

static void cpu_selftest_cpr_not_equivalence_generates_system_error_interrupt_in_user_mode_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL0);
    cpu_selftest_set_user_mode();
    cpu_set_cpr_non_equivalence_interrupt();
    cpu_selftest_assert_cpr_not_equivalence_system_error_interrupt();
}

static void cpu_selftest_spm_bit_in_program_fault_interrupt_status_causes_program_fault_interrupt(TESTCONTEXT *testContext)
{
    cpu_spm_interrupt();
    cpu_selftest_assert_spm_program_fault_interrupt();
}

static void cpu_selftest_system_error_interrupt_not_inhibited_even_if_L0IF_or_L1IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_MS, MS_MASK_B_D_SYS_ERR_EXEC | MS_MASK_LEVEL0);
    cpu_selftest_set_executive_mode();
    cpu_set_interrupt(INT_SYSTEM_ERROR);
    cpu_selftest_assert_interrupt(INT_SYSTEM_ERROR);
}

static void cpu_selftest_cpr_not_equivalence_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext)
{
	cpu_selftest_set_register(REG_MS, MS_MASK_B_D_SYS_ERR_EXEC | MS_MASK_LEVEL0);
	cpu_selftest_set_executive_mode();
    cpu_set_interrupt(INT_CPR_NOT_EQUIVALENCE);
	cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_exchange_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_MS, MS_MASK_B_D_SYS_ERR_EXEC | MS_MASK_LEVEL0);
    cpu_selftest_set_executive_mode();
    cpu_set_interrupt(INT_EXCHANGE);
    cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_peripheral_window_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_MS, MS_MASK_B_D_SYS_ERR_EXEC | MS_MASK_LEVEL0);
    cpu_selftest_set_executive_mode();
    cpu_set_interrupt(INT_PERIPHERAL_WINDOW);
    cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_level_0_interrupt_not_inhibited_if_L1IF_is_set(TESTCONTEXT *testContext)
{
	cpu_selftest_set_register(REG_MS, MS_MASK_B_D_SYS_ERR_EXEC | MS_MASK_LEVEL1);
	cpu_selftest_set_executive_mode();
	cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
	cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_level_1_interrupt_inhibited_if_L0IF_is_set(TESTCONTEXT *testContext)
{
	cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL0);
	cpu_selftest_set_user_mode();
	cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
	cpu_selftest_assert_B_program_fault();
	cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_level_1_interrupt_inhibited_if_L1IF_is_set(TESTCONTEXT *testContext)
{
	cpu_selftest_set_register(REG_MS, MS_MASK_LEVEL1);
	cpu_selftest_set_user_mode();
	cpu_selftest_set_register(REG_BOD, BOD_BOVF_MASK);
	cpu_selftest_assert_B_program_fault();
	cpu_selftest_assert_interrupt_inhibited();
}

static void cpu_selftest_cpr_not_equivalance_interrupt_on_order_fetch_stores_link_that_re_executes_failed_order(TESTCONTEXT *testContext)
{
	cpu_selftest_setup_interrupt_entry_link(INT_CPR_NOT_EQUIVALENCE);
    cpu_selftest_run_code_from_location(0x1FFFFF);
    cpu_selftest_run_continue(); /* must go round again to process interrupt */
    cpu_selftest_assert_interrupt_return_address(INT_CPR_NOT_EQUIVALENCE, 0x1FFFFF);
}

static void cpu_selftest_cpr_not_equivalance_interrupt_on_primary_operand_stores_link_that_re_executes_failed_order(TESTCONTEXT *testContext)
{
    uint32 base = NB_DEFAULT;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    mu5_selftest_setup_cpr(0, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0xC)); /* Overwrite normal CPR 0 so VA 0 is not mapped */
	cpu_selftest_setup_interrupt_entry_link(INT_CPR_NOT_EQUIVALENCE);
    cpu_selftest_run_code_from_location(0x40200000); /* expressed as 16-bit word address */
    cpu_selftest_run_continue(); /* must go round again to process interrupt */
    cpu_selftest_assert_interrupt_return_address(INT_CPR_NOT_EQUIVALENCE, 0x40200000);
}

static void cpu_selftest_cpr_not_equivalance_interrupt_on_secondary_operand_stores_link_that_re_executes_failed_order(TESTCONTEXT *testContext)
{
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_DR);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 0x0));
    mu5_selftest_setup_cpr(0, CPR_VA(0, 0x2010, 0), CPR_RA_LOCAL(SAC_OBEY_ACCESS, 0, 0xC)); /* Overwrite normal CPR 0 so VA 0 is not mapped */
	cpu_selftest_setup_interrupt_entry_link(INT_CPR_NOT_EQUIVALENCE);
	cpu_selftest_run_code_from_location(0x40200000); /* expressed as 16-bit word address */
    cpu_selftest_run_continue(); /* must go round again to process interrupt */
    cpu_selftest_assert_interrupt_return_address(INT_CPR_NOT_EQUIVALENCE, 0x40200000);
}

static void cpu_selftest_no_b_overflow_interrupt_if_b_overflow_is_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_BOD, BOD_IBOVF_MASK);
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_no_acc_zero_divide_interrupt_if_acc_zero_divide_is_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_AOD, AOD_IZDIV_MASK);
    cpu_selftest_load_order_extended(CR_XS, F_DIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000000);
    cpu_selftest_set_register(REG_X, -4 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_zero_divide();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_DOD, DOD_BCHI_MASK);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bounds_check_no_interrupt();
}

static void cpu_selftest_D_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_no_D_interrupt_if_inhibited_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_executive_mode();
    cpu_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_D_interrupt_as_program_fault_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_D_interrupt_as_program_fault();
}

static void cpu_selftest_no_D_interrupt_if_inhibited_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_user_mode();
    cpu_selftest_set_inhibit_program_fault_interrupts();
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_B_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_B_or_D_interrupt_as_system_error();
}

static void cpu_selftest_B_interrupt_as_program_fault_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_B_interrupt_as_program_fault();
}

static void cpu_selftest_acc_interrupt_as_system_error_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_interrupt_as_system_error();
}

static void cpu_selftest_no_acc_interrupt_if_inhibited_in_executive_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_clear_acc_faults_to_system_error_in_exec_mode();
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_acc_interrupt_as_program_fault_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_acc_interrupt_as_program_fault();
}

static void cpu_selftest_no_acc_interrupt_if_inhibited_in_user_mode(TESTCONTEXT *testContext)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_set_user_mode();
    cpu_selftest_set_inhibit_program_fault_interrupts();
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_no_sss_interrupt_if_sss_is_inhibited(TESTCONTEXT *testContext)
{
    uint32 sourceOrigin = 8;
    uint32 destinationOrigin = 16;
    cpu_selftest_load_order(CR_STS1, F_SLGC, K_LITERAL, 0);
    cpu_selftest_set_register(REG_DOD, DOD_SSSI_MASK);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_sss_no_interrupt();
}

static void cpu_selftest_interrupt_stacks_link_in_system_v_store(TESTCONTEXT *testContext)
{
    cpu_selftest_set_register(REG_MS, 0xAA04);
	cpu_selftest_setup_name_base(0xBBBB);
    cpu_set_interrupt(INT_PROGRAM_FAULTS);
	cpu_selftest_setup_interrupt_entry_link(INT_PROGRAM_FAULTS);
    cpu_selftest_run_code_from_location(10);
    mu5_selftest_assert_vstore_contents(testContext, SYSTEM_V_STORE_BLOCK, 28, 0xAA04BBBB0000000A);
}

static void cpu_selftest_interrupt_calls_handler_using_link_in_system_v_store(TESTCONTEXT *testContext)
{
    sac_write_v_store(SYSTEM_V_STORE_BLOCK, 29, 0xBB84CCCC0000000B);
    cpu_set_interrupt(INT_PROGRAM_FAULTS);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xBB84);
    cpu_selftest_assert_reg_equals(REG_NB, 0xCCCC);
    cpu_selftest_assert_reg_equals(REG_CO, 0x0000000B);
}

static void cpu_selftest_interrupt_sets_executive_mode(TESTCONTEXT *testContext)
{
    sac_write_v_store(SYSTEM_V_STORE_BLOCK, 29, 0xBB84CCCC0000000B);
    cpu_selftest_set_register(REG_MS, 0);
    cpu_selftest_set_user_mode();
    cpu_set_interrupt(INT_PROGRAM_FAULTS);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xBB84);
}

static void cpu_selftest_interrupt_sequence_clears_interrupt(TESTCONTEXT *testContext)
{
    cpu_set_interrupt(INT_PROGRAM_FAULTS);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_interrupt_sequence_does_not_clear_interrupt_when_handling_another_interrupt(TESTCONTEXT *testContext)
{
    cpu_set_interrupt(INT_PROGRAM_FAULTS);
    cpu_set_interrupt(INT_SYSTEM_ERROR);
    cpu_selftest_run_code();
    mu5_selftest_assert_interrupt_number(localTestContext, INT_PROGRAM_FAULTS);
}

static void cpu_selftest_write_to_prop_program_fault_status_resets_it(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0xFF);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0);
}

static void cpu_selftest_write_to_prop_system_error_status_resets_it(TESTCONTEXT *testContext)
{
    cpu_selftest_load_organisational_order_extended(F_XNB_PLUS, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFC);
    cpu_selftest_set_register(REG_XNB, 0xAAAA0002);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0xFF);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0);
}

static void cpu_selftest_read_and_write_instruction_counter(TESTCONTEXT *testContext)
{
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0xFFAAAA);
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0xAAAA);
}

static void cpu_selftest_executing_instruction_decrements_instruction_counter(TESTCONTEXT *testContext)
{
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0xFF);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0xFE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_instruction_counter_not_decremented_if_inhibited(TESTCONTEXT *testContext)
{
    cpu_selftest_set_inhibit_instruction_counter();
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0xFF);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0xFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_instruction_counter_not_decremented_if_already_zero(TESTCONTEXT *testContext)
{
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0x00);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0x00);
}

static void cpu_selftest_instruction_counter_zero_generates_interrupt(TESTCONTEXT *testContext)
{
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0x01);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    mu5_selftest_assert_vstore_contents(localTestContext, PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0x00);
    cpu_selftest_assert_interrupt(INT_INSTRUCTION_COUNT_ZERO);
}

static void cpu_selftest_instruction_counter_already_zero_does_not_generate_new_interrupt(TESTCONTEXT *testContext)
{
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_INSTRUCTION_COUNTER, 0x00);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}
