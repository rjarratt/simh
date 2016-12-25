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
#include "mu5_sac.h"

/* Debug flags */
#define LOG_CPU_PERF          (1 << 0)
#define LOG_CPU_DECODE        (1 << 1)

/* This structure is used to allow instruction execution to be table driven. It allows for tables to be nested. At each level the
   execute() function does anything it needs to do before invoking the function on the inner table. The leaf tables have a NULL for
   the inner table.
*/
typedef struct DISPATCH_ENTRY
{
	void (*execute)(uint16 order, struct DISPATCH_ENTRY *innerTable);
	struct DISPATCH_ENTRY *innerTable;
} DISPATCH_ENTRY;


int32 sim_emax;

/* Registers */
uint32 reg_co; /* CO Register */
t_uint64 reg_a; /* A register */

UNIT cpu_unit =
{
	UDATA(NULL, UNIT_FIX | UNIT_BINK, MAXMEMORY)
};

static REG cpu_reg[] =
{
	{ HRDATAD(CO,      reg_co, 32, "program counter") },
	{ HRDATAD(A,       reg_a, 64,  "A register") },
	{ NULL }
};

static uint8 interrupt;
REG *sim_PC = &cpu_reg[0];

static MTAB cpu_mod[] =
{
	{ 0 }
};

/* Debug Flags */
static DEBTAB cpu_debtab[] =
{
	{ "PERF",    LOG_CPU_PERF,      "CPU performance" },
	{ "EVENT",   SIM_DBG_EVENT,     "event dispatch activities" },
	{ "DECODE",  LOG_CPU_DECODE,    "decode instructions" },
	{ NULL,         0 }
};

static const char* cpu_description(DEVICE *dptr) {
	return "Central Processing Unit";
}

t_stat sim_instr(void);

static t_stat cpu_ex(t_value *vptr, t_addr addr, UNIT *uptr, int32 sw);
static t_stat cpu_dep(t_value val, t_addr addr, UNIT *uptr, int32 sw);
static t_stat cpu_reset(DEVICE *dptr);

static void cpu_set_interrupt(uint8 number);
static void cpu_clear_interrupt(uint8 number);
static uint8 cpu_get_interrupt_number(void);
static uint16 cpu_get_cr(uint16 order);
static uint16 cpu_get_f(uint16 order);
static t_uint64 cpu_get_operand(uint16 order);
static void cpu_execute_next_order(void);
static void cpu_execute_illegal_order(uint16 order);
static void cpu_start_interrupt_processing(void);

/* cr functions */
static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable);

/* organisational order functions */
static void cpu_execute_organisational_jump(uint16 order);

/* acc fixed order functions */
static void cpu_execute_acc_fixed_add(uint16 order);

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

static DISPATCH_ENTRY organisationalDispatchTable[] =
{
	{ cpu_execute_illegal_order, NULL },         /* 0 */
	{ cpu_execute_illegal_order, NULL },         /* 1 */
	{ cpu_execute_illegal_order, NULL },         /* 2 */
	{ cpu_execute_illegal_order, NULL },         /* 3 */
	{ cpu_execute_organisational_jump, NULL },   /* 4 */
	{ cpu_execute_illegal_order, NULL },         /* 5 */
	{ cpu_execute_illegal_order, NULL },         /* 6 */
	{ cpu_execute_illegal_order, NULL },         /* 7 */
	{ cpu_execute_illegal_order, NULL },         /* 8 */
	{ cpu_execute_illegal_order, NULL },         /* 9 */
	{ cpu_execute_illegal_order, NULL },         /* 10 */
	{ cpu_execute_illegal_order, NULL },         /* 11*/
	{ cpu_execute_illegal_order, NULL },         /* 12 */
	{ cpu_execute_illegal_order, NULL },         /* 13 */
	{ cpu_execute_illegal_order, NULL },         /* 14 */
	{ cpu_execute_illegal_order, NULL },         /* 15 */
	{ cpu_execute_illegal_order, NULL },         /* 16 */
	{ cpu_execute_illegal_order, NULL },         /* 17 */
	{ cpu_execute_illegal_order, NULL },         /* 18 */
	{ cpu_execute_illegal_order, NULL },         /* 19 */
	{ cpu_execute_illegal_order, NULL },         /* 20 */
	{ cpu_execute_illegal_order, NULL },         /* 21 */
	{ cpu_execute_illegal_order, NULL },         /* 22 */
	{ cpu_execute_illegal_order, NULL },         /* 23 */
	{ cpu_execute_illegal_order, NULL },         /* 24 */
	{ cpu_execute_illegal_order, NULL },         /* 25 */
	{ cpu_execute_illegal_order, NULL },         /* 26 */
	{ cpu_execute_illegal_order, NULL },         /* 27 */
	{ cpu_execute_illegal_order, NULL },         /* 28 */
	{ cpu_execute_illegal_order, NULL },         /* 29 */
	{ cpu_execute_illegal_order, NULL },         /* 30 */
	{ cpu_execute_illegal_order, NULL },         /* 31 */
	{ cpu_execute_illegal_order, NULL },         /* 32 */
	{ cpu_execute_illegal_order, NULL },         /* 33 */
	{ cpu_execute_illegal_order, NULL },         /* 34 */
	{ cpu_execute_illegal_order, NULL },         /* 35 */
	{ cpu_execute_illegal_order, NULL },         /* 36 */
	{ cpu_execute_illegal_order, NULL },         /* 37 */
	{ cpu_execute_illegal_order, NULL },         /* 38 */
	{ cpu_execute_illegal_order, NULL },         /* 39 */
	{ cpu_execute_illegal_order, NULL },         /* 40 */
	{ cpu_execute_illegal_order, NULL },         /* 41 */
	{ cpu_execute_illegal_order, NULL },         /* 42 */
	{ cpu_execute_illegal_order, NULL },         /* 43 */
	{ cpu_execute_illegal_order, NULL },         /* 44 */
	{ cpu_execute_illegal_order, NULL },         /* 45 */
	{ cpu_execute_illegal_order, NULL },         /* 46 */
	{ cpu_execute_illegal_order, NULL },         /* 47 */
	{ cpu_execute_illegal_order, NULL },         /* 48 */
	{ cpu_execute_illegal_order, NULL },         /* 49 */
	{ cpu_execute_illegal_order, NULL },         /* 50 */
	{ cpu_execute_illegal_order, NULL },         /* 51 */
	{ cpu_execute_illegal_order, NULL },         /* 52 */
	{ cpu_execute_illegal_order, NULL },         /* 53 */
	{ cpu_execute_illegal_order, NULL },         /* 54 */
	{ cpu_execute_illegal_order, NULL },         /* 55 */
	{ cpu_execute_illegal_order, NULL },         /* 56 */
	{ cpu_execute_illegal_order, NULL },         /* 57 */
	{ cpu_execute_illegal_order, NULL },         /* 58 */
	{ cpu_execute_illegal_order, NULL },         /* 59 */
	{ cpu_execute_illegal_order, NULL },         /* 60 */
	{ cpu_execute_illegal_order, NULL },         /* 61 */
	{ cpu_execute_illegal_order, NULL },         /* 62 */
	{ cpu_execute_illegal_order, NULL }          /* 63 */
};

static DISPATCH_ENTRY accFixedDispatchTable[] =
{
	{ cpu_execute_illegal_order, NULL },   /* 0 */
	{ cpu_execute_illegal_order, NULL },   /* 1 */
	{ cpu_execute_illegal_order, NULL },   /* 2 */
	{ cpu_execute_illegal_order, NULL },   /* 3 */
	{ cpu_execute_acc_fixed_add, NULL },   /* 4 */
	{ cpu_execute_illegal_order, NULL },   /* 5 */
	{ cpu_execute_illegal_order, NULL },   /* 6 */
	{ cpu_execute_illegal_order, NULL },   /* 7 */
	{ cpu_execute_illegal_order, NULL },   /* 8 */
	{ cpu_execute_illegal_order, NULL },   /* 9 */
	{ cpu_execute_illegal_order, NULL },   /* 10 */
	{ cpu_execute_illegal_order, NULL },   /* 11*/
	{ cpu_execute_illegal_order, NULL },   /* 12 */
	{ cpu_execute_illegal_order, NULL },   /* 13 */
	{ cpu_execute_illegal_order, NULL },   /* 14 */
	{ cpu_execute_illegal_order, NULL },   /* 15 */
};

static DISPATCH_ENTRY crDispatchTable[] =
{
	{ cpu_execute_cr_level, organisationalDispatchTable },                  /* 0 */
	{ cpu_execute_cr_level, NULL },                  /* 1 */
	{ cpu_execute_cr_level, NULL },                  /* 2 */
	{ cpu_execute_cr_level, NULL },                  /* 3 */
	{ cpu_execute_cr_level, NULL },                  /* 4 */
	{ cpu_execute_cr_level, accFixedDispatchTable }, /* 5 */
	{ cpu_execute_cr_level, NULL },                  /* 6 */
	{ cpu_execute_cr_level, NULL }                   /* 7 */
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

		cpu_execute_next_order();

		sim_interval--;
	}

	sim_debug(LOG_CPU_PERF, &cpu_dev, "CPU ran at %.1f MIPS\n", sim_timer_inst_per_sec() / 1000000);

	return reason;
}

/* file loaded is a sequence of 16-bit words */
t_stat sim_load(FILE *ptr, CONST char *cptr, CONST char *fnam, int flag)
{
	t_stat r = SCPE_OK;
	int b;
	uint16 word;
	int msb;
	t_addr origin, limit;

	if (flag) /* dump? */
	{
		r = sim_messagef(SCPE_NOFNC, "Command Not Implemented\n");
	}
	else
	{
		origin = 0;
		limit = (t_addr)cpu_unit.capac * 2;
		if (sim_switches & SWMASK('O')) /* Origin option */
		{
			origin = (t_addr)get_uint(cptr, 16, limit, &r);
			if (r != SCPE_OK)
			{
				r = SCPE_ARG;
			}
		}
	}

	if (r == SCPE_OK)
	{
		msb = 1;
		while ((b = Fgetc(ptr)) != EOF)
		{
			if (origin >= limit)
			{
				r = SCPE_NXM;
				break;
			}

			if (msb)
			{
				word = b << 8;
			}
			else
			{
				word |= b;
				sac_write_16_bit_word(origin, word);
				origin = origin + 1;
			}

			msb = !msb;
		}
	}

	return r;
}

t_stat fprint_sym(FILE *ofile, t_addr addr, t_value *val, UNIT *uptr, int32 sw)
{
	return SCPE_NOFNC;
}

t_stat parse_sym(CONST char *cptr, t_addr addr, UNIT *uptr, t_value *val, int32 sw)
{
	return SCPE_NOFNC;
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

static void cpu_set_interrupt(uint8 number)
{
	interrupt |= 1u << number;
}

static void cpu_clear_interrupt(uint8 number)
{
	interrupt &= ~(1u << number);
}

static uint8 cpu_get_interrupt_number(void)
{
	uint8 i;
	uint8 result = 255;

	for (i = 0; i < 7; i++)
	{
		if (interrupt & (1u << i))
		{
			result = i;
			break;
		}
	}

	return result;
}

static uint16 cpu_get_cr(uint16 order)
{
	uint16 cr = (order >> 13) & 0x7;
	return cr;
}

static uint16 cpu_get_f(uint16 order)
{
	uint16 f;
	uint16 cr = cpu_get_cr(order);
	if (cr == 0)
	{
		f = (order >> 7) & 0x3F;
	}
	else
	{
		f = (order >> 9) & 0xF;
	}

	return f;
}

static t_uint64 cpu_get_operand(uint16 order)
{
	t_uint64 result = 0;
	uint16 k = (order >> 6) & 0x7;

	switch (k)
	{
		case 0:
		{
			result = order & 0x7F;
			result |= (result & 0x40) ? -1 : 0; /* sign extend */
			sim_debug(LOG_CPU_DECODE, &cpu_dev, "LITERAL %lld\n", result);
			break;
		}
	}

	reg_co++; /* TODO: move to handle variable length operands */

	return result;
}

static void cpu_execute_next_order(void)
{
	uint16 order;
	uint16 cr;

	if (interrupt == 0)
	{
		order = sac_read_16_bit_word(reg_co);
		cr = cpu_get_cr(order);

		crDispatchTable[cr].execute(order, crDispatchTable[cr].innerTable);
	}
	else
	{
		cpu_start_interrupt_processing();
	}
}

static void cpu_execute_illegal_order(uint16 order)
{
	cpu_set_interrupt(INT_ILLEGAL_ORDERS);
}

static void cpu_start_interrupt_processing(void)
{
	printf("Interrupt %bu detected - processing TBD\n", cpu_get_interrupt_number());
}

static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable)
{
	uint16 f = cpu_get_f(order);
	innerTable[f].execute(order, innerTable[f].innerTable);
}

static void cpu_execute_acc_fixed_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "A+ ");
	reg_a += cpu_get_operand(order);
}

static void cpu_execute_organisational_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "JUMP ");
	reg_co = cpu_get_operand(order) & 0x7FFF;
}
