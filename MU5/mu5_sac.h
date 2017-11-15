/* mu5_console.h: MU5 Store Access Control definitions

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

#define SAC_USER_ACCESS 0x8
#define SAC_READ_ACCESS 0x4
#define SAC_WRITE_ACCESS 0x2
#define SAC_OBEY_ACCESS 0x1
#define SAC_ALL_EXEC_ACCESS 0x7
#define SAC_ALL_ACCESS 0xF

#define V_STORE_BLOCKS 8
#define V_STORE_BLOCK_SIZE 256

#define SYSTEM_V_STORE_BLOCK 0
#define PROP_V_STORE_BLOCK 1
#define CONSOLE_V_STORE_BLOCK 3
#define SAC_V_STORE_BLOCK 4

#define PROP_V_STORE_PROGRAM_FAULT_STATUS 0
#define PROP_V_STORE_SYSTEM_ERROR_STATUS 1
#define PROP_V_STORE_PROCESS_NUMBER 2
#define PROP_V_STORE_INSTRUCTION_COUNTER 3

#define CONSOLE_V_STORE_CONSOLE_INTERRUPT 0
#define CONSOLE_V_STORE_TIME_UPPER 2
#define CONSOLE_V_STORE_TIME_LOWER 3
#define CONSOLE_V_STORE_DATE_LOWER 4
#define CONSOLE_V_STORE_DATE_UPPER_HOOTER 5
#define CONSOLE_V_STORE_TELETYPE_DATA 6
#define CONSOLE_V_STORE_TELETYPE_CONTROL 7
#define CONSOLE_V_STORE_ENGINEERS_HANDSWITCHES 11

#define SAC_V_STORE_CPR_SEARCH 0
#define SAC_V_STORE_CPR_NUMBER 1
#define SAC_V_STORE_CPR_VA 2
#define SAC_V_STORE_CPR_RA 3
#define SAC_V_STORE_CPR_IGNORE 4
#define SAC_V_STORE_CPR_FIND 5
#define SAC_V_STORE_CPR_ALTERED 6
#define SAC_V_STORE_CPR_REFERENCED 7
#define SAC_V_STORE_CPR_FIND_MASK 9
#define SAC_V_STORE_CPR_NOT_EQUIVALENCE_PSX 16
#define SAC_V_STORE_CPR_NOT_EQUIVALENCE_S 17 
#define SAC_V_STORE_ACCESS_VIOLATION 22
#define SAC_V_STORE_SYSTEM_ERROR_INTERRUPTS 23

typedef struct VSTORE_LINE
{
    t_uint64 value;
    t_uint64(*ReadCallback)(uint8 line);
    void(*WriteCallback)(uint8 line, t_uint64 value);
} VSTORE_LINE;

void sac_reset_state(void);
void sac_set_loading(void);
void sac_clear_loading(void);
t_uint64 sac_read_64_bit_word(t_addr address);
void sac_write_64_bit_word(t_addr address, t_uint64 value);
uint32 sac_read_32_bit_word(t_addr address);
uint32 sac_read_32_bit_word_for_obey(t_addr address);
void sac_write_32_bit_word(t_addr address, uint32 value);
uint16 sac_read_16_bit_word(t_addr address);
uint16 sac_read_16_bit_word_for_obey(t_addr address);
void sac_write_16_bit_word(t_addr address, uint16 value);
uint8 sac_read_8_bit_word(t_addr address);
void sac_write_8_bit_word(t_addr address, uint8 value);

uint32 sac_read_32_bit_word_real_address(t_addr address);
void sac_write_32_bit_word_real_address(t_addr address, uint32 value);
void sac_write_8_bit_word_real_address(t_addr address, uint8 value);

void sac_v_store_register_callback(t_value old_val, struct REG *reg, int index);
void sac_setup_v_store_location(uint8 block, uint8 line, t_uint64(*readCallback)(uint8), void(*writeCallback)(uint8,t_uint64));
void sac_write_v_store(uint8 block, uint8 line, t_uint64 value);
t_uint64 sac_read_v_store(uint8 block, uint8 line);

extern VSTORE_LINE VStore[V_STORE_BLOCKS][V_STORE_BLOCK_SIZE];