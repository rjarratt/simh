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

/* SAC first because it resets the V-Store callbacks which may be set by other devices */
DEVICE *sim_devices[] = { &sac_dev, &cpu_dev, &console_dev, NULL };

const char *sim_stop_messages[] =
{
    "Terminated"
};

static void VMInit(void);

void(*sim_vm_init)(void) = &VMInit;


CTAB mu5_cmd[] = {
    { NULL }
};

static void VMInit()
{
    sim_vm_cmd = mu5_cmd;
	sim_dflt_dev = &cpu_dev;
}




