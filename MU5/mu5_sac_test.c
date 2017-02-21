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

#define VA(P,S,X) ((P <<26 ) | (S << 12) | X)
#define RA(AC,A,LZ) ((AC << 28) | (A << 4) | LZ)

static TESTCONTEXT *localTestContext;
extern DEVICE cpu_dev;
extern DEVICE sac_dev;

void sac_selftest(TESTCONTEXT *testContext);
static void sac_selftest_reset(UNITTEST *test);

static void sac_selftest_set_register_instance(char *name, uint8 index, t_uint64 value);
static void sac_selftest_set_bcpr();
static void sac_selftest_clear_bcpr();
static void sac_selftest_setup_cpr(uint8 cprNumber, uint32 va, uint32 ra);

static void sac_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void sac_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void sac_selftest_assert_vstore_contents(uint8 block, uint8 line, t_uint64 expectedValue);
static void sac_selftest_assert_real_address_memory_contents(t_addr address, uint32 expectedValue);
static void  sac_selftest_assert_memory_contents(t_addr address, uint32 expectedValue);

static void sac_selftest_write_word_with_bcpr_set_writes_real_address(TESTCONTEXT *testContext);
static void sac_selftest_read_word_with_bcpr_set_reads_real_address(TESTCONTEXT *testContext);
static void sac_selftest_write_word_with_bcpr_clear_writes_virtual_address(TESTCONTEXT *testContext);
static void sac_selftest_read_word_with_bcpr_clear_reads_virtual_address(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_uses_PN_if_segment_less_than_8192(TESTCONTEXT *testContext);
static void sac_selftest_virtual_access_ignores_PN_if_segment_is_8192_or_greater(TESTCONTEXT *testContext);
// page sizes
// altered and referenced bits
// cpr neqv interrupt
// cpr multi eqv interrupt

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext);
static void sac_selftest_writing_read_only_vstore_line_does_nothing(TESTCONTEXT *testContext);
static void sac_selftest_read_write_vstore_location_can_be_read_back_after_write(TESTCONTEXT *testContext);
static void sac_selftest_can_write_real_address_to_cpr(TESTCONTEXT *testContext);
static void sac_selftest_can_read_real_address_from_cpr(TESTCONTEXT *testContext);
static void sac_selftest_can_write_virtual_address_to_cpr(TESTCONTEXT *testContext);
static void sac_selftest_can_read_virtual_address_from_cpr(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_using_P_and_X(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_ignoring_P(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_ignoring_X(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_ignoring_P_and_X(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_finds_matches_masking_selected_S_bits(TESTCONTEXT *testContext);
static void sac_selftest_search_cpr_ignores_empty_cprs(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Writing a word with Bypass CPR set writes to a real address", sac_selftest_write_word_with_bcpr_set_writes_real_address },
    { "Reading a word with Bypass CPR set reads from a real address", sac_selftest_read_word_with_bcpr_set_reads_real_address },
    { "Writing a word with Bypass CPR clear writes to a virtual address", sac_selftest_write_word_with_bcpr_clear_writes_virtual_address },
    { "Reading a word with Bypass CPR clear reads from a virtual address", sac_selftest_read_word_with_bcpr_clear_reads_virtual_address },
    { "Virtual access uses PN if segment less than 8192", sac_selftest_virtual_access_uses_PN_if_segment_less_than_8192 },
    { "Virtual access ignores PN if segment greater than or equal to 8192", sac_selftest_virtual_access_ignores_PN_if_segment_is_8192_or_greater },

    { "Reading a write-only V-Store line returns zeroes", sac_selftest_reading_write_only_vstore_line_returns_zeroes },
    { "Writing a read-only V-Store line does nothing", sac_selftest_writing_read_only_vstore_line_does_nothing },
    { "A read/write V-Store line can be read back after writing", sac_selftest_read_write_vstore_location_can_be_read_back_after_write },
    { "Can write a real address to a CPR", sac_selftest_can_write_real_address_to_cpr },
    { "Can read a real address from a CPR", sac_selftest_can_read_real_address_from_cpr },
    { "Can write a virtual address to a CPR", sac_selftest_can_write_virtual_address_to_cpr },
    { "Can read a virtual address from a CPR", sac_selftest_can_read_virtual_address_from_cpr },
    { "CPR SEARCH finds all matches using P and X", sac_selftest_search_cpr_finds_matches_using_P_and_X },
    { "CPR SEARCH finds all matches ignoring P", sac_selftest_search_cpr_finds_matches_ignoring_P },
    { "CPR SEARCH finds all matches ignoring X", sac_selftest_search_cpr_finds_matches_ignoring_X },
    { "CPR SEARCH finds all matches ignoring P and X", sac_selftest_search_cpr_finds_matches_ignoring_P_and_X },
    { "CPR SEARCH finds all matches ignoring selected S bits from CPR FIND MASK ", sac_selftest_search_cpr_finds_matches_masking_selected_S_bits },
    { "CPR SEARCH finds all matches while ignoring any empty CPRs", sac_selftest_search_cpr_ignores_empty_cprs }
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
    sac_reset_state();
    VStoreTestLocation = 0;
}

static void sac_selftest_set_register_instance(char *name, uint8 index, t_uint64 value)
{
    mu5_selftest_set_register_instance(localTestContext, &sac_dev, name, index, value);
}

static void sac_selftest_set_bcpr()
{
    mu5_selftest_set_register(localTestContext, &cpu_dev, REG_MS, MS_MASK_BCPR);
}

static void sac_selftest_clear_bcpr()
{
    mu5_selftest_set_register(localTestContext, &cpu_dev, REG_MS, 0);
}

static void sac_selftest_setup_cpr(uint8 cprNumber, uint32 va, uint32 ra)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, cprNumber);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, ra);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, va);
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
    t_uint64 actualValue = sac_read_v_store(block, line);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, localTestContext->dev, "Expected value in V-Store block %hu line %hu to be %llX, but was %llX\n", block, line, expectedValue, actualValue);
        mu5_selftest_set_failure(localTestContext);
    }
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
    sac_selftest_setup_cpr(0, VA(0, 0, 0), RA(0xF, 0x10, 0));
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0xFFFFFFFE);
    sac_write_32_bit_word(1, 0xFFFFFFFF);
    sac_selftest_assert_real_address_memory_contents(0x11, 0xFFFFFFFF);
}

static void sac_selftest_read_word_with_bcpr_clear_reads_virtual_address(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    sac_selftest_setup_cpr(0, VA(0, 0, 0), RA(0xF, 0x10, 0));
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0xFFFFFFFE);
    sac_write_32_bit_word_real_address(0x11, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(1, 0xAAAAAAAA);
}

static void sac_selftest_virtual_access_uses_PN_if_segment_less_than_8192(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    sac_selftest_setup_cpr(0, VA(0xF, 0, 0), RA(0xF, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0xFFFFFFFE);
    sac_write_32_bit_word_real_address(0x11, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(1, 0xAAAAAAAA);
}

static void sac_selftest_virtual_access_ignores_PN_if_segment_is_8192_or_greater(TESTCONTEXT *testContext)
{
    sac_selftest_clear_bcpr();
    sac_selftest_setup_cpr(0, VA(0, 0x2000, 0), RA(0xF, 0x10, 0));
    sac_write_v_store(PROP_V_STORE_BLOCK, PROP_V_STORE_PROCESS_NUMBER, 0xF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0xFFFFFFFE);
    sac_write_32_bit_word_real_address(0x11, 0xAAAAAAAA);
    sac_selftest_assert_memory_contents(0x20000001, 0xAAAAAAAA);
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

static void sac_selftest_can_read_virtual_address_from_cpr(TESTCONTEXT *testContext)
{
    sac_selftest_set_register_instance(REG_CPR, 31, 0xFFFFFFFFFFFFFFFF);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, 0x000000003FFFFFFF);
}

static void sac_selftest_search_cpr_finds_matches_using_P_and_X(TESTCONTEXT *testContext)
{
    sac_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    sac_selftest_setup_cpr(1, VA(0, 1, 2), 0);
    sac_selftest_setup_cpr(2, VA(1, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 2));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x3);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_P(TESTCONTEXT *testContext)
{
    sac_selftest_setup_cpr(0, VA(0, 1, 2), 0);
    sac_selftest_setup_cpr(1, VA(0, 1, 2), 0);
    sac_selftest_setup_cpr(2, VA(1, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4000000);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xF, 1, 2));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x7);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_X(TESTCONTEXT *testContext)
{
    sac_selftest_setup_cpr(0, VA(1, 1, 2), 0);
    sac_selftest_setup_cpr(1, VA(0, 1, 1), 0);
    sac_selftest_setup_cpr(2, VA(0, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x6);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_P_and_X(TESTCONTEXT *testContext)
{
    sac_selftest_setup_cpr(0, VA(1, 1, 2), 0);
    sac_selftest_setup_cpr(1, VA(0, 1, 1), 0);
    sac_selftest_setup_cpr(2, VA(0, 1, 2), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4000001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xFF, 1, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x7);
}

static void sac_selftest_search_cpr_finds_matches_masking_selected_S_bits(TESTCONTEXT *testContext)
{
    sac_selftest_setup_cpr(0, VA(1, 0x3FF9, 2), 0);
    sac_selftest_setup_cpr(1, VA(1, 0x3FF6, 2), 0);
    sac_selftest_setup_cpr(2, VA(0, 0x3FFD, 1), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4006001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xFF, 0x3FFF, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x5);
}

static void sac_selftest_search_cpr_ignores_empty_cprs(TESTCONTEXT *testContext)
{
    sac_selftest_setup_cpr(0, VA(1, 1, 1), 0);
    sac_selftest_setup_cpr(1, VA(1, 1, 1), 0);
    sac_selftest_setup_cpr(30, VA(1, 1, 1), 0);
    sac_selftest_setup_cpr(31, VA(1, 1, 1), 0);

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_IGNORE, 0x80000001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(1, 1, 1));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x40000002);
}
