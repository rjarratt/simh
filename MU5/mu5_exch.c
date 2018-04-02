/* mu5_exch.c: MU5 Exchange Unit

Copyright (c) 2016-2018, Robert Jarratt

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

This is the MU5 Exchange unit. It is not a SIMH device because it was not
considered a unit in MU5 (see the Block 5 description in Section 9.4 of the
Basic Programming Manual) and its state was accessed through the BTU.
Furthermore, because the emulator is synchronous the Exchange can be
emulated as a passive unit and does not need a service routine, so again a
device is not needed.

Addresses presented to the Exchange are all real addresses in 32-bit units.
The format is as per Fig. 6.10(b) on p128 of the book, and further explained
on p134 of the book, namely:

   4  1            23
+----+-+-----------------------+
+UNIT|V|        ADDRESS        |
+----+-+-----------------------+



*/

#include <assert.h>
#include "sim_defs.h"
#include "sim_disk.h"
#include "mu5_defs.h"
#include "mu5_sac.h"
#include "mu5_drum.h"

static int8 exch_get_unit(t_addr addr);
static t_addr exch_get_unit_address(t_addr addr);

t_uint64 exch_read(t_addr addr)
{
	t_uint64 result = 0;
	int8 unit = exch_get_unit(addr);
	t_addr unit_addr = exch_get_unit_address(addr);
	switch (unit)
	{
		case UNIT_FIXED_HEAD_DISC:
		{
			break;
		}

		case UNIT_LOCAL_STORE:
		{
			break;
		}

		case UNIT_MASS_STORE:
		{
			break;
		}

		default:
		{
			//sim_debug(LOG_ERROR, &btu_dev, "Read unknown (%hhu) store real address %06X\n", unit, address);
			break;
		}
	}

	return result;
}

void exch_write(t_addr addr, t_uint64 value)
{
	int8 unit = exch_get_unit(addr);
	t_addr unit_addr = exch_get_unit_address(addr);
	switch (unit)
	{
		case UNIT_FIXED_HEAD_DISC:
		{
			break;
		}

		case UNIT_LOCAL_STORE:
		{
			break;
		}

		case UNIT_MASS_STORE:
		{
			break;
		}

		default:
		{
			//sim_debug(LOG_ERROR, &btu_dev, "Write unknown (%hhu) store real address %06X, value=%08X\n", unit, address, value);
			break;
		}
	}
}

static int8 exch_get_unit(t_addr addr)
{
	return (addr >> 24) & 0xF;
}

static t_addr exch_get_unit_address(t_addr addr)
{
	return addr & 0xFFFFFF;
}

