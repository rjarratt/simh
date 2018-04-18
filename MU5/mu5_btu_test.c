/* mu5_btu_test.c: MU5 Block Transfer Unit self tests

Copyright (c) 2016-2018, Robert Jarratt

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
#include "mu5_exch.h"
#include "mu5_btu.h"
#include "mu5_test.h"
#include "mu5_btu_test.h"

#define REG_SOURCEADDR "SOURCEADDR"
#define REG_DESTINATIONADDR "DESTINATIONADDR"
#define REG_SIZE "SIZE"
#define REG_TRANSFERSTATUS "TRANSFERSTATUS"
#define REG_BTURIPF "BTURIPF"
#define REG_TRANSFERCOMPLETE "TRANSFERCOMPLETE"

#define TEST_UNIT_NUM 0

static TESTCONTEXT *localTestContext;
extern DEVICE btu_dev;

void btu_selftest(TESTCONTEXT *testContext);
static void btu_selftest_reset(UNITTEST *test);

static void btu_selftest_execute_cycle(void);
static void btu_selftest_execute_cycle_unit(int unitNum);

static void btu_selftest_set_register_instance(char *name, uint8 index, t_uint64 value);
static void btu_selftest_setup_vx_line(t_addr line, t_uint64 value);
static void btu_selftest_setup_request(t_uint64 disc_address);

static void btu_selftest_set_failure(void);
static void btu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void btu_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void btu_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue);
static void btu_selftest_assert_vx_line_contents(t_addr line, t_uint64 expectedValue);

static void btu_selftest_write_to_source_address(TESTCONTEXT *testContext);
static void btu_selftest_read_from_source_address(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Can write to the source address Vx line", btu_selftest_write_to_source_address },
    { "Can read from the source address Vx line", btu_selftest_read_from_source_address },
};

void btu_selftest(TESTCONTEXT *testContext)
{
    int n;

    n = sizeof(tests) / sizeof(UNITTEST);

    localTestContext = testContext;
    localTestContext->dev = &btu_dev;
    mu5_selftest_run_suite(testContext, tests, n, btu_selftest_reset);
}

static void btu_selftest_reset(UNITTEST *test)
{
    sac_reset_state(); /* reset memory that gets set by transfers, including the complete address write. Reset first as it clears the V-store */
    cpu_reset_state(); /* reset interrupts */
    btu_reset_state();
}

static void btu_selftest_execute_cycle(void)
{
    btu_selftest_execute_cycle_unit(TEST_UNIT_NUM);
}

static void btu_selftest_execute_cycle_unit(int unitNum)
{
    UNIT *unit = &btu_dev.units[unitNum];
    unit->action(unit);
}

static void btu_selftest_set_register_instance(char *name, uint8 index, t_uint64 value)
{
    mu5_selftest_set_register_instance(localTestContext, &btu_dev, name, index, value);
}

static void btu_selftest_setup_vx_line(t_addr line, t_uint64 value)
{
    exch_write(RA_VX_BTU(line), value);
}

static void btu_selftest_setup_request(t_uint64 disc_address)
{
    ////btu_selftest_setup_vx_line(DRUM_VX_STORE_STORE_ADDRESS, TEST_STORE_ADDRESS);
    ////btu_selftest_setup_vx_line(DRUM_VX_STORE_COMPLETE_ADDRESS, TEST_COMPLETE_ADDRESS);
    ////btu_selftest_setup_vx_line(DRUM_VX_STORE_DISC_ADDRESS, disc_address);
}

static void btu_selftest_set_failure(void)
{
    mu5_selftest_set_failure(localTestContext);
}

static void btu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_equals(localTestContext, &btu_dev, name, expectedValue);
}

static void btu_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_instance_equals(localTestContext, &btu_dev, name, index, expectedValue);
}

static void btu_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_equals_mask(localTestContext, &btu_dev, name, mask, expectedValue);
}

static void btu_selftest_assert_vx_line_contents(t_addr line, t_uint64 expectedValue)
{
    t_uint64 actualValue = exch_read(RA_VX_BTU(line));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &btu_dev, "Expected value at Vx line %hu to be %016llX, but was %016llX\n", line, expectedValue, actualValue);
        btu_selftest_set_failure();
    }
}

static void btu_selftest_write_to_source_address(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_SOURCE_ADDRESS(TEST_UNIT_NUM), 0xFFFFFFFFA5A5A5A5);
    btu_selftest_assert_reg_instance_equals(REG_SOURCEADDR, TEST_UNIT_NUM, 0x8025A525);
}

static void btu_selftest_read_from_source_address(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_SOURCEADDR, TEST_UNIT_NUM, 0xA5A5A5A5);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_SOURCE_ADDRESS(TEST_UNIT_NUM), 0xA5A5A5A5);
}