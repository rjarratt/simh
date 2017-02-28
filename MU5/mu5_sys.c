/* mu5_sys.c: MU5 system interface

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

char sim_name[] = "MU5";

extern DEVICE cpu_dev;
extern DEVICE sac_dev;
extern DEVICE console_dev;

DEVICE *sim_devices[] = { &cpu_dev, &sac_dev, &console_dev, NULL };

const char *sim_stop_messages[] =
{
    "Terminated"
};

static void VMInit(void);
static t_stat mu5_vstore_cmd(int32 flag, CONST char *ptr);

void(*sim_vm_init)(void) = &VMInit;


CTAB mu5_cmd[] = {
    { "VSTORE", &mu5_vstore_cmd, 0, "vs{tore}                   V-Store access\n", NULL, NULL },
    { NULL }
};

static void VMInit()
{
    sim_vm_cmd = mu5_cmd;
}

static t_stat mu5_vstore_cmd(int32 flag, CONST char *ptr)
{
    char gbuf[CBUFSIZE];
    t_stat result = SCPE_OK;
    uint8 block;
    uint8 line;
    t_uint64 value;

    ptr = get_glyph(ptr, gbuf, 0);
    if (gbuf[0] && strcmp(gbuf, "SET"))
    {
        result = SCPE_ARG;
    }
    else
    {
        ptr = get_glyph(ptr, gbuf, 0);
        block = (uint8)get_uint(gbuf, 16, 7, &result);
        if (result == SCPE_OK)
        {
            ptr = get_glyph(ptr, gbuf, 0);
            line = (uint8)get_uint(gbuf, 16, 255, &result);
        }
        if (result == SCPE_OK)
        {
            ptr = get_glyph(ptr, gbuf, 0);
            value = (t_uint64)get_uint(gbuf, 16, 0xFFFFFFFFFFFFFFFF, &result);
        }

        if (result == SCPE_OK)
        {
            sac_write_v_store(block, line, value);
        }
    }
    return result;
}



