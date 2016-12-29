/* mu5_sac.c: MU5 Store Access Control unit

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

static uint32 LocalStore[MAXMEMORY];

t_uint64 sac_read_64_bit_word(t_addr address)
{
    t_uint64 result = sac_read_32_bit_word(address + 1) << 32 & sac_read_32_bit_word(address);
    return result;
}

void sac_write_64_bit_word(t_addr address, t_uint64 value)
{
    sac_write_32_bit_word(address, (value >> 32) & 0xFFFFFFFF);
    sac_write_32_bit_word(address + 1, value & 0xFFFFFFFF);
}

uint32 sac_read_32_bit_word(t_addr address)
{
    uint32 result = LocalStore[address];
    return result;
}

void sac_write_32_bit_word(t_addr address, uint32 value)
{
    LocalStore[address] = value;
}

uint16 sac_read_16_bit_word(t_addr address)
{
    uint32 fullWord = sac_read_32_bit_word(address >> 1);
    uint16 result = (address & 1) ? fullWord >> 16 : fullWord & 0xFFFF;
    return result;
}

void sac_write_16_bit_word(t_addr address, uint16 value)
{
    uint32 fullWord = sac_read_32_bit_word(address >> 1);
    if (address & 1)
    {
        fullWord = (value << 16) | (fullWord & 0xFFFF);
    }
    else
    {
        fullWord = (fullWord & 0xFFFF0000) | value;
    }
    sac_write_32_bit_word(address >> 1, fullWord);
}
