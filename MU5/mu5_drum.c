/* mu5_sac.c: MU5 Fixed Head Disk Device Unit

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

This is the MU5 Store Access Control unit. The Local Store consisted of four
4096-word memory units, each word containing 64 data bits + 8 parity bits.
The Mass Store consisted of two 128K-word memory units, each word containing
36 bits. The Fixed-head Disc consisted of two 2.4 Mbyte units.

There are believed to be 4 hard-wired CPRs. When asked in April 2017 RNI was
not sure which ones and did not know their values. He said: "Assume 4 for now
and assume they are numbers 28-31. Writing to them would have no effect."

Notes on V and Vx Store Access
------------------------------




Known Limitations
-----------------

*/

#include <assert.h>
#include "sim_defs.h"
#include "sim_disk.h"
#include "mu5_defs.h"
#include "mu5_drum.h"

static t_stat drum_reset(DEVICE *dptr);
t_stat drum_svc(UNIT *uptr);
t_stat drum_attach(UNIT *uptr, CONST char *cptr);
t_stat drum_detach(UNIT *uptr);

static UNIT drum_unit[] =
{
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) },
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) },
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) },
	{ UDATA(&drum_svc, UNIT_FIX | UNIT_BINK | UNIT_ATTABLE | UNIT_DISABLE, DRUM_BLOCKS_PER_BAND * DRUM_WORDS_PER_BLOCK * DRUM_BANDS_PER_UNIT) }
};

static REG drum_reg[] =
{
	{ NULL }
};

static MTAB drum_mod[] =
{
	{ 0 }
};

static DEBTAB drum_debtab[] =
{
	{ NULL,           0 }
};

static const char* drum_description(DEVICE *dptr) {
	return "Fixed-Head Disc Store Unit";
}

DEVICE drum_dev = {
	"DRUM",            /* name */
	drum_unit,         /* units */
	drum_reg,          /* registers */
	drum_mod,          /* modifiers */
	DRUM_NUM_UNITS,    /* numunits */
	16,                /* aradix */
	32,                /* awidth */
	4,                 /* aincr */
	16,                /* dradix */
	32,                /* dwidth */
	NULL,              /* examine */
	NULL,              /* deposit */
	&drum_reset,       /* reset */
	NULL,              /* boot */
	&drum_attach,      /* attach */
	&drum_detach,      /* detach */
	NULL,              /* ctxt */
	DEV_DEBUG | DEV_DISK,         /* flags */
	0,                 /* dctrl */
	drum_debtab,       /* debflags */
	NULL,              /* msize */
	NULL,              /* lname */
	NULL,              /* help */
	NULL,              /* attach_help */
	NULL,              /* help_ctx */
	&drum_description, /* description */
	NULL               /* brk_types */
};

/* reset routine */
static t_stat drum_reset(DEVICE *dptr)
{
	t_stat result = SCPE_OK;
	return result;
}

static t_stat drum_svc(UNIT *uptr)
{
	t_stat result = SCPE_OK;
	return result;
}

t_stat drum_attach(UNIT *uptr, CONST char *cptr)
{
	t_stat r;
	size_t xferElementSize = drum_dev.dwidth / 8;
	size_t sectorSizeBytes = DRUM_WORDS_PER_BLOCK * drum_dev.aincr * xferElementSize;

	r = sim_disk_attach(uptr, cptr, sectorSizeBytes, xferElementSize, 1, 0, "DRUM", 0, 0);
	return r;
}

t_stat drum_detach(UNIT *uptr)
{
	t_stat r;

	r = sim_disk_detach(uptr);                             /* detach unit */
	return r;
}
