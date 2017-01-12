/* mu5_cpu_test.c: MU5 CPU self tests

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
#include "mu5_cpu.h"
#include "mu5_cpu_test.h"
#include "mu5_sac.h"

#define REG_A "A"
#define REG_AEX "AEX"
#define REG_B "B"
#define REG_BOD "BOD"
#define REG_D "D"
#define REG_XD "XD"
#define REG_DT "DT"
#define REG_XDT "XDT"
#define REG_DOD "DOD"
#define REG_NB "NB"
#define REG_XNB "XNB"
#define REG_SN "SN"
#define REG_SF "SF"
#define REG_MS "MS"

#define CR_B 1
#define CR_STS1 2
#define CR_STS2 3
#define CR_FLOAT 7

#define F_LOAD_XDO 0

#define F_LOAD_64 1

#define K_LITERAL 0
#define K_IR 1
#define K_V32 2
#define K_V64 3

#define NP_64_BIT_LITERAL 2

typedef struct TESTCONTEXT
{
    char *testName;
    uint32 currentLoadLocation;
    t_stat result;
} TESTCONTEXT;

typedef struct UNITTEST
{
    char * name;
    void (*runner)(void);
} UNITTEST;

extern DEVICE cpu_dev;

static TESTCONTEXT testContext;

static void cpu_selftest_reset(UNITTEST *test);
static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n);
static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 k, uint8 np);
static void cpu_selftest_load_64_bit_literal(t_uint64 value);
static void cpu_selftest_run_code(void);
static REG *cpu_selftest_get_register(char *name);
static void cpu_selftest_set_register(char *name, t_uint64 value);
static void cpu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue);

static void cpu_selftest_load_operand_6_bit_positive_literal(void);
static void cpu_selftest_load_operand_6_bit_negative_literal(void);
static void cpu_selftest_load_operand_internal_register_0(void);
static void cpu_selftest_load_operand_internal_register_1(void);
static void cpu_selftest_load_operand_internal_register_2(void);
static void cpu_selftest_load_operand_internal_register_3(void);
static void cpu_selftest_load_operand_internal_register_4(void);
static void cpu_selftest_load_operand_internal_register_16(void);
static void cpu_selftest_load_operand_internal_register_17(void);
static void cpu_selftest_load_operand_internal_register_18(void);
static void cpu_selftest_load_operand_internal_register_19(void);
static void cpu_selftest_load_operand_internal_register_20(void);
static void cpu_selftest_load_operand_internal_register_32(void);
static void cpu_selftest_load_operand_internal_register_33(void);
static void cpu_selftest_load_operand_internal_register_34(void);
static void cpu_selftest_load_operand_internal_register_48(void);
static void cpu_selftest_load_operand_32_bit_variable(void);
static void cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned(void);
static void cpu_selftest_load_operand_64_bit_variable(void);
static void cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD(void);

UNITTEST tests[] =
{
	{ "Load operand 6-bit positive literal", cpu_selftest_load_operand_6_bit_positive_literal },
    { "Load operand 6-bit negative literal", cpu_selftest_load_operand_6_bit_negative_literal },
    { "Load operand 32-bit variable", cpu_selftest_load_operand_32_bit_variable },
    { "Load operand 32-bit variable 6-bit offset is unsigned", cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned },
	{ "Load operand internal register 0", cpu_selftest_load_operand_internal_register_0 },
	{ "Load operand internal register 1", cpu_selftest_load_operand_internal_register_1 },
    { "Load operand internal register 2", cpu_selftest_load_operand_internal_register_2 },
    { "Load operand internal register 3", cpu_selftest_load_operand_internal_register_3 },
    { "Load operand internal register 4", cpu_selftest_load_operand_internal_register_4 },
    { "Load operand internal register 16", cpu_selftest_load_operand_internal_register_16 },
    { "Load operand internal register 17", cpu_selftest_load_operand_internal_register_17 },
    { "Load operand internal register 18", cpu_selftest_load_operand_internal_register_18 },
    { "Load operand internal register 19", cpu_selftest_load_operand_internal_register_19 },
    { "Load operand internal register 20", cpu_selftest_load_operand_internal_register_20 },
    { "Load operand internal register 32", cpu_selftest_load_operand_internal_register_32 },
    { "Load operand internal register 33", cpu_selftest_load_operand_internal_register_33 },
    { "Load operand internal register 34", cpu_selftest_load_operand_internal_register_34 },
    { "Load operand internal register 48", cpu_selftest_load_operand_internal_register_48 },
    { "Load operand 64-bit variable", cpu_selftest_load_operand_64_bit_variable },
    { "STS1 XDO Load Loads LS half of XD", cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD }
};

static void cpu_selftest_load_operand_6_bit_positive_literal()
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x1F);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x000000000000001F);
}

static void cpu_selftest_load_operand_6_bit_negative_literal()
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_LITERAL, 0x3F);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0xFFFFFFFFFFFFFFFF);
}

static void cpu_selftest_load_operand_internal_register_0(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 0);
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 0);
	cpu_selftest_set_register(REG_MS, 0xAAAA);
	cpu_selftest_set_register(REG_NB, 0xBBBB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBB00000000);
	cpu_execute_next_order();
	cpu_selftest_assert_reg_equals(REG_A, 0xAAAABBBB00000001);
}

static void cpu_selftest_load_operand_internal_register_1(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 1);
	cpu_selftest_set_register(REG_XNB, 0xAAAAAAAA);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
}

static void cpu_selftest_load_operand_internal_register_2(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 2);
	cpu_selftest_set_register(REG_SN, 0xAAAA);
	cpu_selftest_set_register(REG_NB, 0xBBBB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
}

static void cpu_selftest_load_operand_internal_register_3(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 3);
	cpu_selftest_set_register(REG_SN, 0xAAAA);
	cpu_selftest_set_register(REG_SF, 0xBBBB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAABBBB);
}

static void cpu_selftest_load_operand_internal_register_4(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 4);
	cpu_selftest_set_register(REG_MS, 0x0100);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000001);
}

static void cpu_selftest_load_operand_internal_register_16(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 16);
	cpu_selftest_set_register(REG_D, 0xABABABABABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
}

static void cpu_selftest_load_operand_internal_register_17(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 17);
	cpu_selftest_set_register(REG_XD, 0xABABABABABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
}

static void cpu_selftest_load_operand_internal_register_18(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 18);
	cpu_selftest_set_register(REG_DT, 0xABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
}

static void cpu_selftest_load_operand_internal_register_19(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 19);
	cpu_selftest_set_register(REG_XDT, 0xABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
}

static void cpu_selftest_load_operand_internal_register_20(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 20);
	cpu_selftest_set_register(REG_DOD, 0xABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
}

static void cpu_selftest_load_operand_internal_register_32(void)
{
	// TODO: Awaiting clarification on IR n=32 and n=36
	//cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 32);
	//cpu_selftest_set_register(REG_BOD, 0xAAAAAAA);
	//cpu_selftest_set_register(REG_B, 0xBBBBBBBB);
	//cpu_selftest_run_code();
	//cpu_selftest_assert_reg_equals(REG_A, 0xAAAAAAABBBBBBBB);
}

static void cpu_selftest_load_operand_internal_register_33(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 33);
	cpu_selftest_set_register(REG_BOD, 0xABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x00000000ABABABAB);
}

static void cpu_selftest_load_operand_internal_register_34(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 34);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0x0000000000000000);
}

static void cpu_selftest_load_operand_internal_register_48(void)
{
	cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_IR, 48);
	cpu_selftest_set_register(REG_AEX, 0xABABABABABABABAB);
	cpu_selftest_run_code();
	cpu_selftest_assert_reg_equals(REG_A, 0xABABABABABABABAB);
}

static void cpu_selftest_load_operand_32_bit_variable(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V32, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_32_bit_word(base + n, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
}

static void cpu_selftest_load_operand_32_bit_variable_6_bit_offset_is_unsigned(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x3F;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V32, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_32_bit_word(base + n, 0xAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0x00000000AAAAAAAA);
}

static void cpu_selftest_load_operand_64_bit_variable(void)
{
    uint32 base = 0x00F0;
    int8 n = 0x1;
    cpu_selftest_load_order(CR_FLOAT, F_LOAD_64, K_V64, n);
    cpu_selftest_set_register(REG_NB, base);
    sac_write_64_bit_word(base + 2*n, 0xBBBBBBBBAAAAAAAA);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_A, 0xBBBBBBBBAAAAAAAA);
}


// TODO: p17 SN relative
// TODO: p17 SN interrupt on overflow
// TODO: p17 long instruction offset is signed (?)

static void cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD()
{
    cpu_selftest_set_register(REG_XD, 0xAAAAAAAA00000000);
    cpu_selftest_load_order_extended(CR_STS1, F_LOAD_XDO, K_LITERAL, NP_64_BIT_LITERAL);
    cpu_selftest_load_64_bit_literal(0xBBBBBBBBFFFFFFFF);
    cpu_selftest_run_code();
    cpu_selftest_assert_reg_equals(REG_XD, 0xAAAAAAAAFFFFFFFF);
}

static void cpu_selftest_reset(UNITTEST *test)
{
    // TODO: Loop to reset all registers
	cpu_selftest_set_register(REG_A, 0x0);
	cpu_selftest_set_register(REG_AEX, 0x0);
	cpu_selftest_set_register(REG_B, 0x0);
	cpu_selftest_set_register(REG_BOD, 0x0);
	cpu_selftest_set_register(REG_D, 0x0);
	cpu_selftest_set_register(REG_XD, 0x0);
	cpu_selftest_set_register(REG_DT, 0x0);
	cpu_selftest_set_register(REG_XDT, 0x0);
	cpu_selftest_set_register(REG_DOD, 0x0);
	cpu_selftest_set_register(REG_NB, 0x0);
	cpu_selftest_set_register(REG_XNB, 0x0);
	cpu_selftest_set_register(REG_SN, 0x0);
	cpu_selftest_set_register(REG_SF, 0x0);

    testContext.testName = test->name;
    testContext.currentLoadLocation = 0;
    testContext.result = SCPE_OK;
}

static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n)
{
    uint16 order;
    
    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= (k & 0x7) << 6;
    order |= n & 0x3F;
    sac_write_16_bit_word(testContext.currentLoadLocation, order);
    testContext.currentLoadLocation += 1;
}

static void cpu_selftest_load_order_extended(uint8 cr, uint8 f, uint8 k, uint8 np)
{
    uint16 order;

    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= 0x7 << 6;
    order |= (k & 0x7) << 3;
    order |= np & 0x7;
    sac_write_16_bit_word(testContext.currentLoadLocation, order);
    testContext.currentLoadLocation += 2;
}

static void cpu_selftest_load_64_bit_literal(t_uint64 value)
{
    sac_write_64_bit_word(testContext.currentLoadLocation, value);
    testContext.currentLoadLocation += 8;
}

static void cpu_selftest_run_code(void)
{
    cpu_selftest_set_register("CO", 0);
	cpu_execute_next_order();
}

static REG *cpu_selftest_get_register(char *name)
{
    REG *rptr;
    for (rptr = cpu_dev.registers; rptr->name != NULL; rptr++)
    {
        if (strcmp(rptr->name, name) == 0)
        {
            break;
        }
    }

    if (rptr->name == NULL)
    {
        sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Could not find register %s\n", name);
        testContext.result = SCPE_AFAIL;
        rptr = NULL;
    }

    return rptr;
}

static void cpu_selftest_set_register(char *name, t_uint64 value)
{
    REG *reg = cpu_selftest_get_register(name);
    switch (reg->width)
    {
        case 16:
        {
            *(uint16 *)(reg->loc) = value & 0xFFFF;
            break;
        }
        case 32:
        {
            *(uint32 *)(reg->loc) = value & 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            *(t_uint64 *)(reg->loc) = value & 0xFFFFFFFFFFFFFFFF;
            break;
        }
        default:
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected register width %d for register %s\n", reg->width, name);
            testContext.result = SCPE_AFAIL;
            break;
        }
    }

}

static void cpu_selftest_assert_reg_equals(char *name, t_uint64 expectedValue)
{
    REG *reg = cpu_selftest_get_register(name);
    t_uint64 actualValue;
    t_uint64 mask;
    switch (reg->width)
    {
        case 16:
        {
            actualValue = *(uint16 *)(reg->loc);
            mask = 0xFFFF;
            break;
        }
        case 32:
        {
            actualValue = *(uint32 *)(reg->loc);
            mask = 0xFFFFFFFF;
            break;
        }
        case 64:
        {
            actualValue = *(t_uint64 *)(reg->loc);
            mask = 0xFFFFFFFFFFFFFFFF;
            break;
        }
        default:
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Unexpected register width %d for register %s\n", reg->width, name);
            testContext.result = SCPE_AFAIL;
            break;
        }
    }

    if (testContext.result == SCPE_OK)
    {
        if ((mask & actualValue) != (mask & expectedValue))
        {
            sim_debug(LOG_CPU_SELFTEST_FAIL, &cpu_dev, "Expected value in register %s to be %llX, but was %llX\n", name, mask & expectedValue, mask & actualValue);
            testContext.result = SCPE_AFAIL;
        }
    }
}

t_stat cpu_selftest(void)
{
    t_stat result = SCPE_OK;
    int n;
    int i;

    n = sizeof(tests) / sizeof(UNITTEST);

    for (i = 0; i < n; i++)
    {
        cpu_selftest_reset(&tests[i]);
        tests[i].runner();
        if (testContext.result == SCPE_OK)
        {
            sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%s [OK]\n", tests[i].name);
        }
        else
        {
            result = SCPE_AFAIL;
            sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%s [FAIL]\n", tests[i].name);
        }
    }

    return result;
}