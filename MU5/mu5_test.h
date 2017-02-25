/* mu5_test.h: MU5 self test support

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

#pragma once

#include "mu5_defs.h"

#define TEST_V_STORE_LOCATION_BLOCK 7
#define TEST_V_STORE_LOCATION_LINE 255

#define VA(P,S,X) ((P <<26 ) | (S << 12) | X)
#define RA(AC,A,LZ) ((AC << 28) | (A << 4) | LZ)

typedef struct TESTCONTEXT
{
    DEVICE *dev;
    char *testName;
    uint32 countSuccessful;
    uint32 countFailed;
    t_stat result;
    t_stat overallResult;
} TESTCONTEXT;

typedef struct UNITTEST
{
    char * name;
    void(*runner)(TESTCONTEXT *context);
} UNITTEST;

extern t_uint64 VStoreTestLocation;

void mu5_selftest_start(TESTCONTEXT *context);
void mu5_selftest_run_suite(TESTCONTEXT *context, UNITTEST *unitTests, uint32 numberOfUnitTests, void (*reset)(UNITTEST *unitTest));
t_stat mu5_selftest_end(TESTCONTEXT *context);

void mu5_selftest_set_executive_mode(TESTCONTEXT *context, DEVICE *device);
void mu5_selftest_set_user_mode(TESTCONTEXT *context, DEVICE *device);
void mu5_selftest_set_bcpr(TESTCONTEXT *context, DEVICE *device);
void mu5_selftest_clear_bcpr(TESTCONTEXT *context, DEVICE *device);
void mu5_selftest_setup_cpr(uint8 cprNumber, uint32 va, uint32 ra);

REG *mu5_selftest_find_register(TESTCONTEXT *context, DEVICE *device, char *name);
t_uint64 mu5_selftest_get_register(TESTCONTEXT *context, DEVICE *device, char *name);
t_uint64 mu5_selftest_get_register_instance(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index);
void mu5_selftest_set_register(TESTCONTEXT *context, DEVICE *device, char *name, t_uint64 value);
void mu5_selftest_set_register_instance(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index, t_uint64 value);

void mu5_selftest_assert_fail(TESTCONTEXT *context);
void mu5_selftest_set_failure(TESTCONTEXT *context);
void mu5_selftest_assert_reg_equals(TESTCONTEXT *context, DEVICE *device, char *name, t_uint64 expectedValue);
void mu5_selftest_assert_reg_instance_equals(TESTCONTEXT *context, DEVICE *device, char *name, uint8 index, t_uint64 expectedValue);
void mu5_selftest_assert_no_interrupt(TESTCONTEXT *context);
void mu5_selftest_assert_interrupt_number(TESTCONTEXT *context, int expectedInterruptNumber);
void mu5_selftest_assert_operand_access_violation(TESTCONTEXT *context);
void mu5_selftest_assert_instruction_access_violation(TESTCONTEXT *context);
void mu5_selftest_assert_vstore_contents(TESTCONTEXT *context, uint8 block, uint8 line, t_uint64 expectedValue);

t_uint64 mu5_selftest_read_callback_for_static_64_bit_location(void);
void mu5_selftest_write_callback_for_static_64_bit_location(t_uint64 value);
