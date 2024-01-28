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
#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/osword.h"
#include "oslib/wimp.h"

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

	struct frontend *next;			/**< The next game in the list, or null.	*/
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
 * 
 * \param game_index	The index into gamelist[] of the required game.
 * \param *pointer	The pointer at which to open the game.
 */

void frontend_create_instance(int game_index, wimp_pointer *pointer)
{
	struct frontend *new;
	osbool status_bar = FALSE;
	int number_of_colours = 0;
	float *colours = NULL;

	/* Sanity check the game index that we're to use. */

	if (game_index < 0 || game_index >= gamecount)
		return;

	hourglass_on();

	/* Allocate the memory for the instance from the heap. */

	new = malloc(sizeof(struct frontend));
	if (new == NULL) {
		hourglass_off();
		error_msgs_report_error("NoMemNewGame");
		return;
	}

	/* Initialise the critical entries. */

	new->me = NULL;
	new->window = NULL;

	debug_printf("Creating a new game collection instance: block=0x%x", new);

	/* Link the game into the list, and initialise critical data. */

	new->next = frontend_list;
	frontend_list = new;

	new->window = NULL;

	/* Allow the puzzles to fill up to 3/4 of the screen area. */

	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_XWIND_LIMIT, &(new->x_size));
	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_YWIND_LIMIT, &(new->y_size));

	new->x_size *= 0.75;
	new->y_size *= 0.75;

	/* Create the game window. */

	new->window = game_window_create_instance(new, gamelist[game_index]->name);
	if (new->window == NULL) {
		hourglass_off();
		frontend_delete_instance(new);
		return;
	}

	/* Create the midend, and agree the window size. */

	new->me = midend_new(new, gamelist[game_index], &riscos_drawing, new->window);
	if (new->me == NULL) {
		hourglass_off();
		frontend_delete_instance(new);
		return;
	}

	midend_new_game(new->me);

	midend_size(new->me, &(new->x_size), &(new->y_size), false, 1.0);

	status_bar = midend_wants_statusbar(new->me) ? TRUE : FALSE;

	debug_printf("Agreed on canvas x=%d, y=%d", new->x_size, new->y_size);

	colours = midend_colours(new->me, &number_of_colours);

	game_window_create_canvas(new->window, new->x_size, new->y_size, colours, number_of_colours);
	game_window_open(new->window, status_bar, pointer);

	midend_redraw(new->me);

	hourglass_off();
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

/**
 * Process key events from the game window. These are any
 * mouse click or keypress events handled by the midend.
 *
 * \param *fe		The instance to which the event relates.
 * \param x		The X coordinate of the event.
 * \param y		The Y coordinate of the event.
 * \param button	The button details for the event.
 * \return		TRUE if the event was accepted; otherwise FALSE.
 */

osbool frontend_handle_key_event(struct frontend *fe, int x, int y, int button)
{
	int return_value = PKR_UNUSED;

	debug_printf("Received event: x=%d, y=%d, button=%d", x, y, button);

	if (fe != NULL && fe->me != NULL)
		return_value = midend_process_key(fe->me, x, y, button);

	// TODO -- Handle PKR_QUIT. This could be tricky, as if we delete
	// ourselves now, the game window might struggle when there's no
	// instance left on return!?

	return (return_value == PKR_UNUSED) ? FALSE : TRUE;
}

/**
 * Process a periodic callback from the game window, passing it on
 * to the midend.
 * 
 * \param *fe			The frontend handle.
 * \param tplus			The time in seconds since the last
 *				callback event.
 */

void frontend_timer_callback(frontend *fe, float tplus)
{
	debug_printf("Timer call after %f seconds", tplus);

	if (fe != NULL && fe->me != NULL)
		midend_timer(fe->me, tplus);
}

/**
 * Return details that the game window might need in order to open
 * a window menu.
 *
 * \param *fe			The frontend handle.
 * \param **presets		Pointer to variable in which to return
 *				a pointer to the midend presets menu.
 * \param *limit		Pointer to a variable in which to return
 *				the number of entries in the presets menu.
 * \param *current_preset	Pointer to a variable in which to return
 *				the currently-active preset.
 * \param *can_undo		Pointer to variable in which to return
 *				the undo state of the midend.
 * \param *can_redo		Pointer to variable in which to return
 *				the redo state of the midend.
 */

void frontend_get_menu_info(struct frontend *fe, struct preset_menu **presets, int *limit, int *current_preset, osbool *can_undo, osbool *can_redo)
{
	if (fe == NULL || fe->me == NULL)
		return;

	if (can_undo != NULL)
		*can_undo = midend_can_undo(fe->me) ? TRUE : FALSE;

	if (can_redo != NULL)
		*can_redo = midend_can_redo(fe->me) ? TRUE : FALSE;

	if (presets != NULL)
		*presets = midend_get_presets(fe->me, limit);

	if (current_preset != NULL)
		*current_preset = midend_which_preset(fe->me);
}

/* Below this point are the draing API calls. */

static void riscos_draw_text(void *handle, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text)
{
	debug_printf("\\ODraw Text");

	game_window_write_text(handle, x, y, fontsize, 0, 0, colour, FALSE, text);

}

static void riscos_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
	debug_printf("\\oDraw rectangle from %d,%d, width %d, height %d in colour %d", x, y, h, h, colour);

	game_window_set_colour(handle, colour);
	game_window_plot(handle, os_MOVE_TO, x, y);
	game_window_plot(handle, os_PLOT_RECTANGLE | os_PLOT_TO, x + w - 1, y + h - 1);
}

static void riscos_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
	debug_printf("\\oDraw Line from %d,%d to %d,%d in colour %d", x1, y1, x2, y2, colour);

	game_window_set_colour(handle, colour);
	game_window_plot(handle, os_MOVE_TO, x1, y1);
	game_window_plot(handle, os_PLOT_SOLID | os_PLOT_TO, x2, y2);
}

static void riscos_draw_polygon(void *handle, const int *coords, int npoints, int fillcolour, int outlinecolour)
{
	int i;

	debug_printf("\\oDraw Polygon");

	if (npoints == 0)
		return;

	game_window_set_colour(handle, outlinecolour);

	game_window_start_path(handle, coords[0], coords[1]);
	for (i = 1; i < npoints; i++)
		game_window_add_segment(handle, coords[2 * i], coords[2 * i + 1]);

	game_window_end_path(handle, TRUE, 2, outlinecolour, fillcolour);
}

static void riscos_draw_circle(void *handle, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
	debug_printf("\\oDraw Circle at %d, %d, radius %d, in fill colour %d and outline colour %d", cx, cy, radius, fillcolour, outlinecolour);

	if (fillcolour != -1) {
		game_window_set_colour(handle, fillcolour);
		game_window_plot(handle, os_MOVE_TO, cx, cy);
		game_window_plot(handle, os_PLOT_CIRCLE | os_PLOT_TO, cx + radius, cy);
	}

	game_window_set_colour(handle, outlinecolour);
	game_window_plot(handle, os_MOVE_TO, cx, cy);
	game_window_plot(handle, os_PLOT_CIRCLE_OUTLINE | os_PLOT_TO, cx + radius, cy);
}

static void riscos_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour)
{
	debug_printf("\\ODraw Thick Line");
}

/**
 * Request an update of part of the window canvas.
 *
 * \param *handle	The game window instance pointer.
 * \param x		The X coordinate of the top-left corner of the area.
 * \param y		The Y coordinate of the top-left corner of the area.
 * \param w		The width of the area.
 * \param h		The height of the area.
 */

static void riscos_draw_update(void *handle, int x, int y, int w, int h)
{
	debug_printf("\\oDraw Update");

	game_window_force_redraw(handle, x, y, x + w - 1, y + h - 1);
}

static void riscos_clip(void *handle, int x, int y, int w, int h)
{
	debug_printf("\\oClip");

	game_window_set_clip(handle, x, y, x + w - 1, y + h - 1);
}

static void riscos_unclip(void *handle)
{
	debug_printf("\\oUnclip");

	game_window_clear_clip(handle);
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

/**
 * Update the text in the status bar.
 * 
 * \param *handle	The game window instance pointer.
 * \param *text		The new text to display in the bar.
 */

static void riscos_status_bar(void *handle, const char *text)
{
	game_window_set_status_text(handle, text);
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

	if (randseed == NULL || randseedsize == NULL)
		return;

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

	debug_printf("Returned seed.");
}

/**
 * Activate periodic callbacks to the midend.
 * 
 * \param *fe			The frontend handle.
 */

void activate_timer(frontend *fe)
{
	debug_printf("\\oActivate Timer");

	if (fe != NULL)
		game_window_start_timer(fe->window);
}

/**
 * Deactivate periodic callbacks to the midend.
 * 
 * \param *fe			The frontend handle.
 */

void deactivate_timer(frontend *fe)
{
	debug_printf("\\oDeactivate Timer");

	if (fe != NULL)
		game_window_stop_timer(fe->window);
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
