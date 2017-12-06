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

#define INSTRUCTION_RATE 1000000 /* instructions per second */
#define SCREEN_REFRESH_RATE 50 /* refreshes per second */
#define WIDTH   1365
#define HEIGHT  400
#define DEPTH   32
#define LAMP_HEIGHT 12
#define LAMP_WIDTH 4
#define LAMP_HORIZONTAL_SPACING 12
#define LAMP_VERTICAL_SPACING 42
#define LAMP_OFF_COLOUR 97, 83, 74
#define LAMP_ON_COLOUR 242, 88, 60
#define LAMP_LEVELS 50
#define PANEL_BACKGROUND_COLOUR 114, 111, 104
#define LINE_COLOUR 98, 85, 76
#define LINE_THICKNESS 2
#define LINE_SUB_DIVIDER_THICKNESS 1
#define LAMP_ROWS 6
#define LAMPS_PER_ROW 40
/* Lamp Panel coordinates are the top left of the panel outline*/
#define LAMP_PANEL_X 788
#define LAMP_PANEL_Y 19
#define PANEL_WIDTH (LAMP_HORIZONTAL_SPACING * (LAMPS_PER_ROW + 4))
#define PANEL_HEIGHT (LAMP_VERTICAL_SPACING * LAMP_ROWS)
#define LABEL_BOX_HEIGHT (LAMP_HEIGHT/2)

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
unsigned int CO[32], DL[32],MS[16],SE[16],Interrupt[8];

int update_display = 1;

static void UpdateWholeScreen(void);
static int CalculateLampX(int row, int column);
static int CalculateLampCellX(int row, int column);
static void DrawLamp(int row, int column, int level);
static void DrawLampPanel(void);
SDL_Texture *DrawFilledRectangle(int width, int height, int r, int g, int b);
static void DrawLampPanelOverlayLine(int width, int height, int x, int y);
static void DrawLampRegisterNibbleLabelDivider(int row, int column, int forColumns);
static void DrawLampRegisterNibbleLabelBoundary(int row, int column);
static void DrawLampRegisterHalfBoundaryToLabelDivider(int row, int column);
static void DrawLampRegisterBoundaryToLabelDivider(int row, int column);
static void DrawLampRegisterSubBoundaryToNibbleDivider(int row, int column);
static void DrawLampRegisterSubBoundaryToLabelDivider(int row, int column);
static void DrawLampRegisterBoundaryThick(int row, int column);
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
    if (update_display)
    {
        update_display = 0;
        DrawRegister(5, 4, CO, 32);
        DrawRegister(4, 4, DL, 32);
        DrawRegister(3, 0, MS, 16);
        DrawRegister(3, 16, Interrupt, 8);
        DrawRegister(3, 24, SE, 16);
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

static void DrawLampRegisterNibbleLabelDivider(int row, int column, int forColumns)
{
	int fromX = CalculateLampCellX(row, column);
	int toX = CalculateLampCellX(row, column + forColumns);
	DrawLampPanelOverlayLine(toX - fromX, LINE_SUB_DIVIDER_THICKNESS, fromX, LAMP_PANEL_Y + ((row + 1) * LAMP_VERTICAL_SPACING) - LAMP_HEIGHT);
}

static void DrawLampRegisterNibbleLabelBoundary(int row, int column)
{
	DrawLampPanelOverlayLine(LINE_SUB_DIVIDER_THICKNESS, LABEL_BOX_HEIGHT, CalculateLampX(row, column) - LAMP_WIDTH, LAMP_PANEL_Y + ((row + 1) * LAMP_VERTICAL_SPACING) - (LABEL_BOX_HEIGHT * 2));
}

static void DrawLampRegisterHalfBoundaryToLabelDivider(int row, int column)
{
	DrawLampPanelOverlayLine(LINE_SUB_DIVIDER_THICKNESS, LAMP_VERTICAL_SPACING - (LABEL_BOX_HEIGHT * 4), CalculateLampX(row, column) - LAMP_WIDTH, LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING) + (LABEL_BOX_HEIGHT * 3));
}

static void DrawLampRegisterBoundaryToLabelDivider(int row, int column)
{
	DrawLampPanelOverlayLine(LINE_THICKNESS, LAMP_VERTICAL_SPACING - LABEL_BOX_HEIGHT, CalculateLampX(row, column) - LAMP_WIDTH, LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING));
}

static void DrawLampRegisterSubBoundaryToNibbleDivider(int row, int column)
{
	DrawLampPanelOverlayLine(LINE_SUB_DIVIDER_THICKNESS, LAMP_VERTICAL_SPACING - (LABEL_BOX_HEIGHT * 2), CalculateLampCellX(row, column), LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING));
}

static void DrawLampRegisterSubBoundaryToLabelDivider(int row, int column)
{
    DrawLampPanelOverlayLine(LINE_SUB_DIVIDER_THICKNESS, LAMP_VERTICAL_SPACING - LABEL_BOX_HEIGHT, CalculateLampCellX(row, column), LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING));
}

static void DrawLampRegisterBoundaryThick(int row, int column)
{
    DrawLampPanelOverlayLine(LINE_THICKNESS, LAMP_VERTICAL_SPACING, CalculateLampCellX(row, column), LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING));
}

static void DrawLampRegisterBoundaryThin(int row, int column)
{
    DrawLampPanelOverlayLine(LINE_SUB_DIVIDER_THICKNESS, LAMP_VERTICAL_SPACING, CalculateLampCellX(row, column), LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING));
}

static void DrawLampPanelOverlay(void)
{
    int i;

    /* Outer borders and main row dividers */
    for (i = 0; i <= LAMP_ROWS; i++)
    {
        if (i > 1)
        {
            DrawLampPanelOverlayLine(PANEL_WIDTH, LINE_SUB_DIVIDER_THICKNESS, LAMP_PANEL_X, LAMP_PANEL_Y + (i * LAMP_VERTICAL_SPACING) - (LAMP_HEIGHT / 2));
        }
        else if (i == 1)
        {
            DrawLampPanelOverlayLine(LAMP_HORIZONTAL_SPACING * 8 + LAMP_HORIZONTAL_SPACING / 2 + LAMP_WIDTH / 2, LINE_SUB_DIVIDER_THICKNESS, LAMP_PANEL_X, LAMP_PANEL_Y + (i * LAMP_VERTICAL_SPACING) - LABEL_BOX_HEIGHT);
            DrawLampPanelOverlayLine(LAMP_HORIZONTAL_SPACING * 34 + LAMP_HORIZONTAL_SPACING / 2, LINE_SUB_DIVIDER_THICKNESS, LAMP_PANEL_X + LAMP_HORIZONTAL_SPACING * 10 - LAMP_HORIZONTAL_SPACING/2 + LAMP_WIDTH / 2, LAMP_PANEL_Y + (i * LAMP_VERTICAL_SPACING) - LABEL_BOX_HEIGHT);
        }
        DrawLampPanelOverlayLine(PANEL_WIDTH, LINE_THICKNESS, LAMP_PANEL_X, LAMP_PANEL_Y + (i * LAMP_VERTICAL_SPACING));
    }
    DrawLampPanelOverlayLine(LINE_THICKNESS, PANEL_HEIGHT, LAMP_PANEL_X, LAMP_PANEL_Y);
    DrawLampPanelOverlayLine(LINE_THICKNESS, PANEL_HEIGHT, LAMP_PANEL_X + PANEL_WIDTH, LAMP_PANEL_Y);

    /* row 1 */
    DrawLampRegisterBoundaryThin(0, 8);
    DrawLampRegisterBoundaryThin(0, 9);
    DrawLampRegisterBoundaryThin(0, 14);
    DrawLampRegisterBoundaryThick(0, 20);
    DrawLampRegisterBoundaryThick(0, 29);
	DrawLampRegisterHalfBoundaryToLabelDivider(0, 25);

    /* row 2 */
    DrawLampRegisterNibbleLabelDivider(1, 0, 16);
    DrawLampRegisterBoundaryThin(1, 16);
    DrawLampRegisterBoundaryThick(1, 20);
    DrawLampRegisterBoundaryThick(1, 29);
	DrawLampRegisterSubBoundaryToNibbleDivider(1, 3);
	DrawLampRegisterSubBoundaryToNibbleDivider(1, 7);
	DrawLampRegisterSubBoundaryToNibbleDivider(1, 10);
	DrawLampRegisterSubBoundaryToNibbleDivider(1, 13);
	DrawLampRegisterNibbleLabelBoundary(1, 4);
	DrawLampRegisterNibbleLabelBoundary(1, 8);
	DrawLampRegisterNibbleLabelBoundary(1, 12);

    /* row 3 */
	DrawLampRegisterNibbleLabelDivider(2, 0, 16);
	DrawLampRegisterBoundaryThin(2, 16);
	DrawLampRegisterBoundaryThick(2, 20);
    DrawLampRegisterSubBoundaryToLabelDivider(2, 20);
    DrawLampRegisterSubBoundaryToLabelDivider(2, 24);
    DrawLampRegisterSubBoundaryToLabelDivider(2, 28);
    DrawLampRegisterSubBoundaryToLabelDivider(2, 32);
	DrawLampRegisterSubBoundaryToNibbleDivider(2, 3);
	DrawLampRegisterSubBoundaryToNibbleDivider(2, 7);
	DrawLampRegisterSubBoundaryToNibbleDivider(2, 10);
	DrawLampRegisterSubBoundaryToNibbleDivider(2, 13);
	DrawLampRegisterNibbleLabelBoundary(2, 4);
	DrawLampRegisterNibbleLabelBoundary(2, 8);
	DrawLampRegisterNibbleLabelBoundary(2, 12);

    /* row 4 */
    DrawLampRegisterBoundaryThick(3, 16);
    DrawLampRegisterBoundaryThick(3, 24);

    /* row 5 */
    DrawLampRegisterBoundaryThick(4, 4);
    DrawLampRegisterBoundaryThick(4, 36);
	DrawLampRegisterSubBoundaryToNibbleDivider(5, 5);
	DrawLampRegisterSubBoundaryToNibbleDivider(5, 19);
	DrawLampRegisterSubBoundaryToLabelDivider(4, 8);
    DrawLampRegisterSubBoundaryToLabelDivider(4, 12);
    DrawLampRegisterSubBoundaryToLabelDivider(4, 16);
    DrawLampRegisterSubBoundaryToLabelDivider(4, 20);
    DrawLampRegisterSubBoundaryToLabelDivider(4, 24);
    DrawLampRegisterSubBoundaryToLabelDivider(4, 28);
    DrawLampRegisterSubBoundaryToLabelDivider(4, 32);

    /* row 6 */
    DrawLampRegisterNibbleLabelDivider(5, 4, 32);
//    DrawLampPanelOverlayLine(32 * LAMP_HORIZONTAL_SPACING, LINE_SUB_DIVIDER_THICKNESS, CalculateLampCellX(5, 4), LAMP_PANEL_Y + (6 * LAMP_VERTICAL_SPACING) - LAMP_HEIGHT);
    DrawLampRegisterBoundaryThick(5, 4);
    DrawLampRegisterBoundaryToLabelDivider(5, 36);
    DrawLampRegisterBoundaryToLabelDivider(5, 37);
	DrawLampRegisterNibbleLabelBoundary(5, 8);
	DrawLampRegisterNibbleLabelBoundary(5, 12);
	DrawLampRegisterNibbleLabelBoundary(5, 16);
	DrawLampRegisterNibbleLabelBoundary(5, 20);
	DrawLampRegisterNibbleLabelBoundary(5, 24);
	DrawLampRegisterNibbleLabelBoundary(5, 28);
	DrawLampRegisterNibbleLabelBoundary(5, 32);
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

/* Calculates the X coordinate of where a particular lamp should be */
static int CalculateLampX(int row, int column)
{
    int x;

    if (row < (LAMP_ROWS - 1))
    {
        if (column >= (LAMPS_PER_ROW/2))
        {
            x = LAMP_PANEL_X + LAMP_HORIZONTAL_SPACING + (column + 2) * LAMP_HORIZONTAL_SPACING;
        }
        else
        {
            x = LAMP_PANEL_X + LAMP_HORIZONTAL_SPACING + column * LAMP_HORIZONTAL_SPACING;
        }
    }
    else
    {
        x = LAMP_PANEL_X + LAMP_HORIZONTAL_SPACING + (column + 1) * LAMP_HORIZONTAL_SPACING;
    }

    return x;
}

/* Calculates the X coordinate of where the divider for a particular lamp should be */
static int CalculateLampCellX(int row, int column)
{
    int x;
	if (column == 0)
	{
		x = LAMP_PANEL_X;
	}
	else
	{
		x = CalculateLampX(row, column) - LAMP_WIDTH;
		if (row < (LAMP_ROWS - 1) && column == (LAMPS_PER_ROW / 2))
		{
			x = x - LAMP_HORIZONTAL_SPACING;
		}
	}

    return x;
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

    area.y = LAMP_PANEL_Y + (row * LAMP_VERTICAL_SPACING) + (LAMP_VERTICAL_SPACING/2);
    area.x = CalculateLampX(row, column);
    area.w = LAMP_WIDTH;
    area.h = LAMP_HEIGHT;
    SDL_RenderCopy(sdlRenderer, sprites[level], NULL, &area);
}

static void DrawRegister(int row, int column, unsigned int bits[], UINT8 width)
{
    int i;
    for (i = width - 1; i >= 0; i--)
    {
        DrawLamp(row, column + (width - i - 1), bits[i]);
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

    sim_panel_set_sampling_parameters(panel, INSTRUCTION_RATE / (SCREEN_REFRESH_RATE * LAMP_LEVELS), LAMP_LEVELS);

    if (sim_panel_add_register_bits(panel, "CO", "CPU", 32, CO))
    {
        printf("Error adding register 'CO': %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_add_register_bits(panel, "DL", "CPU", 32, DL))
    {
        printf("Error adding register 'DL': %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_add_register_bits(panel, "MS", "CPU", 16, MS))
    {
        printf("Error adding register 'MS': %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_add_register_bits(panel, "SE", "PROP", 16, SE))
    {
        printf("Error adding register 'SE': %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_add_register_bits(panel, "INTERRUPT", "CPU", 8, Interrupt))
    {
        printf("Error adding register 'INTERRUPT': %s\n", sim_panel_get_error());
        goto Done;
    }

    if (sim_panel_get_registers(panel, NULL)) {
        printf("Error getting register data: %s\n", sim_panel_get_error());
        goto Done;
    }
    if (sim_panel_set_display_callback_interval(panel, &DisplayCallback, NULL, 1000000/ SCREEN_REFRESH_RATE)) {
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
