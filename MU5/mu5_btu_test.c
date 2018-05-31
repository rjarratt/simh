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
static void btu_selftest_cancel_transfer(void);
static int btu_selftest_transfer_in_progress(uint8 unit_num);

static void btu_selftest_set_failure(void);
static void btu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void btu_selftest_assert_reg_instance_equals(char *name, uint8 index, t_uint64 expectedValue);
static void btu_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue);
static void btu_selftest_assert_reg_instance_equals_mask(char *name, uint8 index, t_uint64 mask, t_uint64 expectedValue);
static void btu_selftest_assert_vx_line_contents(t_addr address, t_uint64 expectedValue);
static void btu_selftest_assert_unit_is_active(int unit_num);
static void btu_selftest_assert_unit_is_inactive(int unit_num);
static void btu_selftest_assert_btu_size_is(int unit_num, uint16 expected_size);
static void btu_selftest_assert_transfer_status_incomplete(int unit_num);
static void btu_selftest_assert_transfer_status_complete(int unit_num);
static void btu_selftest_assert_real_address_equals(t_addr address, t_uint64 expectedValue);
static void btu_selftest_assert_peripheral_window_equals(t_uint64 expectedValue);

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
static void btu_selftest_setting_transfer_in_progress_does_not_clear_transfer_complete_bit(TESTCONTEXT *testContext);
static void btu_selftest_counts_down_size_during_transfer(TESTCONTEXT *testContext);
static void btu_selftest_keeps_unit_active_during_transfer(TESTCONTEXT *testContext);
static void btu_selftest_leaves_unit_inactive_at_end_of_transfer(TESTCONTEXT *testContext);
static void btu_selftest_counts_down_minimum_size_transfer(TESTCONTEXT *testContext);
static void btu_selftest_counts_down_maximum_size_transfer(TESTCONTEXT *testContext);
static void btu_selftest_transfer_complete_bit_tracks_progress(TESTCONTEXT *testContext);
static void btu_selftest_clearing_transfer_in_progress_cancels_transfer(TESTCONTEXT *testContext);
static void btu_selftest_completion_of_transfer_sets_transfer_complete(TESTCONTEXT *testContext);
static void btu_selftest_transfer_copies_words_from_source_to_destination(TESTCONTEXT *testContext);
static void btu_selftest_transfer_starts_on_minimum_16_word_boundary(TESTCONTEXT *testContext);
static void btu_selftest_transfer_starts_on_rounded_up_power_of_2_word_boundary(TESTCONTEXT *testContext);
static void btu_selftest_transfer_from_local_with_bit_41_set_transfers_zeroes(TESTCONTEXT *testContext);
static void btu_selftest_transfer_from_mass_with_bit_41_set_transfers_words(TESTCONTEXT *testContext);
static void btu_selftest_cancelling_transfer_stops_word_transfer(TESTCONTEXT *testContext);
static void btu_selftest_transfer_completion_generates_interrupt(TESTCONTEXT *testContext);
static void btu_selftest_transfer_completion_after_cancellation_generates_interrupt(TESTCONTEXT *testContext);
static void btu_selftest_setting_transfer_complete_bit_in_transfer_status_resets_it(TESTCONTEXT *testContext);
static void btu_selftest_clearing_transfer_complete_bit_in_transfer_status_is_ignored(TESTCONTEXT *testContext);
/* writing to transfer completion bit clears it */
/* writing to transfer completion bit clears interrupt */
/* prop vx line to distinguish console and btu, note RNI says the other bits are not real bits they are signals taken from the outputs of the flip-flops registering the interrupts, so they will each go to zero when the relevant interrupt flip-flop is reset by the interrupt service routine */

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
    { "Setting transfer in progress bit does not clear the transfer complete bit", btu_selftest_setting_transfer_in_progress_does_not_clear_transfer_complete_bit },
    { "Counts down the size during a transfer", btu_selftest_counts_down_size_during_transfer },
    { "Keeps the unit active during a transfer", btu_selftest_keeps_unit_active_during_transfer },
    { "Leaves unit inactive at the end of a transfer", btu_selftest_leaves_unit_inactive_at_end_of_transfer },
    { "Counts down the size of a minimum size transfer", btu_selftest_counts_down_minimum_size_transfer },
    { "Counts down the size of a maximum size transfer", btu_selftest_counts_down_maximum_size_transfer },
    { "The transfer complete bit tracks the progress of the transfer", btu_selftest_transfer_complete_bit_tracks_progress },
    { "Clearing the transfer in progress bit cancels a transfer", btu_selftest_clearing_transfer_in_progress_cancels_transfer },
    { "Completion of a transfer sets the corresponding bit in the transfer complete Vx line", btu_selftest_completion_of_transfer_sets_transfer_complete },
    { "Transfer copies words from source to destination", btu_selftest_transfer_copies_words_from_source_to_destination },
    { "Transfer starts on a minimum 16-word boundary", btu_selftest_transfer_starts_on_minimum_16_word_boundary },
    { "Transfer starts on a rounded up power of 2 boundary", btu_selftest_transfer_starts_on_rounded_up_power_of_2_word_boundary },
    { "Transfer from local store with bit 41 set transfers zeroes", btu_selftest_transfer_from_local_with_bit_41_set_transfers_zeroes },
    { "Transfer from mass store with bit 41 set transfers words", btu_selftest_transfer_from_mass_with_bit_41_set_transfers_words },
    { "Cancelling a transfer stops the transfer of words", btu_selftest_cancelling_transfer_stops_word_transfer },
    { "Transfer completion generates an interrupt", btu_selftest_transfer_completion_generates_interrupt },
    { "Transfer completion after cancellation generates an interrupt", btu_selftest_transfer_completion_after_cancellation_generates_interrupt },
    { "Setting the transfer complete bit in the transfer status line resets it", btu_selftest_setting_transfer_complete_bit_in_transfer_status_resets_it },
    { "Clearing the transfer complete bit in the transfer status line is ignored", btu_selftest_clearing_transfer_complete_bit_in_transfer_status_is_ignored }
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

static void btu_selftest_cancel_transfer(void)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0x0);
}

static int btu_selftest_transfer_in_progress(uint8 unit_num)
{
    t_uint64 tc = mu5_selftest_get_register(localTestContext, &btu_dev, REG_TRANSFERCOMPLETE);
    int result = (tc & ((t_uint64)1 << ((4 - unit_num) + 3))) == 0;
    return result;
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

static void btu_selftest_assert_reg_instance_equals_mask(char *name, uint8 index, t_uint64 mask, t_uint64 expectedValue)
{
    mu5_selftest_assert_reg_instance_equals_mask(localTestContext, &btu_dev, name, index, mask, expectedValue);
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

static void btu_selftest_assert_btu_size_is(int unit_num, uint16 expected_size)
{
    uint16 actual = mu5_selftest_get_register_instance(localTestContext, &btu_dev, REG_SIZE, unit_num) & MASK_16;
    if (actual != expected_size)
    {
        sim_debug(LOG_SELFTEST_FAIL, &btu_dev, "Expected size to be %hu, but was %hu\n", expected_size, actual);
        btu_selftest_set_failure();
    }
}

static void btu_selftest_assert_transfer_status_incomplete(int unit_num)
{
    btu_selftest_assert_reg_instance_equals_mask(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0x4, 0x0);

}

static void btu_selftest_assert_transfer_status_complete(int unit_num)
{
    btu_selftest_assert_reg_instance_equals_mask(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0x4, 0x4);

}

static void btu_selftest_assert_real_address_equals(t_addr address, t_uint64 expectedValue)
{
    mu5_selftest_assert_real_address_equals(localTestContext, address, expectedValue);
}

static void btu_selftest_assert_peripheral_window_equals(t_uint64 expectedValue)
{
    mu5_selftest_assert_vstore_contents(localTestContext, PERIPHERAL_WINDOW_V_STORE_BLOCK, PERIPHERAL_WINDOW_V_STORE_MESSAGE_WINDOW, expectedValue);
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
    btu_selftest_assert_reg_instance_equals(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0xA);
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
    btu_selftest_set_register(REG_TRANSFERCOMPLETE, 0xFFF);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_TRANSFER_COMPLETE, 0xF0);
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

static void btu_selftest_setting_transfer_in_progress_does_not_clear_transfer_complete_bit(TESTCONTEXT *testContext)
{
    btu_selftest_setup_vx_line(BTU_VX_STORE_SIZE(TEST_UNIT_NUM), ((uint32)TEST_UNIT_NUM) << 16 | 6);
    btu_selftest_set_register_instance(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0x4);
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0x8);
    btu_selftest_assert_vx_line_contents(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0xC);
}

static void btu_selftest_counts_down_size_during_transfer(TESTCONTEXT *testContext)
{
    int i;
    uint16 expected_size = 6;
    btu_selftest_setup_request(0, 0, expected_size, TEST_UNIT_NUM);
    for (i = 0; i < 4; i++)
    {
        btu_selftest_execute_cycle();
        expected_size -= 2;
        btu_selftest_assert_btu_size_is(TEST_UNIT_NUM, expected_size);
    }
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

static void btu_selftest_counts_down_minimum_size_transfer(TESTCONTEXT *testContext)
{
    int i;
    uint16 expected_size = 0;
    btu_selftest_setup_request(0, 0, expected_size, TEST_UNIT_NUM);
    for (i = 0; i < 1; i++)
    {
        btu_selftest_execute_cycle();
        expected_size -= 2;
        btu_selftest_assert_btu_size_is(TEST_UNIT_NUM, expected_size);
    }

    btu_selftest_assert_unit_is_inactive(TEST_UNIT_NUM);
}

static void btu_selftest_counts_down_maximum_size_transfer(TESTCONTEXT *testContext)
{
    int i;
    uint16 expected_size = 65534;
    btu_selftest_setup_request(0, 0, expected_size, TEST_UNIT_NUM);
    for (i = 0; i < 32768; i++)
    {
        btu_selftest_execute_cycle();
        expected_size -= 2;
        btu_selftest_assert_btu_size_is(TEST_UNIT_NUM, expected_size);
    }

    btu_selftest_assert_unit_is_inactive(TEST_UNIT_NUM);
}

static void btu_selftest_transfer_complete_bit_tracks_progress(TESTCONTEXT *testContext)
{
    int i;
    uint16 expected_size = 65534;
    btu_selftest_setup_request(0, 0, expected_size, TEST_UNIT_NUM);
    for (i = 0; i < 32768; i++)
    {
        btu_selftest_assert_transfer_status_incomplete(TEST_UNIT_NUM);
        btu_selftest_execute_cycle();
    }

    btu_selftest_assert_transfer_status_complete(TEST_UNIT_NUM);
}

static void btu_selftest_clearing_transfer_in_progress_cancels_transfer(TESTCONTEXT *testContext)
{
    uint16 expected_size = 4;
    btu_selftest_setup_request(0, 0, expected_size, TEST_UNIT_NUM);
    btu_selftest_execute_cycle();

    btu_selftest_cancel_transfer();
    btu_selftest_assert_transfer_status_incomplete(TEST_UNIT_NUM);
    btu_selftest_assert_unit_is_active(TEST_UNIT_NUM);
    btu_selftest_execute_cycle();

    btu_selftest_assert_transfer_status_complete(TEST_UNIT_NUM);
    btu_selftest_assert_btu_size_is(TEST_UNIT_NUM, expected_size - 2);
    btu_selftest_assert_unit_is_inactive(TEST_UNIT_NUM);
}

static void btu_selftest_completion_of_transfer_sets_transfer_complete(TESTCONTEXT *testContext)
{
    btu_selftest_setup_request(0, 0, 2, TEST_UNIT_NUM);
    btu_selftest_assert_reg_equals(REG_TRANSFERCOMPLETE, 0x0);
    btu_selftest_execute_cycle();
    btu_selftest_assert_reg_equals(REG_TRANSFERCOMPLETE, 0x0);
    btu_selftest_execute_cycle();
    btu_selftest_assert_reg_equals(REG_TRANSFERCOMPLETE, 1 << ((4 - TEST_UNIT_NUM) + 3));
}

static void btu_selftest_transfer_copies_words_from_source_to_destination(TESTCONTEXT *testContext)
{
    int i;
    int words = 16384; /* 64-bit words, local memory is this size */
    t_uint64 expected;
    t_addr addr;

    for (i = 0; i < words; i++)
    {
        addr = RA_MASS(i << 1);
        expected = i * 256;
        exch_write(addr, expected);
    }

    btu_selftest_setup_request(RA_MASS(0), RA_LOCAL(0), 2 * (words - 1), TEST_UNIT_NUM);

    do
    {
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    for (i = 0; i < words; i++)
    {
        addr = RA_LOCAL(i << 1);
        btu_selftest_assert_real_address_equals(addr, i * 256);
        if (!mu5_selftest_is_test_ok(localTestContext))
        {
            break;
        }
    }
}

static void btu_selftest_transfer_starts_on_minimum_16_word_boundary(TESTCONTEXT *testContext)
{
    int i;
    int words = 2; /* 64-bit words */
    t_uint64 expected;
    t_addr addr;

    for (i = 0; i < words; i++)
    {
        addr = RA_MASS(i << 1);
        expected = i * 256;
        exch_write(addr, expected);
    }

    btu_selftest_setup_request(RA_MASS(4), RA_LOCAL(0x14), 2 * (words - 1), TEST_UNIT_NUM);

    do
    {
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    for (i = 0; i < words; i++)
    {
        addr = RA_LOCAL((0x8 + i) << 1);
        btu_selftest_assert_real_address_equals(addr, i * 256);
        if (!mu5_selftest_is_test_ok(localTestContext))
        {
            break;
        }
    }
}

static void btu_selftest_transfer_starts_on_rounded_up_power_of_2_word_boundary(TESTCONTEXT *testContext)
{
    int i;
    int words = 50; /* 64-bit words */
    t_uint64 expected;
    t_addr addr;

    for (i = 0; i < words; i++)
    {
        addr = RA_MASS(i << 1);
        expected = i * 256;
        exch_write(addr, expected);
    }

    btu_selftest_setup_request(RA_MASS(126), RA_LOCAL(130), 2 * (words - 1), TEST_UNIT_NUM);

    do
    {
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    for (i = 0; i < words; i++)
    {
        addr = RA_LOCAL((64 + i) << 1);
        btu_selftest_assert_real_address_equals(addr, i * 256);
        if (!mu5_selftest_is_test_ok(localTestContext))
        {
            break;
        }
    }
}

static void btu_selftest_transfer_from_local_with_bit_41_set_transfers_zeroes(TESTCONTEXT *testContext)
{
    int i;
    int words = 16; /* 64-bit words */
    t_uint64 expected;
    t_addr addr;

    for (i = 0; i < words; i++)
    {
        expected = i * 256;
        addr = RA_MASS(i << 1);
        exch_write(addr, expected);
        addr = RA_LOCAL(i << 1);
        exch_write(addr, expected);
    }

    btu_selftest_setup_request(RA_LOCAL(0x400000), RA_MASS(0), 2 * (words - 1), TEST_UNIT_NUM);

    do
    {
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    for (i = 0; i < words; i++)
    {
        addr = RA_MASS(i << 1);
        btu_selftest_assert_real_address_equals(addr, 0);
        if (!mu5_selftest_is_test_ok(localTestContext))
        {
            break;
        }
    }
}

static void btu_selftest_transfer_from_mass_with_bit_41_set_transfers_words(TESTCONTEXT *testContext)
{
    int i;
    int words = 16; /* 64-bit words */
    t_uint64 expected;
    t_addr addr;

    for (i = 0; i < words; i++)
    {
        expected = i * 256;
        addr = RA_MASS(i << 1);
        exch_write(addr, expected);
    }

    btu_selftest_setup_request(RA_MASS(0x400000), RA_LOCAL(0), 2 * (words - 1), TEST_UNIT_NUM);

    do
    {
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    for (i = 0; i < words; i++)
    {
        addr = RA_LOCAL(i << 1);
        btu_selftest_assert_real_address_equals(addr, i * 256);
        if (!mu5_selftest_is_test_ok(localTestContext))
        {
            break;
        }
    }
}

static void btu_selftest_cancelling_transfer_stops_word_transfer(TESTCONTEXT *testContext)
{
    int i;
    int words = 16; /* 64-bit words */
    t_uint64 expected;
    t_addr addr;

    for (i = 0; i < words; i++)
    {
        expected = i * 256;
        addr = RA_MASS(i << 1);
        exch_write(addr, expected);
    }

    btu_selftest_setup_request(RA_MASS(0), RA_LOCAL(0), 2 * (words - 1), TEST_UNIT_NUM);

    i = 0;
    do
    {
        if (i++ == 8)
        {
            btu_selftest_cancel_transfer();
        }

        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    for (i = 0; i < words; i++)
    {
        addr = RA_LOCAL(i << 1);
        if (i < 8)
        {
            expected = 0;
        }
        else
        {
            expected = i * 256;
        }

        btu_selftest_assert_real_address_equals(addr, expected);
        if (!mu5_selftest_is_test_ok(localTestContext))
        {
            break;
        }
    }
}

static void btu_selftest_transfer_completion_generates_interrupt(TESTCONTEXT *testContext)
{
    btu_selftest_setup_request(RA_MASS(0), RA_LOCAL(0), 6, TEST_UNIT_NUM);

    do
    {
        mu5_selftest_assert_no_interrupt(testContext);
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    mu5_selftest_assert_interrupt_number(testContext, INT_PERIPHERAL_WINDOW);
    btu_selftest_assert_peripheral_window_equals(TEST_UNIT_NUM);
}

static void btu_selftest_transfer_completion_after_cancellation_generates_interrupt(TESTCONTEXT *testContext)
{
    int i = 0;
    btu_selftest_setup_request(RA_MASS(0), RA_LOCAL(0), 6, TEST_UNIT_NUM);

    do
    {
        if (i++ == 1)
        {
            btu_selftest_cancel_transfer();
        }
        btu_selftest_execute_cycle();
    } while (btu_selftest_transfer_in_progress(TEST_UNIT_NUM));

    mu5_selftest_assert_interrupt_number(testContext, INT_PERIPHERAL_WINDOW);
    btu_selftest_assert_peripheral_window_equals(TEST_UNIT_NUM);
}

static void btu_selftest_setting_transfer_complete_bit_in_transfer_status_resets_it(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0x4);
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0x4);
    btu_selftest_assert_transfer_status_incomplete(TEST_UNIT_NUM);
}

static void btu_selftest_clearing_transfer_complete_bit_in_transfer_status_is_ignored(TESTCONTEXT *testContext)
{
    btu_selftest_set_register_instance(REG_TRANSFERSTATUS, TEST_UNIT_NUM, 0x4);
    btu_selftest_setup_vx_line(BTU_VX_STORE_TRANSFER_STATUS(TEST_UNIT_NUM), 0x0);
    btu_selftest_assert_transfer_status_complete(TEST_UNIT_NUM);
}

