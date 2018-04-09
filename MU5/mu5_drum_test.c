/* mu5_drum_test.c: MU5 Drum self tests

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
#include "mu5_sac.h"
#include "mu5_exch.h"
#include "mu5_drum.h"
#include "mu5_test.h"
#include "mu5_drum_test.h"

#define REG_CURRENTPOSITIONS "CURRENTPOSITIONS"
#define REG_DISCADDRESS "DISCADDRESS"
#define REG_DISCSTATUS "DISCSTATUS"

static TESTCONTEXT *localTestContext;
extern DEVICE cpu_dev;
extern DEVICE drum_dev;

void drum_selftest(TESTCONTEXT *testContext);
static void drum_selftest_reset(UNITTEST *test);

static void drum_selftest_execute_cycle(void);
static void drum_selftest_execute_cycle_unit(int unitNum);

static void drum_selftest_set_failure(void);
static void drum_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void drum_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue);
static void drum_selftest_assert_vx_line_contents(uint8 line, t_uint64 expectedValue);
static void drum_selftest_assert_legal_request();
static void drum_selftest_assert_illegal_request();

static void drum_selftest_current_position_incremented_on_each_cycle(TESTCONTEXT *testContext);
static void drum_selftest_current_position_wraps_after_last_block(TESTCONTEXT *testContext);
static void drum_selftest_current_position_incremented_independently_by_each_unit(TESTCONTEXT *testContext);
static void drum_selftest_read_non_v_address_sets_illegal_request_bit(TESTCONTEXT *testContext);
static void drum_selftest_write_non_v_address_sets_illegal_request_bit(TESTCONTEXT *testContext);
static void drum_selftest_write_to_disc_address(TESTCONTEXT *testContext);
static void drum_selftest_read_from_disc_address(TESTCONTEXT *testContext);


static UNITTEST tests[] =
{
	{ "Each execution cycle of the drum unit advances the current position", drum_selftest_current_position_incremented_on_each_cycle },
	{ "The current position wraps after the last block", drum_selftest_current_position_wraps_after_last_block },
	{ "Each drum unit advances its current position independent of the others", drum_selftest_current_position_incremented_independently_by_each_unit },
	{ "Drum read of non-V address sets illegal request bit", drum_selftest_read_non_v_address_sets_illegal_request_bit },
	{ "Drum write of non-V address sets illegal request bit", drum_selftest_write_non_v_address_sets_illegal_request_bit },
    { "Can write to the disc address Vx line", drum_selftest_write_to_disc_address },
    { "Can read from the disc address Vx line", drum_selftest_read_from_disc_address }
};

void drum_selftest(TESTCONTEXT *testContext)
{
	int n;

	n = sizeof(tests) / sizeof(UNITTEST);

	localTestContext = testContext;
	localTestContext->dev = &drum_dev;
	mu5_selftest_run_suite(testContext, tests, n, drum_selftest_reset);
}

static void drum_selftest_reset(UNITTEST *test)
{
	drum_reset_state();
}

static void drum_selftest_execute_cycle(void)
{
	drum_selftest_execute_cycle_unit(0);
}

static void drum_selftest_execute_cycle_unit(int unitNum)
{
	UNIT *unit = &drum_dev.units[unitNum];
	unit->action(unit);
}

static void drum_selftest_set_failure(void)
{
    mu5_selftest_set_failure(localTestContext);
}

static void drum_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
	mu5_selftest_assert_reg_equals(localTestContext, &drum_dev, name, expectedValue);
}

static void drum_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue)
{
	mu5_selftest_assert_reg_equals_mask(localTestContext, &drum_dev, name, mask, expectedValue);
}

static void drum_selftest_assert_vx_line_contents(uint8 line, t_uint64 expectedValue)
{
    t_uint64 actualValue = exch_read(RA_VX_DRUM(line));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &drum_dev, "Expected value at Vx line %hu to be %016llX, but was %016llX\n", line, expectedValue, actualValue);
        drum_selftest_set_failure();
    }
}

static void drum_selftest_assert_legal_request()
{
	drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, DRUM_DISC_STATUS_ILLEGAL_REQUEST, 0);
}

static void drum_selftest_assert_illegal_request()
{
	drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, DRUM_DISC_STATUS_ILLEGAL_REQUEST, DRUM_DISC_STATUS_ILLEGAL_REQUEST);
}

static void drum_selftest_current_position_incremented_on_each_cycle(TESTCONTEXT *testContext)
{
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 0);
	drum_selftest_execute_cycle();
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 1);
	drum_selftest_execute_cycle();
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 2);
}

static void drum_selftest_current_position_wraps_after_last_block(TESTCONTEXT *testContext)
{
	int i;
	for (i = 0; i < DRUM_BLOCKS_PER_BAND; i++)
	{
		drum_selftest_execute_cycle();
	}
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 0);
}

static void drum_selftest_current_position_incremented_independently_by_each_unit(TESTCONTEXT *testContext)
{
	drum_selftest_execute_cycle_unit(1);
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 1u << 6);
	drum_selftest_execute_cycle_unit(2);
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, (1u << 12) | (1u << 6));
	drum_selftest_execute_cycle_unit(3);
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, (1u << 18) | (1u << 12) | (1u << 6));
}

static void drum_selftest_read_non_v_address_sets_illegal_request_bit(TESTCONTEXT *testContext)
{
	t_addr addr = RA_X(0);
	drum_exch_read(addr);
	drum_selftest_assert_illegal_request();
}

static void drum_selftest_write_non_v_address_sets_illegal_request_bit(TESTCONTEXT *testContext)
{
	t_addr addr = RA_X(0);
	drum_exch_write(addr, 1);
	drum_selftest_assert_illegal_request();
}

static void drum_selftest_write_to_disc_address(TESTCONTEXT *testContext)
{
	exch_write(RA_VX_DRUM(DRUM_VX_STORE_DISC_ADDRESS), 0xFFFFFFFFA5A5A5A5);
	drum_selftest_assert_reg_equals(REG_DISCADDRESS, 0x8025A525);
	drum_selftest_assert_legal_request();
}

static void drum_selftest_read_from_disc_address(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_DISCADDRESS, 0xA5A5A5A5);
    drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, 0x8025A525);
}
