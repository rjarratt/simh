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

typedef struct TESTCONTEXT
{
    char *testName;
    uint32 countSuccessful;
    uint32 countFailed;
    t_stat result;
    t_stat overallResult;
} TESTCONTEXT;

typedef struct UNITTEST
{
    char * name;
    void(*runner)(void);
} UNITTEST;

void mu5_selftest_start(TESTCONTEXT *context);
void mu5_selftest_run_suite(TESTCONTEXT *context, UNITTEST *unitTests, uint32 numberOfUnitTests, void (*reset)(UNITTEST *unitTest));
t_stat mu5_selftest_end(TESTCONTEXT *context);

