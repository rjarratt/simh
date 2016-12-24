/* mu5_cpu.c: MU5 CPU

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
ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Robert Jarratt shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from Robert Jarratt.
*/

#include "mu5_defs.h"

/* Debug flags */
#define LOG_CPU_PERF          (1 << 0)

int32 sim_emax;

/* Registers */
uint32 reg_co; /* CO Register */

UNIT cpu_unit =
{
	UDATA(NULL, UNIT_FIX | UNIT_BINK, 65536 /* TODO: memory size */)
};

static REG cpu_reg[] =
{
	{ HRDATAD(CO,      reg_co, 32, "program counter") },
	{ NULL }
};

REG *sim_PC = &cpu_reg[0];

static MTAB cpu_mod[] =
{
	NULL
};

/* Debug Flags */
static DEBTAB cpu_debtab[] =
{
	{ "PERF",    LOG_CPU_PERF,      "CPU performance" },
	{ "EVENT",   SIM_DBG_EVENT,     "event dispatch activities" },
	{ NULL,         0 }
};

static const char* cpu_description(DEVICE *dptr) {
	return "Central Processing Unit";
}

t_stat sim_instr(void);

static t_stat cpu_ex(t_value *vptr, t_addr addr, UNIT *uptr, int32 sw);
static t_stat cpu_dep(t_value val, t_addr addr, UNIT *uptr, int32 sw);
static t_stat cpu_reset(DEVICE *dptr);

DEVICE cpu_dev = {
	"CPU",            /* name */
	&cpu_unit,        /* units */
	cpu_reg,          /* registers */
	cpu_mod,          /* modifiers */
	1,                /* numunits */
	16,               /* aradix */
	16,               /* awidth */
	1,                /* aincr */
	16,               /* dradix */
	16,               /* dwidth */
	&cpu_ex,          /* examine */
	&cpu_dep,         /* deposit */
	&cpu_reset,       /* reset */
	NULL,             /* boot */
	NULL,             /* attach */
	NULL,             /* detach */
	NULL,             /* ctxt */
	DEV_DEBUG,        /* flags */
	0,                /* dctrl */
	cpu_debtab,       /* debflags */
	NULL,             /* msize */
	NULL,             /* lname */
	NULL,             /* help */
	NULL,             /* attach_help */
	NULL,             /* help_ctx */
	&cpu_description, /* description */
	NULL              /* brk_types */
};

t_stat sim_instr(void)
{
	t_stat reason = SCPE_OK;
	
	while (TRUE)
	{
		if (sim_interval <= 0)
		{
#if !UNIX_PLATFORM
			if ((reason = sim_poll_kbd()) == SCPE_STOP) {   /* poll on platforms without reliable signalling */
				break;
			}
#endif
			if ((reason = sim_process_event()) != SCPE_OK)
			{
				break;
			}
		}

		sim_interval--;
	}

	sim_debug(LOG_CPU_PERF, &cpu_dev, "CPU ran at %.1f MIPS\n", sim_timer_inst_per_sec()/1000000);

	return reason;
}

t_stat sim_load(FILE *ptr, CONST char *cptr, CONST char *fnam, int flag)
{
}

t_stat fprint_sym(FILE *ofile, t_addr addr, t_value *val, UNIT *uptr, int32 sw)
{
}

t_stat parse_sym(CONST char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
}

/* reset routine */
static t_stat cpu_reset(DEVICE *dptr)
{
	return SCPE_OK;
}

/* memory examine */
static t_stat cpu_ex(t_value *vptr, t_addr addr, UNIT *uptr, int32 sw)
{
	return SCPE_AFAIL;
}

/* memory deposit */
static t_stat cpu_dep(t_value val, t_addr addr, UNIT *uptr, int32 sw)
{
	return SCPE_AFAIL;
}