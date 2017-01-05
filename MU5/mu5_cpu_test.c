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

typedef struct UNITTEST
{
    char * name;
    t_stat(*runner)(void);
} UNITTEST;

extern DEVICE cpu_dev;

static uint32 currentLoadLocation;

static void cpu_selftest_reset(void);
static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n);

static t_stat cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD(void);

UNITTEST tests[] =
{
    { "STS1 XDO Load Loads LS half of XD", cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD }
};


static t_stat cpu_selftest_sts1_xdo_load_loads_ls_half_of_XD()
{
    t_stat result = SCPE_AFAIL;
    return result;
}

static void cpu_selftest_reset(void)
{
    currentLoadLocation = 0;
}

static void cpu_selftest_load_order(uint8 cr, uint8 f, uint8 k, uint8 n)
{
    uint16 order;
    
    order = (cr & 0x7) << 13;
    order |= (f & 0xF) << 9;
    order |= (k & 0x7) << 6;
    order |= n & 0x3F;
    sac_write_16_bit_word(currentLoadLocation, order);
    currentLoadLocation += 2;
}

t_stat cpu_selftest(void)
{
    t_stat result = SCPE_OK;
    t_stat unitResult;
    int n;
    int i;

    n = sizeof(tests) / sizeof(UNITTEST);

    for (i = 0; i < n; i++)
    {
        unitResult = tests[i].runner();
        sim_debug(LOG_CPU_SELFTEST, &cpu_dev, "%s [%s]\n", tests[i].name, unitResult == SCPE_OK ? "OK" : "FAIL");
        if (unitResult != SCPE_OK)
        {
            result = SCPE_AFAIL;
        }
    }

    return result;
}