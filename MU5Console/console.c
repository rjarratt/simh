/* console.c: MU5 simulator fron panel

   Copyright (c) 2017, Rob Jarratt. Based on 2015 sample by Mark Pizzolato

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
   MARK PIZZOLATO BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Mark Pizzolato shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Mark Pizzolato.

*/

/* This program provides a basic test of the simh_frontpanel API. */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "sim_frontpanel.h"
#include <signal.h>

#if defined(_WIN32)
#include <windows.h>
#define usleep(n) Sleep(n/1000)
#else
#include <unistd.h>
#endif
const char *sim_path = 
#if defined(_WIN32)
            "mu5.exe";
#else
            "mu5";
#endif

const char *sim_config = 
            "MU5-PANEL.ini";

/* Registers visible on the Front Panel */
unsigned int CO, DL;

int update_display = 1;

static void
DisplayCallback (PANEL *panel, unsigned long long simulation_time, void *context)
{
update_display = 1;
}

static void
DisplayRegisters (PANEL *panel)
{
char buf1[100], buf2[100], buf3[100], buf4[100];
static const char *states[] = {"Halt", "Run "};

//if (!update_display)
//    return;
update_display = 0;
buf1[sizeof(buf1)-1] = buf2[sizeof(buf2)-1] = buf3[sizeof(buf3)-1] = 0;
sprintf(buf1, "%s\r\n", states[sim_panel_get_state(panel)]);
sprintf(buf2, "CO: %08X\r\n", CO);
sprintf(buf3, "DL: %08X\r\n", DL);
buf4[0] = '\0';
#if defined(_WIN32)
if (1) {
    static HANDLE out = NULL;
    CONSOLE_SCREEN_BUFFER_INFO info;
    static COORD origin;
    int written;

    if (out == NULL)
        out = GetStdHandle (STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo (out, &info);
    SetConsoleCursorPosition (out, origin);
    WriteConsoleA(out, buf1, strlen(buf1), &written, NULL);
    WriteConsoleA(out, buf2, strlen(buf2), &written, NULL);
    WriteConsoleA(out, buf3, strlen(buf3), &written, NULL);
    WriteConsoleA(out, buf4, strlen(buf4), &written, NULL);
    SetConsoleCursorPosition (out, info.dwCursorPosition);
    }
#else
#define ESC "\033"
#define CSI ESC "["
printf (CSI "s");   /* Save Cursor Position */
printf (CSI "H");   /* Position to Top of Screen (1,1) */
printf ("%s", buf1);
printf ("%s", buf2);
printf ("%s", buf3);
printf ("%s", buf4);
printf (CSI "s");   /* Restore Cursor Position */
printf ("\r\n");
#endif
}

static
void InitDisplay (void)
{
#if defined(_WIN32)
system ("cls");
#else
printf (CSI "H");   /* Position to Top of Screen (1,1) */
printf (CSI "2J");  /* Clear Screen */
#endif
printf ("\n\n\n\n");
printf ("^C to Halt, Commands: BOOT, CONT, EXIT\n");
}

volatile int halt_cpu = 0;
PANEL *panel, *tape;

void halt_handler (int sig)
{
signal (SIGINT, halt_handler);      /* Re-establish handler for some platforms that implement ONESHOT signal dispatch */
halt_cpu = 1;
sim_panel_flush_debug (panel);
return;
}

int
main (int argc, char **argv)
{
FILE *f;
int debug = 0;

if ((argc > 1) && ((!strcmp("-d", argv[1])) || (!strcmp("-D", argv[1])) || (!strcmp("-debug", argv[1]))))
    debug = 1;
/* Create pseudo config file for a test */
if ((f = fopen (sim_config, "w"))) {
    if (debug) {
        fprintf (f, "set verbose\n");
        fprintf (f, "set debug -n -a simulator.dbg\n");
        fprintf (f, "set cpu conhalt\n");
        fprintf (f, "set remote telnet=2226\n");
        fprintf (f, "set rem-con debug=XMT;RCV;MODE;REPEAT;CMD\n");
        fprintf (f, "set remote notelnet\n");
        }
    fprintf (f, "set console telnet=buffered\n");
    fprintf (f, "set console -u telnet=1927\n");
    /* Start a terminal emulator for the console port */
#if defined(_WIN32)
    fprintf (f, "set env PATH=%%PATH%%;%%ProgramFiles%%\\PuTTY;%%ProgramFiles(x86)%%\\PuTTY\n");
    fprintf (f, "! start PuTTY telnet://localhost:1927\n");
#elif defined(__linux) || defined(__linux__)
    fprintf (f, "! nohup xterm -e 'telnet localhost 1927' &\n");
#elif defined(__APPLE__)
    fprintf (f, "! osascript -e 'tell application \"Terminal\" to do script \"telnet localhost 1927; exit\"'\n");
#endif
	fprintf(f, "vstore set 4 4 0FFFFFF0 ; CPR IGNORE\n");
	fprintf(f, "dep cpr[0]  00000000E3000004\n");
	fprintf(f, "dep cpr[1]  00001000F3010004\n");
	fprintf(f, "dep cpr[2]  00002000E3020004\n");
	fprintf(f, "dep cpr[3]  00003000E3030004\n");
	fprintf(f, "dep cpu ms 0014\n");
	fprintf(f, "load MU5ELR.bin\n");
	fprintf(f, "dep cpu ms 0000\n");
	fprintf(f, "dep cpu ms 0014\n");
	fprintf(f, "load console.bin\n");
	fprintf(f, "dep co 20000\n");
//	fprintf(f, "go\n");
	fclose(f);
    }

InitDisplay();
signal (SIGINT, halt_handler);
panel = sim_panel_start_simulator_debug (sim_path,
                                         sim_config,
                                         2,
                                         debug? "frontpanel.dbg" : NULL);

if (!panel) {
    printf ("Error starting simulator %s with config %s: %s\n", sim_path, sim_config, sim_panel_get_error());
    goto Done;
    }

if (debug) {
    sim_panel_set_debug_mode (panel, DBG_XMT|DBG_RCV|DBG_REQ|DBG_RSP);
    }

//tape = sim_panel_add_device_panel (panel, "TAPE DRIVE");
//
//if (!tape) {
//    printf ("Error adding tape device to simulator: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//
if (sim_panel_add_register (panel, "CO",  NULL, sizeof(CO), &CO)) {
    printf ("Error adding register 'CO': %s\n", sim_panel_get_error());
    goto Done;
    }
//if (sim_panel_add_register_indirect (panel, "CO",  NULL, sizeof(atPC), &atPC)) {
//    printf ("Error adding register indirect 'CO': %s\n", sim_panel_get_error());
//    goto Done;
//    }
if (sim_panel_add_register (panel, "DL",  NULL, sizeof(DL), &DL)) {
    printf ("Error adding register 'DL': %s\n", sim_panel_get_error());
    goto Done;
    }
//
//if (sim_panel_get_registers (panel, NULL)) {
//    printf ("Error getting register data: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (1) {
//    long deadbeef = 0xdeadbeef, beefdead = 0xbeefdead, addr200 = 0x00000200, beefdata;
//
//    if (sim_panel_set_register_value (panel, "R0", "DEADBEEF")) {
//        printf ("Error setting R0 to DEADBEEF: %s\n", sim_panel_get_error());
//        goto Done;
//        }
//    if (sim_panel_gen_deposit (panel, "R1", sizeof(deadbeef), &deadbeef)) {
//        printf ("Error setting R1 to DEADBEEF: %s\n", sim_panel_get_error());
//        goto Done;
//        }
//    if (sim_panel_mem_deposit (panel, sizeof(addr200), &addr200, sizeof(deadbeef), &deadbeef)) {
//        printf ("Error setting 00000200 to DEADBEEF: %s\n", sim_panel_get_error());
//        goto Done;
//        }
//    beefdata = 0;
//    if (sim_panel_gen_examine (panel, "200", sizeof(beefdata), &beefdata)) {
//        printf ("Error getting contents of memory location 200: %s\n", sim_panel_get_error());
//        goto Done;
//        }
//    beefdata = 0;
//    if (sim_panel_mem_examine (panel, sizeof (addr200), &addr200, sizeof (beefdata), &beefdata)) {
//        printf ("Error getting contents of memory location 200: %s\n", sim_panel_get_error());
//        goto Done;
//        }
//    beefdata = 0;
//    if (!sim_panel_gen_examine (panel, "20000000", sizeof(beefdata), &beefdata)) {
//        printf ("Unexpected success getting contents of memory location 20000000: %s\n", sim_panel_get_error());
//        goto Done;
//        }
//    }
if (sim_panel_get_registers (panel, NULL)) {
    printf ("Error getting register data: %s\n", sim_panel_get_error());
    goto Done;
    }
if (sim_panel_set_display_callback_interval (panel, &DisplayCallback, NULL, 200000)) {
    printf ("Error setting automatic display callback: %s\n", sim_panel_get_error());
    goto Done;
    }
if (!sim_panel_get_registers (panel, NULL)) {
    printf ("Unexpected success getting register data: %s\n", sim_panel_get_error());
    goto Done;
    }
sim_panel_clear_error ();
//if (!sim_panel_dismount (panel, "RL0")) {
//    printf ("Unexpected success while dismounting media file from non mounted RL0: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_mount (panel, "RL0", "-N", "TEST-RL.DSK")) {
//    printf ("Error while mounting media file TEST-RL.DSK on RL0: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_dismount (panel, "RL0")) {
//    printf ("Error while dismounting media file from RL0: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//remove ("TEST-RL.DSK");
//if (sim_panel_break_set (panel, "400")) {
//    printf ("Unexpected error establishing a breakpoint: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_break_clear (panel, "400")) {
//    printf ("Unexpected error clearing a breakpoint: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_break_output_set (panel, "\"32..31..30\"")) {
//    printf ("Unexpected error establishing an output breakpoint: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_break_output_clear (panel, "\"32..31..30\"")) {
//    printf ("Unexpected error clearing an output breakpoint: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_break_output_set (panel, "-P \"Normal operation not possible.\"")) {
//    printf ("Unexpected error establishing an output breakpoint: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_break_output_set (panel, "-P \"Device? [XQA0]: \"")) {
//    printf ("Unexpected error establishing an output breakpoint: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (!sim_panel_set_sampling_parameters (panel, 0, 199)) {
//    printf ("Unexpected success setting sampling parameters to 0, 199\n");
//    goto Done;
//    }
//if (!sim_panel_set_sampling_parameters (panel, 199, 0)) {
//    printf ("Unexpected success setting sampling parameters to 199, 0\n");
//    goto Done;
//    }
//if (!sim_panel_add_register_bits (panel, "PSL",  NULL, 32, PSL_bits)) {
//    printf ("Unexpected success setting PSL bits before setting sampling parameters\n");
//    goto Done;
//    }
//if (sim_panel_set_sampling_parameters (panel, 500, 100)) {
//    printf ("Unexpected error setting sampling parameters to 200, 100: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_add_register_indirect_bits (panel, "CO",  NULL, 32, PC_indirect_bits)) {
//    printf ("Error adding register 'PSL' bits: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_add_register_bits (panel, "PSL",  NULL, 32, PSL_bits)) {
//    printf ("Error adding register 'PSL' bits: %s\n", sim_panel_get_error());
//    goto Done;
//    }
//if (sim_panel_add_register_bits (panel, "CO",  NULL, 32, PC_bits)) {
//    printf ("Error adding register 'PSL' bits: %s\n", sim_panel_get_error());
//    goto Done;
//    }

sim_panel_clear_error ();
while (1) {
    size_t i;
    char cmd[512];

    while (sim_panel_get_state (panel) == Halt) {
        DisplayRegisters (panel);
        printf ("SIM> ");
        if (!fgets (cmd, sizeof(cmd)-1, stdin))
            break;
        while (strlen(cmd) && isspace(cmd[strlen(cmd)-1]))
            cmd[strlen(cmd)-1] = '\0';
        while (isspace(cmd[0]))
            memmove (cmd, cmd+1, strlen(cmd));
        for (i=0; i<strlen(cmd); i++) {
            if (islower(cmd[i]))
                cmd[i] = toupper(cmd[i]);
            }
        if (!memcmp("BOOT", cmd, 4)) {
            if (sim_panel_exec_boot (panel, cmd + 4))
                break;
            }
        else if (!strcmp("STEP", cmd)) {
            if (sim_panel_exec_step (panel))
                break;
            }
        else if (!strcmp("CONT", cmd)) {
            if (sim_panel_exec_run (panel))
                break;
            }
        else if (!strcmp("EXIT", cmd))
            goto Done;
        else
            printf ("Huh? %s\r\n", cmd);
        }
    while (sim_panel_get_state (panel) == Run) {
        usleep (100);
        //if (update_display)
            DisplayRegisters(panel);
        if (halt_cpu) {
            halt_cpu = 0;
            sim_panel_exec_halt (panel);
            }
        }
    }

Done:
sim_panel_destroy (panel);

/* Get rid of pseudo config file created above */
remove (sim_config);
}
