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
#define SDL_MAIN_HANDLED
#include <SDL.h>

#if defined(_WIN32)
#include <windows.h>
#define usleep(n) Sleep(n/1000)
#else
#include <unistd.h>
#endif

#define WIDTH   1200
#define HEIGHT  400
#define DEPTH   32

const char *sim_path =
#if defined(_WIN32)
"mu5.exe";
#else
"mu5";
#endif

const char *sim_config =
"MU5-PANEL.ini";

static SDL_Window *sdlWindow;
static SDL_Renderer *sdlRenderer;

/* Registers visible on the Front Panel */
unsigned int CO, DL;

int update_display = 1;

static void UpdateWholeScreen(void);
static void DrawRegisters(void);
static void DrawRegister(int hpos, int vpos, UINT64 value, UINT8 width);

static void
DisplayCallback(PANEL *panel, unsigned long long simulation_time, void *context)
{
    update_display = 1;
}

static void
DisplayRegisters(PANEL *panel)
{
    //char buf1[100], buf2[100], buf3[100], buf4[100];
    //static const char *states[] = {"Halt", "Run "};

    if (!update_display)
        return;
    update_display = 0;
    DrawRegisters();
    UpdateWholeScreen();
}

static void DrawRegisters(void)
{
    DrawRegister(0, 20, CO, 32);
    DrawRegister(0, 50, DL, 32);
}

static SDL_Texture *sprite_from_data(int width, int height, const unsigned char *data)
{
    SDL_Surface *spriteSurface;
    SDL_Texture *spriteTexture;
    unsigned *s, r, g, b;
    int y, x;

    spriteSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,
        width, height, DEPTH, 0, 0, 0, 0);
    SDL_LockSurface(spriteSurface);
    for (y = 0; y < height; ++y) {
        s = (unsigned*)((char*)spriteSurface->pixels + y * spriteSurface->pitch);
        for (x = 0; x < width; ++x) {
            r = *data++;
            g = *data++;
            b = *data++;
            *s++ = SDL_MapRGB(spriteSurface->format, r, g, b);
        }
    }
    SDL_UnlockSurface(spriteSurface);
    spriteTexture = SDL_CreateTextureFromSurface(sdlRenderer, spriteSurface);
    SDL_FreeSurface(spriteSurface);
    return spriteTexture;
}

/*
* Drawing a neon light.
*/
static void draw_lamp(int left, int top, int on)
{
    /* Images created by GIMP: save as C file without alpha channel. */
    static const int lamp_width = 12;
    static const int lamp_height = 12;
    static const unsigned char lamp_on[12 * 12 * 3 + 1] =
        "\0\0\0\0\0\0\0\0\0\13\2\2-\14\14e\31\31e\31\31-\14\14\13\2\2\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0D\20\20\313,,\377??\377CC\377CC\377DD\31333D\21\21\0\0"
        "\0\0\0\0\0\0\0D\20\20\357LL\377\243\243\376~~\37699\376@@\376@@\377AA\357"
        "<<D\21\21\0\0\0\13\2\2\313,,\377\243\243\377\373\373\377\356\356\377NN\377"
        ">>\377@@\377@@\377AA\31333\13\2\2-\14\14\377??\376~~\377\356\356\377\321"
        "\321\377<<\377??\377@@\377@@\376@@\377DD-\14\14e\31\31\377CC\37699\377NN"
        "\377<<\377??\377@@\377@@\377@@\376??\377CCe\31\31e\31\31\377CC\376@@\377"
        ">>\377??\377@@\377@@\377@@\377@@\376??\377CCe\31\31-\14\14\377DD\376@@\377"
        "@@\377@@\377@@\377@@\377@@\377@@\376@@\377DD-\14\14\13\2\2\31333\377AA\377"
        "@@\377@@\377@@\377@@\377@@\377@@\377AA\31333\13\2\2\0\0\0D\21\21\357<<\377"
        "AA\376@@\376??\376??\376@@\377AA\357<<D\21\21\0\0\0\0\0\0\0\0\0D\21\21\313"
        "33\377DD\377CC\377CC\377DD\31333D\21\21\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\13"
        "\2\2-\14\14e\31\31e\31\31-\14\14\13\2\2\0\0\0\0\0\0\0\0\0";
    static const unsigned char lamp_off[12 * 12 * 3 + 1] =
        "\0\0\0\0\0\0\0\0\0\0\0\0\14\2\2\14\2\2\14\2\2\14\2\2\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\25\5\5A\21\21h\32\32c\30\30c\30\30h\32\32A\21\21\25\5\5"
        "\0\0\0\0\0\0\0\0\0\25\5\5\\\30\30""8\16\16\0\0\0\0\0\0\0\0\0\0\0\0""8\16"
        "\16\\\30\30\25\5\5\0\0\0\0\0\0A\21\21""8\16\16\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0""8\16\16A\21\21\0\0\0\14\2\2h\32\32\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0h\32\32\14\2\2\14\2\2c\30\30\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0c\30\30\14\2\2\14\2\2c\30\30\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0c\30\30\14\2\2\14\2\2h\32\32\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0h\32\32\14\2\2\0\0\0A\21\21""8\16\16\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""8\16\16A\21\21\0\0\0\0\0\0\25\5\5\\\30"
        "\30""8\16\16\0\0\0\0\0\0\0\0\0\0\0\0""8\16\16\\\30\30\25\5\5\0\0\0\0\0\0"
        "\0\0\0\25\5\5A\21\21h\32\32c\30\30c\30\30h\32\32A\21\21\25\5\5\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\14\2\2\14\2\2\14\2\2\14\2\2\0\0\0\0\0\0\0\0\0"
        "\0\0\0";

    static unsigned char lamp_mid[sizeof(lamp_on)];
    static SDL_Texture * sprites[2];
    SDL_Rect area;
    int i;

    if (!sprites[0]) {
        sprites[0] = sprite_from_data(lamp_width, lamp_height,
            lamp_off);
    }
    if (!sprites[1]) {
        sprites[1] = sprite_from_data(lamp_width, lamp_height,
            lamp_on);
    }

    area.x = left;
    area.y = top;
    area.w = lamp_width;
    area.h = lamp_height;
    SDL_RenderCopy(sdlRenderer, sprites[on], NULL, &area);
}

static void DrawRegister(int hpos, int vpos, UINT64 value, UINT8 width)
{
    int i;
    int on;
    for (i = width - 1; i >= 0; i--)
    {
        on = (value & (1 << i)) != 0;
        draw_lamp(hpos + (width - i) * 14, vpos, on);
    }
}

static void UpdateWholeScreen(void)
{
    SDL_RenderPresent(sdlRenderer);
}

int CreatePanel()
{
    int result = 0;
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL: unable to init: %s\n", SDL_GetError());
    }
    else
    {
        if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &sdlWindow, &sdlRenderer) != 0)
        {
            SDL_LogError("SDL: unable to create window and renderer: %s\n",SDL_GetError());
        }
        else
        {

            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
            SDL_RenderSetLogicalSize(sdlRenderer, WIDTH, HEIGHT);
            /* Make grey console background */
            SDL_SetRenderDrawColor(sdlRenderer, 114, 111, 104, 255);
            SDL_RenderClear(sdlRenderer);

            ///* Initialize the TTF library */
            //if (TTF_Init() < 0) {
            //    t_stat ret = sim_messagef(SCPE_OPENERR, "SDL: couldn't initialize TTF: %s\n",
            //        SDL_GetError());
            //    SDL_Quit();
            //    return ret;
            //}

            ///* Font colors */
            //background = black;
            //foreground = cyan;

            ///* Open the font file with the requested point size */
            //font_big = TTF_OpenFont(QUOTE(FONTFILE), 16);
            //font_small = TTF_OpenFont(QUOTE(FONTFILE), 9);
            //if (!font_big || !font_small) {
            //    t_stat ret = sim_messagef(SCPE_OPENERR, "SDL: couldn't load font %s: %s\n",
            //        QUOTE(FONTFILE), SDL_GetError());
            //    besm6_close_panel(u, val, cptr, desc);
            //    return ret;
            //}

            DrawRegisters();
            ///* Drawing the static part of the BESM-6 panel */
            //draw_modifiers_static(0, 24, 10);
            //draw_modifiers_static(1, 400, 10);
            //draw_prp_static(24, 170);
            //draw_counters_static(24 + 32 * STEPX, 170);
            //draw_grp_static(24, 230);
            //draw_brz_static(24, 280);

            ///* Make sure all lights are updated */
            //memset(M_lamps, ~0, sizeof(M_lamps));
            //memset(BRZ_lamps, ~0, sizeof(BRZ_lamps));
            //memset(GRP_lamps, ~0, sizeof(GRP_lamps));
            //memset(PRP_lamps, ~0, sizeof(PRP_lamps));
            //memset(PC_lamps, ~0, sizeof(PC_lamps));
            //besm6_draw_panel(1);

            UpdateWholeScreen();
            result = 1;
        }
    }

    return result;
}

static
void InitDisplay(void)
{
    CreatePanel();
#if defined(_WIN32)
    system("cls");
#else
    printf(CSI "H");   /* Position to Top of Screen (1,1) */
    printf(CSI "2J");  /* Clear Screen */
#endif
    printf("\n\n\n\n");
    printf("^C to Halt, Commands: BOOT, CONT, STEP, EXIT\n");
}

volatile int halt_cpu = 0;
PANEL *panel, *tape;

void halt_handler(int sig)
{
    signal(SIGINT, halt_handler);      /* Re-establish handler for some platforms that implement ONESHOT signal dispatch */
    halt_cpu = 1;
    sim_panel_flush_debug(panel);
    return;
}

int my_boot(PANEL *panel)
{
    UINT64 cpr0 = 0x00000000E3000004;
    UINT64 cpr1 = 0x00001000F3010004;
    UINT64 cpr2 = 0x00002000E3020004;
    UINT64 cpr3 = 0x00003000E3030004;
    UINT64 cpr4 = 0x00006000F3040004;
    UINT64 cpr5 = 0x00006010F3050004;
    UINT64 cpr6 = 0x00006020F3050004;
    UINT16 ms = 0x000C;
    UINT32 co = 0xC0000;
    UINT32 cpr_ignore = 0x01FFFFF0;
    UINT32 engineers_handkeys = 0xA000;
    sim_panel_gen_deposit(panel, "sac v[4]", sizeof(cpr0), &cpr_ignore);
    sim_panel_gen_deposit(panel, "cpr[0]", sizeof(cpr0), &cpr0);
    sim_panel_gen_deposit(panel, "cpr[1]", sizeof(cpr1), &cpr1);
    sim_panel_gen_deposit(panel, "cpr[2]", sizeof(cpr2), &cpr2);
    sim_panel_gen_deposit(panel, "cpr[3]", sizeof(cpr3), &cpr3);
    sim_panel_gen_deposit(panel, "cpr[4]", sizeof(cpr4), &cpr4);
    sim_panel_gen_deposit(panel, "cpr[5]", sizeof(cpr5), &cpr5);
    sim_panel_gen_deposit(panel, "cpr[6]", sizeof(cpr6), &cpr6);
    sim_panel_gen_deposit(panel, "con v[11]", sizeof(engineers_handkeys), &engineers_handkeys);
    sim_panel_gen_deposit(panel, "MS", sizeof(ms), &ms);
    sim_panel_gen_deposit(panel, "CO", sizeof(co), &co);
    sim_panel_exec_run(panel);
    return 0;
}

int
main(int argc, char *argv[])
{
    SDL_Event e;
    FILE *f;
    int debug = 0;

    if ((argc > 1) && ((!strcmp("-d", argv[1])) || (!strcmp("-D", argv[1])) || (!strcmp("-debug", argv[1]))))
        debug = 1;
    /* Create pseudo config file for a test */
    if ((f = fopen(sim_config, "w"))) {
        fprintf(f, "set remote telnet=2226\n");
        if (debug) {
            fprintf(f, "set verbose\n");
            fprintf(f, "set debug -n -a simulator.dbg\n");
            //fprintf (f, "set rem-con debug=XMT;RCV;MODE;REPEAT;CMD\n");
            fprintf(f, "set remote notelnet\n");
        }
        //fprintf(f, "set debug stdout\n");
        //fprintf(f, "set cpu debug=decode\n");
        fprintf(f, "set console telnet=buffered\n");
        fprintf(f, "set console -u telnet=1927\n");
        /* Start a terminal emulator for the console port */
#if defined(_WIN32)
        fprintf(f, "set env PATH=%%PATH%%;%%ProgramFiles%%\\PuTTY;%%ProgramFiles(x86)%%\\PuTTY\n");
        fprintf(f, "! start PuTTY telnet://localhost:1927\n");
#elif defined(__linux) || defined(__linux__)
        fprintf(f, "! nohup xterm -e 'telnet localhost 1927' &\n");
#elif defined(__APPLE__)
        fprintf(f, "! osascript -e 'tell application \"Terminal\" to do script \"telnet localhost 1927; exit\"'\n");
#endif
        fprintf(f, "dep sac v[4] 01FFFFF0\n"); /* CPR IGNORE */
        fprintf(f, "dep cpr[0]  00000000E3000004\n");
        fprintf(f, "dep cpr[1]  00001000F3010004\n");
        fprintf(f, "dep cpr[2]  00002000E3020004\n");
        fprintf(f, "dep cpr[3]  00003000E3030004\n");
        fprintf(f, "dep cpr[4]  00006000F3040004\n");
        fprintf(f, "dep cpr[5]  00006010F3050004\n");
        fprintf(f, "dep cpr[6]  00006020F3050004\n");

        fprintf(f, "dep cpu ms 0014\n");
        fprintf(f, "load MU5ELR.bin\n");
        fprintf(f, "load idle.bin\n");
        fclose(f);
    }

    InitDisplay();
    signal(SIGINT, halt_handler);
    panel = sim_panel_start_simulator_debug(sim_path,
        sim_config,
        2,
        debug ? "frontpanel.dbg" : NULL);

    if (!panel) {
        printf("Error starting simulator %s with config %s: %s\n", sim_path, sim_config, sim_panel_get_error());
        goto Done;
    }

    if (debug) {
        sim_panel_set_debug_mode(panel, DBG_XMT | DBG_RCV | DBG_REQ | DBG_RSP);
    }

    if (sim_panel_add_register(panel, "CO", NULL, sizeof(CO), &CO)) {
        printf("Error adding register 'CO': %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_add_register(panel, "DL", NULL, sizeof(DL), &DL)) {
        printf("Error adding register 'DL': %s\n", sim_panel_get_error());
        goto Done;
    }

    if (sim_panel_get_registers(panel, NULL)) {
        printf("Error getting register data: %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_set_display_callback_interval(panel, &DisplayCallback, NULL, 10000)) {
        printf("Error setting automatic display callback: %s\n", sim_panel_get_error());
        goto Done;
    }
    if (!sim_panel_get_registers(panel, NULL)) {
        printf("Unexpected success getting register data: %s\n", sim_panel_get_error());
        goto Done;
    }


    sim_panel_clear_error();
    while (1) {
        size_t i;
        char cmd[512];

        while (sim_panel_get_state(panel) == Halt) {
            DisplayRegisters(panel);
            printf("SIM> ");
            if (!fgets(cmd, sizeof(cmd) - 1, stdin))
                break;
            while (strlen(cmd) && isspace(cmd[strlen(cmd) - 1]))
                cmd[strlen(cmd) - 1] = '\0';
            while (isspace(cmd[0]))
                memmove(cmd, cmd + 1, strlen(cmd));
            for (i = 0; i < strlen(cmd); i++) {
                if (islower(cmd[i]))
                    cmd[i] = toupper(cmd[i]);
            }
            if (!memcmp("BOOT", cmd, 4)) {
                //if (sim_panel_exec_boot(panel, cmd + 4))
                if (my_boot(panel))
                    break;
            }
            else if (!strcmp("STEP", cmd)) {
                if (sim_panel_exec_step(panel))
                    break;
            }
            else if (!strcmp("CONT", cmd)) {
                if (sim_panel_exec_run(panel))
                    break;
            }
            else if (!strcmp("EXIT", cmd))
                goto Done;
            else
                printf("Huh? %s\r\n", cmd);
        }
        while (sim_panel_get_state(panel) == Run)
        {
            SDL_PollEvent(&e);
            if (e.type == SDL_QUIT)
            {
                SDL_Log("Program quit after %i ticks", e.quit.timestamp);
                halt_cpu = 1;
            }
            if (update_display)
            {
                DisplayRegisters(panel);
            }
            if (halt_cpu)
            {
                halt_cpu = 0;
                sim_panel_exec_halt(panel);
            }
        }
    }

Done:
    sim_panel_destroy(panel);

    /* Get rid of pseudo config file created above */
    remove(sim_config);
}
