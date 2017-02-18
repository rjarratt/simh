/* mu5_defs.h: MU5 Store Access Control definitions

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

#include "sim_defs.h"

#define SAC_V_STORE_BLOCK 4
#define SAC_V_STORE_CPR_NUMBER 1
#define SAC_V_STORE_CPR_VA 2
#define SAC_V_STORE_CPR_RA 3

void sac_reset_state(void);
t_uint64 sac_read_64_bit_word(t_addr address);
void sac_write_64_bit_word(t_addr address, t_uint64 value);
uint32 sac_read_32_bit_word(t_addr address);
void sac_write_32_bit_word(t_addr address, uint32 value);
uint16 sac_read_16_bit_word(t_addr address);
void sac_write_16_bit_word(t_addr address, uint16 value);
uint8 sac_read_8_bit_word(t_addr address);
void sac_write_8_bit_word(t_addr address, uint8 value);

void sac_setup_v_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(void), void(*writeCallback)(t_uint64));
void sac_write_v_store(uint8 block, uint8 line, t_uint64 value);
t_uint64 sac_read_v_store(uint8 block, uint8 line);
