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

#define WIDTH   1365
#define HEIGHT  400
#define DEPTH   32
#define LAMP_HEIGHT 12
#define LAMP_WIDTH 4
#define LAMP_HORIZONTAL_SPACING 12
#define LAMP_VERTICAL_SPACING 42
#define LAMP_OFF_COLOUR 97, 83, 74
#define LAMP_ON_COLOUR 254, 254, 190
#define LAMP_LEVELS 50
#define PANEL_BACKGROUND_COLOUR 114, 111, 104
#define LINE_COLOUR 98, 85, 76
#define LINE_THICKNESS 2
#define LAMP_ROWS 6
#define LAMPS_PER_ROW 40
#define LAMP_PANEL_X 800
#define LAMP_PANEL_Y 40

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
unsigned int CO[32], DL[32];

int update_display = 1;

static void UpdateWholeScreen(void);
static void DrawLamp(int row, int column, int level);
static void DrawLampPanel(void);
SDL_Texture *DrawFilledRectangle(int width, int height, int r, int g, int b);
static void DrawLampPanelOverlayLine(int width, int height, int x, int y);
static void DrawLampPanelOverlay(void);
static void DrawRegister(int row, int column, unsigned int bits[], UINT8 width);

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

    if (update_display)
    {
        update_display = 0;
        DrawRegister(4, 4, DL, 32);
        DrawRegister(5, 5, CO, 32);
        UpdateWholeScreen();
    }
}

static void DrawLampPanel(void)
{
    int row;
    int column;
    DrawLampPanelOverlay();
    for (row = 0; row < LAMP_ROWS; row++)
    {
        for (column = 0; column < LAMPS_PER_ROW; column++)
        {
            DrawLamp(row, column, 0);
        }
    }
}

static void DrawLampPanelOverlayLine(int width, int height, int x, int y)
{
    SDL_Rect dstArea;
    dstArea.h = height;
    dstArea.w = width;
    dstArea.x = x;
    dstArea.y = y;
    SDL_RenderCopy(sdlRenderer, DrawFilledRectangle(width, height, LINE_COLOUR), NULL, &dstArea);
}

static void DrawLampPanelOverlay(void)
{
    int i;
    int panelWidth = LAMP_HORIZONTAL_SPACING * (LAMPS_PER_ROW + 4);
    int panelHeight = LAMP_VERTICAL_SPACING * LAMP_ROWS;
    int panelX = LAMP_PANEL_X - LAMP_HORIZONTAL_SPACING;
    int panelY = LAMP_PANEL_Y - (LAMP_VERTICAL_SPACING / 2);
    for (i = 0; i <= LAMP_ROWS; i++)
    {
        DrawLampPanelOverlayLine(panelWidth, LINE_THICKNESS, panelX, panelY + (i * LAMP_VERTICAL_SPACING));
    }
    DrawLampPanelOverlayLine(LINE_THICKNESS, panelHeight, panelX, panelY);
    DrawLampPanelOverlayLine(LINE_THICKNESS, panelHeight, panelX + panelWidth, panelY);
}

SDL_Texture *DrawFilledRectangle(int width, int height, int r, int g, int b)
{
    SDL_Surface *tempSurface;
    SDL_Texture *result;
    tempSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, r, g, b));
    result = SDL_CreateTextureFromSurface(sdlRenderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    return result;
}

static void DrawLamp(int row, int column, int level)
{
    static SDL_Texture * sprites[LAMP_LEVELS + 1];
    SDL_Rect area;
    int fullOff[3] = { LAMP_OFF_COLOUR };
    int fullOn[3] = { LAMP_ON_COLOUR };
    int i;

    if (!sprites[0])
    {
        for (i = 0; i <= LAMP_LEVELS; i++)
        {
            sprites[i] = DrawFilledRectangle(
                LAMP_WIDTH,
                LAMP_HEIGHT,
                fullOff[0] + (i * ((fullOn[0] - fullOff[0]) / LAMP_LEVELS)),
                fullOff[1] + (i * ((fullOn[1] - fullOff[1]) / LAMP_LEVELS)),
                fullOff[2] + (i * ((fullOn[2] - fullOff[2]) / LAMP_LEVELS)));
        }
    }

    area.y = LAMP_PANEL_Y + row * LAMP_VERTICAL_SPACING;
    if (row < (LAMP_ROWS - 1))
    {
        if (column >= 20)
        {
            area.x = LAMP_PANEL_X + (column + 2) * LAMP_HORIZONTAL_SPACING;
        }
        else
        {
            area.x = LAMP_PANEL_X + column * LAMP_HORIZONTAL_SPACING;
        }
    }
    else
    {
        area.x = LAMP_PANEL_X + (column + 1) * LAMP_HORIZONTAL_SPACING;
    }
    area.w = LAMP_WIDTH;
    area.h = LAMP_HEIGHT;
    SDL_RenderCopy(sdlRenderer, sprites[level], NULL, &area);
}

static void DrawRegister(int row, int column, unsigned int bits[], UINT8 width)
{
    int i;
    for (i = width - 1; i >= 0; i--)
    {
        DrawLamp(row, column + (width - i), bits[i]);
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
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL: unable to create window and renderer: %s\n",SDL_GetError());
        }
        else
        {

            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
            SDL_RenderSetLogicalSize(sdlRenderer, WIDTH, HEIGHT);
            /* Make grey console background */
            SDL_SetRenderDrawColor(sdlRenderer, PANEL_BACKGROUND_COLOUR, 255);
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

            DrawLampPanel();
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

    sim_panel_set_sampling_parameters(panel, 10, LAMP_LEVELS);

    if (sim_panel_add_register_bits(panel, "CO", NULL, 32, CO)) {
        printf("Error adding register 'CO': %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_add_register_bits(panel, "DL", NULL, 32, DL)) {
        printf("Error adding register 'DL': %s\n", sim_panel_get_error());
        goto Done;
    }

    if (sim_panel_get_registers(panel, NULL)) {
        printf("Error getting register data: %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_set_display_callback_interval(panel, &DisplayCallback, NULL, 50000)) {
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
