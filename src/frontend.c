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

	new->x_size = 200;
	new->y_size = 200;

	/* Create the game window. */

	new->window = game_window_create_instance(new);
	if (new->window == NULL) {
		frontend_delete_instance(new);
		return;
	}
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

	/* Deallocate the instance block. */

	free(fe);
}

/* Below this point are the draing API calls. */

static void riscos_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text)
{}

static void riscos_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{}

static void riscos_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{}

static void riscos_draw_polygon(void *handle, const int *coords, int npoints, int fillcolour, int outlinecolour)
{}

static void riscos_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{}

static void riscos_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour)
{}

static void riscos_draw_update(void *handle, int x, int y, int w, int h)
{}

static void riscos_clip(void *handle, int x, int y, int w, int h)
{}

static void riscos_unclip(void *handle)
{}

static void riscos_start_draw(void *handle)
{}

static void riscos_end_draw(void *handle)
{}

static void riscos_status_bar(void *handle, const char *text)
{}

static blitter *riscos_blitter_new(void *handle, int w, int h)
{
	return NULL;
}

static void riscos_blitter_free(void *handle, blitter *bl)
{}

static void riscos_blitter_save(void *handle, blitter *bl, int x, int y)
{}

static void riscos_blitter_load(void *handle, blitter *bl, int x, int y)
{}



/* Below this point are the functions that the frontend must provide
 * for the midend.
 *
 * Prototypes are in core/puzzles.h
 */

void get_random_seed(void **randseed, int *randseedsize)
{
}

void activate_timer(frontend *fe)
{

}

void deactivate_timer(frontend *fe)
{

}

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

void frontend_default_colour(frontend *fe, float *output)
{

}
