/* mu5_console_test.c: MU5 CONSOLE self tests

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
#include "mu5_sac.h"
#include "mu5_console.h"
#include "mu5_test.h"
#include "mu5_console_test.h"

static TESTCONTEXT *localTestContext;
extern DEVICE cpu_dev;
extern DEVICE console_dev;

void console_selftest(TESTCONTEXT *testContext);
static void console_selftest_reset(UNITTEST *test);

static void console_selftest_execute_cycle(void);

static void console_selftest_assert_vstore_contents(uint8 block, uint8 line, t_uint64 expectedValue);

static void console_selftest_write_console_interrupt_resets_tci(TESTCONTEXT *testContext);

static void console_selftest_after_writing_teletype_data_in_output_mode_tci_is_set(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Writing CONSOLE INTERRUPT V-Line resets teletype character interrupt bit", console_selftest_write_console_interrupt_resets_tci },

    { "After writing to TELETYPE DATA V-Line in transmit mode the teletype character interrupt bit is set", console_selftest_after_writing_teletype_data_in_output_mode_tci_is_set },
};

void console_selftest(TESTCONTEXT *testContext)
{
    int n;

    n = sizeof(tests) / sizeof(UNITTEST);

    localTestContext = testContext;
    localTestContext->dev = &console_dev;
    mu5_selftest_run_suite(testContext, tests, n, console_selftest_reset);
}

static void console_selftest_reset(UNITTEST *test)
{
    console_reset_state();
}

static void console_selftest_execute_cycle(void)
{
    UNIT *unit = console_dev.units;
    unit->action(unit);
}

static void console_selftest_assert_vstore_contents(uint8 block, uint8 line, t_uint64 expectedValue)
{
    mu5_selftest_assert_vstore_contents(localTestContext, block, line, expectedValue);
}

static void console_selftest_write_console_interrupt_resets_tci(TESTCONTEXT *testContext)
{
    sac_write_v_store(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_CONTROL, 0); /* set output mode */
    sac_write_v_store(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_DATA, 0);
    console_selftest_execute_cycle();
    sac_write_v_store(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_CONSOLE_INTERRUPT, 0x2);
    console_selftest_assert_vstore_contents(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_CONSOLE_INTERRUPT, 0x0);
}

static void console_selftest_after_writing_teletype_data_in_output_mode_tci_is_set(TESTCONTEXT *testContext)
{
    sac_write_v_store(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_CONTROL, 0); /* set output mode */
    sac_write_v_store(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_TELETYPE_DATA, 0);
    console_selftest_execute_cycle();
    console_selftest_assert_vstore_contents(CONSOLE_V_STORE_BLOCK, CONSOLE_V_STORE_CONSOLE_INTERRUPT, 0x2);
}