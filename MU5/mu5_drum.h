/* mu5_drum.h: MU5 Fixed Head Disk Device Unit

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

#define DRUM_NUM_UNITS 4
#define DRUM_BLOCKS_PER_BAND 37
#define DRUM_BYTES_PER_BLOCK 1024
#define DRUM_WORDS_PER_BLOCK 256
#define DRUM_BANDS_PER_UNIT 64

#define DRUM_VX_STORE_DISC_ADDRESS 0
#define DRUM_VX_STORE_STORE_ADDRESS 1
#define DRUM_VX_STORE_DISC_STATUS 2
#define DRUM_VX_STORE_CURRENT_POSITIONS 3
#define DRUM_VX_STORE_COMPLETE_ADDRESS 4

/* Status flags */
#define DRUM_DISC_STATUS_DECODE 0x80000000
#define DRUM_DISC_STATUS_ILLEGAL_REQUEST (0x1 << 13)
#define DRUM_DISC_STATUS_INPUT_PARITY_ERROR (0x1 << 10)
#define DRUM_DISC_STATUS_IGNORE_PARITY_ERROR (0x1 << 5)
#define DRUM_DISC_STATUS_END_TRANSFER (0x1 << 4)
#define DRUM_ABSENT_MASK(unit) (1 << (17 + (4 * unit)))

void drum_reset_state(void);
t_uint64 drum_exch_read(t_addr addr);
void drum_exch_write(t_addr addr, t_uint64 value);
