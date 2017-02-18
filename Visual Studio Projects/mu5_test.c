/* mu5_test.c: MU5 self test support

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

#include "mu5_test.h"

extern DEVICE cpu_dev;

static void mu5_reset_test(TESTCONTEXT *context, UNITTEST *unitTest, void(*reset)(UNITTEST *unitTest));

static void mu5_reset_test(TESTCONTEXT *context, UNITTEST *unitTest, void(*reset)(UNITTEST *unitTest))
{
    context->testName = unitTest->name;
    context->result = SCPE_OK;
    reset(unitTest);
}

void mu5_selftest_start(TESTCONTEXT *context)
{
    context->countFailed = 0;
    context->countSuccessful = 0;
    context->result = SCPE_OK;
}

void mu5_selftest_run_suite(TESTCONTEXT *context, UNITTEST *unitTests, uint32 numberOfUnitTests, void(*reset)(UNITTEST *unitTest))
{
    uint32 i;
    for (i = 0; i < numberOfUnitTests; i++)
    {
        UNITTEST *test = &unitTests[i];
        mu5_reset_test(context, test, reset);
        test->runner();
        if (context->result == SCPE_OK)
        {
            sim_debug(LOG_CPU_SELFTEST, context->dev, "%s [OK]\n", test->name);
            context->countSuccessful++;
        }
        else
        {
            context->overallResult = SCPE_AFAIL;
            sim_debug(LOG_CPU_SELFTEST, context->dev, "%s [FAIL]\n", test->name);
            context->countFailed++;
        }
    }
}

t_stat mu5_selftest_end(TESTCONTEXT *context)
{
    sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "\n");
    if (context->countFailed == 0)
    {
        sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "ALL %d TESTS PASSED\n", context->countSuccessful);
    }
    else
    {
        sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%d of %d TESTS PASSED, %d FAILED\n", context->countSuccessful, context->countFailed + context->countSuccessful, context->countFailed);
    }

    return context->overallResult;
}
