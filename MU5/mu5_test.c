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

#include <assert.h>
#include "mu5_test.h"
#include "mu5_sac.h"
#include "mu5_cpu.h"

extern DEVICE cpu_dev;

t_uint64 VStoreTestLocation;

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
        test->runner(context);
        if (context->result == SCPE_OK)
        {
            sim_debug(LOG_CPU_SELFTEST_DETAIL, context->dev, "%s [OK]\n", test->name);
            context->countSuccessful++;
        }
        else
        {
            context->overallResult = SCPE_AFAIL;
            sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "%s [FAIL]\n", test->name);
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
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "%d of %d TESTS PASSED, %d FAILED\n", context->countSuccessful, context->countFailed + context->countSuccessful, context->countFailed);
    }

    return context->overallResult;
}

void mu5_selftest_set_executive_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_EXEC;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_set_user_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() & ~MS_MASK_EXEC;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_assert_fail(TESTCONTEXT *context)
{
    sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Test failed\n");
    mu5_selftest_set_failure(context);
}

void mu5_selftest_set_failure(TESTCONTEXT *context)
{
    context->result = SCPE_AFAIL;
}

void mu5_selftest_assert_reg_equals(TESTCONTEXT *context, DEVICE *device, char *name, t_uint64 expectedValue)
{
    t_uint64 actualValue = mu5_selftest_get_register(context, device, name);

    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Expected value in register %s to be %llX, but was %llX\n", name, expectedValue, actualValue);
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_reg_instance_equals(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index, t_uint64 expectedValue)
{
    t_uint64 actualValue = mu5_selftest_get_register_instance(context, device, name, index);

    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Expected value in register %s[%hu] to be %llX, but was %llX\n", name, index, expectedValue, actualValue);
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_no_interrupt(TESTCONTEXT *context)
{
    if (cpu_get_interrupt_number() != 255)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Unexpected interrupt\n");
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_interrupt_number(TESTCONTEXT *context, int expectedInterruptNumber)
{
    if (cpu_get_interrupt_number() != expectedInterruptNumber)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Expected interrupt %d to have occurred\n", expectedInterruptNumber);
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_operand_access_violation(TESTCONTEXT *context)
{
    mu5_selftest_assert_interrupt_number(context, INT_PROGRAM_FAULTS);
    mu5_selftest_assert_vstore_contents(context, SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x2);
}

void mu5_selftest_assert_instruction_access_violation(TESTCONTEXT *context)
{
    mu5_selftest_assert_interrupt_number(context, INT_PROGRAM_FAULTS);
    mu5_selftest_assert_vstore_contents(context, SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x6);
}

void mu5_selftest_assert_vstore_contents(TESTCONTEXT *context, uint8 block, uint8 line, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_v_store(block, line);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Expected value in V-Store block %hu line %hu to be %llX, but was %llX\n", block, line, expectedValue, actualValue);
        mu5_selftest_set_failure(context);
    }
}

t_uint64 mu5_selftest_read_callback_for_static_64_bit_location(void)
{
    return VStoreTestLocation;
}

void mu5_selftest_write_callback_for_static_64_bit_location(t_uint64 value)
{
    VStoreTestLocation = value;
}

void mu5_selftest_set_bcpr(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_BCPR;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_clear_bcpr(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() & ~MS_MASK_BCPR;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_setup_cpr(uint8 cprNumber, uint32 va, uint32 ra)
{
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_NUMBER, cprNumber);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_RA, ra);
    sac_write_v_store(SAC_V_STORE_BLOCK, SAC_V_STORE_CPR_VA, va);
}

REG *mu5_selftest_find_register(TESTCONTEXT *context, DEVICE *device, char *name)
{
    REG *rptr;
    for (rptr = device->registers; rptr->name != NULL; rptr++)
    {
        if (strcmp(rptr->name, name) == 0)
        {
            break;
        }
    }

    if (rptr->name == NULL)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, device, "Could not find register %s\n", name);
        mu5_selftest_set_failure(context);
        rptr = NULL;
    }

    return rptr;
}

t_uint64 mu5_selftest_get_register(TESTCONTEXT *context, DEVICE *device, char *name)
{
    return mu5_selftest_get_register_instance(context, device, name, 0);
}

t_uint64 mu5_selftest_get_register_instance(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index)
{
    t_uint64 result = 0;
    t_uint64 mask;
    void *loc;
    REG *reg = mu5_selftest_find_register(context, device, name);

    assert(index >= 0 && index < reg->depth);
    if (reg->depth == 1)
    {
        loc = reg->loc;
    }
    else
    {
        loc = (uint8 *)reg->loc + index * (reg->width/8);
    }

    switch (reg->width)
    {
        case 16:
        {
            result = *(uint16 *)(loc);
            mask = 0xFFFF;
            break;
        }
        case 32:
        {
            result = *(uint32 *)(loc);
            mask = 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            result = *(t_uint64 *)(loc);
            mask = 0xFFFFFFFFFFFFFFFF;
            break;
        }
        default:
        {
            mu5_selftest_set_failure(context);
            break;
        }
    }

    return result & mask;

}

void mu5_selftest_set_register(TESTCONTEXT *context, DEVICE *device, char *name, t_uint64 value)
{
    mu5_selftest_set_register_instance(context, device, name, 0, value);
}

void mu5_selftest_set_register_instance(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index, t_uint64 value)
{
    REG *reg = mu5_selftest_find_register(context, device, name);
    void *loc;

    assert(index >= 0 && index < reg->depth);
    if (reg->depth == 1)
    {
        loc = reg->loc;
    }
    else
    {
        loc = (uint8 *)reg->loc + index * (reg->width / 8);
    }

    switch (reg->width)
    {
        case 16:
        {
            *(uint16 *)(loc) = value & 0xFFFF;
            break;
        }
        case 32:
        {
            *(uint32 *)(loc) = value & 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            *(t_uint64 *)(loc) = value & 0xFFFFFFFFFFFFFFFF;
            break;
        }
        default:
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, context->dev, "Unexpected register width %d for register %s\n", reg->width, name);
            mu5_selftest_set_failure(context);
            break;
        }
    }
}
