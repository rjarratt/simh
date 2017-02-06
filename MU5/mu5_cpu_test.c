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
#include "mu5_cpu_test.h"
#include "mu5_sac.h"

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

#define F_RELJUMP 0
#define F_EXIT 1
#define F_ABSJUMP 4
#define F_RETURN 5
#define F_STACKLINK 15
#define F_MS_LOAD 16
#define F_DL_LOAD 17
#define F_SPM 18
#define F_SETLINK 19
#define F_SF_LOAD_NB_PLUS 26
#define F_NB_LOAD 28
#define F_NB_LOAD_SF_PLUS 29
#define F_BRANCH_EQ 32
#define F_BRANCH_NE 33
#define F_BRANCH_GE 34
#define F_BRANCH_LT 35
#define F_BRANCH_LE 36
#define F_BRANCH_GT 37
#define F_BRANCH_OVF 38
#define F_BRANCH_BN 39

#define K_LITERAL 0
#define K_IR 1
#define K_V32 2
#define K_V64 3
#define K_SB 4
#define K_SB_5 5
#define K_S0 6

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

#define AOD_OPSIZ_MASK 0x00001000
#define AOD_IOVF_MASK 0x00000200
#define AOD_IZDIV_MASK 0x00000080
#define AOD_OVF_MASK 0x00000010
#define AOD_ZDIV_MASK 0x00000004

#define BOD_IBOVF_MASK 0x80000000
#define BOD_BOVF_MASK 0x04000000

#define DOD_ITS_MASK 0x00000002
#define DOD_SSS_MASK 0x00000008
#define DOD_BCH_MASK 0x00000020
#define DOD_SSSI_MASK 0x00000040
#define DOD_BCHI_MASK 0x00000100

#define MS_TEST_MASK 0x0F00
#define MS_OVERFLOW_MASK 0x0800
#define TEST_EQUALS 0x0000
#define TEST_GREATER_THAN 0x0400
#define TEST_LESS_THAN 0x0600
#define TEST_OVERFLOW 0x0800

typedef struct TESTCONTEXT
{
    char *testName;
    uint32 currentLoadLocation;
    t_stat result;
} TESTCONTEXT;

typedef struct UNITTEST
{
    char * name;
    void (*runner)(void);
} UNITTEST;

extern DEVICE cpu_dev;

static TESTCONTEXT testContext;

static void cpu_selftest_reset(UNITTEST *test);
static void cpu_selftest_set_load_location(uint32 location);
static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n);
static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 kp, uint8 np);
static void cpu_selftest_load_organisational_order_literal(uint8 f,uint8 n);
static void cpu_selftest_load_organisational_order_extended(uint8 f, uint8 kp, uint8 np);
static void cpu_selftest_load_16_bit_literal(uint16 value);
static void cpu_selftest_load_32_bit_literal(uint32 value);
static void cpu_selftest_load_64_bit_literal(t_uint64 value);
static t_uint64 cpu_selftest_create_descriptor(uint8 type, uint8 size, uint32 bound, uint32 origin);
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
static void cpu_selftest_set_aod_operand_32_bit();
static void cpu_selftest_set_aod_operand_64_bit();
static void cpu_selftest_set_executive_mode(void);
static void cpu_selftest_set_user_mode(void);
static void cpu_selftest_run_code(void);
static void cpu_selftest_run_code_from_location(uint32 location);
static REG *cpu_selftest_find_register(char *name);
static t_uint64 cpu_selftest_get_register(char *name);
static void cpu_selftest_set_register(char *name, t_uint64 value);

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
static void cpu_selftest_assert_no_b_overflow(void);
static void cpu_selftest_assert_no_b_overflow_interrupt(void);
static void cpu_selftest_assert_b_overflow(void);
static void cpu_selftest_assert_b_overflow_interrupt(void);
static void cpu_selftest_assert_no_a_overflow(void);
static void cpu_selftest_assert_no_a_overflow_interrupt(void);
static void cpu_selftest_assert_a_overflow(void);
static void cpu_selftest_assert_a_overflow_interrupt(void);
static void cpu_selftest_assert_no_a_zero_divide(void);
static void cpu_selftest_assert_no_a_zero_divide_interrupt(void);
static void cpu_selftest_assert_a_zero_divide(void);
static void cpu_selftest_assert_a_zero_divide_interrupt(void);
static void cpu_selftest_assert_interrupt(void);
static void cpu_selftest_assert_no_interrupt(void);
static void cpu_selftest_assert_d_interrupt(char *name, uint32 mask);
static void cpu_selftest_assert_no_d_interrupt(char *name, uint32 mask);
static void cpu_selftest_assert_bound_check_interrupt(void);
static void cpu_selftest_assert_its_interrupt(void);
static void cpu_selftest_assert_sss_interrupt(void);
static void cpu_selftest_assert_no_bound_check_interrupt(void);
static void cpu_selftest_assert_no_its_interrupt(void);
static void cpu_selftest_assert_no_sss_interrupt(void);
static void cpu_selftest_assert_segment_overflow_interrupt(void);
static void cpu_selftest_assert_test_equals(void);
static void cpu_selftest_assert_test_greater_than(void);
static void cpu_selftest_assert_test_less_than(void);
static void cpu_selftest_assert_test_no_overflow(void);
static void cpu_selftest_assert_test_overflow(void);
static void cpu_selftest_assert_operand_size_32(void);
static void cpu_selftest_assert_operand_size_64(void);
static void cpu_selftest_assert_fail(void);

static void cpu_selftest_16_bit_instruction_advances_co_by_1(void);
static void cpu_selftest_32_bit_instruction_advances_co_by_2(void);
static void cpu_selftest_48_bit_instruction_advances_co_by_3(void);
static void cpu_selftest_80_bit_instruction_advances_co_by_5(void);

static void cpu_selftest_load_operand_6_bit_positive_literal(void);
static void cpu_selftest_load_operand_6_bit_negative_literal(void);

static void cpu_selftest_load_operand_internal_register_0(void);
static void cpu_selftest_load_operand_internal_register_1(void);
static void cpu_selftest_load_operand_internal_register_2(void);
static void cpu_selftest_load_operand_internal_register_3(void);
static void cpu_selftest_load_operand_internal_register_4(void);
static void cpu_selftest_load_operand_internal_register_16(void);
static void cpu_selftest_load_operand_internal_register_17(void);
static void cpu_selftest_load_operand_internal_register_18(void);
static void cpu_selftest_load_operand_internal_register_19(void);
static void cpu_selftest_load_operand_internal_register_20(void);
static void cpu_selftest_load_operand_internal_register_32(void);
static void cpu_selftest_load_operand_internal_register_33(void);
static void cpu_selftest_load_operand_internal_register_34(void);
static void cpu_selftest_load_operand_internal_register_48(void);

static void cpu_selftest_load_operand_32_bit_variable(void);
static void cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned(void);
static void cpu_selftest_load_operand_64_bit_variable(void);
static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(void);
static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(void);
static void cpu_selftest_load_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(void);
static void cpu_selftest_load_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(void);
static void cpu_selftest_load_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(void);
static void cpu_selftest_load_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(void);
static void cpu_selftest_load_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(void);
static void cpu_selftest_load_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(void);

static void cpu_selftest_load_operand_16_bit_signed_positive_literal(void);
static void cpu_selftest_load_operand_16_bit_signed_negative_literal(void);
static void cpu_selftest_load_operand_32_bit_signed_positive_literal(void);
static void cpu_selftest_load_operand_32_bit_signed_negative_literal(void);
static void cpu_selftest_load_operand_64_bit_literal_np_2(void);
static void cpu_selftest_load_operand_64_bit_literal_np_3(void);
static void cpu_selftest_load_operand_16_bit_unsigned_literal(void);
static void cpu_selftest_load_operand_32_bit_unsigned_literal(void);
static void cpu_selftest_load_operand_64_bit_literal_np_6(void);
static void cpu_selftest_load_operand_64_bit_literal_np_7(void);
static void cpu_selftest_load_operand_extended_literal_kp_1(void);

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_sf(void);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_zero(void);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb(void);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb(void);
static void cpu_selftest_load_operand_extended_32_bit_variable_from_stack(void);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref(void);
static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref(void);

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_sf(void);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero(void);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb(void);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb(void);
static void cpu_selftest_load_operand_extended_64_bit_variable_from_stack(void);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref(void);
static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref(void);

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_zero(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_stack(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_dr(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_64_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_16_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_8_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_4_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_b_relative_descriptor_1_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(void);
static void cpu_selftest_load_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(void);

static void cpu_selftest_store_operand_6_bit_literal_generates_interrupt(void);

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt(void);
static void cpu_selftest_store_operand_internal_register_1_generates_interrupt(void);
static void cpu_selftest_store_operand_internal_register_2_generates_interrupt(void);
static void cpu_selftest_store_operand_internal_register_3_generates_interrupt(void);
static void cpu_selftest_store_operand_internal_register_4_generates_interrupt(void);
static void cpu_selftest_store_operand_internal_register_16(void);
static void cpu_selftest_store_operand_internal_register_17(void);
static void cpu_selftest_store_operand_internal_register_18(void);
static void cpu_selftest_store_operand_internal_register_19(void);
static void cpu_selftest_store_operand_internal_register_20(void);
static void cpu_selftest_store_operand_internal_register_32(void);
static void cpu_selftest_store_operand_internal_register_33(void);
static void cpu_selftest_store_operand_internal_register_34(void);
static void cpu_selftest_store_operand_internal_register_48(void);

static void cpu_selftest_store_operand_32_bit_variable(void);
static void cpu_selftest_store_operand_64_bit_variable(void);
static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(void);
static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(void);
static void cpu_selftest_store_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(void);
static void cpu_selftest_store_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(void);
static void cpu_selftest_store_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(void);
static void cpu_selftest_store_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(void);
static void cpu_selftest_store_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(void);
static void cpu_selftest_store_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(void);

static void cpu_selftest_store_operand_extended_literal_kp_0_generates_interrupt(void);
static void cpu_selftest_store_operand_extended_literal_kp_1_generates_interrupt(void);

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf(void);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero(void);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb(void);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb(void);
static void cpu_selftest_store_operand_extended_32_bit_variable_from_stack(void);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb_ref(void);
static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref(void);

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf(void);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero(void);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb(void);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb(void);
static void cpu_selftest_store_operand_extended_64_bit_variable_from_stack(void);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb_ref(void);
static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref(void);

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_zero(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_stack(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_dr(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_64_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_16_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_8_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_4_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_b_relative_descriptor_1_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(void);
static void cpu_selftest_store_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(void);

static void cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD(void);
static void cpu_selftest_sts1_xd_load_loads_whole_of_XD(void);
static void cpu_selftest_sts1_stack_stacks_operand(void);
static void cpu_selftest_sts1_xd_store_stores_xd_to_operand(void);
static void cpu_selftest_sts1_xd_store_to_secondary_operand_generates_interrupt(void);
static void cpu_selftest_sts1_xdb_load_loads_bound_in_XD(void);
static void cpu_selftest_sts1_xchk_operand_negative_clears_DOD_XCH_bit(void);
static void cpu_selftest_sts1_xchk_operand_ge_XDB_clears_DOD_XCH_bit(void);
static void cpu_selftest_sts1_xchk_operand_within_XDB_sets_DOD_XCH_bit(void);
static void cpu_selftest_sts1_smod_adds_signed_operand_to_D_origin(void);
static void cpu_selftest_sts1_smod_scales_modifier_for_type_0(void);
static void cpu_selftest_sts1_smod_scales_modifier_for_type_1(void);
static void cpu_selftest_sts1_smod_scales_modifier_for_type_2(void);
static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_0_when_US_set(void);
static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_2_when_US_set(void);
static void cpu_selftest_sts1_smod_does_not_check_bounds(void);
static void cpu_selftest_sts1_xmod_adds_signed_operand_to_XD_origin_subtracts_from_bound(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_64_bit_value(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_32_bit_value(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_16_bit_value(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_8_bit_value(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_within_byte(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_crossing_byte(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_within_byte(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_crossing_byte(void);
static void cpu_selftest_sts1_xmod_checks_bounds_for_type_0(void);
static void cpu_selftest_sts1_xmod_checks_bounds_for_type_1(void);
static void cpu_selftest_sts1_xmod_checks_bounds_for_type_2(void);
static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_0_when_BC_set(void);
static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_1_when_BC_set(void);
static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_2_when_BC_set(void);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_not_8_bit(void);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_is_type_3(void);
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_is_type_3(void);
static void cpu_selftest_sts1_slgc_processes_type_0_descriptors(void);
static void cpu_selftest_sts1_slgc_processes_type_2_descriptors(void);
static void cpu_selftest_sts1_slgc_processes_long_vector(void);
static void cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out(void);
static void cpu_selftest_sts1_slgc_processes_L0(void);
static void cpu_selftest_sts1_slgc_processes_L1(void);
static void cpu_selftest_sts1_slgc_processes_L2(void);
static void cpu_selftest_sts1_slgc_processes_L3(void);
static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_source_not_8_bit(void);
static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts1_smvb_generates_checks_bound_on_destination_not_0(void);
static void cpu_selftest_sts1_smvb_copies_byte_with_mask(void);
static void cpu_selftest_sts1_smvb_uses_filler_when_source_empty_with_mask(void);
static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_source_not_8_bit(void);
static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts1_smvf_copies_to_zero_length_destination(void);
static void cpu_selftest_sts1_smvf_copies_bytes_and_fills_with_mask(void);
static void cpu_selftest_sts1_talu_returns_test_register_greater_than_if_not_found(void);
static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_0(void);
static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_2(void);
static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_source_not_8_bit(void);
static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical(void);
static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical_with_filler(void);
static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_destination_is_shorter_and_subset_is_equal(void);
static void cpu_selftest_sts1_scmp_returns_test_register_greater_than_if_source_byte_greater_than_destination_byte(void);
static void cpu_selftest_sts1_scmp_returns_test_register_less_than_if_source_byte_less_than_destination_byte(void);
static void cpu_selftest_sts1_sub1_loads_XD_and_modifies_it(void);
static void cpu_selftest_sts1_sub1_calculates_B(void);
static void cpu_selftest_sts1_sub1_modifies_D(void);

static void cpu_selftest_sts2_do_load_loads_ls_half_of_D(void);
static void cpu_selftest_sts2_d_load_loads_whole_of_D(void);
static void cpu_selftest_sts2_d_stack_load_stacks_D_loads_new_D(void);
static void cpu_selftest_sts2_d_store_stores_d_to_operand(void);
static void cpu_selftest_sts2_d_store_to_secondary_operand_generates_interrupt(void);
static void cpu_selftest_sts2_db_load_loads_bound_in_D(void);
static void cpu_selftest_sts2_mdr_advances_location_and_loads_D_with_operand_pointed_to_by_D(void);
static void cpu_selftest_sts2_mod_adds_signed_operand_to_D_origin_subtracts_from_bound(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_64_bit_value(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_32_bit_value(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_16_bit_value(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_8_bit_value(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_within_byte(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_crossing_byte(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_within_byte(void);
static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_crossing_byte(void);
static void cpu_selftest_sts2_mod_checks_bounds_for_type_0(void);
static void cpu_selftest_sts2_mod_checks_bounds_for_type_1(void);
static void cpu_selftest_sts2_mod_checks_bounds_for_type_2(void);
static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_0_when_BC_set(void);
static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_1_when_BC_set(void);
static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_2_when_BC_set(void);
static void cpu_selftest_sts2_rmod_loads_least_significant_half_of_D_and_adds_to_origin(void);
static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_is_type_3(void);
static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L0(void);
static void cpu_selftest_sts2_blgc_processes_type_1_descriptor_L1(void);
static void cpu_selftest_sts2_blgc_processes_type_2_descriptor_L2(void);
static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L3(void);
static void cpu_selftest_sts2_bmvb_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts2_bmvb_generates_checks_bound_on_destination_not_0(void);
static void cpu_selftest_sts2_bmvb_copies_byte_with_mask(void);
static void cpu_selftest_sts2_bmve_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts2_bmve_copies_byte_with_mask_to_whole_destination(void);
static void cpu_selftest_sts2_smvf_copies_bytes_and_fills_with_mask(void);
static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_is_type_3(void);
static void cpu_selftest_sts2_bscn_sets_less_than_if_byte_not_found(void);
static void cpu_selftest_sts2_bscn_finds_byte_in_type_0_descriptor(void);
static void cpu_selftest_sts2_bscn_finds_byte_in_type_1_descriptor(void);
static void cpu_selftest_sts2_bscn_finds_byte_in_type_2_descriptor(void);
static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_not_8_bit(void);
static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_is_type_3(void);
static void cpu_selftest_sts2_bcmp_sets_equals_if_byte_found_in_all_elements(void);
static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_smaller(void);
static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_larger(void);
static void cpu_selftest_sts2_bcmp_finds_byte_in_type_0_descriptor(void);
static void cpu_selftest_sts2_bcmp_finds_byte_in_type_1_descriptor(void);
static void cpu_selftest_sts2_bcmp_finds_byte_in_type_2_descriptor(void);
static void cpu_selftest_sts2_sub2_modifies_XD(void);
static void cpu_selftest_sts2_sub2_calculates_B(void);
static void cpu_selftest_sts2_sub2_modifies_D_existing_D(void);

static void cpu_selftest_b_load_loads_B(void);
static void cpu_selftest_b_load_and_decrement_loads_B_and_subtracts_1(void);
static void cpu_selftest_b_load_and_decrement_flags_overflow(void);
static void cpu_selftest_b_stack_and_load_stacks_B_and_loads_B(void);
static void cpu_selftest_b_store_stores_B(void);
static void cpu_selftest_b_add_adds_operand_to_B(void);
static void cpu_selftest_b_add_flags_overflow(void);
static void cpu_selftest_b_sub_subtracts_operand_from_B(void);
static void cpu_selftest_b_sub_flags_overflow(void);
static void cpu_selftest_b_mul_multiplies_operand_by_B(void);
static void cpu_selftest_b_mul_flags_overflow(void);
static void cpu_selftest_b_xor(void);
static void cpu_selftest_b_or(void);
static void cpu_selftest_b_shift_shifts_left_for_positive_operand(void);
static void cpu_selftest_b_shift_shifts_right_for_negative_operand(void);
static void cpu_selftest_b_shift_flags_overflow(void);
static void cpu_selftest_b_and(void);
static void cpu_selftest_b_rsub_subtracts_B_from_operand(void);
static void cpu_selftest_b_rsub_flags_overflow(void);
static void cpu_selftest_b_comp_sets_less_than_when_B_less_than_operand(void);
static void cpu_selftest_b_comp_sets_equals_when_B_equals_operand(void);
static void cpu_selftest_b_comp_sets_greater_than_when_B_greater_than_operand(void);
static void cpu_selftest_b_comp_sets_overflow(void);
static void cpu_selftest_b_cinc_compares_B_with_operand(void);
static void cpu_selftest_b_cinc_increments_B(void);
static void cpu_selftest_b_cinc_flags_overflow(void);

static void cpu_selftest_x_load_loads_X(void);
static void cpu_selftest_x_stack_and_load_stacks_X_and_loads_X(void);
static void cpu_selftest_x_store_stores_X(void);
static void cpu_selftest_x_add_adds_operand_to_X(void);
static void cpu_selftest_x_add_flags_overflow(void);
static void cpu_selftest_x_sub_subtracts_operand_from_X(void);
static void cpu_selftest_x_sub_flags_overflow(void);
static void cpu_selftest_x_mul_multiplies_operand_by_X(void);
static void cpu_selftest_x_mul_flags_overflow(void);
static void cpu_selftest_x_div_divides_X_by_operand(void);
static void cpu_selftest_x_div_flags_divide_by_zero(void);
static void cpu_selftest_x_xor(void);
static void cpu_selftest_x_or(void);
static void cpu_selftest_x_shift_shifts_left_for_positive_operand(void);
static void cpu_selftest_x_shift_shifts_right_for_negative_operand(void);
static void cpu_selftest_x_shift_flags_overflow(void);
static void cpu_selftest_x_and(void);
static void cpu_selftest_x_rsub_subtracts_X_from_operand(void);
static void cpu_selftest_x_rsub_flags_overflow(void);
static void cpu_selftest_x_comp_sets_less_than_when_X_less_than_operand(void);
static void cpu_selftest_x_comp_sets_equals_when_X_equals_operand(void);
static void cpu_selftest_x_comp_sets_greater_than_when_X_greater_than_operand(void);
static void cpu_selftest_x_comp_sets_overflow(void);
static void cpu_selftest_x_rdiv_divides_operand_by_X(void);
static void cpu_selftest_x_rdiv_flags_divide_by_zero(void);

static void cpu_selftest_a_load_loads_AOD(void);
static void cpu_selftest_a_stack_and_load_stacks_AOD_and_loads_AOD(void);
static void cpu_selftest_a_store_stores_AOD(void);
static void cpu_selftest_a_add_adds_operand_to_A(void);
static void cpu_selftest_a_sub_subtracts_operand_from_A(void);
static void cpu_selftest_a_mul_multiplies_operand_by_A(void);
static void cpu_selftest_a_xor(void);
static void cpu_selftest_a_or(void);
static void cpu_selftest_a_shift_shifts_left_for_positive_operand(void);
static void cpu_selftest_a_shift_shifts_right_for_negative_operand(void);
static void cpu_selftest_a_and(void);
static void cpu_selftest_a_rsub_subtracts_A_from_operand(void);
static void cpu_selftest_a_comp_sets_less_than_when_A_less_than_operand(void);
static void cpu_selftest_a_comp_sets_equals_when_A_equals_operand(void);
static void cpu_selftest_a_comp_sets_greater_than_when_A_greater_than_operand(void);

static void cpu_selftest_dec_load_loads_AEX(void);
static void cpu_selftest_dec_stack_and_load_stacks_AEX_and_loads_AEX(void);
static void cpu_selftest_dec_store_stores_AEX(void);
static void cpu_selftest_dec_comp_sets_overflow_when_AOD_and_operand_non_zero(void);
static void cpu_selftest_dec_comp_clears_overflow_when_AOD_and_operand_is_zero(void);

static void cpu_selftest_flt_load_single_loads_32_bits_into_A(void);
static void cpu_selftest_flt_load_double_loads_64_bits_into_A(void);
static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_32_bits(void);
static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_64_bits(void);
static void cpu_selftest_flt_store_stores_A_32_bits(void);
static void cpu_selftest_flt_store_stores_A_64_bits(void);

static void cpu_selftest_org_relative_jump_jumps_forward(void);
static void cpu_selftest_org_relative_jump_jumps_backward(void);
/* TODO: static void cpu_selftest_org_relative_across_segement_boundary_generates_interrupt(void); */
static void cpu_selftest_org_exit_resets_link_in_executive_mode(void);
static void cpu_selftest_org_exit_resets_link_except_privileged_ms_bits_in_user_mode(void);
static void cpu_selftest_org_absolute_jump(void);
static void cpu_selftest_org_return_sets_SF_and_unstacks_link(void);
static void cpu_selftest_org_return_resets_link_except_privileged_ms_bits_in_user_mode(void);
static void cpu_selftest_org_return_does_not_pop_stack_if_operand_is_not_stack_but_sets_NB(void);
static void cpu_selftest_org_stacklink_puts_link_on_stack_adding_operand_to_stacked_CO(void);
static void cpu_selftest_org_stacklink_treats_operand_as_signed(void);
/*static void cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_crosses_segment_boundary(void);*/
static void cpu_selftest_org_ms_load_sets_unmasked_bits_only_in_executive_mode(void);
static void cpu_selftest_org_ms_load_does_not_set_masked_bits_in_executive_mode(void);
static void cpu_selftest_org_ms_load_does_not_set_privileged_unmasked_bits_in_user_mode(void);
static void cpu_selftest_org_dl_load_sets_dl_pseudo_register(void);
static void cpu_selftest_org_spm_dummy(void);
static void cpu_selftest_org_setlink_stores_link(void);
static void cpu_selftest_org_sf_load_nb_plus_adds_NB_to_signed_operand_and_stores_to_SF(void);
static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_overflow(void);
static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_underflow(void);
static void cpu_selftest_org_nb_load_loads_nb(void);
static void cpu_selftest_org_nb_load_sf_plus_adds_SF_to_signed_operand_and_stores_to_NB(void);
static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_overflow(void);
static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_underflow(void);
static void cpu_selftest_org_branch_test_branch_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_branch_test_branch_not_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn);
static void cpu_selftest_org_br_eq_does_not_branch_on_false(void);
static void cpu_selftest_org_br_eq_does_branches_on_true(void);
static void cpu_selftest_org_br_ne_does_not_branch_on_false(void);
static void cpu_selftest_org_br_ne_does_branches_on_true(void);
static void cpu_selftest_org_br_ge_does_not_branch_on_false(void);
static void cpu_selftest_org_br_ge_does_branches_on_true(void);
static void cpu_selftest_org_br_lt_does_not_branch_on_false(void);
static void cpu_selftest_org_br_lt_does_branches_on_true(void);
static void cpu_selftest_org_br_le_does_not_branch_on_false(void);
static void cpu_selftest_org_br_le_does_branches_on_true(void);
static void cpu_selftest_org_br_gt_does_not_branch_on_false(void);
static void cpu_selftest_org_br_gt_does_branches_on_true(void);
static void cpu_selftest_org_br_ovf_does_not_branch_on_false(void);
static void cpu_selftest_org_br_ovf_does_branches_on_true(void);
static void cpu_selftest_org_br_bn_does_not_branch_on_false(void);
static void cpu_selftest_org_br_bn_does_branches_on_true(void);

static void cpu_selftest_no_b_overflow_interrupt_if_b_overflow_is_inhibited(void);
static void cpu_selftest_no_zero_divide_interrupt_if_zero_divide_is_inhibited(void);
static void cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited(void);
static void cpu_selftest_no_sss_interrupt_if_sss_is_inhibited(void);

UNITTEST tests[] =
{
    { "16-bit instruction advances CO by 1", cpu_selftest_16_bit_instruction_advances_co_by_1 },
    { "32-bit instruction advances CO by 2", cpu_selftest_32_bit_instruction_advances_co_by_2 },
    { "48-bit instruction advances CO by 3", cpu_selftest_48_bit_instruction_advances_co_by_3 },
    { "80-bit instruction advances CO by 5", cpu_selftest_80_bit_instruction_advances_co_by_5 },

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

    { "Load operand 32-bit variable", cpu_selftest_load_operand_32_bit_variable },
    { "Load operand 32-bit variable 6-bit offset is unsigned", cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned },
    { "Load operand 64-bit variable", cpu_selftest_load_operand_64_bit_variable },
    { "Load operand 32-bit via B-relative descriptor at 6-bit offset for k=4", cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4 },
    { "Load operand 32-bit via B-relative descriptor at 6-bit offset for k=5", cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5 },
    { "Load operand 64-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset },
    { "Load operand 16-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset },
    { "Load operand 8-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset },
    { "Load operand 4-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset },
    { "Load operand 1-bit via B-relative descriptor at 6-bit offset", cpu_selftest_load_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset },
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
    { "Load operand 32-bit variable extended from (NB)", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref },
    { "Load operand 32-bit variable extended from (XNB)", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref },

    { "Load operand 64-bit variable extended offset from stack", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_sf },
    { "Load operand 64-bit variable extended offset from zero", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero },
    { "Load operand 64-bit variable extended offset from NB", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb },
    { "Load operand 64-bit variable extended offset from XNB", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb },
    { "Load operand 64-bit variable extended from stack", cpu_selftest_load_operand_extended_64_bit_variable_from_stack },
    { "Load operand 64-bit variable extended from (NB)", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref },
    { "Load operand 64-bit variable extended from (XNB)", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref },

    { "Load operand 32-bit extended from b-relative descriptor from SF for kp=4", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4 },
    { "Load operand 32-bit extended from b-relative descriptor from SF for kp=5", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5 },
    { "Load operand 32-bit extended from b-relative descriptor from zero", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_zero },
    { "Load operand 32-bit extended from b-relative descriptor from NB", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb },
    { "Load operand 32-bit extended from b-relative descriptor from XNB", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb },
    { "Load operand 32-bit extended from b-relative descriptor from stack", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_stack },
    { "Load operand 32-bit extended from b-relative descriptor from D", cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_dr },
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
    { "Load operand 32-bit extended from 0-relative descriptor from (NB)", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref },
    { "Load operand 32-bit extended from 0-relative descriptor from (XNB)", cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref },
    { "Load operand 64-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_64_bit_value_from_nb },
    { "Load operand 16-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_16_bit_value_from_nb },
    { "Load operand 8-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_8_bit_value_from_nb },
    { "Load operand 4-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_4_bit_value_from_nb },
    { "Load operand 1-bit extended from 0-relative descriptor from NB", cpu_selftest_load_operand_extended_zero_relative_descriptor_1_bit_value_from_nb },

    { "Store operand 6-bit literal generates interrupt", cpu_selftest_store_operand_6_bit_literal_generates_interrupt },

    { "Store operand internal register 0 generates an interrupt", cpu_selftest_store_operand_internal_register_0_generates_interrupt },
    { "Store operand internal register 1 generates an interrupt", cpu_selftest_store_operand_internal_register_1_generates_interrupt },
    { "Store operand internal register 2 generates an interrupt", cpu_selftest_store_operand_internal_register_2_generates_interrupt },
    { "Store operand internal register 3 generates an interrupt", cpu_selftest_store_operand_internal_register_3_generates_interrupt },
    { "Store operand internal register 4 generates an interrupt", cpu_selftest_store_operand_internal_register_4_generates_interrupt },
    { "Store operand internal register 16", cpu_selftest_store_operand_internal_register_16 },
    { "Store operand internal register 17", cpu_selftest_store_operand_internal_register_17 },
    { "Store operand internal register 18", cpu_selftest_store_operand_internal_register_18 },
    { "Store operand internal register 19", cpu_selftest_store_operand_internal_register_19 },
    { "Store operand internal register 20", cpu_selftest_store_operand_internal_register_20 },
    { "Store operand internal register 32", cpu_selftest_store_operand_internal_register_32 },
    { "Store operand internal register 33", cpu_selftest_store_operand_internal_register_33 },
    { "Store operand internal register 34", cpu_selftest_store_operand_internal_register_34 },
    { "Store operand internal register 48", cpu_selftest_store_operand_internal_register_48 },

    { "Store operand 32-bit variable", cpu_selftest_store_operand_32_bit_variable },
    { "Store operand 64-bit variable", cpu_selftest_store_operand_64_bit_variable },
    { "Store operand 32-bit via B-relative descriptor at 6-bit offset for k=4", cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4 },
    { "Store operand 32-bit via B-relative descriptor at 6-bit offset for k=5", cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5 },
    { "Store operand 64-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset },
    { "Store operand 16-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset },
    { "Store operand 8-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset },
    { "Store operand 4-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset },
    { "Store operand 1-bit via B-relative descriptor at 6-bit offset", cpu_selftest_store_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset },
    { "Store operand 64-bit via 0-relative descriptor at 6-bit offset", cpu_selftest_store_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset },

    { "Store operand to extended literal generates interrupt, k'=0", cpu_selftest_store_operand_extended_literal_kp_0_generates_interrupt },
    { "Store operand to extended literal generates interrupt, k'=1", cpu_selftest_store_operand_extended_literal_kp_1_generates_interrupt },

    { "Store operand 32-bit variable extended offset from stack", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf },
    { "Store operand 32-bit variable extended offset from zero", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero },
    { "Store operand 32-bit variable extended offset from NB", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb },
    { "Store operand 32-bit variable extended offset from XNB", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb },
    { "Store operand 32-bit variable extended from stack", cpu_selftest_store_operand_extended_32_bit_variable_from_stack },
    { "Store operand 32-bit variable extended from (NB)", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb_ref },
    { "Store operand 32-bit variable extended from (XNB)", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref },

    { "Store operand 64-bit variable extended offset from stack", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf },
    { "Store operand 64-bit variable extended offset from zero", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero },
    { "Store operand 64-bit variable extended offset from NB", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb },
    { "Store operand 64-bit variable extended offset from XNB", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb },
    { "Store operand 64-bit variable extended from stack", cpu_selftest_store_operand_extended_64_bit_variable_from_stack },
    { "Store operand 64-bit variable extended from (NB)", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb_ref },
    { "Store operand 64-bit variable extended from (XNB)", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref },

    { "Store operand 32-bit extended from b-relative descriptor from SF for kp=4", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4 },
    { "Store operand 32-bit extended from b-relative descriptor from SF for kp=5", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5 },
    { "Store operand 32-bit extended from b-relative descriptor from zero", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_zero },
    { "Store operand 32-bit extended from b-relative descriptor from NB", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb },
    { "Store operand 32-bit extended from b-relative descriptor from XNB", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb },
    { "Store operand 32-bit extended from b-relative descriptor from stack", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_stack },
    { "Store operand 32-bit extended from b-relative descriptor from D", cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_dr },
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
    { "Store operand 32-bit extended from 0-relative descriptor from (NB)", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref },
    { "Store operand 32-bit extended from 0-relative descriptor from (XNB)", cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref },
    { "Store operand 64-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_64_bit_value_from_nb },
    { "Store operand 16-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_16_bit_value_from_nb },
    { "Store operand 8-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_8_bit_value_from_nb },
    { "Store operand 4-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_4_bit_value_from_nb },
    { "Store operand 1-bit extended from 0-relative descriptor from NB", cpu_selftest_store_operand_extended_zero_relative_descriptor_1_bit_value_from_nb },

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
    { "B COMP sets overflow", cpu_selftest_b_comp_sets_overflow },
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
    { "DEC COMP sets overlow when AND of AOD with operand is non-zero", cpu_selftest_dec_comp_sets_overflow_when_AOD_and_operand_non_zero },
    { "DEC COMP clears overlow when AND of AOD with operand is zero", cpu_selftest_dec_comp_clears_overflow_when_AOD_and_operand_is_zero },

    { "FLT Load Single loads 32 bits into A", cpu_selftest_flt_load_single_loads_32_bits_into_A },
    { "FLT Load Double loads 64 bits into A", cpu_selftest_flt_load_double_loads_64_bits_into_A },
    { "FLT Stack and Load stacks A and loads it (32 bits)", cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_32_bits },
    { "FLT Stack and Load stacks A and loads it (64 bits)", cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_64_bits },
    { "FLT Store stores A (32 bits)", cpu_selftest_flt_store_stores_A_32_bits },
    { "FLT Store stores A (64 bits)", cpu_selftest_flt_store_stores_A_64_bits },

    { "Relative Jump jumps forward", cpu_selftest_org_relative_jump_jumps_forward },
    { "Relative Jump jumps backward", cpu_selftest_org_relative_jump_jumps_backward },
    /* TODO: { "Relative jump across segment boundary generates interrupt", cpu_selftest_org_relative_across_segement_boundary_generates_interrupt }, */
    { "EXIT resets the link in executive mode", cpu_selftest_org_exit_resets_link_in_executive_mode },
    { "EXIT resets the link except the privileged MS bits is user mode", cpu_selftest_org_exit_resets_link_except_privileged_ms_bits_in_user_mode },
    { "Absolute jump jumps to new location", cpu_selftest_org_absolute_jump },
    { "RETURN sets SF and unstacks link", cpu_selftest_org_return_sets_SF_and_unstacks_link },
    { "RETURN resets the link except privileged MS bits in user mode", cpu_selftest_org_return_resets_link_except_privileged_ms_bits_in_user_mode },
    { "RETURN does not pop stack if operand is not stack but does set SF to NB", cpu_selftest_org_return_does_not_pop_stack_if_operand_is_not_stack_but_sets_NB },
    { "STACK LINK puts link on the stack and adds the operand to the stacked value of CO", cpu_selftest_org_stacklink_puts_link_on_stack_adding_operand_to_stacked_CO },
    { "STACK LINK treats operand as signed", cpu_selftest_org_stacklink_treats_operand_as_signed },
    /*{ "STACK LINK generates an interrupt when adding the operand to CO crosses a segment boundary", cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_crosses_segment_boundary },*/
    { "MS= sets unmasked bits only when in executive mode", cpu_selftest_org_ms_load_sets_unmasked_bits_only_in_executive_mode },
    { "MS= does not set masked bits in executive mode", cpu_selftest_org_ms_load_does_not_set_masked_bits_in_executive_mode },
    { "MS= does not set privileged bits even if unmasked when in user mode", cpu_selftest_org_ms_load_does_not_set_privileged_unmasked_bits_in_user_mode },
    { "DL= loads pseudo register for the display lamps", cpu_selftest_org_dl_load_sets_dl_pseudo_register },
    { "SPM dummy order", cpu_selftest_org_spm_dummy },
    { "SETLINK stores the link", cpu_selftest_org_setlink_stores_link },
    { "SF=NB+ adds NB to signed operand and stores result to SF", cpu_selftest_org_sf_load_nb_plus_adds_NB_to_signed_operand_and_stores_to_SF },
    { "SF=NB+ generates interrupt on segment overflow", cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_overflow },
    { "SF=NB+ generates interrupt on segment underflow", cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_underflow },
    { "NB= loads NB", cpu_selftest_org_nb_load_loads_nb },
    { "NB=SF+ adds SF to signed operand and stores result to NB", cpu_selftest_org_nb_load_sf_plus_adds_SF_to_signed_operand_and_stores_to_NB },
    { "NB=SF+ generates interrupt on segment overflow", cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_overflow },
    { "NB=SF+ generates interrupt on segment underflow", cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_underflow },
    { "Branch on eq does not branch on false", cpu_selftest_org_br_eq_does_not_branch_on_false },
    { "Branch on eq branches on true", cpu_selftest_org_br_eq_does_branches_on_true },
    { "Branch on ne does not branch on false", cpu_selftest_org_br_ne_does_not_branch_on_false },
    { "Branch on ne branches on true", cpu_selftest_org_br_ne_does_branches_on_true },
    { "Branch on ge does not branch on false", cpu_selftest_org_br_ge_does_not_branch_on_false },
    { "Branch on ge branches on true", cpu_selftest_org_br_ge_does_branches_on_true },
    { "Branch on lt does not branch on false", cpu_selftest_org_br_lt_does_not_branch_on_false },
    { "Branch on lt branches on true", cpu_selftest_org_br_lt_does_branches_on_true },
    { "Branch on le does not branch on false", cpu_selftest_org_br_le_does_not_branch_on_false },
    { "Branch on le branches on true", cpu_selftest_org_br_le_does_branches_on_true },
    { "Branch on gt does not branch on false", cpu_selftest_org_br_gt_does_not_branch_on_false },
    { "Branch on gt branches on true", cpu_selftest_org_br_gt_does_branches_on_true },
    { "Branch on ovf does not branch on false", cpu_selftest_org_br_ovf_does_not_branch_on_false },
    { "Branch on ovf branches on true", cpu_selftest_org_br_ovf_does_branches_on_true },
    { "Branch on bn does not branch on false", cpu_selftest_org_br_bn_does_not_branch_on_false },
    { "Branch on bn branches on true", cpu_selftest_org_br_bn_does_branches_on_true },

    { "No B overflow interrupt if B overflow is inhibited", cpu_selftest_no_b_overflow_interrupt_if_b_overflow_is_inhibited },
    { "No zero divide interrupt if zero divide is inhibited", cpu_selftest_no_zero_divide_interrupt_if_zero_divide_is_inhibited },
    { "No bounds check interrupt if bounds check is inhibited", cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited },
    { "No SSS interrupt if SSS interrupt is inhibited", cpu_selftest_no_sss_interrupt_if_sss_is_inhibited }
};

// TODO: test for illegal combinations, e.g. store to literal, V32 or V64 (k=2/3) with DR (n'=5).

static void cpu_selftest_reset(UNITTEST *test)
{
    cpu_reset_state();

    testContext.testName = test->name;
    testContext.currentLoadLocation = 0;
    testContext.result = SCPE_OK;
}

static void cpu_selftest_set_load_location(uint32 location)
{
    testContext.currentLoadLocation = location;
}

static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n)
{
    uint16 order;

    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= (k & 0x7) << 6;
    order |= n & 0x3F;
    sac_write_16_bit_word(testContext.currentLoadLocation, order);
    testContext.currentLoadLocation += 1;
}

static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 kp, uint8 np)
{
    uint16 order;

    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= 0x7 << 6;
    order |= (kp & 0x7) << 3;
    order |= np & 0x7;
    sac_write_16_bit_word(testContext.currentLoadLocation, order);
    testContext.currentLoadLocation += 1;
}

static void cpu_selftest_load_organisational_order_literal(uint8 f, uint8 n)
{
    uint16 order;

    order = 0;
    order |= (f & 0x3F) << 7;
    order |= n & 0x3F;
    sac_write_16_bit_word(testContext.currentLoadLocation, order);
    testContext.currentLoadLocation += 1;
}

static void cpu_selftest_load_organisational_order_extended(uint8 f, uint8 kp, uint8 np)
{
    uint16 order;

    order = 0;
    order |= (f & 0x3F) << 7;
    order |= 0x1 << 6;
    order |= (kp & 0x7) << 3;
    order |= np & 0x7;
    sac_write_16_bit_word(testContext.currentLoadLocation, order);
    testContext.currentLoadLocation += 1;
}

static void cpu_selftest_load_16_bit_literal(uint16 value)
{
    sac_write_16_bit_word(testContext.currentLoadLocation, value);
    testContext.currentLoadLocation += 1;
}

static void cpu_selftest_load_32_bit_literal(uint32 value)
{
    sac_write_16_bit_word(testContext.currentLoadLocation, (value >> 16) & 0xFFFF);
    sac_write_16_bit_word(testContext.currentLoadLocation + 1, value & 0xFFFF);
    testContext.currentLoadLocation += 2;
}

static void cpu_selftest_load_64_bit_literal(t_uint64 value)
{
    sac_write_16_bit_word(testContext.currentLoadLocation, (value >> 48) & 0xFFFF);
    sac_write_16_bit_word(testContext.currentLoadLocation + 1, (value >> 32) & 0xFFFF);
    sac_write_16_bit_word(testContext.currentLoadLocation + 2, (value >> 16) & 0xFFFF);
    sac_write_16_bit_word(testContext.currentLoadLocation + 3, value & 0xFFFF);
    testContext.currentLoadLocation += 4;
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

static void cpu_selftest_set_executive_mode(void)
{
    cpu_selftest_set_register(REG_MS, 0x0004);
}

static void cpu_selftest_set_user_mode(void)
{
    cpu_selftest_set_register(REG_MS, 0x0000);
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

static REG *cpu_selftest_find_register(char *name)
{
    REG *rptr;
    for (rptr = cpu_dev.registers; rptr->name != NULL; rptr++)
    {
        if (strcmp(rptr->name, name) == 0)
        {
            break;
        }
    }

    if (rptr->name == NULL)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Could not find register %s\n", name);
        testContext.result = SCPE_AFAIL;
        rptr = NULL;
    }

    return rptr;
}

static t_uint64 cpu_selftest_get_register(char *name)
{
    t_uint64 result = 0;
    t_uint64 mask;
    REG *reg = cpu_selftest_find_register(name);
    switch (reg->width)
    {
        case 16:
        {
            result = *(uint16 *)(reg->loc);
            mask = 0xFFFF;
            break;
        }
        case 32:
        {
            result = *(uint32 *)(reg->loc);
            mask = 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            result = *(t_uint64 *)(reg->loc);
            mask = 0xFFFFFFFFFFFFFFFF;
            break;
        }
        default:
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected register width %d for register %s\n", reg->width, name);
            testContext.result = SCPE_AFAIL;
            break;
        }
    }

    return result & mask;
}

static void cpu_selftest_set_register(char *name, t_uint64 value)
{
    REG *reg = cpu_selftest_find_register(name);
    switch (reg->width)
    {
        case 16:
        {
            *(uint16 *)(reg->loc) = value & 0xFFFF;
            break;
        }
        case 32:
        {
            *(uint32 *)(reg->loc) = value & 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            *(t_uint64 *)(reg->loc) = value & 0xFFFFFFFFFFFFFFFF;
            break;
        }
        default:
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected register width %d for register %s\n", reg->width, name);
            testContext.result = SCPE_AFAIL;
            break;
        }
    }
}

static void cpu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
    t_uint64 actualValue = cpu_selftest_get_register(name);

    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value in register %s to be %llX, but was %llX\n", name, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_reg_equals_mask(char *name, t_uint64 expectedValue, t_uint64 mask)
{
    t_uint64 actualValue = cpu_selftest_get_register(name);

    if ((mask & actualValue) != (mask & expectedValue))
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value in register %s to be %llX, but was %llX for mask %llX\n", name, mask & expectedValue, mask & actualValue, mask);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_memory_contents_32_bit(t_addr address, uint32 expectedValue)
{
    uint32 actualValue = sac_read_32_bit_word(address);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at address %08X to be %08X, but was %08X\n", address, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_memory_contents_64_bit(t_addr address, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_64_bit_word(address);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at address %08X to be %016llX, but was %016llX\n", address, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_vector_content_64_bit(t_addr origin, uint32 offset, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_64_bit_word(cpu_selftest_get_64_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %016llX, but was %016llX\n", offset, origin, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_vector_content_32_bit(t_addr origin, uint32 offset, uint32 expectedValue)
{
    uint32 actualValue = sac_read_32_bit_word(cpu_selftest_get_32_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %08X, but was %08X\n", offset, origin, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_vector_content_16_bit(t_addr origin, uint32 offset, uint16 expectedValue)
{
    uint16 actualValue = sac_read_16_bit_word(cpu_selftest_get_16_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %04X, but was %04X\n", offset, origin, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_vector_content_8_bit(t_addr origin, uint32 offset, uint8 expectedValue)
{
    uint8 actualValue = sac_read_8_bit_word(cpu_selftest_get_8_bit_vector_element_address(origin, offset));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %02X, but was %02X\n", offset, origin, expectedValue, actualValue);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_vector_content_4_bit(t_addr origin, uint32 offset, uint8 expectedValue)
{
    uint8 byte = sac_read_8_bit_word(cpu_selftest_get_4_bit_vector_element_address(origin, offset));
    uint8 shift = 4 * (1 - (offset & 0x1));
    uint8 actualNibble = (byte >> shift) & 0xF;
    if (actualNibble != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %01X, but was %01X\n", offset, origin, expectedValue, actualNibble);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_vector_content_1_bit(t_addr origin, uint32 offset, uint8 expectedValue)
{
    uint8 byte = sac_read_8_bit_word(cpu_selftest_get_1_bit_vector_element_address(origin, offset));
    uint8 shift = 7 - (offset & 0x7);
    uint8 actualBit = (byte >> shift) & 0x1;
    if (actualBit != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value at element %u of vector at %08X to be %01X, but was %01X\n", offset, origin, expectedValue, actualBit);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_no_b_overflow(void)
{
    t_uint64 bod = cpu_selftest_get_register(REG_BOD);
    if (bod & BOD_BOVF_MASK)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected B overflow\n");
        testContext.result = SCPE_AFAIL;
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
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected B overflow\n");
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_b_overflow_interrupt(void)
{
    cpu_selftest_assert_interrupt();
    cpu_selftest_assert_b_overflow();
}

static void cpu_selftest_assert_no_a_overflow(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (aod &AOD_OVF_MASK)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected A overflow\n");
        testContext.result = SCPE_AFAIL;
    }
}
static void cpu_selftest_assert_no_a_overflow_interrupt(void)
{
    cpu_selftest_assert_no_interrupt();
    cpu_selftest_assert_no_a_overflow();
}

static void cpu_selftest_assert_a_overflow(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (!(aod & AOD_OVF_MASK))
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected A overflow\n");
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_a_overflow_interrupt(void)
{
    cpu_selftest_assert_interrupt();
    cpu_selftest_assert_a_overflow();
}

static void cpu_selftest_assert_no_a_zero_divide(void)
{
    t_uint64 aod = cpu_selftest_get_register(REG_AOD);
    if (aod &AOD_ZDIV_MASK)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected A zero divide\n");
        testContext.result = SCPE_AFAIL;
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
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected A zero divide\n");
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_a_zero_divide_interrupt(void)
{
    cpu_selftest_assert_interrupt();
    cpu_selftest_assert_a_zero_divide();
}

static void cpu_selftest_assert_interrupt(void)
{
    if (cpu_get_interrupt_number() == 255)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected interrupt to have occurred\n");
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_no_interrupt(void)
{
    if (cpu_get_interrupt_number() != 255)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected interrupt\n");
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_d_interrupt(char *name, uint32 mask)
{
    if (cpu_get_interrupt_number() != INT_PROGRAM_FAULTS)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected %s interrupt to have occurred\n", name);
        testContext.result = SCPE_AFAIL;
    }

    uint32 dod = cpu_selftest_get_register(REG_DOD) & 0xFFFFFFFF;
    if (!(dod & mask))
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected %s bit to be set in DOD\n", name);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_no_d_interrupt(char *name, uint32 mask)
{
    cpu_selftest_assert_no_interrupt();

    uint32 dod = cpu_selftest_get_register(REG_DOD) & 0xFFFFFFFF;
    if (dod & mask)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected %s bit to be clear in DOD\n", name);
        testContext.result = SCPE_AFAIL;
    }
}

static void cpu_selftest_assert_bound_check_interrupt(void)
{
    cpu_selftest_assert_d_interrupt("BCH", DOD_BCH_MASK);
}

static void cpu_selftest_assert_its_interrupt(void)
{
    cpu_selftest_assert_d_interrupt("ITS", DOD_ITS_MASK);
}

static void cpu_selftest_assert_sss_interrupt(void)
{
    cpu_selftest_assert_d_interrupt("SSS", DOD_SSS_MASK);
}

static void cpu_selftest_assert_no_bound_check_interrupt(void)
{
    cpu_selftest_assert_no_d_interrupt("BCH", DOD_BCH_MASK);
}

static void cpu_selftest_assert_no_its_interrupt(void)
{
    cpu_selftest_assert_no_d_interrupt("ITS", DOD_ITS_MASK);
}

static void cpu_selftest_assert_no_sss_interrupt(void)
{
    cpu_selftest_assert_no_d_interrupt("SSS", DOD_SSS_MASK);
}

static void cpu_selftest_assert_segment_overflow_interrupt(void)
{
    if (cpu_get_interrupt_number() != INT_PROGRAM_FAULTS)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected segment overflow interrupt to have occurred\n");
        testContext.result = SCPE_AFAIL;
    }

    /* TODO: Other checks to ensure this is a segment overflow */
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

static void cpu_selftest_assert_fail(void)
{
    sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Test failed\n");
    testContext.result = SCPE_AFAIL;
}

t_stat cpu_selftest(void)
{
    t_stat result = SCPE_OK;
    int n;
    int i;
    int countSuccessful = 0;
    int countFailed = 0;

    n = sizeof(tests) / sizeof(UNITTEST);

    for (i = 0; i < n; i++)
    {
        cpu_selftest_reset(&tests[i]);
        tests[i].runner();
        if (testContext.result == SCPE_OK)
        {
            sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%s [OK]\n", tests[i].name);
            countSuccessful++;
        }
        else
        {
            result = SCPE_AFAIL;
            sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%s [FAIL]\n", tests[i].name);
            countFailed++;
        }
    }

    sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "\n");
    if (countFailed == 0)
    {
        sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "ALL %d TESTS PASSED\n", n);
    }
    else
    {
        sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%d of %d TESTS PASSED, %d FAILED\n", countSuccessful, n, countFailed);
    }

    return result;
}

static void cpu_selftest_16_bit_instruction_advances_co_by_1(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_32_bit_instruction_advances_co_by_2(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_48_bit_instruction_advances_co_by_3(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 3);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_80_bit_instruction_advances_co_by_5(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x7FFFFFFFFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 5);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_6_bit_positive_literal()
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000001F);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_6_bit_negative_literal()
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x3F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_0(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 0);
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 0);
    cpu_selftest_set_register(REG_MS, 0xAAAA);
    cpu_selftest_set_register(REG_NB, 0xBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBB00000000);
    cpu_execute_next_order();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBB00000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_1(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 1);
    cpu_selftest_set_register(REG_XNB, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_2(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 2);
    cpu_selftest_set_register(REG_SN, 0xAAAA);
    cpu_selftest_set_register(REG_NB, 0xBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_3(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 3);
    cpu_selftest_set_register(REG_SN, 0xAAAA);
    cpu_selftest_set_register(REG_SF, 0xBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_4(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 4);
    cpu_selftest_set_register(REG_MS, 0x0100);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_16(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 16);
    cpu_selftest_set_register(REG_D, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_17(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 17);
    cpu_selftest_set_register(REG_XD, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_18(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 18);
    cpu_selftest_set_register(REG_DT, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_19(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 19);
    cpu_selftest_set_register(REG_XDT, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_20(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 20);
    cpu_selftest_set_register(REG_DOD, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_32(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 32);
    cpu_selftest_set_register(REG_BOD, 0xAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xBBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_33(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 33);
    cpu_selftest_set_register(REG_BOD, 0xABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_34(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 34);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_internal_register_48(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 48);
    cpu_selftest_set_register(REG_AEX, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_variable(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V32, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_32_bit_word(base + n, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x3F;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V32, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_32_bit_word(base + n, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_variable(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2*n, 0xBBBBBBBBAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xBBBBBBBBAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

// TODO: p17 SN relative
// TODO: p17 SN interrupt on overflow
// TODO: p17 long instruction offset is signed (?)

static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4()
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5()
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB_5, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_load_16_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000AAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000000000AA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, 0, 0xAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000000B);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, 0, 0x40);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_S0, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_16_bit_signed_positive_literal(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0x7FFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000007FFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_16_bit_signed_negative_literal(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_SIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_signed_positive_literal(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000007FFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_signed_negative_literal(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_2(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_3(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL_3);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_16_bit_unsigned_literal(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_16_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_16_bit_literal(0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000FFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_32_bit_unsigned_literal(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_6(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL_6);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_64_bit_literal_np_7(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL, NP_64_BIT_LITERAL_7);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_literal_kp_1(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, KP_LITERAL_1, NP_16_BIT_UNSIGNED_LITERAL);
    sac_write_16_bit_word(1, 0xFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000FFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_sf(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(base + n, 0xAAAABBBB);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_zero(void)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(n, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(base + n, 0xAAAABBBB);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_32_bit_word(base + n, 0xAAAABBBB);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_from_stack(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_STACK);
    sac_write_32_bit_word(base, 0xAAAABBBB);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_NB_REF);
    sac_write_32_bit_word(base, 0xAAAABBBB);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_XNB);
    sac_write_32_bit_word(base, 0xAAAABBBB);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_sf(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(base + (n * 2), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero(void)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(n * 2, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(base + (n * 2), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(base + (n * 2), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_from_stack(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_STACK);
    sac_write_64_bit_word(base, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_NB_REF);
    sac_write_64_bit_word(base, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_XNB);
    sac_write_64_bit_word(base, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB_5, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_zero(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_stack(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_STACK);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_dr(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_DR);
    cpu_selftest_set_register(REG_B, vecoffset);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB_REF);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_XNB_REF);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_64_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_16_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_load_16_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000AABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_8_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000000000AB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_4_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_load_4_bit_value_to_descriptor_location(vecorigin, vecoffset, 0xC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000000C);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_b_relative_descriptor_1_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_load_1_bit_value_to_descriptor_location(vecorigin, vecoffset, 0x1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_STACK);
    cpu_selftest_set_register(REG_SF, base);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB_REF);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_XNB_REF);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_load_32_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_load_64_bit_value_to_descriptor_location(vecorigin, 0, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_load_16_bit_value_to_descriptor_location(vecorigin, 0, 0xAABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000AABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_load_8_bit_value_to_descriptor_location(vecorigin, 0, 0xAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000000000AB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_load_4_bit_value_to_descriptor_location(vecorigin, 0, 0xC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x000000000000000C);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_load_1_bit_value_to_descriptor_location(vecorigin, 0, 0x1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_6_bit_literal_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 0);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_1_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XNB, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_2_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 2);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_3_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_SF, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_4_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 4);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_16(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 16);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_17(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 17);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_18(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 18);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DT, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_19(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 19);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XDT, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_20(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 20);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_32(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 32);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_BOD, 0xAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_B, 0xBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_33(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 33);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_BOD, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_34(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 34);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_48(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_IR, 48);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AEX, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_32_bit_variable(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V32, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xBBBBBBBBAAAAAAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base + n, 0xAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_64_bit_variable(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2*n, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_4(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_32_bit_value_at_6_bit_offset_k_5(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB_5, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAAAAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_64_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, vecoffset, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_16_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000AAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_16_bit(vecorigin, vecoffset, 0xAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_8_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000000000AA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_8_bit(vecorigin, vecoffset, 0xAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_4_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000000B);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_4_bit(vecorigin, vecoffset, 0xB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_b_relative_descriptor_1_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_SB, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_1_bit(vecorigin, vecoffset, 0x1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_zero_relative_descriptor_64_bit_value_at_6_bit_offset(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_S0, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, 0, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_literal_kp_0_generates_interrupt(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, KP_LITERAL, NP_64_BIT_LITERAL_6);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_extended_literal_kp_1_generates_interrupt(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, KP_LITERAL_1, NP_64_BIT_LITERAL_6);
    cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base + n, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero(void)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(n, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base + n, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base + n, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}
static void cpu_selftest_store_operand_extended_32_bit_variable_from_stack(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_STACK);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base, 0xAAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_NB_REF);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V32, NP_XNB);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_32_bit(base, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero(void)
{
    uint16 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(n * 2, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint16 n = 0x1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}
static void cpu_selftest_store_operand_extended_64_bit_variable_from_stack(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_STACK);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_NB_REF);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_V64, NP_XNB);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_4(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_sf_kp_5(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB_5, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_zero(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_stack(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_STACK);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_dr(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
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

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB_REF);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_32_bit_value_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_XNB_REF);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, vecoffset, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_64_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, vecoffset, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_16_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000AABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_16_bit(vecorigin, vecoffset, 0xAABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_8_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000000000AB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_8_bit(vecorigin, vecoffset, 0xAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_4_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000000C);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_4_bit(vecorigin, vecoffset, 0xC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_b_relative_descriptor_1_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    uint32 vecoffset = 1;
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_1_bit(vecorigin, vecoffset, 0x1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_sf(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_zero(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x4;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_STACK);
    cpu_selftest_set_register(REG_SF, base);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_dr(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB_REF);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_XNB_REF);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_64_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_64_bit(vecorigin, 0, 0xAAAABBBBCCCCDDDD);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_16_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000AABB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_16_bit(vecorigin, 0, 0xAABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_8_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x00000000000000AB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_8_bit(vecorigin, 0, 0xAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_4_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x000000000000000C);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_4_bit(vecorigin, 0, 0xC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_1_bit_value_from_nb(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    int8 n = 0x2;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_A, 0x0000000000000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_1_bit(vecorigin, 0, 0x1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD(void)
{
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XDO, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xd_load_loads_whole_of_XD(void)
{
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_stack_stacks_operand(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_STS1, F_STACK, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xd_store_stores_xd_to_operand(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS1, F_STORE_XD, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xd_store_to_secondary_operand_generates_interrupt(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS1, F_STORE_XD, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_sts1_xdb_load_loads_bound_in_XD(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XDB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFCCCCCC);
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xAACCCCCCBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xchk_operand_negative_clears_DOD_XCH_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XCHK, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_DOD, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0x00000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xchk_operand_ge_XDB_clears_DOD_XCH_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XCHK, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 0));
    cpu_selftest_set_register(REG_DOD, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0x00000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xchk_operand_within_XDB_sets_DOD_XCH_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XCHK, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 0));
    cpu_selftest_set_register(REG_DOD, 0x00000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0x00000001);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_adds_signed_operand_to_D_origin(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 3));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_scales_modifier_for_type_0(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_scales_modifier_for_type_1(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 2, 5));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_scales_modifier_for_type_2(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 12));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_0_when_US_set(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 4) | DESCRIPTOR_US_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 5) | DESCRIPTOR_US_MASK);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_smod_does_not_scale_modifier_for_type_2_when_US_set(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 4) | DESCRIPTOR_US_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 5) | DESCRIPTOR_US_MASK);
    cpu_selftest_assert_no_interrupt();
}


static void cpu_selftest_sts1_smod_does_not_check_bounds(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_set_register(REG_DOD, 0x00000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 4));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 12));
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_xmod_adds_signed_operand_to_XD_origin_subtracts_from_bound(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_64_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_32_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 1, 12));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_16_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 1, 10));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_8_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_within_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 1, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value_crossing_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000003);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 5, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_within_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000004);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 8, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 4, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value_crossing_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000009);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 16, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 7, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_checks_bounds_for_type_0(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts1_xmod_checks_bounds_for_type_1(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts1_xmod_checks_bounds_for_type_2(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_0_when_BC_set(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_1_when_BC_set(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_xmod_does_not_check_bounds_for_type_2_when_BC_set(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_is_type_3(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_is_type_3(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_slgc_processes_type_0_descriptors(void)
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

static void cpu_selftest_sts1_slgc_processes_type_2_descriptors(void)
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

static void cpu_selftest_sts1_slgc_processes_long_vector(void)
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

static void cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out(void)
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

static void cpu_selftest_sts1_slgc_processes_L0(void)
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

static void cpu_selftest_sts1_slgc_processes_L1(void)
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

static void cpu_selftest_sts1_slgc_processes_L2(void)
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

static void cpu_selftest_sts1_slgc_processes_L3(void)
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

static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_source_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_smvb_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_smvb_generates_checks_bound_on_destination_not_0(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts1_smvb_copies_byte_with_mask(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_smvb_uses_filler_when_source_empty_with_mask(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_source_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_smvf_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SMVF, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_smvf_copies_to_zero_length_destination(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_smvf_copies_bytes_and_fills_with_mask(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_talu_returns_test_register_greater_than_if_not_found(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_0(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_talu_returns_test_register_equals_if_found_in_type_2(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_source_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_scmp_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_SCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_strings_identical_with_filler(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_equals_if_destination_is_shorter_and_subset_is_equal(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_greater_than_if_source_byte_greater_than_destination_byte(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_scmp_returns_test_register_less_than_if_source_byte_less_than_destination_byte(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts1_sub1_loads_XD_and_modifies_it(void)
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

static void cpu_selftest_sts1_sub1_calculates_B(void)
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

static void cpu_selftest_sts1_sub1_modifies_D(void)
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

static void cpu_selftest_sts2_do_load_loads_ls_half_of_D(void)
{
    cpu_selftest_set_register(REG_D, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS2, F_LOAD_DO, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_load_loads_whole_of_D(void)
{
    cpu_selftest_set_register(REG_D, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS2, F_LOAD_D, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xBBBBBBBBFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_stack_load_stacks_D_loads_new_D(void)
{
    cpu_selftest_set_register(REG_SF, 0x00F0);
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_load_order_extended(CR_STS2, F_STACK_LOAD_D, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, 0x00F2);
    cpu_selftest_assert_reg_equals(REG_D, 0xCCCCCCCCFFFFFFFF);
    cpu_selftest_assert_memory_contents_64_bit(0x000000F2, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_store_stores_d_to_operand(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS2, F_STORE_D, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_d_store_to_secondary_operand_generates_interrupt(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_STS2, F_STORE_D, K_SB, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_sts2_db_load_loads_bound_in_D(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_LOAD_DB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFCCCCCC);
    cpu_selftest_set_register(REG_D, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xAACCCCCCBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mdr_advances_location_and_loads_D_with_operand_pointed_to_by_D()
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

static void cpu_selftest_sts2_mod_adds_signed_operand_to_D_origin_subtracts_from_bound(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_64_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_32_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 1, 12));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_16_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 1, 10));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_8_bit_value(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_within_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 1, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_4_bit_value_crossing_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000003);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 5, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_within_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000004);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 8, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 4, 8));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_scales_modifier_for_1_bit_value_crossing_byte(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000009);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 16, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 7, 9));
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_mod_checks_bounds_for_type_0(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts2_mod_checks_bounds_for_type_1(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts2_mod_checks_bounds_for_type_2(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_0_when_BC_set(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_1_when_BC_set(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts2_mod_does_not_check_bounds_for_type_2_when_BC_set(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_MOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8) | DESCRIPTOR_BC_MASK);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts2_rmod_loads_least_significant_half_of_D_and_adds_to_origin(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_RMOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_set_register(REG_D, 0xBBBBBBBB11111111);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xAAAAAAAACCCCCCCC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_blgc_generates_its_interrupt_if_destination_is_type_3(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BLGC, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L0(void)
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

static void cpu_selftest_sts2_blgc_processes_type_1_descriptor_L1(void)
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

static void cpu_selftest_sts2_blgc_processes_type_2_descriptor_L2(void)
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

static void cpu_selftest_sts2_blgc_processes_type_0_descriptor_L3(void)
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

static void cpu_selftest_sts2_bmvb_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_bmvb_generates_checks_bound_on_destination_not_0(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BMVB, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 0, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_bound_check_interrupt();
}

static void cpu_selftest_sts2_bmvb_copies_byte_with_mask(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts2_bmve_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BMVE, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_bmve_copies_byte_with_mask_to_whole_destination(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts2_smvf_copies_bytes_and_fills_with_mask(void)
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
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_bscn_generates_its_interrupt_if_destination_is_type_3(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BSCN, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_bscn_sets_less_than_if_byte_not_found(void)
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

static void cpu_selftest_sts2_bscn_finds_byte_in_type_0_descriptor(void)
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

static void cpu_selftest_sts2_bscn_finds_byte_in_type_1_descriptor(void)
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

static void cpu_selftest_sts2_bscn_finds_byte_in_type_2_descriptor(void)
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

static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_not_8_bit(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 16));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_bcmp_generates_its_interrupt_if_destination_is_type_3(void)
{
    cpu_selftest_load_order_extended(CR_STS2, F_BCMP, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x0000000000000000);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_MISCELLANEOUS, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts2_bcmp_sets_equals_if_byte_found_in_all_elements(void)
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

static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_smaller(void)
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

static void cpu_selftest_sts2_bcmp_sets_less_than_if_byte_differs_and_is_larger(void)
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

static void cpu_selftest_sts2_bcmp_finds_byte_in_type_0_descriptor(void)
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

static void cpu_selftest_sts2_bcmp_finds_byte_in_type_1_descriptor(void)
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

static void cpu_selftest_sts2_bcmp_finds_byte_in_type_2_descriptor(void)
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

static void cpu_selftest_sts2_sub2_modifies_XD(void)
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

static void cpu_selftest_sts2_sub2_calculates_B(void)
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

static void cpu_selftest_sts2_sub2_modifies_D_existing_D(void)
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

static void cpu_selftest_b_load_loads_B(void)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_B, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_load_and_decrement_loads_B_and_subtracts_1(void)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x0000000F);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x0000000E);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_load_and_decrement_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_b_stack_and_load_stacks_B_and_loads_B(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_B, F_STACK_LOAD_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0x00000000AAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_store_stores_B(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_B, F_STORE_B, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_add_adds_operand_to_B(void)
{
    cpu_selftest_load_order_extended(CR_B, F_ADD_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x0AAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x0AAAAAA9);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_add_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_ADD_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_b_sub_subtracts_operand_from_B(void)
{
    cpu_selftest_load_order_extended(CR_B, F_SUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xAAAAAAAB);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_sub_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_SUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_b_mul_multiplies_operand_by_B(void)
{
    cpu_selftest_load_order_extended(CR_B, F_MUL_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-5);
    cpu_selftest_set_register(REG_B, 6);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, -30 & 0xFFFFFFFF);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_mul_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_MUL_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_b_xor(void)
{
    cpu_selftest_load_order_extended(CR_B, F_XOR_B, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x66666666);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_or(void)
{
    cpu_selftest_load_order_extended(CR_B, F_OR_B, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xEEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_shift_shifts_left_for_positive_operand(void)
{
    cpu_selftest_load_order(CR_B, F_SHIFT_L_B, K_LITERAL, 0x02);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFF8);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_shift_shifts_right_for_negative_operand(void)
{
    cpu_selftest_load_order(CR_B, F_SHIFT_L_B, K_LITERAL, 0x3E);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xE0000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_shift_flags_overflow(void)
{
    cpu_selftest_load_order(CR_B, F_SHIFT_L_B, K_LITERAL, 0x01);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_b_and(void)
{
    cpu_selftest_load_order_extended(CR_B, F_AND_B, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x88888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_b_rsub_subtracts_B_from_operand(void)
{
    cpu_selftest_load_order_extended(CR_B, F_RSUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xAAAAAAAB);
    cpu_selftest_assert_no_b_overflow_interrupt();
}

static void cpu_selftest_b_rsub_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_RSUB_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_b_comp_sets_less_than_when_B_less_than_operand(void)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_comp_sets_equals_when_B_equals_operand(void)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0xFFFFFFFF);
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_comp_sets_greater_than_when_B_greater_than_operand(void)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_B, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x00000001);
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_comp_sets_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_COMP_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_B, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x80000000);
    cpu_selftest_assert_test_overflow();
    cpu_selftest_assert_b_overflow();
}

static void cpu_selftest_b_cinc_compares_B_with_operand(void)
{
    cpu_selftest_load_order_extended(CR_B, F_CINC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFE);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_cinc_increments_B(void)
{
    cpu_selftest_load_order_extended(CR_B, F_CINC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_B, 0x00000000);
    cpu_selftest_assert_no_b_overflow();
}

static void cpu_selftest_b_cinc_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_B, F_CINC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_B, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow_interrupt();
}

static void cpu_selftest_x_load_loads_X(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_LOAD_X, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_stack_and_load_stacks_X_and_loads_X(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_XS, F_STACK_LOAD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_X, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0x00000000AAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_store_stores_X(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_XS, F_STORE_X, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_X, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_add_adds_operand_to_X(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x0AAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x0AAAAAA9);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_x_add_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_ADD_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_overflow_interrupt();
}

static void cpu_selftest_x_sub_subtracts_operand_from_X(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_SUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xAAAAAAAB);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_x_sub_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_SUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_overflow_interrupt();
}

static void cpu_selftest_x_mul_multiplies_operand_by_X(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_MUL_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-5);
    cpu_selftest_set_register(REG_X, 6);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, -30 & 0xFFFFFFFF);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_x_mul_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_MUL_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_X, 0x7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_overflow_interrupt();
}

static void cpu_selftest_x_div_divides_X_by_operand(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_DIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-2);
    cpu_selftest_set_register(REG_X, -4 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x00000002);
    cpu_selftest_assert_no_a_zero_divide_interrupt();
}

static void cpu_selftest_x_div_flags_divide_by_zero(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_DIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000000);
    cpu_selftest_set_register(REG_X, -4 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_zero_divide_interrupt();
}

static void cpu_selftest_x_xor(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_XOR_X, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x66666666);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_or(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_OR_X, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xEEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_shift_shifts_left_for_positive_operand(void)
{
    cpu_selftest_load_order(CR_XS, F_SHIFT_L_X, K_LITERAL, 0x02);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFE);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFF8);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_shift_shifts_right_for_negative_operand(void)
{
    cpu_selftest_load_order(CR_XS, F_SHIFT_L_X, K_LITERAL, 0x3E);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xE0000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_shift_flags_overflow(void)
{
    cpu_selftest_load_order(CR_XS, F_SHIFT_L_X, K_LITERAL, 0x01);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_overflow_interrupt();
}

static void cpu_selftest_x_rdiv_divides_operand_by_X(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_RDIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-4);
    cpu_selftest_set_register(REG_X, -2 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x00000002);
    cpu_selftest_assert_no_a_zero_divide_interrupt();
}

static void cpu_selftest_x_rdiv_flags_divide_by_zero(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_RDIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(-4);
    cpu_selftest_set_register(REG_X, 0x00000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_zero_divide_interrupt();
}

static void cpu_selftest_x_and(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_AND_X, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x88888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_x_rsub_subtracts_X_from_operand(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_RSUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAAA);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xAAAAAAAB);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_x_rsub_flags_overflow(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_RSUB_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_a_overflow_interrupt();
}

static void cpu_selftest_x_comp_sets_less_than_when_X_less_than_operand(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x7FFFFFFF);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_no_a_overflow();
}

static void cpu_selftest_x_comp_sets_equals_when_X_equals_operand(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0xFFFFFFFF);
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_no_a_overflow();
}

static void cpu_selftest_x_comp_sets_greater_than_when_X_greater_than_operand(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_X, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x00000001);
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_no_a_overflow();
}

static void cpu_selftest_x_comp_sets_overflow(void)
{
    cpu_selftest_load_order_extended(CR_XS, F_COMP_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_X, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_X, 0x80000000);
    cpu_selftest_assert_test_overflow();
    cpu_selftest_assert_a_overflow();
}

static void cpu_selftest_a_load_loads_AOD(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_LOAD_AOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFEDC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AOD, 0x0000000000001EDC);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_stack_and_load_stacks_AOD_and_loads_AOD(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_AU, F_STACK_LOAD_AOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_AOD, 0xBBBBBBBBFFFFFEDC);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0x0000000000001EDC);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_reg_equals(REG_AOD, 0x0000000000001FFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_store_stores_AOD(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_AU, F_STORE_AOD, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_AOD, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0x0000000000001FFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_add_adds_operand_to_A(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_ADD_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000100000000);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_a_sub_subtracts_operand_from_A(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_SUB_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFE);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_a_mul_multiplies_operand_by_A(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_MUL_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000002);
    cpu_selftest_set_register(REG_A, 0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000100000000);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_a_xor(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_XOR_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000066666666);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_or(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_OR_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000EEEEEEEE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_shift_shifts_left_for_positive_operand(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_SHIFT_L_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAA82);
    cpu_selftest_set_register(REG_A, 0x0000000180000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000600000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_shift_shifts_right_for_negative_operand(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_SHIFT_L_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAAAAFE);
    cpu_selftest_set_register(REG_A, 0x0000000180000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000060000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_and(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_AND_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAAAAAAAAAA);
    cpu_selftest_set_register(REG_A, 0xCCCCCCCCCCCCCCCC);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x0000000088888888);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_a_rsub_subtracts_A_from_operand(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_RSUB_A, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFF);
    cpu_selftest_set_register(REG_A, 0x00000001);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFE);
    cpu_selftest_assert_no_a_overflow_interrupt();
}

static void cpu_selftest_a_comp_sets_less_than_when_A_less_than_operand(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_COMP_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFF80000000);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF7FFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFF7FFFFFFF);
    cpu_selftest_assert_test_less_than();
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_a_comp_sets_equals_when_A_equals_operand(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_COMP_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFF80000000);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAA80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAA80000000);
    cpu_selftest_assert_test_equals();
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_a_comp_sets_greater_than_when_A_greater_than_operand(void)
{
    cpu_selftest_load_order_extended(CR_AU, F_COMP_A, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFF7FFFFFFF);
    cpu_selftest_set_register(REG_A, 0xFFFFFFFF80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFF80000000);
    cpu_selftest_assert_test_greater_than();
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_dec_load_loads_AEX(void)
{
    cpu_selftest_load_order_extended(CR_ADC, F_LOAD_AEX, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AEX, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dec_stack_and_load_stacks_AEX_and_loads_AEX(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_ADC, F_STACK_LOAD_AEX, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_AEX, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_reg_equals(REG_AEX, 0xCCCCCCCCFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dec_store_stores_AEX(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_ADC, F_STORE_AEX, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_AEX, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_dec_comp_sets_overflow_when_AOD_and_operand_non_zero(void)
{
    cpu_selftest_load_order_extended(CR_ADC, F_COMP_AOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFFFFFFF);
    cpu_selftest_set_register(REG_AOD, 0x00000000000011F1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AOD, 0x00000000000011F1);
    cpu_selftest_assert_test_overflow();
}

static void cpu_selftest_dec_comp_clears_overflow_when_AOD_and_operand_is_zero(void)
{
    cpu_selftest_load_order_extended(CR_ADC, F_COMP_AOD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFFFFFFFFFFFFF);
    cpu_selftest_set_register(REG_AOD, 0xFFFFFFFFFFFFE000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AOD, 0xFFFFFFFFFFFFE000);
    cpu_selftest_assert_test_no_overflow();
}

static void cpu_selftest_flt_load_single_loads_32_bits_into_A(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_32, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xBBBBBBBB00000000);
    cpu_selftest_assert_operand_size_32();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_load_double_loads_64_bits_into_A(void)
{
    cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_operand_size_64();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_32_bits(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STACK_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_set_aod_operand_32_bit();
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0x00000000AAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFF00000000);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_stack_and_load_stacks_A_and_loads_A_64_bits(void)
{
    uint32 base = 0x00F0;
    cpu_selftest_load_order_extended(CR_FLOAT, F_STACK_LOAD, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xCCCCCCCCFFFFFFFF);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0xAAAAAAAABBBBBBBB);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_reg_equals(REG_A, 0xCCCCCCCCFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_store_stores_A_32_bits(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, n);
    cpu_selftest_set_aod_operand_32_bit();
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0x00000000AAAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_flt_store_stores_A_64_bits(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x2;
    cpu_selftest_load_order(CR_FLOAT, F_STORE, K_V64, n);
    cpu_selftest_set_aod_operand_64_bit();
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_A, 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAAAAAAFFFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_relative_jump_jumps_forward(void)
{
    cpu_selftest_load_organisational_order_literal(F_RELJUMP, 8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_relative_jump_jumps_backward(void)
{
    cpu_selftest_set_load_location(8);
    cpu_selftest_load_organisational_order_literal(F_RELJUMP, 0x3F);
    cpu_selftest_run_code_from_location(8);
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000007);
    cpu_selftest_assert_no_interrupt();
}

/* TODO: static void cpu_selftest_org_relative_across_segement_boundary_generates_interrupt(void) { cpu_selftest_assert_fail(); } */

static void cpu_selftest_org_exit_resets_link_in_executive_mode(void)
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

static void cpu_selftest_org_exit_resets_link_except_privileged_ms_bits_in_user_mode(void)
{
    cpu_selftest_load_organisational_order_extended(F_EXIT, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xAAFFBBBBFFFFFFFF);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xAA00);
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x7FFFFFFF);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_absolute_jump(void)
{
    cpu_selftest_set_load_location(8);
    cpu_selftest_load_organisational_order_literal(F_ABSJUMP, 0x10);
    cpu_selftest_run_code_from_location(8);
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000010);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_return_sets_SF_and_unstacks_link(void)
{
    uint32 base = 32;
    cpu_selftest_load_organisational_order_extended(F_RETURN, K_V64, NP_STACK);
    sac_write_64_bit_word(base, 0xFFFFBBBBAAAAAAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, base - 2);
    cpu_selftest_assert_reg_equals(REG_MS, 0xFFFF);
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x2AAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_return_resets_link_except_privileged_ms_bits_in_user_mode(void)
{
    uint32 base = 32;
    cpu_selftest_load_organisational_order_extended(F_RETURN, K_V64, NP_STACK);
    sac_write_64_bit_word(base, 0xFFFFBBBBAAAAAAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_user_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xFF00);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_return_does_not_pop_stack_if_operand_is_not_stack_but_sets_NB(void)
{
    uint32 base = 32;
    cpu_selftest_load_organisational_order_extended(F_RETURN, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xFFFFBBBBAAAAAAAA);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, base);
    cpu_selftest_assert_reg_equals(REG_MS, 0xFFFF);
    cpu_selftest_assert_reg_equals(REG_NB, 0xBBBA);
    cpu_selftest_assert_reg_equals(REG_CO, 0x2AAAAAAA);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_ms_load_sets_unmasked_bits_only_in_executive_mode(void)
{
    cpu_selftest_load_organisational_order_extended(F_MS_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAFFBBFF);
    cpu_selftest_set_executive_mode();
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xAABB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_ms_load_does_not_set_masked_bits_in_executive_mode(void)
{
    cpu_selftest_load_organisational_order_extended(F_MS_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xBBCC2233);
    cpu_selftest_set_register(REG_MS, 0x00FF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0x88EE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_ms_load_does_not_set_privileged_unmasked_bits_in_user_mode(void)
{
    cpu_selftest_load_organisational_order_extended(F_MS_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAFFBBFF);
    cpu_selftest_set_register(REG_MS, 0x00C8);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0xAAC8);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_dl_load_sets_dl_pseudo_register(void)
{
    cpu_selftest_load_organisational_order_extended(F_DL_LOAD, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xAAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DL, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_spm_dummy(void)
{
    cpu_selftest_load_organisational_order_extended(F_SPM, KP_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 3);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_setlink_stores_link(void)
{
    uint32 base = 32;
    cpu_selftest_set_load_location(10);
    cpu_selftest_load_organisational_order_extended(F_SETLINK, K_V64, NP_0);
    cpu_selftest_load_16_bit_literal(base);
    cpu_selftest_set_register(REG_MS, 0xAAAAAAAA);
    cpu_selftest_set_register(REG_NB, 0xBBBBBBBB);
    cpu_selftest_run_code_from_location(10);
    cpu_selftest_assert_memory_contents_64_bit(base * 2, 0xAAAABBBB0000000A);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_sf_load_nb_plus_adds_NB_to_signed_operand_and_stores_to_SF(void)
{
    cpu_selftest_load_organisational_order_literal(F_SF_LOAD_NB_PLUS, 0x3F);
    cpu_selftest_set_register(REG_NB, 10);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SF, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_stacklink_puts_link_on_stack_adding_operand_to_stacked_CO(void)
{
    uint32 base = 32;
    cpu_selftest_set_load_location(10);
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0x000000000000000A);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_MS, 0xAAAA);
    cpu_selftest_set_register(REG_NB, 0xBBBA);
    cpu_selftest_run_code_from_location(10);
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0xAAAABBBA00000014);
    cpu_selftest_assert_reg_equals(REG_SF, base + 2);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_stacklink_treats_operand_as_signed(void)
{
    /* Comment from RNI:
    The adder used to add the operand to the value of CO for a STACKLINK operation is the same adder that executes control transfers.
    Since these can jump backwards or forwards, the adder just treats the operand as a signed value in every case. For STACKLINK it
    will normally be a small positive number, as you surmise, the value depending on the number of parameters being passed, as shown
    in the example on page 62.
    */
    uint32 base = 32;
    cpu_selftest_set_load_location(10);
    cpu_selftest_load_organisational_order_extended(F_STACKLINK, KP_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFE);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_run_code_from_location(10);
    cpu_selftest_assert_memory_contents_64_bit(base + 2, 0x0000000000000008);
    cpu_selftest_assert_no_interrupt();
}

/*static void cpu_selftest_org_stacklink_generates_interrupt_when_adding_operand_to_CO_crosses_segment_boundary(void);*/

static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_overflow(void)
{
    cpu_selftest_load_organisational_order_extended(F_SF_LOAD_NB_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x0000FFFF);
    cpu_selftest_set_register(REG_NB, 10);
    cpu_selftest_run_code();
    cpu_selftest_assert_segment_overflow_interrupt();
}

static void cpu_selftest_org_sf_load_nb_plus_generates_interrupt_on_segment_underflow(void)
{
    cpu_selftest_load_organisational_order_extended(F_SF_LOAD_NB_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFB);
    cpu_selftest_set_register(REG_NB, 2);
    cpu_selftest_run_code();
    cpu_selftest_assert_segment_overflow_interrupt();
}

static void cpu_selftest_org_nb_load_loads_nb(void)
{
    cpu_selftest_load_organisational_order_extended(F_NB_LOAD, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xABABCFCF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_NB, 0xCFCE);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_nb_load_sf_plus_adds_SF_to_signed_operand_and_stores_to_NB(void)
{
    cpu_selftest_load_organisational_order_literal(F_NB_LOAD_SF_PLUS, 0x3F);
    cpu_selftest_set_register(REG_SF, 10);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_NB, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_overflow(void)
{
    cpu_selftest_load_organisational_order_extended(F_NB_LOAD_SF_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x0000FFFF);
    cpu_selftest_set_register(REG_SF, 10);
    cpu_selftest_run_code();
    cpu_selftest_assert_segment_overflow_interrupt();
}

static void cpu_selftest_org_nb_load_sf_plus_generates_interrupt_on_segment_underflow(void)
{
    cpu_selftest_load_organisational_order_extended(F_NB_LOAD_SF_PLUS, K_LITERAL, NP_32_BIT_UNSIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0xFFFFFFFB);
    cpu_selftest_set_register(REG_SF, 2);
    cpu_selftest_run_code();
    cpu_selftest_assert_segment_overflow_interrupt();
}

static void cpu_selftest_org_branch_test_branch_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 ms = ((t0 & 1) << 11) | ((t1 & 1) << 10) | ((t2 & 1) << 9) | ((bn & 1) << 8);
    cpu_selftest_set_load_location(0); /* reset load location as we may call this helper more than once in a test to test different combinations */
    cpu_selftest_load_organisational_order_literal(f, 8);
    cpu_selftest_set_register(REG_MS, ms);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 0x00000008);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_branch_test_branch_not_taken(uint8 f, uint16 t0, uint16 t1, uint16 t2, uint16 bn)
{
    uint16 ms = ((t0 & 1) << 11) | ((t1 & 1) << 10) | ((t2 & 1) << 9) | ((bn & 1) << 8);
    cpu_selftest_set_load_location(0); /* reset load location as we may call this helper more than once in a test to test different combinations */
    cpu_selftest_load_organisational_order_literal(f, 8);

    cpu_selftest_set_register(REG_MS, ms);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_org_br_eq_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_EQ, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_EQ, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_eq_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_EQ, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_EQ, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_ne_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_NE, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_NE, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_ne_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_NE, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_NE, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_ge_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GE, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GE, 0, 1, 1, 0);
}

static void cpu_selftest_org_br_ge_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GE, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GE, 1, 1, 0, 1);
}

static void cpu_selftest_org_br_lt_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LT, 1, 1, 0, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LT, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_lt_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LT, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LT, 0, 0, 1, 0);
}

static void cpu_selftest_org_br_le_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LE, 1, 1, 0, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_LE, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_le_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 1, 0, 0, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 0, 0, 0, 0);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_LE, 0, 1, 1, 0);
}

static void cpu_selftest_org_br_gt_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 1, 0, 0, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 1, 0, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 0, 1, 1, 0);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 0, 0, 0, 0);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_GT, 0, 0, 1, 0);
}

static void cpu_selftest_org_br_gt_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GT, 1, 1, 0, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_GT, 0, 1, 0, 0);
}

static void cpu_selftest_org_br_ovf_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_OVF, 0, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_OVF, 0, 0, 0, 0);
}

static void cpu_selftest_org_br_ovf_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_OVF, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_OVF, 1, 0, 0, 0);
}

static void cpu_selftest_org_br_bn_does_not_branch_on_false(void)
{
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_BN, 1, 1, 1, 0);
    cpu_selftest_org_branch_test_branch_not_taken(F_BRANCH_BN, 0, 0, 0, 0);
}
static void cpu_selftest_org_br_bn_does_branches_on_true(void)
{
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_BN, 1, 1, 1, 1);
    cpu_selftest_org_branch_test_branch_taken(F_BRANCH_BN, 0, 0, 0, 1);
}

static void cpu_selftest_no_b_overflow_interrupt_if_b_overflow_is_inhibited(void)
{
    cpu_selftest_set_register(REG_BOD, BOD_IBOVF_MASK);
    cpu_selftest_load_order_extended(CR_B, F_LOAD_DEC_B, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x80000000);
    cpu_selftest_run_code();
    cpu_selftest_assert_b_overflow();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_no_zero_divide_interrupt_if_zero_divide_is_inhibited(void)
{
    cpu_selftest_set_register(REG_AOD, AOD_IZDIV_MASK);
    cpu_selftest_load_order_extended(CR_XS, F_DIV_X, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000000);
    cpu_selftest_set_register(REG_X, -4 & 0xFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited(void)
{
    cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
    cpu_selftest_load_32_bit_literal(0x00000001);
    cpu_selftest_set_register(REG_DOD, DOD_BCHI_MASK);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
    cpu_selftest_run_code();
    cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_no_sss_interrupt_if_sss_is_inhibited(void)
{
    uint32 sourceOrigin = 8;
    uint32 destinationOrigin = 16;
    cpu_selftest_load_order(CR_STS1, F_SLGC, K_LITERAL, 0);
    cpu_selftest_set_register(REG_DOD, DOD_SSSI_MASK);
    cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, sourceOrigin));
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_ADDRESS_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, destinationOrigin));
    cpu_selftest_run_code();
    cpu_selftest_assert_no_sss_interrupt();
}

