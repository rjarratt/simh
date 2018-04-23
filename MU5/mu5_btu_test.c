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

#define TEST_UNIT_NUM 3

static TESTCONTEXT *localTestContext;
extern DEVICE btu_dev;

void btu_selftest(TESTCONTEXT *testContext);
static void btu_selftest_reset(UNITTEST *test);

static void btu_selftest_execute_cycle(void);
static void btu_selftest_execute_cycle_unit(int unitNum);

static void btu_selftest_set_register(char *name, t_uint64 value);
static void btu_selftest_set_register_instance(char *name, uint8 index, t_uint64 value);
static void btu_selftest_setup_vx_line(t_addr address, t_uint64 value);
static void btu_selftest_setup_request(t_addr from, t_addr to, uint16 size, uint8 unit_num);

static void btu_selftest_set_failure(void);
static void btu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void btu_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void btu_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue);
static void btu_selftest_assert_vx_line_contents(t_addr address, t_uint64 expectedValue);
static void btu_selftest_assert_unit_is_active(int unit_num);
static void btu_selftest_assert_unit_is_inactive(int unit_num);

static void btu_selftest_write_to_source_address(TESTCONTEXT *testContext);
static void btu_selftest_read_from_source_address(TESTCONTEXT *testContext);
static void btu_selftest_write_to_source_address_of_different_unit(TESTCONTEXT *testContext);
static void btu_selftest_read_from_source_address_of_different_unit(TESTCONTEXT *testContext);
static void btu_selftest_write_to_non_existent_vx_block_is_ignored(TESTCONTEXT *testContext);
static void btu_selftest_read_from_non_existent_vx_block_returns_zero(TESTCONTEXT *testContext);
static void btu_selftest_write_to_destination_address(TESTCONTEXT *testContext);
static void btu_selftest_read_from_destination_address(TESTCONTEXT *testContext);
static void btu_selftest_write_to_size(TESTCONTEXT *testContext);
static void btu_selftest_read_from_size(TESTCONTEXT *testContext);
static void btu_selftest_write_to_transfer_status(TESTCONTEXT *testContext);
static void btu_selftest_read_from_transfer_status(TESTCONTEXT *testContext);
static void btu_selftest_write_to_ripf(TESTCONTEXT *testContext);
static void btu_selftest_read_from_ripf(TESTCONTEXT *testContext);
static void btu_selftest_write_to_transfer_complete_is_ignored(TESTCONTEXT *testContext);
static void btu_selftest_read_from_transfer_complete(TESTCONTEXT *testContext);
static void btu_selftest_unit_is_initially_inactive(TESTCONTEXT *testContext);
static void btu_selftest_setting_transfer_in_progress_activates_unit(TESTCONTEXT *testContext);
static void btu_selftest_counts_down_size_during_transfer(TESTCONTEXT *testContext);
static void btu_selftest_keeps_unit_active_during_transfer(TESTCONTEXT *testContext);
static void btu_selftest_stops_counting_down_size_at_end_of_transfer(TESTCONTEXT *testContext);
static void btu_selftest_leaves_unit_inactive_at_end_of_transfer(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Can write to the source address Vx line", btu_selftest_write_to_source_address },
    { "Can read from the source address Vx line", btu_selftest_read_from_source_address },
    { "Can write to the source address Vx line of a different unit", btu_selftest_write_to_source_address_of_different_unit },
    { "Can read from the source address Vx line of a different unit", btu_selftest_read_from_source_address_of_different_unit },
    { "Write to a non-existent Vx block is ignored", btu_selftest_write_to_non_existent_vx_block_is_ignored },
    { "Read from a non-existent Vx block returns zero", btu_selftest_read_from_non_existent_vx_block_returns_zero },
    { "Can write to the destination address Vx line", btu_selftest_write_to_destination_address },
    { "Can read from the destination address Vx line", btu_selftest_read_from_destination_address },
    { "Can write to the size Vx line", btu_selftest_write_to_size },
    { "Can read from the size Vx line", btu_selftest_read_from_size },
    { "Can write to the transfer status Vx line", btu_selftest_write_to_transfer_status },
    { "Can read from the transfer status Vx line", btu_selftest_read_from_transfer_status },
    { "Can write to the ripf Vx line", btu_selftest_write_to_ripf },
    { "Can read from the ripf Vx line", btu_selftest_read_from_ripf },
    { "Write to the transfer complete Vx line is ignored", btu_selftest_write_to_transfer_complete_is_ignored },
    { "Can read from the transfer complete Vx line", btu_selftest_read_from_transfer_complete },
    { "Unit is initially inactive", btu_selftest_unit_is_initially_inactive },
    { "Setting the transfer in progress bit activates the unit", btu_selftest_setting_transfer_in_progress_activates_unit },
    { "Counts down the size during a transfer", btu_selftest_counts_down_size_during_transfer },
    { "Keeps the unit active during a transfer", btu_selftest_keeps_unit_active_during_transfer },
    { "Stops counting down the size at the end of a transfer", btu_selftest_stops_counting_down_size_at_end_of_transfer },
    { "Leaves unit inactive at the end of a transfer", btu_selftest_leaves_unit_inactive_at_end_of_transfer }
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

static void btu_selftest_set_register(char *name, t_uint64 value)
{
    mu5_selftest_set_register(localTestContext, &btu_dev, name, value);
}

static void btu_selftest_set_register_instance(char *name, uint8 index, t_uint64 value)
{
    mu5_selftest_set_register_instance(localTestContext, &btu_dev, name, index, value);
}

static void btu_selftest_setup_vx_line(t_addr address, t_uint64 value)
{
    exch_write(RA_VX_BTU(address), value);
}

static void btu_selftest_setup_request(t_addr from, t_addr to, uint16 size, uint8 unit_num)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_SOURCE_ADDRESS(unit_num), from);
    btu_selftest_setup_vx_line(BTU_VX_STORE_DESTINATION_ADDRESS(unit_num), to);
    btu_selftest_setup_vx_line(BTU_VX_STORE_SIZE(unit_num), ((uint32)unit_num) << 16 | size);
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_STATUS(unit_num), 0x8);
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

static void btu_selftest_assert_vx_line_contents(t_addr address, t_uint64 expectedValue)
{
    t_uint64 actualValue = exch_read(RA_VX_BTU(address));
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, &btu_dev, "Expected value at Vx line %hu to be %016llX, but was %016llX\n", address, expectedValue, actualValue);
        btu_selftest_set_failure();
    }
}

static void btu_selftest_assert_unit_is_active(int unit_num)
{
    UNIT *unit = &btu_dev.units[unit_num];
    if (!sim_is_active(unit))
    {
        sim_debug(LOG_SELFTEST_FAIL, &btu_dev, "Expected unit to be active but it was inactive\n");
        btu_selftest_set_failure();
    }
}

static void btu_selftest_assert_unit_is_inactive(int unit_num)
{
    UNIT *unit = &btu_dev.units[unit_num];
    if (sim_is_active(unit))
    {
        sim_debug(LOG_SELFTEST_FAIL, &btu_dev, "Expected unit to be inactive but it was active\n");
        btu_selftest_set_failure();
    }
}

static void btu_selftest_write_to_source_address(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_SOURCE_ADDRESS(TEST_UNIT_NUM), 0xFFFFFFFFA5A5A5A5);
    btu_selftest_assert_reg_instance_equals(REG_SOURCEADDR, TEST_UNIT_NUM, 0x05A5A5A5);
}

static void btu_selftest_read_from_source_address(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_SOURCEADDR, TEST_UNIT_NUM, 0xA5A5A5A5);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_SOURCE_ADDRESS(TEST_UNIT_NUM), 0x05A5A5A5);
}

static void btu_selftest_write_to_source_address_of_different_unit(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_SOURCE_ADDRESS(BTU_NUM_UNITS - 1), 0xFFFFFFFFA5A5A5A5);
    btu_selftest_assert_reg_instance_equals(REG_SOURCEADDR, BTU_NUM_UNITS - 1, 0x05A5A5A5);
}

static void btu_selftest_read_from_source_address_of_different_unit(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_SOURCEADDR, BTU_NUM_UNITS - 1, 0xA5A5A5A5);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_SOURCE_ADDRESS(BTU_NUM_UNITS - 1), 0x05A5A5A5);
}

static void btu_selftest_write_to_non_existent_vx_block_is_ignored(TESTCONTEXT *testContext)
{
    int i;
    for (i = 0; i < BTU_NUM_UNITS; i++)
    {
        btu_selftest_set_register_instance(REG_SOURCEADDR, i, 0xA5A5A5A5);
    }

    btu_selftest_setup_vx_line(BTU_VX_STORE_SOURCE_ADDRESS(BTU_NUM_UNITS + 2), 0xFFFFFFFFFFFFFFFF);

    for (i = 0; i < BTU_NUM_UNITS; i++)
    {
        btu_selftest_assert_reg_instance_equals(REG_SOURCEADDR, i, 0x05A5A5A5);
    }
}

static void btu_selftest_read_from_non_existent_vx_block_returns_zero(TESTCONTEXT *testContext)
{
    int i;
    for (i = 0; i < BTU_NUM_UNITS; i++)
    {
        btu_selftest_set_register_instance(REG_SOURCEADDR, i, 0xA5A5A5A5);
    }

    btu_selftest_setup_vx_line(BTU_VX_STORE_SOURCE_ADDRESS(BTU_NUM_UNITS + 2), 0xFFFFFFFFFFFFFFFF);

    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_SOURCE_ADDRESS(BTU_NUM_UNITS + 2), 0);
}

static void btu_selftest_write_to_destination_address(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_DESTINATION_ADDRESS(TEST_UNIT_NUM), 0xFFFFFFFFA5A5A5A5);
    btu_selftest_assert_reg_instance_equals(REG_DESTINATIONADDR, TEST_UNIT_NUM, 0x05A5A5A5);
}

static void btu_selftest_read_from_destination_address(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_DESTINATIONADDR, TEST_UNIT_NUM, 0xA5A5A5A5);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_DESTINATION_ADDRESS(TEST_UNIT_NUM), 0x05A5A5A5);
}

static void btu_selftest_write_to_size(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_SIZE(TEST_UNIT_NUM), 0xFFFFFFFFFFFFFFFF);
    btu_selftest_assert_reg_instance_equals(REG_SIZE, TEST_UNIT_NUM, 0x000FFFFF);
}

static void btu_selftest_read_from_size(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_SIZE, TEST_UNIT_NUM, 0x0005A5A5);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_SIZE(TEST_UNIT_NUM), 0x0005A5A5);
}

static void btu_selftest_write_to_transfer_status(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0xFFFFFFFFFFFFFFFF);
    btu_selftest_assert_reg_instance_equals(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0xE);
}

static void btu_selftest_read_from_transfer_status(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0xE);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0xE);
}

static void btu_selftest_write_to_ripf(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_BTU_RIPF, 0xFFFFFFFFFFFFFFFF);
    btu_selftest_assert_reg_equals(REG_BTURIPF, 0x1);
}

static void btu_selftest_read_from_ripf(TESTCONTEXT *testContext)
{
    btu_selftest_set_register(REG_BTURIPF, 0xF);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_BTU_RIPF, 0x1);
}

static void btu_selftest_write_to_transfer_complete_is_ignored(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_COMPLETE, 0xFFFFFFFFFFFFFFFF);
    btu_selftest_assert_reg_equals(REG_TRANSFERCOMPLETE, 0);
}

static void btu_selftest_read_from_transfer_complete(TESTCONTEXT *testContext)
{
    btu_selftest_set_register(REG_TRANSFERCOMPLETE, 0xFF);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_TRANSFER_COMPLETE, 0xF);
}

static void btu_selftest_unit_is_initially_inactive(TESTCONTEXT *testContext)
{
    btu_selftest_assert_unit_is_inactive(TEST_UNIT_NUM);
}

static void btu_selftest_setting_transfer_in_progress_activates_unit(TESTCONTEXT *testContext)
{
    btu_selftest_setup_request(0, 0, 2, TEST_UNIT_NUM);
    btu_selftest_assert_unit_is_active(TEST_UNIT_NUM);
}

static void btu_selftest_counts_down_size_during_transfer(TESTCONTEXT *testContext)
{
    mu5_selftest_assert_fail(testContext);
}

static void btu_selftest_keeps_unit_active_during_transfer(TESTCONTEXT *testContext)
{
    int i;
    btu_selftest_setup_request(0, 0, 6, TEST_UNIT_NUM);
    for (i = 0; i < 3; i++)
    {
        btu_selftest_execute_cycle();
        btu_selftest_assert_unit_is_active(TEST_UNIT_NUM);
    }
}

static void btu_selftest_stops_counting_down_size_at_end_of_transfer(TESTCONTEXT *testContext)
{
    mu5_selftest_assert_fail(testContext);
}

static void btu_selftest_leaves_unit_inactive_at_end_of_transfer(TESTCONTEXT *testContext)
{
    int i;
    btu_selftest_setup_request(0, 0, 6, TEST_UNIT_NUM);
    for (i = 0; i < 4; i++)
    {
        btu_selftest_execute_cycle();
    }

    btu_selftest_assert_unit_is_inactive(TEST_UNIT_NUM);
}
