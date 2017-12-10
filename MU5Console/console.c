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
#include <time.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>

#if defined(_WIN32)
#include <windows.h>
#define usleep(n) Sleep(n/1000)
#else
#include <unistd.h>
#endif

#define INSTRUCTION_RATE 1000000 /* instructions per second */
#define SCREEN_REFRESH_RATE 25 /* refreshes per second */
#define WIDTH   1900 /* reduced in size, aspect ratio should be about 3.4:1 */
#define HEIGHT  800
#define DEPTH   32
#define LAMP_HEIGHT 30
#define LAMP_WIDTH 10
#define LAMP_HORIZONTAL_SPACING (LAMP_WIDTH * 3)
#define LAMP_VERTICAL_SPACING (LAMP_HEIGHT * 3 + LAMP_HEIGHT / 2)

#define LAMP_OFF_COLOUR 97, 83, 74
#define LAMP_ON_COLOUR 242, 88, 60
#define LAMP_LEVELS 50
#define PANEL_BACKGROUND_COLOUR 114, 111, 104 /* TODO: Use SDL_Color instead */
#define LINE_COLOUR 98, 85, 76
#define LINE_THICKNESS 2
#define LINE_SUB_DIVIDER_THICKNESS 1
#define LAMP_ROWS 6
#define LAMPS_PER_ROW 40
#define PANEL_WIDTH (LAMP_HORIZONTAL_SPACING * (LAMPS_PER_ROW + 4))
#define PANEL_HEIGHT (LAMP_VERTICAL_SPACING * LAMP_ROWS)
/* Lamp Panel coordinates are the top left of the panel outline*/
#define LAMP_PANEL_X (WIDTH - PANEL_WIDTH - (1 * LAMP_HORIZONTAL_SPACING))
#define LAMP_PANEL_Y 19
#define TIME_PANEL_X 50
#define TIME_PANEL_Y (LAMP_PANEL_Y)
#define TIME_PANEL_MARGIN 5
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
static TTF_Font *ttfLabel;
static TTF_Font *ttfTime;
static const SDL_Color white = { 255, 255, 255, 0 };
static const SDL_Color black = { 0,   0,   0, 0 };
static const SDL_Color panelBackground = { PANEL_BACKGROUND_COLOUR, 0 };
static const SDL_Color lineColour = { LINE_COLOUR, 0 };
static const SDL_Color lampOnColour = { LAMP_ON_COLOUR, 0 };
static const SDL_Color lampOffColour = { LAMP_OFF_COLOUR, 0 };

/* Registers visible on the Front Panel */
unsigned int CO[32], DL[32],MS[16],SE[16],Interrupt[8];
UINT64 TimeUpper;
UINT64 TimeLower;

int update_display = 1;

int clock_gettime(int clk_id, struct timespec *tp); /* defined in sim_frontpanel.c */
static void UpdateWholeScreen(void);
static void DisplayTime(void);
static int CalculateLampX(int row, int column);
static int CalculateLampCellX(int row, int column);
static void DrawLamp(int row, int column, int level);
static void DrawLampPanel(void);
static void DrawPanelText(int x, int y, char *text, TTF_Font *font, int updateable);
SDL_Texture *DrawFilledRectangle(int width, int height, SDL_Color colour);
static void DrawLampPanelOverlayLine(int width, int height, int x, int y);
static void DrawLampRegisterNibbleLabelDivider(int row, int column, int forColumns);
static void DrawLampRegisterNibbleLabelBoundary(int row, int column);
static void DrawLampRegisterHalfBoundaryToLabelDivider(int row, int column);
static void DrawLampRegisterBoundaryToLabelDivider(int row, int column);
static void DrawLampRegisterSubBoundaryToNibbleDivider(int row, int column);
static void DrawLampRegisterSubBoundaryToLabelDivider(int row, int column);
static void DrawLampRegisterBoundaryThick(int row, int column);
static void DrawPanelUpperLabel(int row, int column, char *text);
static void DrawPanelLowerLabel(int row, int column, char *text);
static void DrawLampPanelOverlay(void);
static void DrawRegister(int row, int column, unsigned int bits[], UINT8 width);
static int SetupRegister(char *device, char *name, UINT64 *address);
static int SetupSampledRegister(char *device, char *name, int bits, int *data);

static void
DisplayCallback(PANEL *panel, unsigned long long simulation_time, void *context)
{
    update_display = 1;
}

static void DisplayRegisters(void)
{
    if (update_display)
    {
        update_display = 0;
        DrawRegister(5, 4, CO, 32);
        DrawRegister(4, 4, DL, 32);
        DrawRegister(3, 0, MS, 16);
        DrawRegister(3, 16, Interrupt, 8);
        DrawRegister(3, 24, SE, 16);
		DisplayTime();
        UpdateWholeScreen();
    }
}

static void DisplayTime(void)
{
	char time[10];
	int hours;
	int mins;
	int secs;
	static SDL_Rect timeArea;
	static SDL_Texture *timePanelTexture;

	if (timePanelTexture == NULL)
	{
		timeArea.x = TIME_PANEL_X;
		timeArea.y = TIME_PANEL_Y;
		TTF_SizeText(ttfTime, "00 00 00", &timeArea.w, &timeArea.h);
		timeArea.w += 2 * TIME_PANEL_MARGIN;
		timeArea.h += 2 * TIME_PANEL_MARGIN;
		timePanelTexture = DrawFilledRectangle(timeArea.w, timeArea.h, black);
	}

	hours = (TimeUpper >> 8) & 0xFF;
	mins = TimeUpper & 0xFF;
	secs = (TimeLower >> 8) & 0xFF;
	sprintf(time, "%02X %02X %02X", hours, mins, secs);
	SDL_RenderCopy(sdlRenderer, timePanelTexture, NULL, &timeArea);
	DrawPanelText(TIME_PANEL_X + TIME_PANEL_MARGIN, TIME_PANEL_Y + TIME_PANEL_MARGIN, time, ttfTime, TRUE);

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

static void DrawPanelText(int x, int y, char *text, TTF_Font *font, int updateable)
{
	SDL_Surface *surface;
	SDL_Texture *texture;
	SDL_Rect dstArea;

	if (updateable)
	{
		surface = TTF_RenderText_Shaded(font, text, lampOnColour, black);
	}
	else
	{
		surface = TTF_RenderText_Solid(font, text, black);
	}

	texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);

	dstArea.x = x;
	dstArea.y = y;
	dstArea.h = surface->h;
	dstArea.w = surface->w;

	SDL_RenderCopy(sdlRenderer, texture, NULL, &dstArea);
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

static void DrawLampPanelOverlayLine(int width, int height, int x, int y)
{
    SDL_Rect dstArea;
	SDL_Texture *texture;
    dstArea.h = height;
    dstArea.w = width;
    dstArea.x = x;
    dstArea.y = y;
	texture = DrawFilledRectangle(width, height, lineColour);
    SDL_RenderCopy(sdlRenderer, texture, NULL, &dstArea);
	SDL_DestroyTexture(texture);
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

static void DrawPanelUpperLabel(int row, int column, char *text)
{
	DrawPanelText(CalculateLampCellX(row, column) + (LAMP_HORIZONTAL_SPACING / 2), LAMP_PANEL_Y + ((row) * LAMP_VERTICAL_SPACING) + (LAMP_HEIGHT / 2) + 2, text, ttfLabel, FALSE);
}

static void DrawPanelLowerLabel(int row, int column, char *text)
{
	DrawPanelText(CalculateLampCellX(row, column) + (LAMP_HORIZONTAL_SPACING / 2), LAMP_PANEL_Y + ((row + 1) * LAMP_VERTICAL_SPACING) - (LAMP_HEIGHT / 2) + 2, text, ttfLabel, FALSE);
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

	DrawPanelLowerLabel(0, 3, "PROP WAIT");
	DrawPanelLowerLabel(0, 11, "SAC");
	DrawPanelLowerLabel(0, 17, "OBS");
	DrawPanelLowerLabel(0, 23, "BUSY SIGNALS");
	DrawPanelLowerLabel(0, 34, "TEST SWITCHES");

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

	DrawPanelLowerLabel(1, 7, "(???) PROP FINAL FUNCTION");
	DrawPanelLowerLabel(1, 22, "FINGER FLIP FLOPS");
	DrawPanelLowerLabel(1, 32, "PARITY SWITCHES OFF NORMAL");

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

	DrawPanelLowerLabel(2, 7, "(???) PROP FIRST FUNCTION");
	DrawPanelLowerLabel(2, 28, "TELETYPE BUFFER");

    /* row 4 */
    DrawLampRegisterBoundaryThick(3, 16);
    DrawLampRegisterBoundaryThick(3, 24);

	DrawPanelLowerLabel(3, 5, "MACHINE STATUS");
	DrawPanelLowerLabel(3, 19, "INTERRUPT ENTRY");
	DrawPanelLowerLabel(3, 31, "SYSTEM ERROR");

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

	DrawPanelLowerLabel(4, 0, "PROP VALID");
	DrawPanelLowerLabel(4, 19, "DISPLAY (EDL)");
	DrawPanelLowerLabel(4, 36, "LOCAL ST. FAIL SOFT MODE");

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

	DrawPanelUpperLabel(5, 11, "SEGMENT");
	DrawPanelUpperLabel(5, 27, "LINE");
	DrawPanelLowerLabel(5, 0, "PROCESS NUMBER");
	DrawPanelLowerLabel(5, 19, "CONTROL");
}

SDL_Texture *DrawFilledRectangle(int width, int height, SDL_Color colour)
{
    SDL_Surface *tempSurface;
    SDL_Texture *result;
    tempSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, colour.r, colour.g, colour.b));
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
	SDL_Color colour;
    int i;

    if (!sprites[0])
    {
        for (i = 0; i <= LAMP_LEVELS; i++)
        {
			colour.r = lampOffColour.r + (i * ((lampOnColour.r - lampOffColour.r) / LAMP_LEVELS));
			colour.g = lampOffColour.g + (i * ((lampOnColour.g - lampOffColour.g) / LAMP_LEVELS));
			colour.b = lampOffColour.b + (i * ((lampOnColour.b - lampOffColour.b) / LAMP_LEVELS));
			sprites[i] = DrawFilledRectangle(LAMP_WIDTH, LAMP_HEIGHT, colour);
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
	// TODO: Tidy up all the creation so anything that could fail or not be found is done first and clean up.
    int result = 0;
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL: unable to init: %s\n", SDL_GetError());
    }
	else
	{
		if (TTF_Init() < 0)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL: couldn't initialize TTF: %s\n", SDL_GetError());
		}
		else
		{
			result = 1;
		}
	}

	if (result)
    {
        if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &sdlWindow, &sdlRenderer) != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL: unable to create window and renderer: %s\n",SDL_GetError());
			result = 0;
        }
        else
        {

            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
            SDL_RenderSetLogicalSize(sdlRenderer, WIDTH, HEIGHT);
			//SDL_RenderSetScale(sdlRenderer, 1, 2);
            /* Make grey console background */
            SDL_SetRenderDrawColor(sdlRenderer, PANEL_BACKGROUND_COLOUR, 255);
            SDL_RenderClear(sdlRenderer);


			ttfLabel = TTF_OpenFont("\\windows\\fonts\\cour.ttf", LAMP_HEIGHT / 3);
			if (ttfLabel == NULL)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL: couldn't load font %s: %s\n", "\\windows\\fonts\\cour.ttf", SDL_GetError());
				result = 0;
			}

			ttfTime = TTF_OpenFont("ledreali.ttf", LAMP_HEIGHT * 2);
			if (ttfTime == NULL)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL: couldn't load font %s: %s\n", "ledreali.ttf", SDL_GetError());
				result = 0;
			}

            DrawLampPanel();
			DisplayRegisters();

            UpdateWholeScreen();
        }
    }

    return result;
}

static
void InitDisplay(void)
{
    CreatePanel();
    printf("^C to Halt, Click to boot\n");
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

static int SetupRegister(char *device, char *name, UINT64 *address)
{
	int result = 1;
	if (sim_panel_add_register(panel, name, device, sizeof(UINT64), address))
	{
		SDL_Log("Error adding register %s %s: %s\n", device, name, sim_panel_get_error());
		result = 0;
	}

	return result;
}

static int SetupSampledRegister(char *device, char *name, int bits, int *data)
{
	int result = 1;
	if (sim_panel_add_register_bits(panel, name, device, bits, data))
	{
		SDL_Log("Error adding sampled register %s %s: %s\n", device, name, sim_panel_get_error());
		result = 0;
	}

	return result;
}

void LogOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
	struct timespec time_now;
	clock_gettime(0, &time_now);
	printf("%lld.%03d %s\n", (long long)(time_now.tv_sec), (int)(time_now.tv_nsec / 1000000), message);
}

int
main(int argc, char *argv[])
{
    SDL_Event e;
    FILE *f;
    int debug = 0;
	int setupOk = 1;

	SDL_LogSetOutputFunction(LogOutputFunction, NULL);

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
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Error starting simulator %s with config %s: %s\n", sim_path, sim_config, sim_panel_get_error());
        goto Done;
    }

    if (debug) {
		sim_panel_set_debug_mode(panel, DBG_XMT | DBG_RCV | DBG_REQ | DBG_RSP);
    }

    sim_panel_set_sampling_parameters(panel, INSTRUCTION_RATE / (SCREEN_REFRESH_RATE * LAMP_LEVELS), LAMP_LEVELS);

	setupOk &= SetupRegister("CON", "TIMEUPPER", &TimeUpper);
	setupOk &= SetupRegister("CON", "TIMELOWER", &TimeLower);
	setupOk &= SetupSampledRegister("CPU", "CO", 32, CO);
	setupOk &= SetupSampledRegister("CPU", "DL", 32, DL);
	setupOk &= SetupSampledRegister("CPU", "MS", 16, MS);
	setupOk &= SetupSampledRegister("CPU", "INTERRUPT", 8, Interrupt);
	setupOk &= SetupSampledRegister("PROP", "SE", 16, SE);

    if (sim_panel_set_display_callback_interval(panel, &DisplayCallback, NULL, 1000000/ SCREEN_REFRESH_RATE)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Error setting automatic display callback: %s\n", sim_panel_get_error());
        goto Done;
    }

	if (setupOk)
	{
		sim_panel_clear_error();
		do
		{
			SDL_PollEvent(&e);
			if (e.type == SDL_QUIT)
			{
				SDL_Log("Program quit after %i ticks", e.quit.timestamp);
				halt_cpu = 1;
			}
			else if (e.type == SDL_MOUSEBUTTONUP)
			{
				if (sim_panel_get_state(panel) != Run)
				{
					my_boot(panel);
				}
			}
			if (update_display)
			{
				DisplayRegisters();
			}
			if (halt_cpu)
			{
				halt_cpu = 0;
				sim_panel_exec_halt(panel);
			}
		} while (e.type != SDL_QUIT);
	}

Done:
    sim_panel_destroy(panel);

    /* Get rid of pseudo config file created above */
    remove(sim_config);
}
