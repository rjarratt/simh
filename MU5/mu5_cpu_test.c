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

#define CR_B 1
#define CR_STS1 2
#define CR_STS2 3
#define CR_FLOAT 7

#define F_LOAD_XDO 0
#define F_LOAD_XD 1
#define F_STACK 2
#define F_STORE_XD 3
#define F_LOAD_XDB 4
#define F_XCHK 5
#define F_SMOD 6
#define F_XMOD 7
#define F_SLGC 8

#define F_LOAD_64 1
#define F_STORE_64 3

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

#define DOD_ITS_MASK 0x00000002
#define DOD_BCH_MASK 0x00000020
#define DOD_BCHI_MASK 0x00000100

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
static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n);
static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 k, uint8 np);
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
static void cpu_selftest_run_code(void);
static REG *cpu_selftest_get_register(char *name);
static void cpu_selftest_set_register(char *name, t_uint64 value);

static void cpu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void cpu_selftest_assert_memory_contents_32_bit(t_addr address, uint32 expectedValue);
static void cpu_selftest_assert_memory_contents_64_bit(t_addr address, t_uint64 expectedValue);
static void cpu_selftest_assert_vector_content_64_bit(t_addr origin, uint32 offset, t_uint64 expectedValue);
static void cpu_selftest_assert_vector_content_32_bit(t_addr origin, uint32 offset, uint32 expectedValue);
static void cpu_selftest_assert_vector_content_16_bit(t_addr origin, uint32 offset, uint16 expectedValue);
static void cpu_selftest_assert_vector_content_8_bit(t_addr origin, uint32 offset, uint8 expectedValue);
static void cpu_selftest_assert_vector_content_4_bit(t_addr origin, uint32 offset, uint8 expectedValue);
static void cpu_selftest_assert_vector_content_1_bit(t_addr origin, uint32 offset, uint8 expectedValue);
static void cpu_selftest_assert_interrupt(void);
static void cpu_selftest_assert_no_interrupt(void);
static void cpu_selftest_assert_d_interrupt(char *name, uint32 mask);
static void cpu_selftest_assert_no_d_interrupt(char *name, uint32 mask);
static void cpu_selftest_assert_bound_check_interrupt(void);
static void cpu_selftest_assert_its_interrupt(void);
static void cpu_selftest_assert_no_bound_check_interrupt(void);
static void cpu_selftest_assert_no_its_interrupt(void);
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

static void cpu_selftest_load_operand_extended_literal_kp_0_generates_interrupt(void);
static void cpu_selftest_load_operand_extended_literal_kp_1_generates_interrupt(void);

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
static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value(void);
static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value(void);
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
static void cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out(void);
static void cpu_selftest_sts1_slgc_processes_source_0_dest_0(void);
static void cpu_selftest_sts1_slgc_processes_source_0_dest_1(void);
static void cpu_selftest_sts1_slgc_processes_source_1_dest_0(void);
static void cpu_selftest_sts1_slgc_processes_source_1_dest_1(void);

static void cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited(void);

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

	{ "Load operand to extended literal generates interrupt, k'=0", cpu_selftest_load_operand_extended_literal_kp_0_generates_interrupt },
	{ "Load operand to extended literal generates interrupt, k'=1", cpu_selftest_load_operand_extended_literal_kp_1_generates_interrupt },

	{ "Store operand 32-bit variable extended offset from stack", cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf },
	{ "Store operand 32-bit variable extended offset from zero", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_zero },
	{ "Store operand 32-bit variable extended offset from NB", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb },
	{ "Store operand 32-bit variable extended offset from XNB", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb },
	{ "Store operand 32-bit variable extended from stack", cpu_selftest_load_operand_extended_32_bit_variable_from_stack },
	{ "Store operand 32-bit variable extended from (NB)", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_nb_ref },
	{ "Store operand 32-bit variable extended from (XNB)", cpu_selftest_load_operand_extended_32_bit_variable_offset_from_xnb_ref },

	{ "Store operand 64-bit variable extended offset from stack", cpu_selftest_store_operand_extended_64_bit_variable_offset_from_sf },
	{ "Store operand 64-bit variable extended offset from zero", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_zero },
	{ "Store operand 64-bit variable extended offset from NB", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb },
	{ "Store operand 64-bit variable extended offset from XNB", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb },
	{ "Store operand 64-bit variable extended from stack", cpu_selftest_load_operand_extended_64_bit_variable_from_stack },
	{ "Store operand 64-bit variable extended from (NB)", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_nb_ref },
	{ "Store operand 64-bit variable extended from (XNB)", cpu_selftest_load_operand_extended_64_bit_variable_offset_from_xnb_ref },

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
	{ "XMOD scales modifier for 4-bit value", cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value },
	{ "XMOD scales modifier for 1-bit value", cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value },
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
	{ "SLGC generates SSS interrupt if source runs out", cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out },
	{ "SLGC processes source=0 and destination=0", cpu_selftest_sts1_slgc_processes_source_0_dest_0 },
	{ "SLGC processes source=0 and destination=1", cpu_selftest_sts1_slgc_processes_source_0_dest_1 },
	{ "SLGC processes source=1 and destination=0", cpu_selftest_sts1_slgc_processes_source_1_dest_0 },
	{ "SLGC processes source=1 and destination=1", cpu_selftest_sts1_slgc_processes_source_1_dest_1 },

	{ "No bounds check interrupt if bounds check is inhibited", cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited }
};

// TODO: test for illegal combinations, e.g. store to literal, V32 or V64 (k=2/3) with DR (n'=5).

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
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_LITERAL, 0x1F);
    cpu_selftest_run_code();
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_0_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 0xFFFFFFFFFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_reg_equals(REG_CO, 1);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_1_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 1);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XNB, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_2_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 2);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_NB, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_3_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 3);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_SN, 0);
    cpu_selftest_assert_reg_equals(REG_SF, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_4_generates_interrupt(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 4);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_MS, 0);
    cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_internal_register_16(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 16);
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_D, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_17(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 17);
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_18(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 18);
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DT, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_19(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 19);
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XDT, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_20(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 20);
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_DOD, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_32(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 32);
    cpu_selftest_set_register(REG_A, 0xAAAAAAABBBBBBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_BOD, 0xAAAAAAA);
    cpu_selftest_assert_reg_equals(REG_B, 0xBBBBBBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_33(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 33);
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_BOD, 0xABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_34(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 34);
    cpu_selftest_set_register(REG_A, 0x00000000ABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_internal_register_48(void)
{
    cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_IR, 48);
    cpu_selftest_set_register(REG_A, 0xABABABABABABABAB);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_AEX, 0xABABABABABABABAB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_32_bit_variable(void)
{
	uint32 base = 0x00F0;
	int8 n = 0x1;
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_V32, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_V64, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB_5, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_SB, n);
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
	cpu_selftest_load_order(CR_FLOAT, F_STORE_64, K_S0, n);
	cpu_selftest_set_register(REG_A, 0xAAAAAAAABBBBBBBB);
	cpu_selftest_set_register(REG_NB, base);
	sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
	cpu_selftest_run_code();
	cpu_selftest_assert_vector_content_64_bit(vecorigin, 0, 0xAAAAAAAABBBBBBBB);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_load_operand_extended_literal_kp_0_generates_interrupt(void)
{
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, KP_LITERAL, NP_64_BIT_LITERAL_6);
	cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
	cpu_selftest_run_code();
	cpu_selftest_assert_interrupt();
}

static void cpu_selftest_load_operand_extended_literal_kp_1_generates_interrupt(void)
{
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, KP_LITERAL_1, NP_64_BIT_LITERAL_6);
	cpu_selftest_load_64_bit_literal(0xAAAABBBBCCCCDDDD);
	cpu_selftest_run_code();
	cpu_selftest_assert_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_sf(void)
{
	uint32 base = 0x00F0;
	uint16 n = 0x1;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V32, NP_SF);
	cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_set_register(REG_SF, base);
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_32_bit(base + n, 0xAAAABBBB);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_zero(void)
{
	uint16 n = 0x4;
	cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V32, NP_0);
	cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_32_bit(n, 0xAAAABBBB);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_nb(void)
{
	uint32 base = 0x00F0;
	uint16 n = 0x1;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V32, NP_NB);
	cpu_selftest_load_16_bit_literal(n);
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
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V32, NP_XNB);
	cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_32_bit(base + n, 0xAAAABBBB);
	cpu_selftest_assert_no_interrupt();
}
static void cpu_selftest_store_operand_extended_32_bit_variable_from_stack(void)
{
	uint32 base = 0x00F0;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V32, NP_STACK);
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
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V32, NP_NB_REF);
	cpu_selftest_set_register(REG_A, 0xFFFFFFFFAAAABBBB);
	cpu_selftest_set_register(REG_NB, base);
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_32_bit(base, 0xAAAABBBB);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_32_bit_variable_offset_from_xnb_ref(void)
{
	uint32 base = 0x00F0;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V32, NP_XNB);
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
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V64, NP_SF);
	cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_set_register(REG_SF, base);
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAABBBBCCCCDDDD);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_zero(void)
{
	uint16 n = 0x4;
	cpu_selftest_load_order_extended(CR_FLOAT, F_LOAD_64, K_V64, NP_0);
	cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_64_bit(n * 2, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_nb(void)
{
	uint32 base = 0x00F0;
	uint16 n = 0x1;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V64, NP_NB);
	cpu_selftest_load_16_bit_literal(n);
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
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V64, NP_XNB);
	cpu_selftest_load_16_bit_literal(n);
	cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_64_bit(base + (n * 2), 0xAAAABBBBCCCCDDDD);
	cpu_selftest_assert_no_interrupt();
}
static void cpu_selftest_store_operand_extended_64_bit_variable_from_stack(void)
{
	uint32 base = 0x00F0;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V64, NP_STACK);
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
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V64, NP_NB_REF);
	cpu_selftest_set_register(REG_A, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_set_register(REG_NB, base);
	cpu_selftest_run_code();
	cpu_selftest_assert_memory_contents_64_bit(base, 0xAAAABBBBCCCCDDDD);
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_64_bit_variable_offset_from_xnb_ref(void)
{
	uint32 base = 0x00F0;
	cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_V64, NP_XNB);
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB_5, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_STACK);
    cpu_selftest_set_register(REG_SF, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_DR);
    cpu_selftest_set_register(REG_B, vecoffset);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB_REF);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_XNB_REF);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_SB, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    cpu_selftest_set_register(REG_B, vecoffset);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_SF);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_SF, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_0);
    cpu_selftest_load_16_bit_literal(n);
    sac_write_64_bit_word(2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_XNB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_stack(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_STACK);
    cpu_selftest_set_register(REG_SF, base);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_DR);
    cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_nb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB_REF);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
    cpu_selftest_set_register(REG_A, 0x00000000AAAABBBB);
    cpu_selftest_run_code();
    cpu_selftest_assert_vector_content_32_bit(vecorigin, 0, 0xAAAABBBB);
    cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_store_operand_extended_zero_relative_descriptor_32_bit_value_from_xnb_ref(void)
{
    uint32 base = 0x00F0;
    uint32 vecorigin = cpu_selftest_byte_address_from_word_address(0x0F00);
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_XNB_REF);
    cpu_selftest_set_register(REG_XNB, base); // TODO: upper half of XNB provides segment
    sac_write_64_bit_word(base, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_32_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_16_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, vecorigin));
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
    cpu_selftest_load_order_extended(CR_FLOAT, F_STORE_64, K_S0, NP_NB);
    cpu_selftest_load_16_bit_literal(n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2 * n, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 2, vecorigin));
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

static void cpu_selftest_sts1_xmod_scales_modifier_for_4_bit_value(void)
{
	cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
	cpu_selftest_load_32_bit_literal(0x00000001);

	cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 8));
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 1, 8));
	cpu_selftest_assert_no_interrupt();

	cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 3, 8));
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_4_BIT, 2, 9));
	cpu_selftest_assert_no_interrupt();
}

static void cpu_selftest_sts1_xmod_scales_modifier_for_1_bit_value(void)
{
	cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
	cpu_selftest_load_32_bit_literal(0x00000001);

	cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 8, 8));
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 7, 8));
	cpu_selftest_assert_no_interrupt();

	cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 9, 8));
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_1_BIT, 8, 9));
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
	cpu_selftest_load_order(CR_STS1, F_SLGC, K_LITERAL, 0);
	cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_64_BIT, 1, 8));
	cpu_selftest_set_register(REG_D, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_STRING, DESCRIPTOR_SIZE_8_BIT, 1, 16));
	cpu_selftest_run_code();
	cpu_selftest_assert_its_interrupt();
}

static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_not_8_bit(void){}
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_source_is_type_3(void){}
static void cpu_selftest_sts1_slgc_generates_its_interrupt_if_destination_is_type_3(void){}
static void cpu_selftest_sts1_slgc_processes_type_0_descriptors(void){}
static void cpu_selftest_sts1_slgc_processes_type_2_descriptors(void){}
static void cpu_selftest_sts1_slgc_generates_sss_interrupt_if_source_runs_out(void){}
static void cpu_selftest_sts1_slgc_processes_source_0_dest_0(void){}
static void cpu_selftest_sts1_slgc_processes_source_0_dest_1(void){}
static void cpu_selftest_sts1_slgc_processes_source_1_dest_0(void){}
static void cpu_selftest_sts1_slgc_processes_source_1_dest_1(void){}

static void cpu_selftest_no_bounds_check_interrupt_if_bounds_check_is_inhibited(void)
{
	cpu_selftest_load_order_extended(CR_STS1, F_XMOD, K_LITERAL, NP_32_BIT_SIGNED_LITERAL);
	cpu_selftest_load_32_bit_literal(0x00000001);
	cpu_selftest_set_register(REG_DOD, DOD_BCHI_MASK);
	cpu_selftest_set_register(REG_XD, cpu_selftest_create_descriptor(DESCRIPTOR_TYPE_GENERAL_VECTOR, DESCRIPTOR_SIZE_8_BIT, 1, 8));
	cpu_selftest_run_code();
	cpu_selftest_assert_no_bound_check_interrupt();
}

static void cpu_selftest_reset(UNITTEST *test)
{
    cpu_reset_state();

    testContext.testName = test->name;
    testContext.currentLoadLocation = 0;
    testContext.result = SCPE_OK;
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

static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 k, uint8 np)
{
    uint16 order;

    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= 0x7 << 6;
    order |= (k & 0x7) << 3;
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

static void cpu_selftest_run_code(void)
{
    cpu_selftest_set_register("CO", 0);
    cpu_execute_next_order();
}

static REG *cpu_selftest_get_register(char *name)
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

static void cpu_selftest_set_register(char *name, t_uint64 value)
{
    REG *reg = cpu_selftest_get_register(name);
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
    REG *reg = cpu_selftest_get_register(name);
    t_uint64 actualValue;
    t_uint64 mask;
    switch (reg->width)
    {
        case 16:
        {
            actualValue = *(uint16 *)(reg->loc);
            mask = 0xFFFF;
            break;
        }
        case 32:
        {
            actualValue = *(uint32 *)(reg->loc);
            mask = 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            actualValue = *(t_uint64 *)(reg->loc);
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

    if (testContext.result == SCPE_OK)
    {
        if ((mask & actualValue) != (mask & expectedValue))
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value in register %s to be %llX, but was %llX\n", name, mask & expectedValue, mask & actualValue);
            testContext.result = SCPE_AFAIL;
        }
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

	REG *reg_dod = cpu_selftest_get_register(REG_DOD);
	uint32 dod = *(uint32 *)reg_dod->loc;
	if (!(dod & mask))
	{
		sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected %s bit to be set in DOD\n", name);
		testContext.result = SCPE_AFAIL;
	}
}

static void cpu_selftest_assert_no_d_interrupt(char *name, uint32 mask)
{
	cpu_selftest_assert_no_interrupt();

	REG *reg_dod = cpu_selftest_get_register(REG_DOD);
	uint32 dod = *(uint32 *)reg_dod->loc;
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

static void cpu_selftest_assert_no_bound_check_interrupt(void)
{
	cpu_selftest_assert_no_d_interrupt("BCH", DOD_BCH_MASK);
}

static void cpu_selftest_assert_no_its_interrupt(void)
{
	cpu_selftest_assert_no_d_interrupt("ITS", DOD_ITS_MASK);
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
		sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "ALL TESTS PASSED\n");
	}
	else
	{
		sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%d of %d TESTS PASSED, %d FAILED\n", countSuccessful, n, countFailed);
	}

    return result;
}