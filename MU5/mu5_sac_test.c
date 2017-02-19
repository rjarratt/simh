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

#include "mu5_sac.h"
#include "mu5_test.h"
#include "mu5_sac_test.h"

#define REG_CPR "CPR"

#define VA(P,S,X) ((P <<26 ) | (S << 12) | X)

static TESTCONTEXT *localTestContext;
extern DEVICE sac_dev;

void sac_selftest(TESTCONTEXT *testContext);
static void sac_selftest_reset(UNITTEST *test);

static void sac_selftest_set_register_instance(char *name, uint8 index, t_uint64 value);

static void sac_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void sac_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void sac_selftest_assert_vstore_contents(uint8 block, uint8 line, t_uint64 expectedValue);

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
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 2);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(1, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 2));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x3);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_P(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 2);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(1, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4000000);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xF, 1, 2));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x7);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_X(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(1, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 1));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 2);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0, 1, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x6);
}

static void sac_selftest_search_cpr_finds_matches_ignoring_P_and_X(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(1, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 1));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 2);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 1, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4000001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xFF, 1, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x7);
}

static void sac_selftest_search_cpr_finds_matches_masking_selected_S_bits(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 0);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(1, 0x3FF9, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 1);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(1, 0x3FF6, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 2);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, VA(0, 0x3FFD, 2));

    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND_MASK, 0x4006001);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_SEARCH, VA(0xFF, 0x3FFF, 0xFFF));

    sac_selftest_assert_vstore_contents(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_FIND, 0x5);
}

static void sac_selftest_search_cpr_ignores_empty_cprs(TESTCONTEXT *testContext) {}
