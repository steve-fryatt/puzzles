/* Copyright 2024, Stephen Fryatt
 *
 * This file is part of Puzzles:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: frontend.c
 *
 * Frontend collection implementation.
 */

/* ANSI C header files */

#include <stdarg.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/types.h"
#include "oslib/osword.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"

/* Application header files */

#include "frontend.h"

#include "core/puzzles.h"

#include "game_window.h"

/* The game collection data structure. */

struct frontend {
	int x_size;				/**< The X size of the window, in game pixels.	*/
	int y_size;				/**< The Y size of the window, in game pixels.	*/

	midend *me;				/**< The associated midend.			*/

	struct game_window_block *window;	/**< The associated game window instance.	*/

	struct frontend *next;	/**< The next game in the list, or null.	*/
};

/* Constants. */

/**
 * The maximum length of message that fatal() can report, once expanded.
 */

#define FRONTEND_MAX_FATAL_MESSAGE 256

/* Global variables. */

/**
 * The list of active game windows.
 */

static struct frontend *frontend_list = NULL;

/* Static function prototypes. */

static void riscos_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text);
static void riscos_draw_rect(void *handle, int x, int y, int w, int h, int colour);
static void riscos_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour);
static void riscos_draw_polygon(void *handle, const int *coords, int npoints, int fillcolour, int outlinecolour);
static void riscos_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour);
static void riscos_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour);
static void riscos_draw_update(void *handle, int x, int y, int w, int h);
static void riscos_clip(void *handle, int x, int y, int w, int h);
static void riscos_unclip(void *handle);
static void riscos_start_draw(void *handle);
static void riscos_end_draw(void *handle);
static void riscos_status_bar(void *handle, const char *text);
static blitter *riscos_blitter_new(void *handle, int w, int h);
static void riscos_blitter_free(void *handle, blitter *bl);
static void riscos_blitter_save(void *handle, blitter *bl, int x, int y);
static void riscos_blitter_load(void *handle, blitter *bl, int x, int y);

/* The drawing API. */

static const struct drawing_api riscos_drawing = {
	riscos_draw_text,
	riscos_draw_rect,
	riscos_draw_line,
	riscos_draw_polygon,
	riscos_draw_circle,
	riscos_draw_update,
	riscos_clip,
	riscos_unclip,
	riscos_start_draw,
	riscos_end_draw,
	riscos_status_bar,
	riscos_blitter_new,
	riscos_blitter_free,
	riscos_blitter_save,
	riscos_blitter_load,

	/* The printing API. */

	NULL, // riscos_begin_doc,
	NULL, // riscos_begin_page,
	NULL, // riscos_begin_puzzle,
	NULL, // riscos_end_puzzle,
	NULL, // riscos_end_page,
	NULL, // riscos_end_doc,
	NULL, // riscos_line_width,
	NULL, // riscos_line_dotted,

	/* Text fallback. */

	NULL, // riscos_text_fallback,

	/* Thick lines. */

	NULL, // riscos_draw_thick_line,
};

/**
 * Initialise a new game and open its window.
 */

void frontend_create_instance(void)
{
	struct frontend *new;

	/* Allocate the memory for the instance from the heap. */

	new = malloc(sizeof(struct frontend));
	if (new == NULL) {
		error_msgs_report_error("NoMemNewGame");
		return;
	}

	debug_printf("Creating a new game collection instance: block=0x%x", new);

	/* Link the game into the list, and initialise critical data. */

	new->next = frontend_list;
	frontend_list = new;

	new->window = NULL;

	new->x_size = 800;
	new->y_size = 800;

	/* Create the game window. */

	new->window = game_window_create_instance(new);
	if (new->window == NULL) {
		frontend_delete_instance(new);
		return;
	}

	/* Create the midend, and agree the window size. */

	new->me = midend_new(new, gamelist[1], &riscos_drawing, new->window);
	if (new->me == NULL) {
		frontend_delete_instance(new);
		return;
	}

	midend_new_game(new->me);

	midend_size(new->me, &(new->x_size), &(new->y_size), false, 1.0);

	debug_printf("Agreed on canvas x=%d, y=%d", new->x_size, new->y_size);

	game_window_create_canvas(new->window, new->x_size, new->y_size);

	midend_redraw(new->me);

}

/**
 * Delete a frontend instance.
 *
 * \param *fe	The instance to be deleted.
 */

void frontend_delete_instance(struct frontend *fe)
{
	struct frontend **list;

	if (fe == NULL)
		return;

	debug_printf("Deleting a game instance: block=0x%x", fe);

	/* Delink the instance from the list. */

	list = &frontend_list;

	while (*list != NULL && *list != fe)
		list = &((*list)->next);

	if (*list != NULL)
		*list = fe->next;

	/* Delete the window. */

	if (fe->window != NULL)
		game_window_delete_instance(fe->window);

	/* Delete the midend. */

	if (fe->me != NULL)
		midend_free(fe->me);

	/* Deallocate the instance block. */

	free(fe);
}

/* Below this point are the draing API calls. */

static void riscos_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text)
{
	debug_printf("\\ODraw Text");
}

static void riscos_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
	debug_printf("\\ODraw rectangle");
}

static void riscos_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
	debug_printf("\\oDraw Line from %d,%d to %d,%d in colour %d", x1, y1, x2, y2, colour);

	game_window_plot(handle, os_PLOT_SOLID | os_MOVE_TO, x1, y1);
	game_window_plot(handle, os_PLOT_SOLID | os_PLOT_TO, x2, y2);
}

static void riscos_draw_polygon(void *handle, const int *coords, int npoints, int fillcolour, int outlinecolour)
{
	debug_printf("\\ODraw Polygon");
}

static void riscos_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
	debug_printf("\\ODraw Circle");
}

static void riscos_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour)
{
	debug_printf("\\ODraw Thick Line");
}

static void riscos_draw_update(void *handle, int x, int y, int w, int h)
{
	debug_printf("\\ODraw Update");
}

static void riscos_clip(void *handle, int x, int y, int w, int h)
{
	debug_printf("\\OClip");
}

static void riscos_unclip(void *handle)
{
	debug_printf("\\OUnclip");
}

static void riscos_start_draw(void *handle)
{
	debug_printf("\\GStart Draw");

	game_window_start_draw(handle);
}

static void riscos_end_draw(void *handle)
{
	debug_printf("\\GEnd Draw");

	game_window_end_draw(handle);
}

static void riscos_status_bar(void *handle, const char *text)
{
	debug_printf("\\OStatus Bar");
}

static blitter *riscos_blitter_new(void *handle, int w, int h)
{
	debug_printf("\\OBlitter New");

	return NULL;
}

static void riscos_blitter_free(void *handle, blitter *bl)
{
	debug_printf("\\OBlitter Free");
}

static void riscos_blitter_save(void *handle, blitter *bl, int x, int y)
{
	debug_printf("\\OBlitter Save");
}

static void riscos_blitter_load(void *handle, blitter *bl, int x, int y)
{
	debug_printf("\\OBlitter Load");
}



/* Below this point are the functions that the frontend must provide
 * for the midend.
 *
 * Prototypes are in core/puzzles.h
 */

/**
 * Obtain a random seed for the midend to use. In line with the suggestion
 * in the documentation, we do this by requesting a five byte RTC value
 * from the OS.
 * 
 * \param **randseed		Pointer to a variable to hold a pointer to the
 *				random seed data.
 * \param *randseedsize		Pointer to variable to hold the size of the
 *				random seed data on exit.
 */

void get_random_seed(void **randseed, int *randseedsize)
{
	oswordreadclock_utc_block *rtc;

	debug_printf("Get Random Seed");

	rtc = malloc(sizeof(oswordreadclock_utc_block));
	if (rtc == NULL) {
		*randseed = NULL;
		randseedsize = 0;
		return;
	}

	rtc->op = oswordreadclock_OP_UTC;
	oswordreadclock_utc(rtc);

	*randseed = &(rtc->utc);
	*randseedsize = sizeof(os_date_and_time);
}

void activate_timer(frontend *fe)
{
	debug_printf("\\OActivate Timer");
}

void deactivate_timer(frontend *fe)
{
	debug_printf("\\ODeactivate Timer");
}

/**
 * Report a fatal error to the user, using the standard printf() syntax
 * and functionality. Expanded text is limited to 256 characters including
 * a null terminator.
 * 
 * This function does not return.
 *
 * \param *fmt			A standard printf() formatting string.
 * \param ...			Additional printf() parameters as required.
 */

void fatal(const char *fmt, ...)
{
	char	s[FRONTEND_MAX_FATAL_MESSAGE];
	va_list	ap;

	va_start(ap, fmt);
	vsnprintf(s, FRONTEND_MAX_FATAL_MESSAGE, fmt, ap);
	va_end(ap);

	s[FRONTEND_MAX_FATAL_MESSAGE - 1] = '\0';
	error_report_fatal(s);
}

/**
 * Return details of the preferred default colour, which will be
 * "Wimp Light Grey".
 * 
 * \param *fe			The frontend handle.
 * \param *output		An array to hold the colour values.
 */

void frontend_default_colour(frontend *fe, float *output)
{
	byte pal_data[80];
	os_palette *palette = (os_palette *) &pal_data;
	os_error *error;

	debug_printf("\\OFrontend Default Colour");

	output[0] = output[1] = output[2] = 1.0f;

	error = xwimp_read_palette(palette);
	if (error != NULL)
		return;

	output[0] = ((float) ((palette->entries[1] >>  8) & 0xff)) / 255.0f;
	output[1] = ((float) ((palette->entries[1] >> 16) & 0xff)) / 255.0f;
	output[2] = ((float) ((palette->entries[1] >> 24) & 0xff)) / 255.0f;
}
