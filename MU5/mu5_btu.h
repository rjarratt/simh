/* mu5_btu.h: MU5 Block Transfer Unit

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
*/

#include "sim_defs.h"

#define BTU_NUM_UNITS 4

#define BTU_VX_STORE_PARITY_BLOCK 4

#define BTU_VX_STORE_SOURCE_ADDRESS_LINE 0
#define BTU_VX_STORE_DESTINATION_ADDRESS_LINE 1
#define BTU_VX_STORE_SIZE_LINE 2
#define BTU_VX_STORE_TRANSFER_STATUS_LINE 3
#define BTU_VX_STORE_BTU_RIPF_LINE 1
#define BTU_VX_STORE_TRANSFER_COMPLETE_LINE 2

#define BTU_VX_STORE_SOURCE_ADDRESS(unit)  VX_ADDR(unit, BTU_VX_STORE_SOURCE_ADDRESS_LINE)
#define BTU_VX_STORE_DESTINATION_ADDRESS(unit)  VX_ADDR(unit, BTU_VX_STORE_DESTINATION_ADDRESS_LINE)
#define BTU_VX_STORE_SIZE(unit)  VX_ADDR(unit, BTU_VX_STORE_SIZE_LINE)
#define BTU_VX_STORE_TRANSFER_STATUS(unit)  VX_ADDR(unit, BTU_VX_STORE_TRANSFER_STATUS_LINE)

#define BTU_VX_STORE_BTU_RIPF  VX_ADDR(BTU_VX_STORE_PARITY_BLOCK, BTU_VX_STORE_BTU_RIPF_LINE)
#define BTU_VX_STORE_TRANSFER_COMPLETE  VX_ADDR(BTU_VX_STORE_PARITY_BLOCK, BTU_VX_STORE_TRANSFER_COMPLETE_LINE)

void btu_reset_state(void);
t_uint64 btu_exch_read(t_addr addr);
void btu_exch_write(t_addr addr, t_uint64 value);
