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

static TESTCONTEXT *localTestContext;
extern DEVICE sac_dev;

void sac_selftest(TESTCONTEXT *testContext);
static void sac_selftest_reset(UNITTEST *test);

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext);

static UNITTEST tests[] =
{
    { "Reading a write-only V-Store line returns zeroes", sac_selftest_reading_write_only_vstore_line_returns_zeroes },
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
}

static void sac_selftest_reading_write_only_vstore_line_returns_zeroes(TESTCONTEXT *testContext)
{
    //mu5_assert_fail();
}

