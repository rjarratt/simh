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
ROBERT JARRATT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Robert Jarratt shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from Robert Jarratt.

Known Limitations
Z register is not implemented.

*/

#include <assert.h>
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

/* The CPU did not have a STOP instruction. For the purposes of emulation I want to be able to stop it anyway so the emulation stops.
This is what RNI told me:

It wasn't (stopped), otherwise it wouldn't have been able to respond to
interrupts. Instead, it did a "loop stop". An instruction made up
entirely of 0's is a relative jump by zero, i.e. it has no effect other
than to keep the CPU running. I had to invent a STOP instruction for my
HASE simulation model. Of course MU5 itself could be stopped from the
console by pressing the reset button. Being an asynchronous machine, it
often stopped during commissioning, so the restart procedure was to
press the "reset", "interrupt" and "go" buttons. The "go" button
inserted a pulse into the PROP timing mechanism. The interrupt in this
case was the "engineer's interrupt", which had the highest priority and
caused two fixed instructions wired into the Instruction Buffer Unit to
be executed. I can't remember the details but they resulted in an
absolute jump to the start of the OS code. The first thing this did was
to set the MS register to some appropriate setting.

*/
static int cpu_stopped = 0;

int32 sim_emax;

UNIT cpu_unit =
{
	UDATA(NULL, UNIT_FIX | UNIT_BINK, MAXMEMORY)
};
BITFIELD aod_bits[] = {
	BIT(DBLLEN),  /* Double length +/- */
	BIT(IROUND),  /* Inhibit rounding */
	BIT(ZDIV),    /* Zero divide indicator */
	BIT(DECOVF),  /* Decimal overflow indicator */
	BIT(FXPOVF),  /* Fixed point overflow indicator */
	BIT(FLPUNF),  /* Floating point underflow indicator */
	BIT(FLPOVF),  /* Floating point overflow indicator */
	BIT(IZDIV),   /* Inhibit zero divide interrupt */
	BIT(IDECOVF), /* Inhibit decimal overflow interrupt */
	BIT(IFXPOVF), /* Inhibit fixed point overflow interrupt */
	BIT(IFLPUNF), /* Inhibit floating point underflow interrupt */
	BIT(IFLPOVF), /* Inhibit floating point overflow interrupt */
	BIT(OPSIZ64), /* Operand size (0/1 meaning 32/64 bits */
	BITNCF(51),    
	ENDBITS
};

static t_uint64 mask_aod_opsiz64 = 0xFFFFFFFFFFFFEFFF;

BITFIELD ms_bits[] = {
	BIT(L0IF),    /* Level 0 interrupt flip-flop */
	BIT(L1IF),    /* Level 1 interrupt flip-flop */
	BIT(EXEC),    /* Exec Mode flip-flop */
	BIT(AF),      /* A faults to System Error in Exec Mode */
	BIT(BDF),     /* B and D faults to System Error in Exec Mode */
	BIT(ICI),     /* Instruction counter inhibit */
	BIT(BNS),     /* Bypass name store */
	BIT(BCPR),    /* Bypass CPRs */
				  
	BIT(BN),      /* Boolean */
	BIT(T2),      /* T2 - less than 0 */
	BIT(T1),      /* T1 - not equal 0 */
	BIT(T0),      /* T0 - overflow */
	BITNCF(2),    /* Spare in section 6, SPM & Spare in section 7 */
	BIT(IPF),     /* Inhibit program fault interrupts */
	BIT(DS),      /* Force DR instead of S */
	ENDBITS
};

static uint16 mask_ms_bn = 0xFFFFFEFF;

/* Register backing values */
static uint32 reg_b_backing_value;     /* B register */
static uint32 reg_bod_backing_value;   /* BOD register */
static t_uint64 reg_a_backing_value;   /* A register */
static t_uint64 reg_aod_backing_value; /* AOD register */
static t_uint64 reg_aex_backing_value; /* AEX register */
static uint32 reg_x_backing_value;     /* X register */
static uint16 reg_ms_backing_value;    /* MS register */
static uint16 reg_nb_backing_value;    /* NB register */
static uint32 reg_xnb_backing_value;   /* XNB register */
static uint16 reg_sn_backing_value;    /* SN register */
static uint16 reg_sf_backing_value;    /* SF register */
static uint32 reg_co_backing_value;    /* CO Register */
static t_uint64 reg_d_backing_value;   /* D Register */
static t_uint64 reg_xd_backing_value;  /* XD Register */
static uint32 reg_dod_backing_value;   /* DOD Register */
static uint32 reg_dt_backing_value;    /* DT Register */
static uint32 reg_xdt_backing_value;   /* XDT Register */

static REG cpu_reg[] =
{
	{ HRDATAD(B,    reg_b_backing_value,       32,    "B register") },
	{ HRDATAD(BOD,  reg_bod_backing_value,     32,    "BOD register") },
	{ HRDATAD(A,    reg_a_backing_value,       64,    "Accumulator") },
	{ GRDATADF(AOD, reg_aod_backing_value, 16, 64, 0, "AOD register", aod_bits) },
	{ HRDATAD(AEX,  reg_aex_backing_value,     64,    "Accumulator extension") },
	{ HRDATAD(X,    reg_x_backing_value,       32,    "X register") },
	{ GRDATADF(MS,  reg_ms_backing_value, 16,  16, 0, "Machine status register", ms_bits) },
	{ HRDATAD(NB,   reg_nb_backing_value,      16,    "Name Base register") },
	{ HRDATAD(XNB,  reg_xnb_backing_value,     32,    "X register") },
	{ HRDATAD(SN,   reg_sn_backing_value,      16,    "Name Segment Number register") },
	{ HRDATAD(SF,   reg_sf_backing_value,      16,    "Stack Front register") },
	{ HRDATAD(CO,   reg_co_backing_value,      32,    "Program counter") },
	{ HRDATAD(D,    reg_d_backing_value,       64,    "Data descriptor register") },
	{ HRDATAD(XD,   reg_xd_backing_value,      64,    "XD register") },
	{ HRDATAD(DOD,  reg_dod_backing_value,     32,    "DOD register") },
	{ HRDATAD(DT,   reg_dt_backing_value,      32,    "DT register") },
	{ HRDATAD(XDT,  reg_xdt_backing_value,     32,    "XDT register") },
	{ NULL }
};

REG *reg_b    = &cpu_reg[0];
REG *reg_bod  = &cpu_reg[1];
REG *reg_a	  = &cpu_reg[2];
REG *reg_aod  = &cpu_reg[3];
REG *reg_aex  = &cpu_reg[4];
REG *reg_x	  = &cpu_reg[5];
REG *reg_ms	  = &cpu_reg[6];
REG *reg_nb	  = &cpu_reg[7];
REG *reg_xnb  = &cpu_reg[8];
REG *reg_sn	  = &cpu_reg[9];
REG *reg_sf	  = &cpu_reg[10];
REG *reg_co	  = &cpu_reg[11];
REG *reg_d	  = &cpu_reg[12];
REG *reg_xd	  = &cpu_reg[13];
REG *reg_dod  = &cpu_reg[14];
REG *reg_dt	  = &cpu_reg[15];
REG *reg_xdt  = &cpu_reg[16];

static uint8 interrupt;
REG *sim_PC = &cpu_reg[11];

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

/* SCP provides get_rval and put_rval, but these are too slow */
static SIM_INLINE void cpu_set_register_16(REG *reg, uint16 value);
static SIM_INLINE void cpu_set_register_32(REG *reg, uint32 value);
static SIM_INLINE void cpu_set_register_64(REG *reg, t_uint64 value);
static SIM_INLINE void cpu_set_register_bit_64(REG *reg, t_uint64 mask, uint8 value);
static SIM_INLINE uint16 cpu_get_register_16(REG *reg);
static SIM_INLINE uint32 cpu_get_register_32(REG *reg);
static SIM_INLINE t_uint64 cpu_get_register_64(REG *reg);
static SIM_INLINE t_uint64 cpu_get_register_bit_16(REG *reg, uint16 mask);

static void cpu_set_interrupt(uint8 number);
static void cpu_clear_interrupt(uint8 number);
static uint8 cpu_get_interrupt_number(void);
static uint16 cpu_get_cr(uint16 order);
static uint16 cpu_get_f(uint16 order);
static uint16 cpu_get_k(uint16 order);
static t_uint64 cpu_get_operand_6_bit_literal(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_extended_literal(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand_internal_register(uint16 order, uint32 instructionAddress, int *instructionLength);
static t_uint64 cpu_get_operand(uint16 order);
static void cpu_execute_next_order(void);
static void cpu_execute_illegal_order(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_start_interrupt_processing(void);

/* cr functions */
static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable);

/* organisational order functions */
static void cpu_execute_organisational_relative_jump(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_absolute_jump(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_organisational_NB_load(uint16 order, DISPATCH_ENTRY *innerTable);

/* acc fixed order functions */
static void cpu_execute_acc_fixed_add(uint16 order, DISPATCH_ENTRY *innerTable);

/* floating point order functions */
static void cpu_execute_flp_load_single(uint16 order, DISPATCH_ENTRY *innerTable);
static void cpu_execute_flp_load_double(uint16 order, DISPATCH_ENTRY *innerTable);

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
	{ cpu_execute_organisational_relative_jump, NULL }, /* 0 */
	{ cpu_execute_illegal_order,                NULL }, /* 1 */
	{ cpu_execute_illegal_order,                NULL }, /* 2 */
	{ cpu_execute_illegal_order,                NULL }, /* 3 */
	{ cpu_execute_organisational_absolute_jump, NULL }, /* 4 */
	{ cpu_execute_illegal_order,                NULL }, /* 5 */
	{ cpu_execute_illegal_order,                NULL }, /* 6 */
	{ cpu_execute_illegal_order,                NULL }, /* 7 */
	{ cpu_execute_illegal_order,                NULL }, /* 8 */
	{ cpu_execute_illegal_order,                NULL }, /* 9 */
	{ cpu_execute_illegal_order,                NULL }, /* 10 */
	{ cpu_execute_illegal_order,                NULL }, /* 11*/
	{ cpu_execute_illegal_order,                NULL }, /* 12 */
	{ cpu_execute_illegal_order,                NULL }, /* 13 */
	{ cpu_execute_illegal_order,                NULL }, /* 14 */
	{ cpu_execute_illegal_order,                NULL }, /* 15 */
	{ cpu_execute_illegal_order,                NULL }, /* 16 */
	{ cpu_execute_illegal_order,                NULL }, /* 17 */
	{ cpu_execute_illegal_order,                NULL }, /* 18 */
	{ cpu_execute_illegal_order,                NULL }, /* 19 */
	{ cpu_execute_illegal_order,                NULL }, /* 20 */
	{ cpu_execute_illegal_order,                NULL }, /* 21 */
	{ cpu_execute_illegal_order,                NULL }, /* 22 */
	{ cpu_execute_illegal_order,                NULL }, /* 23 */
	{ cpu_execute_illegal_order,                NULL }, /* 24 */
	{ cpu_execute_illegal_order,                NULL }, /* 25 */
	{ cpu_execute_illegal_order,                NULL }, /* 26 */
	{ cpu_execute_illegal_order,                NULL }, /* 27 */
	{ cpu_execute_organisational_NB_load,       NULL }, /* 28 */
	{ cpu_execute_illegal_order,                NULL }, /* 29 */
	{ cpu_execute_illegal_order,                NULL }, /* 30 */
	{ cpu_execute_illegal_order,                NULL }, /* 31 */
	{ cpu_execute_illegal_order,                NULL }, /* 32 */
	{ cpu_execute_illegal_order,                NULL }, /* 33 */
	{ cpu_execute_illegal_order,                NULL }, /* 34 */
	{ cpu_execute_illegal_order,                NULL }, /* 35 */
	{ cpu_execute_illegal_order,                NULL }, /* 36 */
	{ cpu_execute_illegal_order,                NULL }, /* 37 */
	{ cpu_execute_illegal_order,                NULL }, /* 38 */
	{ cpu_execute_illegal_order,                NULL }, /* 39 */
	{ cpu_execute_illegal_order,                NULL }, /* 40 */
	{ cpu_execute_illegal_order,                NULL }, /* 41 */
	{ cpu_execute_illegal_order,                NULL }, /* 42 */
	{ cpu_execute_illegal_order,                NULL }, /* 43 */
	{ cpu_execute_illegal_order,                NULL }, /* 44 */
	{ cpu_execute_illegal_order,                NULL }, /* 45 */
	{ cpu_execute_illegal_order,                NULL }, /* 46 */
	{ cpu_execute_illegal_order,                NULL }, /* 47 */
	{ cpu_execute_illegal_order,                NULL }, /* 48 */
	{ cpu_execute_illegal_order,                NULL }, /* 49 */
	{ cpu_execute_illegal_order,                NULL }, /* 50 */
	{ cpu_execute_illegal_order,                NULL }, /* 51 */
	{ cpu_execute_illegal_order,                NULL }, /* 52 */
	{ cpu_execute_illegal_order,                NULL }, /* 53 */
	{ cpu_execute_illegal_order,                NULL }, /* 54 */
	{ cpu_execute_illegal_order,                NULL }, /* 55 */
	{ cpu_execute_illegal_order,                NULL }, /* 56 */
	{ cpu_execute_illegal_order,                NULL }, /* 57 */
	{ cpu_execute_illegal_order,                NULL }, /* 58 */
	{ cpu_execute_illegal_order,                NULL }, /* 59 */
	{ cpu_execute_illegal_order,                NULL }, /* 60 */
	{ cpu_execute_illegal_order,                NULL }, /* 61 */
	{ cpu_execute_illegal_order,                NULL }, /* 62 */
	{ cpu_execute_illegal_order,                NULL }  /* 63 */
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

static DISPATCH_ENTRY floatingPointDispatchTable[] =
{
	{ cpu_execute_flp_load_single, NULL },   /* 0 */
	{ cpu_execute_flp_load_double, NULL },   /* 1 */
	{ cpu_execute_illegal_order, NULL },   /* 2 */
	{ cpu_execute_illegal_order, NULL },   /* 3 */
	{ cpu_execute_illegal_order, NULL },   /* 4 */
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
	{ cpu_execute_cr_level, organisationalDispatchTable }, /* 0 */
	{ cpu_execute_cr_level, NULL },                        /* 1 */
	{ cpu_execute_cr_level, NULL },                        /* 2 */
	{ cpu_execute_cr_level, NULL },                        /* 3 */
	{ cpu_execute_cr_level, NULL },                        /* 4 */
	{ cpu_execute_cr_level, accFixedDispatchTable },       /* 5 */
	{ cpu_execute_cr_level, NULL },                        /* 6 */
	{ cpu_execute_cr_level, floatingPointDispatchTable }  /* 7 */
};

t_stat sim_instr(void)
{
	t_stat reason = SCPE_OK;
	cpu_stopped = 0;

	while (!cpu_stopped)
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

	if (cpu_stopped)
	{
		reason = SCPE_STOP;
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
	cpu_set_register_32(reg_co, 0); /* TODO: probably needs to be reset to start of OS (upper half of memory) */
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

static SIM_INLINE void cpu_set_register_16(REG *reg, uint16 value)
{
	assert(reg->width == 16);
	*(uint16 *)(reg->loc) = value;
}

static SIM_INLINE void cpu_set_register_32(REG *reg, uint32 value)
{
	assert(reg->width == 32);
	*(uint32 *)(reg->loc) = value;
}

static SIM_INLINE void cpu_set_register_64(REG *reg, t_uint64 value)
{
	assert(reg->width == 64);
	*(t_uint64 *)(reg->loc) = value;
}

static SIM_INLINE void cpu_set_register_bit_64(REG *reg, t_uint64 mask, int value)
{
	assert(reg->width == 64);
	if (value)
	{
		*(t_uint64 *)(reg->loc) = *(t_uint64 *)(reg->loc) | ~mask;
	}
	else
	{
		*(t_uint64 *)(reg->loc) = *(t_uint64 *)(reg->loc) & mask;
	}
}

static SIM_INLINE uint16 cpu_get_register_16(REG *reg)
{
	assert(reg->width == 16);
	return *(uint16 * )(reg->loc);
}

static SIM_INLINE uint32 cpu_get_register_32(REG *reg)
{
	assert(reg->width == 32);
	return *(uint32 *)(reg->loc);
}

static SIM_INLINE t_uint64 cpu_get_register_64(REG *reg)
{
	assert(reg->width == 64);
	return *(t_uint64 *)(reg->loc);
}

static SIM_INLINE t_uint64 cpu_get_register_bit_16(REG *reg, uint16 mask)
{
	t_uint64 result;
	assert(reg->width == 16);
	if (*(uint16 *)(reg->loc) & mask)
	{
		result = 1;
	}
	else
	{
		result = 0;
	}

	return result;
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

/* returns operand kind k, for organisational orders it returns 7 for extended operands, so k is the same across all order types */
static uint16 cpu_get_k(uint16 order)
{
	uint16 k;
	uint16 cr = cpu_get_cr(order);
	if (cr == 0)
	{
		k = (order >> 6) & 0x1;
		if (k == 1)
		{
			k = 7;
		}
	}
	else
	{
		k = (order >> 6) & 0x7;
	}

	return k;
}

static t_uint64 cpu_get_operand_6_bit_literal(uint16 order, uint32 instructionAddress, int *instructionLength)
{
	t_uint64 result = 0;
	result = order & 0x7F;
	result |= (result & 0x40) ? -1 : 0; /* sign extend */
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "%lld\n", result);
	return result;
}

static t_uint64 cpu_get_operand_extended_literal(uint16 order, uint32 instructionAddress, int *instructionLength)
{
	t_uint64 result = 0;
	int i;
	uint8 nprime = order & 0x3;
	uint8 unsignedLiteral = (order >> 2) & 0x1;

	if (nprime > 2)
	{
		/* The MU5 Basic Programming Manual does not list n'==3 as valid, but the Roland and Ibbett book lists it as another 64-bit option */
		cpu_set_interrupt(INT_ILLEGAL_ORDERS);
	}
	else
	{
		int words = (nprime == 2) ? 4 : nprime + 1;
		uint16 lastWord;
		for (i = 0; i < words; i++)
		{
			lastWord = sac_read_16_bit_word(instructionAddress + 1 + i);
			result |= (t_uint64)lastWord << (i * 16);
		}

		if (!unsignedLiteral)
		{
			result |= (lastWord & 0x8000) ? -1 : 0; /* sign extend */
		}

		*instructionLength += words;
	}

	if (unsignedLiteral)
	{
		sim_debug(LOG_CPU_DECODE, &cpu_dev, "%llu\n", result);
	}
	else
	{
		sim_debug(LOG_CPU_DECODE, &cpu_dev, "%lld\n", result);
	}

	return result;
}

static t_uint64 cpu_get_operand_internal_register(uint16 order, uint32 instructionAddress, int *instructionLength)
{
	t_uint64 result = 0;
	uint8 n = order & 0x7;

	switch (n)
	{
	case 0:
	{
		result = (cpu_get_register_16(reg_ms) << 48) | (cpu_get_register_16(reg_nb) << 32) | cpu_get_register_32(reg_co);
		break;
	}
	case 1:
	{
		result = cpu_get_register_32(reg_xnb);
		break;
	}
	case 2:
	{
		result = (cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_nb);
		break;
	}
	case 3:
	{
		result = (cpu_get_register_16(reg_sn) << 16) | cpu_get_register_16(reg_sf);
		break;
	}
	case 4:
	{
		result = cpu_get_register_bit_16(reg_ms, mask_ms_bn);
		break;
	}
	case 16:
	{
		result = cpu_get_register_64(reg_d);
		break;
	}
	case 17:
	{
		result = cpu_get_register_64(reg_xd);
		break;
	}
	case 18:
	{
		result = cpu_get_register_32(reg_dt);
		break;
	}
	case 19:
	{
		result = cpu_get_register_32(reg_xdt);
		break;
	}
	case 20:
	{
		result = cpu_get_register_32(reg_dod);
		break;
	}
	case 32:
	{
		result = cpu_get_register_32(reg_b);
		break;
	}
	case 33:
	{
		result = cpu_get_register_32(reg_bod);
		break;
	}
	case 34:
	{
		result = 0; /* cpu_get_register_32(reg_z); */ /* Z is an "imaginary" register, see p31 of Morris & Ibbett book. Z not implemented for now. */
		break;
	}
	case 36:
	{
		result = (cpu_get_register_32(reg_bod) << 32) | cpu_get_register_32(reg_b);
		break;
	}
	case 48:
	{
		result = cpu_get_register_64(reg_aex);
		break;
	}
	default:
	{
		result = 0;
		break;
	}
	}

	sim_debug(LOG_CPU_DECODE, &cpu_dev, "R%hu\n", n);

	return result;
}

static t_uint64 cpu_get_operand(uint16 order)
{
	t_uint64 result = 0;
	uint8 instructionLength = 1;
	uint16 k = cpu_get_k(order);
	uint32 instructionAddress = cpu_get_register_32(reg_co);

	switch (k)
	{
	case 0:
	{
		result = cpu_get_operand_6_bit_literal(order, instructionAddress, &instructionLength);
		break;
	}
	case 1:
	{
		result = cpu_get_operand_internal_register(order, instructionAddress, &instructionLength);
		break;
	}
	case 7:
	{
		uint8 extendedKind = (order >> 3) & 0x7;

		if (extendedKind == 0)
		{
			result = cpu_get_operand_extended_literal(order, instructionAddress, &instructionLength);
		}
		else
		{
			cpu_set_interrupt(INT_ILLEGAL_ORDERS);
		}
		break;
	}
	default:
	{
		cpu_set_interrupt(INT_ILLEGAL_ORDERS);
	}
	}

	cpu_set_register_32(reg_co, instructionAddress + instructionLength);

	return result;
}

static void cpu_execute_next_order(void)
{
	uint16 order;
	uint16 cr;

	if (interrupt == 0)
	{
		order = sac_read_16_bit_word(cpu_get_register_32(reg_co));
		cr = cpu_get_cr(order);

		crDispatchTable[cr].execute(order, crDispatchTable[cr].innerTable);
	}
	else
	{
		cpu_start_interrupt_processing();
	}
}

static void cpu_execute_illegal_order(uint16 order, DISPATCH_ENTRY *innerTable)
{
	cpu_set_interrupt(INT_ILLEGAL_ORDERS);
}

static void cpu_start_interrupt_processing(void)
{
	printf("Interrupt %hu detected - processing TBD\n", cpu_get_interrupt_number());
	cpu_stopped = 1; /* TODO: temporary halt CPU until implement interrupt processing */
}

static void cpu_execute_cr_level(uint16 order, DISPATCH_ENTRY *innerTable)
{
	uint16 f = cpu_get_f(order);
	innerTable[f].execute(order, innerTable[f].innerTable);
}

static void cpu_execute_organisational_relative_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "-> ");
	int32 relative = (int32)(cpu_get_operand(order) & 0xFFFFFFFF);
	int32 relativeTo = (int32)cpu_get_register_32(reg_co);
	uint32 newCo = (uint32)(relativeTo + relative);
	cpu_set_register_32(reg_co, newCo);
	// TODO: cross-segment generates interrupt

	/* The real MU5 did not have a STOP instruction, see comment above the declaration of cpu_stopped */
	if (relative == 0)
	{
		cpu_stopped = 1;
	}
}

static void cpu_execute_organisational_absolute_jump(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "JUMP ");
	cpu_set_register_32(reg_co, cpu_get_operand(order) & 0x7FFFFFFF);
}

static void cpu_execute_organisational_NB_load(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "NB= ");
	cpu_set_register_16(reg_nb, cpu_get_operand(order) & 0xFFFE); /* LS bit of NB is always zero */
}

static void cpu_execute_acc_fixed_add(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "A+ ");
	cpu_set_register_64(reg_a, cpu_get_register_64(reg_a) + cpu_get_operand(order)); // TODO: overflow
}

static void cpu_execute_flp_load_single(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "=(32) ");
	cpu_set_register_bit_64(reg_aod, mask_aod_opsiz64, 0);
	cpu_set_register_64(reg_a, (cpu_get_operand(order) << 32) & 0xFFFFFFFF00000000);
}

static void cpu_execute_flp_load_double(uint16 order, DISPATCH_ENTRY *innerTable)
{
	sim_debug(LOG_CPU_DECODE, &cpu_dev, "=(64) ");
	cpu_set_register_bit_64(reg_aod, mask_aod_opsiz64, 1);
	cpu_set_register_64(reg_a, cpu_get_operand(order));
}
