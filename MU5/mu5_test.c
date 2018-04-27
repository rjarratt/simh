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
static uint8 mu5_selftest_reg_byte_width(REG *reg);

static void mu5_reset_test(TESTCONTEXT *context, UNITTEST *unitTest, void(*reset)(UNITTEST *unitTest))
{
    context->testName = unitTest->name;
    context->result = SCPE_OK;
    reset(unitTest);
}

static uint8 mu5_selftest_reg_byte_width(REG *reg)
{
    uint8 result;

    if (reg->width <= 16)
    {
        result = 2;
    }
    else if (reg->width <= 32)
    {
        result = 4;
    }
    else
    {
        result = 8;
    }

    return result;
}

void mu5_selftest_start(TESTCONTEXT *context)
{
    context->countFailed = 0;
    context->countSuccessful = 0;
    context->result = SCPE_OK;
    context->overallResult = SCPE_OK;
}

void mu5_selftest_run_suite(TESTCONTEXT *context, UNITTEST *unitTests, uint32 numberOfUnitTests, void(*reset)(UNITTEST *unitTest))
{
    uint32 i;
    for (i = 0; i < numberOfUnitTests; i++)
    {
        UNITTEST *test = &unitTests[i];
        mu5_reset_test(context, test, reset);
        if (test->runner != NULL)
        {
            test->runner(context);
            if (context->result == SCPE_OK)
            {
                sim_debug(LOG_SELFTEST_DETAIL, context->dev, "%s [OK]\n", test->name);
                context->countSuccessful++;
            }
            else
            {
                context->overallResult = SCPE_AFAIL;
                sim_debug(LOG_SELFTEST_FAIL, context->dev, "%s [FAIL]\n", test->name);
                context->countFailed++;
            }
        }
        else
        {
            context->overallResult = SCPE_AFAIL;
            sim_debug(LOG_SELFTEST_FAIL, context->dev, "%s [UNDEFINED]\n", test->name);
            context->countFailed++;
        }
    }
}

t_stat mu5_selftest_end(TESTCONTEXT *context)
{
    if (context->countFailed == 0)
    {
        sim_debug(LOG_SELFTEST, &cpu_dev, "ALL %d TESTS PASSED\n", context->countSuccessful);
    }
    else
    {
        sim_debug(LOG_SELFTEST_FAIL, &cpu_dev, "%d of %d TESTS PASSED, %d FAILED\n", context->countSuccessful, context->countFailed + context->countSuccessful, context->countFailed);
    }

    return context->overallResult;
}

void mu5_selftest_set_level0_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_LEVEL0;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_set_level1_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_LEVEL1;
    mu5_selftest_set_register(context, device, REG_MS, ms);
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

void mu5_selftest_clear_acc_faults_to_system_error_in_exec_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() & ~MS_MASK_A_SYS_ERR_EXEC;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_set_acc_faults_to_system_error_in_exec_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_A_SYS_ERR_EXEC;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_clear_b_and_d_faults_to_system_error_in_exec_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() & ~MS_MASK_B_D_SYS_ERR_EXEC;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_set_b_and_d_faults_to_system_error_in_exec_mode(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_B_D_SYS_ERR_EXEC;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_clear_inhibit_program_fault_interrupts(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() & ~MS_MASK_INH_PROG_FLT;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_set_inhibit_program_fault_interrupts(TESTCONTEXT *context, DEVICE *device)
{
    uint16 ms = cpu_get_ms() | MS_MASK_INH_PROG_FLT;
    mu5_selftest_set_register(context, device, REG_MS, ms);
}

void mu5_selftest_assert_fail(TESTCONTEXT *context)
{
    sim_debug(LOG_SELFTEST_FAIL, context->dev, "Test failed\n");
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
		sim_debug(LOG_SELFTEST_FAIL, context->dev, "Expected value in register %s to be %llX, but was %llX\n", name, expectedValue, actualValue);
		mu5_selftest_set_failure(context);
	}
}

void mu5_selftest_assert_reg_equals_mask(TESTCONTEXT *context, DEVICE *device, char *name, t_uint64 mask, t_uint64 expectedValue)
{
	t_uint64 actualValue = mu5_selftest_get_register(context, device, name);

	if ((actualValue & mask) != expectedValue)
	{
		sim_debug(LOG_SELFTEST_FAIL, context->dev, "Expected value in register %s to be %llX, but was %llX (masked %llx)\n", name, expectedValue, actualValue & mask, mask);
		mu5_selftest_set_failure(context);
	}
}

void mu5_selftest_assert_reg_instance_equals(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index, t_uint64 expectedValue)
{
    t_uint64 actualValue = mu5_selftest_get_register_instance(context, device, name, index);

    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, context->dev, "Expected value in register %s[%hu] to be %llX, but was %llX\n", name, (unsigned short)index, expectedValue, actualValue);
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_reg_instance_equals_mask(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index, t_uint64 mask, t_uint64 expectedValue)
{
    t_uint64 actualValue = mu5_selftest_get_register_instance(context, device, name, index);

    if ((actualValue & mask) != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, context->dev, "Expected value in register %s[%hu] to be %llX, but was %llX (masked %llx)\n", name, (unsigned short)index, expectedValue, actualValue & mask, mask);
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_no_system_error(TESTCONTEXT *context)
{
    mu5_selftest_assert_vstore_contents(context, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0);
}

void mu5_selftest_assert_no_program_fault(TESTCONTEXT *context)
{
    mu5_selftest_assert_vstore_contents(context, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0);
}

void mu5_selftest_assert_interrupt_inhibited(TESTCONTEXT *context)
{
    if (cpu_get_interrupt_number() != 255)
    {
        sim_debug(LOG_SELFTEST_FAIL, context->dev, "Unexpected interrupt\n");
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_no_interrupt(TESTCONTEXT *context)
{
    if (cpu_get_interrupt_number() != 255)
    {
        sim_debug(LOG_SELFTEST_FAIL, context->dev, "Unexpected interrupt\n");
        mu5_selftest_set_failure(context);
    }

    mu5_selftest_assert_no_system_error(context);
    mu5_selftest_assert_no_program_fault(context);
}

void mu5_selftest_assert_interrupt_number(TESTCONTEXT *context, int expectedInterruptNumber)
{
    int interruptNumber = cpu_get_interrupt_number();
    if (interruptNumber != expectedInterruptNumber)
    {
        sim_debug(LOG_SELFTEST_FAIL, context->dev, "Found interrupt %d when expected interrupt %d to have occurred\n", interruptNumber, expectedInterruptNumber);
        mu5_selftest_set_failure(context);
    }
}

void mu5_selftest_assert_access_violation_as_system_error(TESTCONTEXT *context)
{
    mu5_selftest_assert_interrupt_number(context, INT_SYSTEM_ERROR);
    mu5_selftest_assert_vstore_contents(context, PROP_V_STORE_BLOCK, PROP_V_STORE_SYSTEM_ERROR_STATUS, 0x0004);
}

void mu5_selftest_assert_access_violation_as_illegal_order(TESTCONTEXT *context)
{
    mu5_selftest_assert_interrupt_number(context, INT_ILLEGAL_ORDERS);
    mu5_selftest_assert_vstore_contents(context, PROP_V_STORE_BLOCK, PROP_V_STORE_PROGRAM_FAULT_STATUS, 0x0800);
}

void mu5_selftest_assert_operand_access_violation_as_system_error(TESTCONTEXT *context)
{
    mu5_selftest_assert_access_violation_as_system_error(context);
    mu5_selftest_assert_vstore_contents(context, SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x2);
}

void mu5_selftest_assert_operand_access_violation_as_illegal_order(TESTCONTEXT *context)
{
    mu5_selftest_assert_access_violation_as_illegal_order(context);
    mu5_selftest_assert_vstore_contents(context, SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x2);
}

void mu5_selftest_assert_instruction_access_violation_as_system_error(TESTCONTEXT *context)
{
    mu5_selftest_assert_vstore_contents(context, SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x6);
}

void mu5_selftest_assert_instruction_access_violation_as_system_error_interrupt(TESTCONTEXT *context)
{
    mu5_selftest_assert_access_violation_as_system_error(context);
    mu5_selftest_assert_instruction_access_violation_as_system_error(context);
}

void mu5_selftest_assert_instruction_access_violation_as_illegal_order(TESTCONTEXT *context)
{
    mu5_selftest_assert_access_violation_as_illegal_order(context);
    mu5_selftest_assert_vstore_contents(context, SAC_V_STORE_BLOCK, SAC_V_STORE_ACCESS_VIOLATION, 0x0006);
}

void mu5_selftest_assert_vstore_contents(TESTCONTEXT *context, uint8 block, uint8 line, t_uint64 expectedValue)
{
    t_uint64 actualValue = sac_read_v_store(block, line);
    if (actualValue != expectedValue)
    {
        sim_debug(LOG_SELFTEST_FAIL, context->dev, "Expected value in V-Store block %hu line %hu to be %llX, but was %llX\n", (unsigned short)block, (unsigned short)line, expectedValue, actualValue);
        mu5_selftest_set_failure(context);
    }
}

t_uint64 mu5_selftest_read_callback_for_static_64_bit_location(uint8 line)
{
    return VStoreTestLocation;
}

void mu5_selftest_write_callback_for_static_64_bit_location(uint8 line, t_uint64 value)
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
        sim_debug(LOG_SELFTEST_FAIL, device, "Could not find register %s\n", name);
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
        loc = (uint8 *)reg->loc + index * mu5_selftest_reg_byte_width(reg);
    }

    mask = ((t_uint64)1 << reg->width) - 1;
    if (reg->width <= 16)
    {
        result = *(uint16 *)(loc);
    }
    else if(reg->width <= 32)
    {
        result = *(uint32 *)(loc);
    }
    else
    {
        result = *(t_uint64 *)(loc);
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
    t_uint64 mask;

    assert(index >= 0 && index < reg->depth);
    if (reg->depth == 1)
    {
        cpu_set_register(reg, value);
    }
    else
    {
        loc = (uint8 *)reg->loc + index * mu5_selftest_reg_byte_width(reg);

        mask = ((t_uint64)1 << reg->width) - 1;
        if (reg->width <= 16)
        {
            *(uint16 *)(loc) = value & mask & 0xFFFF;
        }
        else if (reg->width <= 32)
        {
            *(uint32 *)(loc) = value & mask & 0xFFFFFFFF;
        }
        else
        {
            *(t_uint64 *)(loc) = value & mask;
        }
    }
}
