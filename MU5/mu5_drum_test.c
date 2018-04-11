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
#define REG_STOREADDRESS "STOREADDRESS"
#define REG_DISCSTATUS "DISCSTATUS"
#define REG_COMPLETEADDRESS "COMPLETEADDRESS"

#define TEST_UNIT_NUM 3
#define TEST_COMPLETE_ADDRESS RA_LOCAL(0)
#define TEST_STORE_ADDRESS RA_LOCAL(2)

#define READ 1
#define WRITE 0

#define DISC_ADDRESS(rw, disc, band, block, size) (((t_uint64)(rw) << 30) | ((t_uint64)(disc) << 20) | ((t_uint64)(band) << 14) | ((t_uint64)(block) << 8) | (t_uint64)(size))

static TESTCONTEXT *localTestContext;
extern DEVICE cpu_dev;
extern DEVICE drum_dev;

void drum_selftest(TESTCONTEXT *testContext);
static void drum_selftest_detach_existing_file(void);
static void drum_selftest_attach_test_file(void);
static void drum_selftest_detach_and_delete_test_file(void);
static void drum_selftest_reset(UNITTEST *test);

static void drum_selftest_execute_cycle(void);
static void drum_selftest_execute_cycle_unit(int unitNum);

static void drum_selftest_setup_vx_line(uint8 line, t_uint64 value);
static void drum_selftest_setup_request(t_uint64 disc_address);

static void drum_selftest_set_failure(void);
static void drum_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);
static void drum_selftest_assert_reg_equals_mask(char *name, t_uint64 mask, t_uint64 expectedValue);
static void drum_selftest_assert_vx_line_contents(uint8 line, t_uint64 expectedValue);
static void drum_selftest_assert_legal_request();
static void drum_selftest_assert_illegal_request();
static void drum_selftest_assert_request_incomplete();
static void drum_selftest_assert_request_complete();
static void drum_selftest_assert_store_address_equals(t_uint64 expectedValue);

static void drum_selftest_current_position_incremented_on_each_cycle(TESTCONTEXT *testContext);
static void drum_selftest_current_position_wraps_after_last_block(TESTCONTEXT *testContext);
static void drum_selftest_current_position_incremented_independently_by_each_unit(TESTCONTEXT *testContext);
static void drum_selftest_read_non_v_address_sets_illegal_request_bit(TESTCONTEXT *testContext);
static void drum_selftest_write_non_v_address_sets_illegal_request_bit(TESTCONTEXT *testContext);
static void drum_selftest_write_to_disc_address(TESTCONTEXT *testContext);
static void drum_selftest_read_from_disc_address(TESTCONTEXT *testContext);
static void drum_selftest_write_to_store_address(TESTCONTEXT *testContext);
static void drum_selftest_read_from_store_address(TESTCONTEXT *testContext);
static void drum_selftest_read_from_disc_status(TESTCONTEXT *testContext);
static void drum_selftest_disc_status_unit_always_zero(TESTCONTEXT *testContext);
static void drum_selftest_disc_status_write_does_not_change_readonly_bits(TESTCONTEXT *testContext);
static void drum_selftest_disc_status_write_resets_writeable_bits(TESTCONTEXT *testContext);
static void drum_selftest_write_to_current_positions_ignored(TESTCONTEXT *testContext);
static void drum_selftest_read_from_current_positions(TESTCONTEXT *testContext);
static void drum_selftest_write_to_complete_address(TESTCONTEXT *testContext);
static void drum_selftest_read_from_complete_address(TESTCONTEXT *testContext);
static void drum_selftest_io_request_for_unattached_disk_sets_illegal_request(TESTCONTEXT *testContext);
static void drum_selftest_io_request_for_invalid_block_sets_illegal_request(TESTCONTEXT *testContext);
static void drum_selftest_io_request_for_zero_size_sets_illegal_request(TESTCONTEXT *testContext);
static void drum_selftest_io_request_for_invalid_size_sets_illegal_request(TESTCONTEXT *testContext);
static void drum_selftest_io_request_for_unit_0_store_address_sets_illegal_request(TESTCONTEXT *testContext);
static void drum_selftest_io_request_for_unit_0_complete_address_sets_illegal_request(TESTCONTEXT *testContext);
static void drum_selftest_io_waits_for_starting_block(TESTCONTEXT *testContext);
static void drum_selftest_io_waits_for_full_revolution_if_starting_block_is_current_block(TESTCONTEXT *testContext);
static void drum_selftest_io_transfer_stops_updating_disc_address_on_completion(TESTCONTEXT *testContext);
static void drum_selftest_io_transfer_of_n_blocks_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext, int n);
static void drum_selftest_io_transfer_of_1_block_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext);
static void drum_selftest_io_transfer_of_3_blocks_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext);
static void drum_selftest_io_transfer_of_37_blocks_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext);
static void drum_selftest_io_transfer_updates_disc_status_on_completion(TESTCONTEXT *testContext);
static void drum_selftest_io_transfer_updates_store_address_during_transfer(TESTCONTEXT *testContext);
// TODO write to complete address

static UNITTEST tests[] =
{
	{ "Each execution cycle of the drum unit advances the current position", drum_selftest_current_position_incremented_on_each_cycle },
	{ "The current position wraps after the last block", drum_selftest_current_position_wraps_after_last_block },
	{ "Each drum unit advances its current position independent of the others", drum_selftest_current_position_incremented_independently_by_each_unit },
	{ "Drum read of non-V address sets illegal request bit", drum_selftest_read_non_v_address_sets_illegal_request_bit },
	{ "Drum write of non-V address sets illegal request bit", drum_selftest_write_non_v_address_sets_illegal_request_bit },
    { "Can write to the disc address Vx line", drum_selftest_write_to_disc_address },
    { "Can read from the disc address Vx line", drum_selftest_read_from_disc_address },
    { "Can write to the store address Vx line", drum_selftest_write_to_store_address },
    { "Can read from the store address Vx line", drum_selftest_read_from_store_address },
    { "Can read from the disc status Vx line", drum_selftest_read_from_disc_status },
    { "Unit number always 0 in disc status Vx line ", drum_selftest_disc_status_unit_always_zero },
    { "Writing to disc status Vx line does not affect readonly bits", drum_selftest_disc_status_write_does_not_change_readonly_bits },
    { "Writing 1s to the writeable disc status Vx line bits resets them", drum_selftest_disc_status_write_resets_writeable_bits },
    { "Writes to the current positions Vx line are ignored", drum_selftest_write_to_current_positions_ignored },
    { "Can read from the current positions Vx line", drum_selftest_read_from_current_positions },
    { "Can write to the complete address Vx line", drum_selftest_write_to_complete_address },
    { "Can read from the complete address Vx line", drum_selftest_read_from_complete_address },
    { "I/O request to an unattached disk sets illegal request bit", drum_selftest_io_request_for_unattached_disk_sets_illegal_request },
    { "I/O request for invalid block sets illegal request bit", drum_selftest_io_request_for_invalid_block_sets_illegal_request },
    { "I/O request for zero size sets illegal request bit", drum_selftest_io_request_for_zero_size_sets_illegal_request },
    { "I/O request for invalid size sets illegal request bit", drum_selftest_io_request_for_invalid_size_sets_illegal_request },
	{ "I/O request for unit 0 store address sets illegal request bit", drum_selftest_io_request_for_unit_0_store_address_sets_illegal_request },
	{ "I/O request for unit 0 complete address sets illegal request bit", drum_selftest_io_request_for_unit_0_complete_address_sets_illegal_request },
    { "I/O operation waits for drum to rotate to starting block", drum_selftest_io_waits_for_starting_block },
	{ "I/O operation waits for drum to do a full rotation if the starting block is the current block", drum_selftest_io_waits_for_full_revolution_if_starting_block_is_current_block },
	{ "I/O operation stops updating disc address on completion", drum_selftest_io_transfer_stops_updating_disc_address_on_completion },
	{ "I/O operation to transfer 1 block updates disc address during transfer and then stops updating it", drum_selftest_io_transfer_of_1_block_updates_disc_address_and_stops_updating_on_completion },
	{ "I/O operation to transfer 3 blocks updates disc address during transfer and then stops updating it", drum_selftest_io_transfer_of_3_blocks_updates_disc_address_and_stops_updating_on_completion },
	{ "I/O operation to transfer 37 updates disc address during transfer and then stops updating it", drum_selftest_io_transfer_of_37_blocks_updates_disc_address_and_stops_updating_on_completion },
	{ "I/O operation updates disc status on completion", drum_selftest_io_transfer_updates_disc_status_on_completion },
	{ "I/O operation updates store address during transfer", drum_selftest_io_transfer_updates_store_address_during_transfer }
// TODO: Interrupts
    // TODO: Read
    // TODO: Write
    // TODO: multiple blocks
    // TODO: Update store address, complete address, disc status
};

void drum_selftest(TESTCONTEXT *testContext)
{
	int n;

	n = sizeof(tests) / sizeof(UNITTEST);

	localTestContext = testContext;
	localTestContext->dev = &drum_dev;
    drum_selftest_detach_existing_file();
	mu5_selftest_run_suite(testContext, tests, n, drum_selftest_reset);
}

static void drum_selftest_detach_existing_file(void)
{
    UNIT *unit = &drum_dev.units[TEST_UNIT_NUM];
    drum_dev.detach(unit);
}

static void drum_selftest_attach_test_file(void)
{
    UNIT *unit = &drum_dev.units[TEST_UNIT_NUM];
    drum_dev.attach(unit, tmpnam(NULL));
}

static void drum_selftest_detach_and_delete_test_file(void)
{
    UNIT *unit = &drum_dev.units[TEST_UNIT_NUM];
    drum_dev.detach(unit);
    remove(unit->filename);
}

static void drum_selftest_reset(UNITTEST *test)
{
	drum_reset_state();
}

static void drum_selftest_execute_cycle(void)
{
	drum_selftest_execute_cycle_unit(TEST_UNIT_NUM);
}

static void drum_selftest_execute_cycle_unit(int unitNum)
{
	UNIT *unit = &drum_dev.units[unitNum];
	unit->action(unit);
}

static void drum_selftest_setup_vx_line(uint8 line, t_uint64 value)
{
    exch_write(RA_VX_DRUM(line), value);
}

static void drum_selftest_setup_request(t_uint64 disc_address)
{
	drum_selftest_setup_vx_line(DRUM_VX_STORE_STORE_ADDRESS, TEST_STORE_ADDRESS);
	drum_selftest_setup_vx_line(DRUM_VX_STORE_COMPLETE_ADDRESS, TEST_COMPLETE_ADDRESS);
	drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_ADDRESS, disc_address);
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
	drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, DRUM_DISC_STATUS_DECODE | DRUM_DISC_STATUS_ILLEGAL_REQUEST, 0);
}

static void drum_selftest_assert_illegal_request()
{
	drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, DRUM_DISC_STATUS_DECODE | DRUM_DISC_STATUS_ILLEGAL_REQUEST, DRUM_DISC_STATUS_DECODE | DRUM_DISC_STATUS_ILLEGAL_REQUEST);
}

static void drum_selftest_assert_request_incomplete()
{
	drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, DRUM_DISC_STATUS_DECODE | DRUM_DISC_STATUS_END_TRANSFER, 0);
}

static void drum_selftest_assert_request_complete()
{
	drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, DRUM_DISC_STATUS_DECODE | DRUM_DISC_STATUS_END_TRANSFER, DRUM_DISC_STATUS_DECODE | DRUM_DISC_STATUS_END_TRANSFER);
}

static void drum_selftest_assert_store_address_equals(t_uint64 expectedValue)
{
	drum_selftest_assert_reg_equals_mask(REG_STOREADDRESS, RA_MASK_FULL, expectedValue);
}

static void drum_selftest_current_position_incremented_on_each_cycle(TESTCONTEXT *testContext)
{
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 0);
	drum_selftest_execute_cycle();
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 1 << (TEST_UNIT_NUM * 6));
	drum_selftest_execute_cycle();
	drum_selftest_assert_reg_equals(REG_CURRENTPOSITIONS, 2 << (TEST_UNIT_NUM * 6));
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
    drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_ADDRESS, 0xFFFFFFFFA5A5A5A5);
    drum_selftest_assert_reg_equals(REG_DISCADDRESS, 0x8025A525);
}

static void drum_selftest_read_from_disc_address(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_DISCADDRESS, 0xA5A5A5A5);
    drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, 0x8025A525);
}

static void drum_selftest_write_to_store_address(TESTCONTEXT *testContext)
{
    drum_selftest_setup_vx_line(DRUM_VX_STORE_STORE_ADDRESS, 0xFFFFFFFFFFFFFFFF);
    drum_selftest_assert_reg_equals(REG_STOREADDRESS, 0x0FFFFFFF);
    drum_selftest_assert_legal_request();
}

static void drum_selftest_read_from_store_address(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_STOREADDRESS, 0xA5A5A5);
    drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_STORE_ADDRESS, 0xA5A5A5);
}

static void drum_selftest_read_from_disc_status(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_DISCSTATUS, 0xA5A5A5);
    drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_STATUS, 0xA5A5A0);
}

static void drum_selftest_disc_status_unit_always_zero(TESTCONTEXT *testContext)
{
    drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, 0xF, 0);
}

static void drum_selftest_disc_status_write_does_not_change_readonly_bits(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_DISCSTATUS, 0xFFFFFFFF);
    drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_STATUS, 0);
    drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, 0xFFFFFFFF, 0xFFFFFFFF);

    mu5_selftest_set_register(testContext, &drum_dev, REG_DISCSTATUS, 0x00000000);
    drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_STATUS, 0xFFFFFFFF);
    drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, 0xFFFFFFFF, 0x00000000);
}

static void drum_selftest_disc_status_write_resets_writeable_bits(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_DISCSTATUS, 0xFFFFFFFF);
    drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_STATUS, 0xFFFFFFFF);
    drum_selftest_assert_reg_equals_mask(REG_DISCSTATUS, 0xFFFFFFFF, 0x7FFFDBCF);
    drum_selftest_assert_legal_request();
}

static void drum_selftest_write_to_current_positions_ignored(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_CURRENTPOSITIONS, 0xFFFFFFFF);
    drum_selftest_setup_vx_line(DRUM_VX_STORE_CURRENT_POSITIONS, 0);
    drum_selftest_assert_reg_equals_mask(REG_CURRENTPOSITIONS, 0xFFFFFFFF, 0xFFFFFFFF);
    drum_selftest_assert_legal_request();
}

static void drum_selftest_read_from_current_positions(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_CURRENTPOSITIONS, 0xFFFFFFFF);
    drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_CURRENT_POSITIONS, 0x03FFFFFF);
}

static void drum_selftest_write_to_complete_address(TESTCONTEXT *testContext)
{
    drum_selftest_setup_vx_line(DRUM_VX_STORE_COMPLETE_ADDRESS, 0xFFFFFFFFFFFFFFFF);
    drum_selftest_assert_reg_equals(REG_COMPLETEADDRESS, 0x0FFFFFFF);
    drum_selftest_assert_legal_request();
}

static void drum_selftest_read_from_complete_address(TESTCONTEXT *testContext)
{
    mu5_selftest_set_register(testContext, &drum_dev, REG_COMPLETEADDRESS, 0xA5A5A5A);
    drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_COMPLETE_ADDRESS, 0xA5A5A5A);
}

static void drum_selftest_io_request_for_unattached_disk_sets_illegal_request(TESTCONTEXT *testContext)
{
    drum_selftest_detach_and_delete_test_file();
	drum_selftest_setup_request(DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 0, 1));
    drum_selftest_assert_illegal_request();
}

static void drum_selftest_io_request_for_invalid_block_sets_illegal_request(TESTCONTEXT *testContext)
{
    drum_selftest_attach_test_file();
	drum_selftest_setup_request(DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, DRUM_BLOCKS_PER_BAND, 1));
    drum_selftest_assert_illegal_request();
}

static void drum_selftest_io_request_for_zero_size_sets_illegal_request(TESTCONTEXT *testContext)
{
    drum_selftest_attach_test_file();
	drum_selftest_setup_request(DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 0, 0));
    drum_selftest_assert_illegal_request();
}

static void drum_selftest_io_request_for_invalid_size_sets_illegal_request(TESTCONTEXT *testContext)
{
    drum_selftest_attach_test_file();
	drum_selftest_setup_request(DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 0, DRUM_BLOCKS_PER_BAND));
    drum_selftest_assert_illegal_request();
}

static void drum_selftest_io_request_for_unit_0_store_address_sets_illegal_request(TESTCONTEXT *testContext)
{
	drum_selftest_attach_test_file();
	drum_selftest_setup_vx_line(DRUM_VX_STORE_STORE_ADDRESS, 0);
	drum_selftest_setup_vx_line(DRUM_VX_STORE_COMPLETE_ADDRESS, RA_LOCAL(2));
	drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_ADDRESS, DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 0, 1));
	drum_selftest_assert_illegal_request();
}

static void drum_selftest_io_request_for_unit_0_complete_address_sets_illegal_request(TESTCONTEXT *testContext)
{
	drum_selftest_attach_test_file();
	drum_selftest_setup_vx_line(DRUM_VX_STORE_STORE_ADDRESS, RA_LOCAL(0));
	drum_selftest_setup_vx_line(DRUM_VX_STORE_COMPLETE_ADDRESS, 0);
	drum_selftest_setup_vx_line(DRUM_VX_STORE_DISC_ADDRESS, DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 0, 1));
	drum_selftest_assert_illegal_request();
}

static void drum_selftest_io_waits_for_starting_block(TESTCONTEXT *testContext)
{
	t_uint64 request = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 2, 1);

	drum_selftest_setup_request(request);
	drum_selftest_assert_legal_request();

	drum_selftest_execute_cycle();
	drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, request);
	drum_selftest_execute_cycle();
	drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, request);
	drum_selftest_execute_cycle();
	drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 3, 0));
}

static void drum_selftest_io_waits_for_full_revolution_if_starting_block_is_current_block(TESTCONTEXT *testContext)
{
	int i;
	t_uint64 request = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 0, 1);

	drum_selftest_setup_request(request);
	drum_selftest_assert_legal_request();

	for (i = 0; i < DRUM_BLOCKS_PER_BAND; i++)
	{
		drum_selftest_execute_cycle();
		drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, request);
	}

	drum_selftest_execute_cycle();
	drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 1, 0));
}

static void drum_selftest_io_transfer_stops_updating_disc_address_on_completion(TESTCONTEXT *testContext)
{
	int i;
	t_uint64 request = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 1, 1);
	t_uint64 final = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 2, 0);

	drum_selftest_setup_request(request);
	drum_selftest_assert_legal_request();

	drum_selftest_execute_cycle();

	for (i = 0; i < DRUM_BLOCKS_PER_BAND; i++)
	{
		drum_selftest_execute_cycle();
		drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, final);
	}
}

static void drum_selftest_io_transfer_of_n_blocks_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext, int n)
{
	int i;
	t_uint64 request = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 1, n);
	t_uint64 final = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, (1 + n) % DRUM_BLOCKS_PER_BAND, 0);

	drum_selftest_setup_request(request);
	drum_selftest_assert_legal_request();

	drum_selftest_execute_cycle();

	for (i = 0; i < n; i++)
	{
		drum_selftest_execute_cycle();
		drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, (2 + i) % DRUM_BANDS_PER_UNIT, n-i-1));
	}

	for (i = 0; i < DRUM_BLOCKS_PER_BAND; i++)
	{
		drum_selftest_execute_cycle();
		drum_selftest_assert_vx_line_contents(DRUM_VX_STORE_DISC_ADDRESS, final);
	}
}

static void drum_selftest_io_transfer_of_1_block_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext)
{
	drum_selftest_io_transfer_of_n_blocks_updates_disc_address_and_stops_updating_on_completion(testContext, 1);
}

static void drum_selftest_io_transfer_of_3_blocks_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext)
{
	drum_selftest_io_transfer_of_n_blocks_updates_disc_address_and_stops_updating_on_completion(testContext, 3);
}

static void drum_selftest_io_transfer_of_37_blocks_updates_disc_address_and_stops_updating_on_completion(TESTCONTEXT *testContext)
{
	drum_selftest_io_transfer_of_n_blocks_updates_disc_address_and_stops_updating_on_completion(testContext, 3);
}

static void drum_selftest_io_transfer_updates_disc_status_on_completion(TESTCONTEXT *testContext)
{
	int i;
	t_uint64 request = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 1, 2);

	drum_selftest_setup_request(request);
	drum_selftest_assert_legal_request();
	drum_selftest_assert_request_incomplete();

	drum_selftest_execute_cycle();
	drum_selftest_assert_request_incomplete();
	drum_selftest_execute_cycle();
	drum_selftest_assert_request_incomplete();

	drum_selftest_execute_cycle();
	drum_selftest_assert_request_complete();

	for (i = 0; i < DRUM_BLOCKS_PER_BAND; i++)
	{
		drum_selftest_execute_cycle();
		drum_selftest_assert_request_complete();
	}
}

static void drum_selftest_io_transfer_updates_store_address_during_transfer(TESTCONTEXT *testContext)
{
	int i;
	t_uint64 request = DISC_ADDRESS(READ, TEST_UNIT_NUM, 0, 1, 2);
	drum_selftest_setup_request(request);

	drum_selftest_execute_cycle();
	drum_selftest_assert_store_address_equals(TEST_STORE_ADDRESS);

	drum_selftest_execute_cycle();
	drum_selftest_assert_store_address_equals(TEST_STORE_ADDRESS + DRUM_WORDS_PER_BLOCK);

	drum_selftest_execute_cycle();
	drum_selftest_assert_store_address_equals(TEST_STORE_ADDRESS + (2 * DRUM_WORDS_PER_BLOCK));

	for (i = 0; i < DRUM_BLOCKS_PER_BAND; i++)
	{
		drum_selftest_execute_cycle();
		drum_selftest_assert_store_address_equals(TEST_STORE_ADDRESS + (2 * DRUM_WORDS_PER_BLOCK));
	}
}
