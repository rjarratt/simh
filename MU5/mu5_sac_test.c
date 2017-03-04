/* mu5_sac_test.c: MU5 SAC self tests

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
#include "mu5_sac.h"
#include "mu5_test.h"
#include "mu5_sac_test.h"

#define REG_CPR "CPR"

#define REG_MS "MS"

static TESTCONTEXT *localTestContext;
extern DEVICE cpu_dev;
extern DEVICE sac_dev;

void sac_selftest(TESTCONTEXT *testContext);
static void sac_selftest_reset(UNITTEST *test);

static void sac_selftest_set_register_instance(char *name, uint8 index, t_uint64 value);
static void sac_selftest_set_bcpr();
static void sac_selftest_clear_bcpr();

static void sac_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void sac_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void sac_selftest_assert_vstore_contents(uint8 block, uint8 line, t_uint64 expectedValue);
static void sac_selftest_assert_real_address_memory_contents(t_addr address, uint32 expectedValue);
static void sac_selftest_assert_memory_contents(t_addr address, uint32 expectedValue);
static void sac_selftest_assert_operand_access_violation(void);
static void sac_selftest_assert_instruction_access_violation(void);

static void sac_selftest_write_word_with_bcpr_set_writes_real_address(TESTCONTEXT *testContext);
static void sac_selftest_read_word_with_bcpr_set_reads_real_address(TESTCONTEXT *testContext);
static void sac_selftest_write_word_with_bcpr_clear_writes_virtual_address(TESTCONTEXT *testContext);
static void sac_selftest_read_word_with_bcpr_clear_reads_virtual_address(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_uses_PN_if_segment_less_than_8192(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_ignores_PN_if_segment_is_8192_or_greater(TESTCONTEXT *testContext);
static void sac_selftest_virtual_read_updates_cpr_referenced_bit(TESTCONTEXT *testContext);
static void sac_selftest_virtual_read_for_obey_updates_cpr_referenced_bit(TESTCONTEXT *testContext);
static void sac_selftest_virtual_write_updates_cpr_altered_bit(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_of_smallest_page_size(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_of_largest_page_size(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_of_mixed_page_size(TESTCONTEXT *testContext);
static void sac_selftest_cpr_not_equivalence_generates_not_equivalence_interrupt(TESTCONTEXT *testContext);
static void sac_selftest_cpr_not_equivalence_sets_cpr_not_equivalence_v_lines(TESTCONTEXT *testContext);
static void sac_selftest_cpr_multiple_equivalence_generates_system_error_interrupt(TESTCONTEXT *testContext);
static void sac_selftest_write_to_obey_only_page_generates_access_violation(TESTCONTEXT *testContext);
static void sac_selftest_write_to_read_only_page_generates_access_violation(TESTCONTEXT *testContext);
static void sac_selftest_read_from_obey_only_page_generates_access_violation(TESTCONTEXT *testContext);
static void sac_selftest_obey_from_read_only_page_generates_instruction_access_violation(TESTCONTEXT *testContext);
static void sac_selftest_obey_from_write_only_page_generates_instruction_access_violation(TESTCONTEXT *testContext);
static void sac_selftest_read_from_write_only_page_is_permitted(TESTCONTEXT *testContext);
static void sac_selftest_obey_from_obey_only_page_is_permitted(TESTCONTEXT *testContext);
static void sac_selftest_read_from_read_only_page_is_permitted(TESTCONTEXT *testContext);
static void sac_selftest_write_to_write_only_page_is_permitted(TESTCONTEXT *testContext);
static void sac_selftest_executive_mode_access_to_executive_mode_page_is_permitted(TESTCONTEXT *testContext);
static void sac_selftest_executive_mode_access_to_user_mode_page_is_permitted(TESTCONTEXT *testContext);
static void sac_selftest_user_mode_access_to_executive_mode_page_generates_access_violation(TESTCONTEXT *testContext);

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext);
static void sac_selftest_writing_read_only_vstore_line_does_nothing(TESTCONTEXT *testContext);
static void sac_selftest_read_write_vstore_location_can_be_read_back_after_write(TESTCONTEXT *testContext);
static void sac_selftest_can_write_real_address_to_cpr(TESTCONTEXT *testContext);
static void sac_selftest_can_read_real_address_from_cpr(TESTCONTEXT *testContext);
static void sac_selftest_can_write_virtual_address_to_cpr(TESTCONTEXT *testContext);
static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_ignore_bit(TESTCONTEXT *testContext);
static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_find_bit(TESTCONTEXT *testContext);
static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_referenced_bit(TESTCONTEXT *testContext);
static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_altered_bit(TESTCONTEXT *testContext);
static void sac_selftest_can_read_virtual_address_from_cpr(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_using_P_and_X(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_ignoring_P(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_ignoring_X(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_ignoring_P_and_X(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_masking_selected_S_bits(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_ignores_empty_cprs(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_does_not_update_cpr_referenced_bits(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_does_not_update_cpr_altered_bits(TESTCONTEXT *testContext);
static void sac_selftest_writing_any_value_to_cpr_find_resets_it_to_zero(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_updates_find_result(TESTCONTEXT *testContext);
static void sac_selftest_write_to_access_violation_resets_it_to_zero(TESTCONTEXT *testContext);
static void sac_selftest_write_to_system_error_interrupts_resets_it_to_zero(TESTCONTEXT *testContext); /* TODO: other items are also reset, see p93 */
static void sac_selftest_write_to_cpr_not_equivalence_psx_not_equivalance_lines_to_zero(TESTCONTEXT *testContext);

static void sac_selftest_write_v_real_address_writes_to_system_v_store(TESTCONTEXT *testContext);
static void sac_selftest_read_v_real_address_reads_system_v_store(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Writing a word with Bypass CPR set writes to a real address", sac_selftest_write_word_with_bcpr_set_writes_real_address },
    { "Reading a word with Bypass CPR set reads from a real address", sac_selftest_read_word_with_bcpr_set_reads_real_address },
    { "Writing a word with Bypass CPR clear writes to a virtual address", sac_selftest_write_word_with_bcpr_clear_writes_virtual_address },
    { "Reading a word with Bypass CPR clear reads from a virtual address", sac_selftest_read_word_with_bcpr_clear_reads_virtual_address },
    { "Virtual access uses PN if segment less than 8192", sac_selftest_virtual_access_uses_PN_if_segment_less_than_8192 },
    { "Virtual access ignores PN if segment greater than or equal to 8192", sac_selftest_virtual_access_ignores_PN_if_segment_is_8192_or_greater },
    { "Virtual read updates the CPR Referenced bit", sac_selftest_virtual_read_updates_cpr_referenced_bit },
    { "Virtual read for obey updates the CPR Referenced bit", sac_selftest_virtual_read_for_obey_updates_cpr_referenced_bit },
    { "Virtual write updates the CPR Altered bit", sac_selftest_virtual_read_updates_cpr_referenced_bit },
    { "Virtual access to smallest page size", sac_selftest_virtual_access_of_smallest_page_size },
    { "Virtual access to largest page size", sac_selftest_virtual_access_of_largest_page_size },
    { "Virtual access to mixed page size", sac_selftest_virtual_access_of_mixed_page_size },
    { "CPR not-equivalence generates a not-equivalence interrupt", sac_selftest_cpr_not_equivalence_generates_not_equivalence_interrupt },
    { "CPR not-equivalence sets not-equivalence V lines", sac_selftest_cpr_not_equivalence_sets_cpr_not_equivalence_v_lines },
    { "CPR multiple-equivalence error generates a system error interrupt", sac_selftest_cpr_multiple_equivalence_generates_system_error_interrupt },
    { "Write to Obey only page generates access violation", sac_selftest_write_to_obey_only_page_generates_access_violation },
    { "Write to Read only page generates access violation", sac_selftest_write_to_read_only_page_generates_access_violation },
    { "Read from Obey only page generates access violation", sac_selftest_read_from_obey_only_page_generates_access_violation },
    { "Obey from Read only page generates access violation", sac_selftest_obey_from_read_only_page_generates_instruction_access_violation },
    { "Obey from Write only page generates access violation", sac_selftest_obey_from_write_only_page_generates_instruction_access_violation },
    { "Read from Write only page is permitted", sac_selftest_read_from_write_only_page_is_permitted },
    { "Obey from Obey only page is permitted", sac_selftest_obey_from_obey_only_page_is_permitted },
    { "Read from Read only page is permitted", sac_selftest_read_from_read_only_page_is_permitted },
    { "Write to Write only page is permitted", sac_selftest_write_to_write_only_page_is_permitted },
    { "Executive mode access to a executive mode page is permitted", sac_selftest_executive_mode_access_to_executive_mode_page_is_permitted },
    { "Executive mode access to a user mode page is permitted",  sac_selftest_executive_mode_access_to_user_mode_page_is_permitted },
    { "User mode access to an executive mode page generates access violation",  sac_selftest_user_mode_access_to_executive_mode_page_generates_access_violation },

    { "Reading a write-only V-Store line returns zeroes", sac_selftest_reading_write_only_vstore_line_returns_zeroes },
    { "Writing a read-only V-Store line does nothing", sac_selftest_writing_read_only_vstore_line_does_nothing },
    { "A read/write V-Store line can be read back after writing", sac_selftest_read_write_vstore_location_can_be_read_back_after_write },
    { "Can write a real address to a CPR", sac_selftest_can_write_real_address_to_cpr },
    { "Can read a real address from a CPR", sac_selftest_can_read_real_address_from_cpr },
    { "Can write a virtual address to a CPR", sac_selftest_can_write_virtual_address_to_cpr },
    { "Writing a virtual address to a CPR clears the associated ignore bit", sac_selftest_writing_virtual_address_to_cpr_clears_associated_ignore_bit },
    { "Writing a virtual address to a CPR clears the associated find bit", sac_selftest_writing_virtual_address_to_cpr_clears_associated_find_bit },
    { "Writing a virtual address to a CPR clears the associated referenced bit", sac_selftest_writing_virtual_address_to_cpr_clears_associated_referenced_bit },
    { "Writing a virtual address to a CPR clears the associated altered bit", sac_selftest_writing_virtual_address_to_cpr_clears_associated_altered_bit },
    { "Can read a virtual address from a CPR", sac_selftest_can_read_virtual_address_from_cpr },
    { "CPR SEARCH finds all matches using P and X", sac_selftest_search_cpr_finds_matches_using_P_and_X },
    { "CPR SEARCH finds all matches ignoring P", sac_selftest_search_cpr_finds_matches_ignoring_P },
    { "CPR SEARCH finds all matches ignoring X", sac_selftest_search_cpr_finds_matches_ignoring_X },
    { "CPR SEARCH finds all matches ignoring P and X", sac_selftest_search_cpr_finds_matches_ignoring_P_and_X },
    { "CPR SEARCH finds all matches ignoring selected S bits from CPR FIND MASK ", sac_selftest_search_cpr_finds_matches_masking_selected_S_bits },
    { "CPR SEARCH finds all matches while ignoring any empty CPRs", sac_selftest_search_cpr_ignores_empty_cprs },
    { "CPR SEARCH does not update CPR REFERENCED bits", sac_selftest_search_cpr_does_not_update_cpr_referenced_bits },
    { "CPR SEARCH does not update CPR ALTERED bits", sac_selftest_search_cpr_does_not_update_cpr_altered_bits },
    { "Writing any value to CPR FIND resets it to zero", sac_selftest_writing_any_value_to_cpr_find_resets_it_to_zero },
    { "CPR SEARCH updates the result in CPR FIND", sac_selftest_search_cpr_updates_find_result },
    { "Write to ACCESS VIOLATION resets it to zero", sac_selftest_write_to_access_violation_resets_it_to_zero },
    { "Write to SYSTEM ERROR INTERRUPT resets it to zero", sac_selftest_write_to_system_error_interrupts_resets_it_to_zero },
    { "Write to CPR NOT EQUIVALENCE PSX resets not-equivalence lines to zero", sac_selftest_write_to_cpr_not_equivalence_psx_not_equivalance_lines_to_zero },

    { "Write to a V real address writes to the System V-Store", sac_selftest_write_v_real_address_writes_to_system_v_store },
    { "Read from a V real address reads from the System V-Store", sac_selftest_read_v_real_address_reads_system_v_store }

};

void sac_selftest(TESTCONTEXT *testContext)
{
    int n;

    n = sizeof(tests) / sizeof(UNITTEST);

    localTestContext = testContext;
    localTestContext->dev = &sac_dev;
    mu5_selftest_run_suite(testContext, tests, n, sac_selftest_reset);
}

static void sac_selftest_reset(UNITTEST *test)
{
    sac_reset_state(); /* reset SAC first because it clears the V-Store callbacks which may be set by other devices */
    cpu_reset_state();
    VStoreTestLocation = 0;
}

static void sac_selftest_set_register_instance(char *name, uint8 index, t_uint64 value)
{
    mu5_selftest_set_register_instance(localTestContext, &sac_dev, name, index, value);
}

static void sac_selftest_set_bcpr()
{
    mu5_selftest_set_bcpr(localTestContext, &cpu_dev);
}

static void sac_selftest_clear_bcpr()
{
    mu5_selftest_clear_bcpr(localTestContext, &cpu_dev);
}

static void sac_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_equals(localTestContext, &sac_dev, name, expectedValue);
}

static void sac_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_instance_equals(localTestContext, &sac_dev, name, index, expectedValue);
}

static void sac_selftest_assert_vstore_contents(uint8 block, uint8 line, t_uint64 expectedValue)
{
    mu5_selftest_assert_vstore_contents(localTestContext, block, line, expectedValue);
}

static void sac_selftest_assert_real_address_memory_contents(t_addr address, uint32 expectedValue)
{
    uint32 actualValue = sac_read_32_bit_word_real_address(address);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, localTestContext->dev, "Expected value at real address 0x%X to be %X, but was %X\n", address, expectedValue, actualValue);
        mu5_selftest_set_failure(localTestContext);
    }
}

static void  sac_selftest_assert_memory_contents(t_addr address, uint32 expectedValue)
{
    uint32 actualValue = sac_read_32_bit_word(address);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, localTestContext->dev, "Expected value at address 0x%X to be %X, but was %X\n", address, expectedValue, actualValue);
        mu5_selftest_set_failure(localTestContext);
    }
}

static void sac_selftest_assert_operand_access_violation(void)
{
    mu5_selftest_assert_operand_access_violation(localTestContext);
}

static void sac_selftest_assert_instruction_access_violation(void)
{
    mu5_selftest_assert_instruction_access_violation(localTestContext);
}

static void sac_selftest_write_word_with_bcpr_set_writes_real_address(TESTCONTEXT *testContext)
{
    sac_selftest_set_bcpr();
    sac_write_32_bit_word(1234, 0xFFFFFFFF);
    sac_selftest_assert_real_address_memory_contents(1234, 0xFFFFFFFF);
}

static void sac_selftest_read_word_with_bcpr_set_reads_real_address(TESTCONTEXT *testContext)
{
    sac_selftest_set_bcpr();
    sac_write_32_bit_word_real_address(1234, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(1234, 0xAAAAAAAA);
}

static void sac_selftest_write_word_with_bcpr_clear_writes_virtual_address(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_32_bit_word(1, 0xFFFFFFFF);
    sac_selftest_assert_real_address_memory_contents(0x11, 0xFFFFFFFF);
}

static void sac_selftest_read_word_with_bcpr_clear_reads_virtual_address(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_32_bit_word_real_address(0x11, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(1, 0xAAAAAAAA);
}

static void sac_selftest_virtual_access_uses_PN_if_segment_less_than_8192(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word_real_address(0x11, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(1, 0xAAAAAAAA);
}

static void sac_selftest_virtual_access_ignores_PN_if_segment_is_8192_or_greater(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0, 0x2000, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word_real_address(0x11, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(0x20000001, 0xAAAAAAAA);
}

static void sac_selftest_virtual_read_updates_cpr_referenced_bit(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0x0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    mu5_selftest_setup_cpr(1, VA(0xF, 0x2000, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0x00000001);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0x00000000);
}

static void sac_selftest_virtual_read_for_obey_updates_cpr_referenced_bit(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0x0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    mu5_selftest_setup_cpr(1, VA(0xF, 0x2000, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word_for_obey(1);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0x00000001);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0x00000000);
}

static void sac_selftest_virtual_write_updates_cpr_altered_bit(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0, 0x0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    mu5_selftest_setup_cpr(1, VA(1, 0x2000, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word(1, 0);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0x00000001);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0x00000000);
}

static void sac_selftest_virtual_access_of_smallest_page_size(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x00, 0));
    mu5_selftest_setup_cpr(1, VA(0xF, 0, 1), RA(SAC_ALL_ACCESS, 0x40, 0));
    mu5_selftest_setup_cpr(2, VA(0xF, 0, 2), RA(SAC_ALL_ACCESS, 0x80, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word_real_address(0x4F, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(0x1F, 0xAAAAAAAA);
}

static void sac_selftest_virtual_access_of_largest_page_size(TESTCONTEXT *testContext)
{
    /* with only 32K 32-bit words of local store the segments have to be made to have the segments overlap in the local
       store for this test to work. Furthermore the local store is not big enough to house a full 64K-word segment, so the highest address we can use is 32K */
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x20000, 0xC));
    mu5_selftest_setup_cpr(1, VA(0xF, 1, 0), RA(SAC_ALL_ACCESS, 0x00000, 0xC));
    mu5_selftest_setup_cpr(2, VA(0xF, 2, 0), RA(SAC_ALL_ACCESS, 0x40000, 0xC));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word_real_address(0x7FFF, 0xAAAAAAAA); /* 7FFF is not full size because of local store limit */
    sac_selftest_assert_memory_contents(0x17FFF, 0xAAAAAAAA);
}

static void sac_selftest_virtual_access_of_mixed_page_size(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x0000, 0xC));
    mu5_selftest_setup_cpr(1, VA(0xF, 1, 0), RA(SAC_ALL_ACCESS, 0x1000, 0x6));
    mu5_selftest_setup_cpr(2, VA(0xF, 1, 0x40), RA(SAC_ALL_ACCESS, 0x2000, 0x6));
    mu5_selftest_setup_cpr(3, VA(0xF, 2, 0), RA(SAC_ALL_ACCESS, 0x4000, 0xC));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word_real_address(0x23FF, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(0x107FF, 0xAAAAAAAA);
}

static void sac_selftest_cpr_not_equivalence_generates_not_equivalence_interrupt(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    sac_read_32_bit_word(0x10001);
    mu5_selftest_assert_interrupt_number(testContext, INT_CPR_NOT_EQUIVALENCE);
}

static void sac_selftest_cpr_not_equivalence_sets_cpr_not_equivalence_v_lines(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(0x1001A0A1);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_PSX, 0x3D001A0A);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_S, 0x1001);
}

static void sac_selftest_cpr_multiple_equivalence_generates_system_error_interrupt(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    mu5_selftest_setup_cpr(1, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    mu5_selftest_assert_interrupt_number(testContext, INT_SYSTEM_ERROR);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_SYSTEM_ERROR_INTERRUPTS, 0x40);
}

static void sac_selftest_write_to_obey_only_page_generates_access_violation(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_OBEY_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word(1, 0);
    sac_selftest_assert_operand_access_violation();
}

static void sac_selftest_write_to_read_only_page_generates_access_violation(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_READ_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word(1, 0);
    sac_selftest_assert_operand_access_violation();
}

static void sac_selftest_read_from_obey_only_page_generates_access_violation(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_OBEY_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    sac_selftest_assert_operand_access_violation();
}

static void sac_selftest_obey_from_read_only_page_generates_instruction_access_violation(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_READ_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word_for_obey(1);
    sac_selftest_assert_instruction_access_violation();
}

static void sac_selftest_obey_from_write_only_page_generates_instruction_access_violation(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_WRITE_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word_for_obey(1);
    sac_selftest_assert_instruction_access_violation();
}

static void sac_selftest_read_from_write_only_page_is_permitted(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_WRITE_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void sac_selftest_obey_from_obey_only_page_is_permitted(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_OBEY_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word_for_obey(1);
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void sac_selftest_read_from_read_only_page_is_permitted(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_READ_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void sac_selftest_write_to_write_only_page_is_permitted(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_WRITE_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word(1, 0);
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void sac_selftest_executive_mode_access_to_executive_mode_page_is_permitted(TESTCONTEXT *testContext)
{
    mu5_selftest_set_executive_mode(testContext, &cpu_dev);
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_WRITE_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word(1, 0);
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void sac_selftest_executive_mode_access_to_user_mode_page_is_permitted(TESTCONTEXT *testContext)
{
    mu5_selftest_set_executive_mode(testContext, &cpu_dev);
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_USER_ACCESS | SAC_WRITE_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_32_bit_word(1, 0);
    mu5_selftest_assert_no_interrupt(localTestContext);
}

static void sac_selftest_user_mode_access_to_executive_mode_page_generates_access_violation(TESTCONTEXT *testContext)
{
    mu5_selftest_set_user_mode(testContext, &cpu_dev);
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_READ_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    sac_selftest_assert_operand_access_violation();
}

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext)
{
    sac_setup_v_store_location(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, NULL, mu5_selftest_write_callback_for_static_64_bit_location);
    VStoreTestLocation = ~0;
    sac_selftest_assert_vstore_contents(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0);
}

static void sac_selftest_writing_read_only_vstore_line_does_nothing(TESTCONTEXT *testContext)
{
    sac_setup_v_store_location(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, mu5_selftest_read_callback_for_static_64_bit_location, NULL);
    sac_write_v_store(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, ~0);
    sac_selftest_assert_vstore_contents(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0);
    if (VStoreTestLocation != 0)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, testContext->dev, "Expected value in V-Store test backing location to be %llX, but was %llX\n", 0, VStoreTestLocation);
        mu5_selftest_set_failure(testContext);
    }
}

static void sac_selftest_read_write_vstore_location_can_be_read_back_after_write(TESTCONTEXT *testContext)
{
    sac_setup_v_store_location(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, mu5_selftest_read_callback_for_static_64_bit_location, mu5_selftest_write_callback_for_static_64_bit_location);
    sac_write_v_store(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, ~0);
    sac_selftest_assert_vstore_contents(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, ~0);
}

static void sac_selftest_can_write_real_address_to_cpr(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, 0xAAAAAAAAFFFFFFFF);
    sac_selftest_assert_reg_instance_equals(REG_CPR, 31, 0x000000007FFFFFFF);
}

static void sac_selftest_can_read_real_address_from_cpr(TESTCONTEXT *testContext)
{
    sac_selftest_set_register_instance(REG_CPR, 31, 0xFFFFFFFFFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, 0x000000007FFFFFFF);
}

static void sac_selftest_can_write_virtual_address_to_cpr(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, 0xAAAAAAAAFFFFFFFF);
    sac_selftest_assert_reg_instance_equals(REG_CPR, 31, 0x3FFFFFFF00000000);
}

static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_ignore_bit(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, 0xAAAAAAAAFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0x7FFFFFFF);
}

static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_find_bit(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 2));
    mu5_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x0);
}

static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_referenced_bit(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0xFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, 0xAAAAAAAAFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0x7FFFFFFF);
}

static void sac_selftest_writing_virtual_address_to_cpr_clears_associated_altered_bit(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0xFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, 0xAAAAAAAAFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0x7FFFFFFF);
}

static void sac_selftest_can_read_virtual_address_from_cpr(TESTCONTEXT *testContext)
{
    sac_selftest_set_register_instance(REG_CPR, 31, 0xFFFFFFFFFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, 0x000000003FFFFFFF);
}

static void sac_selftest_search_cpr_finds_matches_using_P_and_X(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(1, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(2, VA(1, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 2));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x3);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_P(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(1, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(2, VA(1, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4000000);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xF, 1, 2));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x7);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_X(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(1, 1, 2), 0);
    mu5_selftest_setup_cpr(1, VA(0, 1, 1), 0);
    mu5_selftest_setup_cpr(2, VA(0, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x6);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_P_and_X(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(1, 1, 2), 0);
    mu5_selftest_setup_cpr(1, VA(0, 1, 1), 0);
    mu5_selftest_setup_cpr(2, VA(0, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4000001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xFF, 1, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x7);
}

static void sac_selftest_search_cpr_finds_matches_masking_selected_S_bits(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(1, 0x3FF9, 2), 0);
    mu5_selftest_setup_cpr(1, VA(1, 0x3FF6, 2), 0);
    mu5_selftest_setup_cpr(2, VA(0, 0x3FFD, 1), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4006001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xFF, 0x3FFF, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x5);
}

static void sac_selftest_search_cpr_ignores_empty_cprs(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(1, 1, 1), 0);
    mu5_selftest_setup_cpr(1, VA(1, 1, 1), 0);
    mu5_selftest_setup_cpr(30, VA(1, 1, 1), 0);
    mu5_selftest_setup_cpr(31, VA(1, 1, 1), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0x80000001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(1, 1, 1));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x40000002);
}

static void sac_selftest_search_cpr_does_not_update_cpr_referenced_bits(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(1, 1, 1), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0xFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0xFFFFFFFE);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(1, 1, 1));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_REFERENCED, 0xFFFFFFFF);
}

static void sac_selftest_search_cpr_does_not_update_cpr_altered_bits(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(1, 1, 1), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0xFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0xFFFFFFFE);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(1, 1, 1));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_ALTERED, 0xFFFFFFFF);
}

static void sac_selftest_writing_any_value_to_cpr_find_resets_it_to_zero(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(1, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(2, VA(1, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 2));
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0xFFFFFFFFFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x0);
}

static void sac_selftest_search_cpr_updates_find_result(TESTCONTEXT *testContext)
{
    mu5_selftest_setup_cpr(0, VA(0, 1, 1), 0);
    mu5_selftest_setup_cpr(1, VA(0, 1, 2), 0);
    mu5_selftest_setup_cpr(2, VA(0, 1, 3), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 2));
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 3));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x6);
}

static void sac_selftest_write_to_access_violation_resets_it_to_zero(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_OBEY_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0xFFFFFFFFFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x0);
}

static void sac_selftest_write_to_system_error_interrupts_resets_it_to_zero(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    mu5_selftest_setup_cpr(1, VA(0xF, 0, 0), RA(SAC_ALL_ACCESS, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_read_32_bit_word(1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_SYSTEM_ERROR_INTERRUPTS, 0xFFFFFFFFFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_SYSTEM_ERROR_INTERRUPTS, 0x0);
}

static void sac_selftest_write_to_cpr_not_equivalence_psx_not_equivalance_lines_to_zero(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    sac_read_32_bit_word(1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_PSX, 0xFFFFFFFFFFFFFFFF);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_PSX, 0x0);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NOT_EQUIVALENCE_S, 0x0);
}

static void sac_selftest_write_v_real_address_writes_to_system_v_store(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0x0, 0, 0), RA(SAC_ALL_ACCESS,0x800000, 0xC));

    sac_write_64_bit_word(0x000, 0xABCDDCBA01233210);
    sac_write_64_bit_word(0x1FE, 0xBCDDCBA012332105);
    sac_selftest_assert_vstore_contents(SYSTEM_V_STORE_BLOCK, 0x00, 0xABCDDCBA01233210);
    sac_selftest_assert_vstore_contents(SYSTEM_V_STORE_BLOCK, 0xFF, 0xBCDDCBA012332105);
}

static void sac_selftest_read_v_real_address_reads_system_v_store(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    mu5_selftest_setup_cpr(0, VA(0x0, 0, 0), RA(SAC_ALL_ACCESS, 0x800000, 0xC));

    sac_write_v_store(SYSTEM_V_STORE_BLOCK, 0xFF, 0xABCDDCBA01233210);
    sac_write_v_store(SYSTEM_V_STORE_BLOCK, 0x00, 0xBCDDCBA012332105);
    sac_selftest_assert_memory_contents(0x1FE, 0xABCDDCBA);
    sac_selftest_assert_memory_contents(0x1FF, 0x01233210);
    sac_selftest_assert_memory_contents(0x000, 0xBCDDCBA0);
    sac_selftest_assert_memory_contents(0x001, 0x12332105);
}
