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

#define CPR_REG "CPR"

static TESTCONTEXT *localTestContext;
extern DEVICE sac_dev;

void sac_selftest(TESTCONTEXT *testContext);
static void sac_selftest_reset(UNITTEST *test);

static void sac_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void sac_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void sac_selftest_assert_vstore_contents(TESTCONTEXT *testContext, uint8 block, uint8 line, t_uint64 expectedValue);

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext);
static void sac_selftest_writing_read_only_vstore_line_does_nothing(TESTCONTEXT *testContext);
static void sac_selftest_read_write_vstore_location_can_be_read_back_after_write(TESTCONTEXT *testContext);
static void sac_selftest_can_write_real_address_to_cpr(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Reading a write-only V-Store line returns zeroes", sac_selftest_reading_write_only_vstore_line_returns_zeroes },
    { "Writing a read-only V-Store line does nothing", sac_selftest_writing_read_only_vstore_line_does_nothing },
    { "A read/write V-Store line can be read back after writing", sac_selftest_read_write_vstore_location_can_be_read_back_after_write },
    { "Can write a real address to a CPR", sac_selftest_can_write_real_address_to_cpr }
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

static void sac_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_equals(localTestContext, &sac_dev, name, expectedValue);
}

static void sac_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_instance_equals(localTestContext, &sac_dev, name, index, expectedValue);
}

static void sac_selftest_assert_vstore_contents(TESTCONTEXT *testContext, uint8 block, uint8 line, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_v_store(block, line);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, testContext->dev, "Expected value in V-Store block %hu line %hu to be %llX, but was %llX\n", block, line, expectedValue, actualValue);
        mu5_selftest_set_failure(testContext);
    }
}

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext)
{
    sac_setup_v_store_location(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, NULL, mu5_selftest_write_callback_for_static_64_bit_location);
    VStoreTestLocation = ~0;
    sac_selftest_assert_vstore_contents(testContext, TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0);
}

static void sac_selftest_writing_read_only_vstore_line_does_nothing(TESTCONTEXT *testContext)
{
    sac_setup_v_store_location(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, mu5_selftest_read_callback_for_static_64_bit_location, NULL);
    sac_write_v_store(TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, ~0);
    sac_selftest_assert_vstore_contents(testContext, TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, 0);
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
    sac_selftest_assert_vstore_contents(testContext, TEST_V_STORE_LOCATION_BLOCK, TEST_V_STORE_LOCATION_LINE, ~0);
}

static void sac_selftest_can_write_real_address_to_cpr(TESTCONTEXT *testContext)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, 31);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, 0xFFFFFFFFFFFFFFFF);
    sac_selftest_assert_reg_instance_equals(CPR_REG, 31, 0x000000007FFFFFFF);
}
