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
#include <string.h>
#include <fenv.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/types.h"
#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/osword.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"

/* Application header files */

#include "frontend.h"

#include "core/puzzles.h"

#include "game_window.h"
#include "help.h"

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

static osbool frontend_message_mode_change(wimp_message *message);
static void frontend_negotiate_game_size(struct frontend *fe);
static void frontend_write(void *ctx, const void *buf, int len);
static bool frontend_read(void *ctx, void *buf, int len);
static void riscos_draw_text(drawing *dr, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text);
static void riscos_draw_rect(drawing *dr, int x, int y, int w, int h, int colour);
static void riscos_draw_line(drawing *dr, int x1, int y1, int x2, int y2, int colour);
static void riscos_draw_polygon(drawing *dr, const int *coords, int npoints, int fillcolour, int outlinecolour);
static void riscos_draw_circle(drawing *dr, int cx, int cy, int radius, int fillcolour, int outlinecolour);
static void riscos_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour);
static void riscos_draw_update(drawing *dr, int x, int y, int w, int h);
static void riscos_clip(drawing *dr, int x, int y, int w, int h);
static void riscos_unclip(drawing *dr);
static void riscos_start_draw(drawing *dr);
static void riscos_end_draw(drawing *dr);
static void riscos_status_bar(drawing *dr, const char *text);
static blitter *riscos_blitter_new(drawing *dr, int w, int h);
static void riscos_blitter_free(drawing *dr, blitter *bl);
static void riscos_blitter_save(drawing *dr, blitter *bl, int x, int y);
static void riscos_blitter_load(drawing *dr, blitter *bl, int x, int y);

/* The drawing API. */

static const struct drawing_api riscos_drawing = {
	/* The API version. */

	.version = 1,

	/* The core drawing functions. */

	.draw_text = riscos_draw_text,
	.draw_rect = riscos_draw_rect,
	.draw_line = riscos_draw_line,
	.draw_polygon = riscos_draw_polygon,
	.draw_circle = riscos_draw_circle,
	.draw_update = riscos_draw_update,
	.clip = riscos_clip,
	.unclip = riscos_unclip,
	.start_draw = riscos_start_draw,
	.end_draw = riscos_end_draw,
	.status_bar = riscos_status_bar,
	.blitter_new = riscos_blitter_new,
	.blitter_free = riscos_blitter_free,
	.blitter_save = riscos_blitter_save,
	.blitter_load = riscos_blitter_load,

	/* The printing API. */

	.begin_doc = NULL,
	.begin_page = NULL,
	.begin_puzzle = NULL,
	.end_puzzle = NULL,
	.end_page = NULL,
	.end_doc = NULL,
	.line_width = NULL,
	.line_dotted = NULL,

	/* Text fallback. */

	.text_fallback = NULL,

	/* Thick lines. */

	.draw_thick_line = NULL, // riscos_draw_thick_line,
};

/**
 * Initialise the front-end.
 */

void frontend_initialise(void)
{
	event_add_message_handler(message_MODE_CHANGE, EVENT_MESSAGE_INCOMING, frontend_message_mode_change);
}

/**
 * Load a game file into a new game instance, and open its window.
 *
 * \param *filename	The filename of the game to be loaded.
 */
void frontend_load_game_file(char *filename)
{
	char *gamename = NULL;
	const char *error = NULL;
	int game_index = -1;
	FILE *file = NULL;
	wimp_pointer pointer;

	if (filename == NULL)
		return;

	/* Open the file */

	file = fopen(filename, "r");
	if (file == NULL) {
		error_msgs_report_error("FileLoadFail");
		return;
	}

	hourglass_on();

	/* Find the game that the file relates to. */

	error = identify_game(&gamename, frontend_read, file);
	if (error != NULL) {
		fclose(file);
		hourglass_off();
		error_msgs_param_report_error("FileLoadErr", (char *) error, NULL, NULL, NULL);
		return;
	}

	for (int i = 0; i < gamecount; i++) {
		if (strcmp(gamename, gamelist[i]->name) == 0) {
			game_index = i;
			break;
		}
	}

	free(gamename);

	/* If we found a game, load the file. */

	if (game_index >= 0 && game_index < gamecount) {
		rewind(file);
		wimp_get_pointer_info(&pointer);
		frontend_create_instance(game_index, &pointer, file);
	}

	/* Close the file. */

	fclose(file);

	hourglass_off();
}

/**
 * Initialise a new game and open its window.
 *
 * \param game_index	The index into gamelist[] of the required game.
 * \param *pointer	The pointer at which to open the game.
 * \param *file		A file from which to load the game state, or NULL
 *			to create a new game from scratch.
 */

void frontend_create_instance(int game_index, wimp_pointer *pointer, FILE *file)
{
	struct frontend *new;
	osbool status_bar = FALSE;
	const game *game = NULL;
	fenv_t fpexcepts;
	const char *error = NULL;

	/* Sanity check the game index that we're to use. */

	if (game_index < 0 || game_index >= gamecount)
		return;

	hourglass_on();

	game = gamelist[game_index];

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

	// debug_printf"Creating a new game collection instance: block=0x%x", new);

	/* Link the game into the list, and initialise critical data. */

	new->next = frontend_list;
	frontend_list = new;

	new->window = NULL;

	/* Create the game window. */

	new->window = game_window_create_instance(new, game->name);
	if (new->window == NULL) {
		hourglass_off();
		frontend_delete_instance(new);
		return;
	}

	/* Create the midend, and agree the window size. */

	new->me = midend_new(new, game, &riscos_drawing, new->window);
	if (new->me == NULL) {
		hourglass_off();
		frontend_delete_instance(new);
		return;
	}

	feholdexcept(&fpexcepts);

	midend_new_game(new->me);

	if (file != NULL) {
		error = midend_deserialise(new->me, frontend_read, file);
		if (error != NULL) {
			hourglass_off();
			frontend_delete_instance(new);
			error_msgs_param_report_error("FileLoadErr", (char *) error, NULL, NULL, NULL);
			return;
		}
	}

	frontend_negotiate_game_size(new);

	feclearexcept(FE_ALL_EXCEPT);
	feupdateenv(&fpexcepts);

	status_bar = midend_wants_statusbar(new->me) ? TRUE : FALSE;

	game_window_open(new->window, status_bar, pointer);

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

	// debug_printf"Deleting a game instance: block=0x%x", fe);

	/* Delink the instance from the list. */

	list = &frontend_list;

	while (*list != NULL && *list != fe)
		list = &((*list)->next);

	if (*list != NULL)
		*list = fe->next;

	/* Delete the midend first, so that our infrastructure
	 * remains in place.
	 */

	if (fe->me != NULL)
		midend_free(fe->me);

	/* Then delete the window, and tidy up anything that the
	 * midend doesn't do.
	 */

	if (fe->window != NULL)
		game_window_delete_instance(fe->window);

	/* Deallocate the instance block. */

	free(fe);
}

/**
 * Perform an action through the frontend.
 *
 * \param *fe		The instance to which the action relates.
 * \param action	The action to carry out.
 * \return		The outcome of the action.
 */

enum frontend_event_outcome frontend_perform_action(struct frontend *fe, enum frontend_action action)
{
	const char *error;
	enum frontend_event_outcome outcome = FRONTEND_EVENT_UNKNOWN;
	fenv_t fpexcepts;
	const game *game;

	if (fe == NULL || fe->me == NULL)
		return FRONTEND_EVENT_REJECTED;

	feholdexcept(&fpexcepts);

	switch (action) {
	case FRONTEND_ACTION_SIMPLE_NEW:
		hourglass_on();
		midend_new_game(fe->me);
		frontend_negotiate_game_size(fe);
		hourglass_off();
		outcome = FRONTEND_EVENT_ACCEPTED;
		break;
	case FRONTEND_ACTION_RESTART:
		midend_restart_game(fe->me);
		outcome = FRONTEND_EVENT_ACCEPTED;
		break;
	case FRONTEND_ACTION_SOLVE:
		error = midend_solve(fe->me);
		if (error != NULL)
			error_msgs_param_report_error("SolveErr", (char *) error, NULL, NULL, NULL);
		outcome = FRONTEND_EVENT_ACCEPTED;
		break;
	case FRONTEND_ACTION_HELP:
		game = midend_which_game(fe->me);
		if (game != NULL)
			help_launch(game->htmlhelp_topic);
	default:
		outcome = FRONTEND_EVENT_REJECTED;
		break;
	}

	feclearexcept(FE_ALL_EXCEPT);
	feupdateenv(&fpexcepts);

	return outcome;
}

/**
 * Start a new game from the supplied parameters.
 *
 * \param *fe		The instance to which the action relates.
 * \param *params	The parameters to use for the new game.
 */

void frontend_start_new_game_from_parameters(struct frontend *fe, struct game_params *params)
{
	if (fe == NULL || fe->me == NULL || params == NULL)
		return;

	midend_set_params(fe->me, params);
	frontend_perform_action(fe, FRONTEND_ACTION_SIMPLE_NEW);
}

/**
 * Process key events from the game window. These are any
 * mouse click or keypress events handled by the midend.
 *
 * \param *fe		The instance to which the event relates.
 * \param x		The X coordinate of the event.
 * \param y		The Y coordinate of the event.
 * \param button	The button details for the event.
 * \return		The outcome of the event.
 */

enum frontend_event_outcome frontend_handle_key_event(struct frontend *fe, int x, int y, int button)
{
	int outcome = PKR_UNUSED;

	// debug_printf"Received event: x=%d, y=%d, button=%d, outcome=%d", x, y, button, outcome);

	if (fe != NULL && fe->me != NULL)
		outcome = midend_process_key(fe->me, x, y, button);

	if (outcome == PKR_QUIT)
		return FRONTEND_EVENT_EXIT;

	return (outcome == PKR_UNUSED) ? FRONTEND_EVENT_REJECTED : FRONTEND_EVENT_ACCEPTED;
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
	// debug_printf"Timer call after %f seconds", tplus);

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
 * \param *can_configure	Pointer to variable in which to return
 *				the configure state of the midend.
 * \param *can_undo		Pointer to variable in which to return
 *				the undo state of the midend.
 * \param *can_redo		Pointer to variable in which to return
 *				the redo state of the midend.
 * \param *can_solve		Pointer to variable in which to return
 *				the solve state of the midend.
 */

void frontend_get_menu_info(struct frontend *fe, struct preset_menu **presets, int *limit,
		int *current_preset, osbool *can_configure, osbool *can_undo, osbool *can_redo, osbool *can_solve)
{
	const game *game;

	if (fe == NULL || fe->me == NULL)
		return;

	game = midend_which_game(fe->me);

	if (can_undo != NULL)
		*can_undo = midend_can_undo(fe->me) ? TRUE : FALSE;

	if (can_redo != NULL)
		*can_redo = midend_can_redo(fe->me) ? TRUE : FALSE;

	if (presets != NULL)
		*presets = midend_get_presets(fe->me, limit);

	if (current_preset != NULL)
		*current_preset = midend_which_preset(fe->me);

	if (game != NULL && can_solve != NULL)
		*can_solve = (game->can_solve == TRUE) ? TRUE : FALSE;

	if (game != NULL && can_configure != NULL)
		*can_configure = (game->can_configure == TRUE) ? TRUE : FALSE;
}

/**
 * Return details of a configuration set from the midend.
 *
 * \param *fe			The frontend handle.
 * \param type			The configuration type to be returned.
 * \param **config_data		Pointer to a variable in which to return
 *				a pointer to the configuration data.
 * \param **window_title	Pointer to a variable in which to return
 *				a pointer to the proposed window title.
 */

void frontend_get_config_info(struct frontend *fe, int type, config_item **config_data, char **window_title)
{
	if (fe == NULL || fe->me == NULL)
		return;

	*config_data = midend_get_config(fe->me, type, window_title);
}

/**
 * Update details of a configuration set to the midend.
 *
 * \param *fe			The frontend handle.
 * \param type			The configuration type being supplied.
 * \param *config_data		Pointer to the updated configuration data.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool frontend_set_config_info(struct frontend *fe, int type, config_item *config_data)
{
	const char *error;

	if (fe == NULL || fe->me == NULL || config_data == NULL)
		return FALSE;

	error = midend_set_config(fe->me, type, config_data);
	if (error != NULL)
		error_msgs_param_report_error("SetConfigErr", (char *) error, NULL, NULL, NULL);

	return (error == NULL) ? TRUE : FALSE;
}

/**
 * Handle incoming Message_ModeChange.
 *
 * \param *message		The message data block from the Wimp.
 */

static osbool frontend_message_mode_change(wimp_message *message)
{
	struct frontend *fe = frontend_list;

	// debug_printf"\\LMessage Mode Change!!");

	while (fe != NULL) {
		frontend_negotiate_game_size(fe);
		fe = fe->next;
	}

	return TRUE;
}

/**
 * Struct re-negotiate the size of the game canvas with the midend.
 *
 * \param *fe			The frontend handle.
 */

static void frontend_negotiate_game_size(struct frontend *fe)
{
	int number_of_colours = 0;
	float *colours = NULL;

	if (fe == NULL || fe->me == NULL)
		return;

	/* Allow the puzzles to fill up to 3/4 of the screen area. */

	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_XWIND_LIMIT, &(fe->x_size));
	os_read_mode_variable(os_CURRENT_MODE, os_MODEVAR_YWIND_LIMIT, &(fe->y_size));

	fe->x_size *= 0.75;
	fe->y_size *= 0.75;

	midend_size(fe->me, &(fe->x_size), &(fe->y_size), false, 1.0);

	// debug_printf"Agreed on canvas x=%d, y=%d", fe->x_size, fe->y_size);

	colours = midend_colours(fe->me, &number_of_colours);

	game_window_create_canvas(fe->window, fe->x_size, fe->y_size, colours, number_of_colours);

	midend_force_redraw(fe->me);
}

/**
 * Save a game to disc as a Puzzle file.
 *
 * \param *fe			The frontend handle.
 * \param *filename		The filename of the target file.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool frontend_save_game_file(struct frontend *fe, char *filename)
{
	FILE *file;

	if (fe == NULL || fe->me == NULL || filename == NULL)
		return FALSE;

	/* Open the file */

	file = fopen(filename, "w");
	if (file == NULL) {
		error_msgs_report_error("FileSaveFail");
		return FALSE;
	}

	hourglass_on();

	midend_serialise(fe->me, frontend_write, file);

	/* Close the file and set the filetype. */

	fclose(file);
	osfile_set_type(filename, (bits) dataxfer_TYPE_PUZZLE);

	hourglass_off();

	return TRUE;
}

/**
 * An frwite() wrapper for use by midend serialisation routines.
 *
 * \param *ctx			The context pointer, which is the FILE struct
 *				in use for IO.
 * \param *buf			Pointer to the buffer containing text to be
 *				written.
 * \param len			The number of characters to be written.
 */

static void frontend_write(void *ctx, const void *buf, int len)
{
	FILE *file = ctx;

	if (ctx != NULL && buf != NULL)
		fwrite(buf, sizeof(char), len, file);
}

/**
 * An fread() wrapped for use by midend serialisation routines.
 *
 * \param *ctx			The context pointer, which is the FILE struct
 *				in use for IO.
 * \param *buf			Pointer to the buffer to hold the text read in.
 * \param len			The number of characters to be read.
 * \return			TRUE on success; FALSE on failure.
 */

static bool frontend_read(void *ctx, void *buf, int len)
{
	FILE *file = ctx;

	if (ctx == NULL || buf == NULL)
		return FALSE;

	if (fread(buf, sizeof(char), len, file) != len)
		return FALSE;

	return TRUE;
}

/* Below this point are the drawing API calls. */

/**
 * Write a line of text in a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param x		The X coordinate at which to write the text.
 * \param y		The Y coordinate at which to write the text.
 * \param fonttype	The type of font face to be used.
 * \param fontsize	The size of the text in pixels.
 * \param align		The alignment of the text around the coordinates.
 * \param *text		The text to write.
 */

static void riscos_draw_text(drawing *dr, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\ODraw Text");

	game_window_write_text(window, x, y, fontsize, align, colour, (fonttype == FONT_FIXED) ? TRUE : FALSE, text);
}

/**
 * Draw a filled rectangle in a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param x		The X coordinate of the top-left of the rectangle (inclusive).
 * \param y		The Y coordinate of the top-left of the rectangle (inclusive).
 * \param w		The width of the rectangle.
 * \param h		The height of the rectangle.
 * \param colour	The colour in which to draw the line.
 */

static void riscos_draw_rect(drawing *dr, int x, int y, int w, int h, int colour)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\vDraw rectangle from %d,%d, width %d, height %d in colour %d", x, y, w, h, colour);

	game_window_set_colour(window, colour);
	game_window_plot(window, os_MOVE_TO, x, y + h - 1);
	game_window_plot(window, os_PLOT_RECTANGLE | os_PLOT_TO, x + w - 1, y);
}

/**
 * Draw a straight line in a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param x1		The X coordinate of the start of the line (inclusive).
 * \param y1		The Y coordinate of the start of the line (inclusive).
 * \param x2		The X coordinate of the end of the line (inclusive).
 * \param y2		The Y coordinate of the end of the line (inclusive).
 * \param colour	The colour in which to draw the line.
 */

static void riscos_draw_line(drawing *dr, int x1, int y1, int x2, int y2, int colour)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\vDraw Line from %d,%d to %d,%d in colour %d", x1, y1, x2, y2, colour);

	game_window_set_colour(window, colour);
	game_window_plot(window, os_MOVE_TO, x1, y1);
	game_window_plot(window, os_PLOT_SOLID | os_PLOT_TO, x2, y2);
}

/**
 * Draw a closed polygon in a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param *coords	An array of pairs of X, Y coordinates of the points on
 *			the outline (inclusive).
 * \param npoints	The number of points on the outline.
 * \param fillcolour	The colour to use for the fill, or -1 for none.
 * \param outlinecolour	The colour to use for the outline, or -1 for none.
 */

static void riscos_draw_polygon(drawing *dr, const int *coords, int npoints, int fillcolour, int outlinecolour)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);
	int i;

	// debug_printf"\\vDraw Polygon...");

	if (npoints == 0)
		return;

	game_window_set_colour(window, outlinecolour);

	game_window_start_path(window, coords[0], coords[1]);
	for (i = 1; i < npoints; i++)
		game_window_add_segment(window, coords[2 * i], coords[2 * i + 1]);

	game_window_end_path(window, TRUE, 2, outlinecolour, fillcolour);
}

/**
 * Draw a circle in a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param cx		The X coordinate of the centre of the circle.
 * \param cy		The Y coordinate of the centre of the circle.
 * \param radius	The radius of the circle.
 * \param fillcolour	The colour to use for the fill, or -1 for none.
 * \param outlinecolour	The colour to use for the outline, or -1 for none.
 */

static void riscos_draw_circle(drawing *dr, int cx, int cy, int radius, int fillcolour, int outlinecolour)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\vDraw Circle at %d, %d, radius %d, in fill colour %d and outline colour %d", cx, cy, radius, fillcolour, outlinecolour);

	if (fillcolour != -1) {
		game_window_set_colour(window, fillcolour);
		game_window_plot(window, os_MOVE_TO, cx, cy);
		game_window_plot(window, os_PLOT_CIRCLE | os_PLOT_TO, cx + radius, cy);
	}

	game_window_set_colour(window, outlinecolour);
	game_window_plot(window, os_MOVE_TO, cx, cy);
	game_window_plot(window, os_PLOT_CIRCLE_OUTLINE | os_PLOT_TO, cx + radius, cy);
}

static void riscos_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour)
{
	// debug_printf"\\ODraw Thick Line");
}

/**
 * Request an update of part of the window canvas.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param x		The X coordinate of the top-left corner of the area.
 * \param y		The Y coordinate of the top-left corner of the area.
 * \param w		The width of the area.
 * \param h		The height of the area.
 */

static void riscos_draw_update(drawing *dr, int x, int y, int w, int h)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\oDraw Update");

	game_window_force_redraw(window, x, y, x + w - 1, y + h - 1);
}

/**
 * Set a graphics clipping rectangle in a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param x		The X coordinate of the top-left of the rectangle (inclusive).
 * \param y		The Y coordinate of the top-left of the rectangle (inclusive).
 * \param w		The width of the rectangle.
 * \param h		The height of the rectangle.
 */

static void riscos_clip(drawing *dr, int x, int y, int w, int h)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\vClip from %d,%d, width %d, height %d", x, y, w, h);

	game_window_set_clip(window, x, y, x + w - 1, y + h - 1);
}

/**
 * Clear a graphics clipping rectangle from a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 */

static void riscos_unclip(drawing *dr)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\vUnclip");

	game_window_clear_clip(window);
}

/**
 * Start the drawing process within a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 */

static void riscos_start_draw(drawing *dr)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\GStart Draw");

	game_window_start_draw(window);
}

/**
 * End the drawing process within a puzzle window.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 */

static void riscos_end_draw(drawing *dr)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	// debug_printf"\\GEnd Draw");

	game_window_end_draw(window);
}

/**
 * Update the text in the status bar.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param *text		The new text to display in the bar.
 */

static void riscos_status_bar(drawing *dr, const char *text)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	game_window_set_status_text(window, text);
}

/**
 * Create a new blitter
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param w		The required width of the blitter.
 * \param h		The required height of the blitter.
 * \return		Pointer to the new blitter, or NULL.
 */

static blitter *riscos_blitter_new(drawing *dr, int w, int h)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	return game_window_create_blitter(window, w, h);
}

/**
 * Free the resources related to a blitter.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param *bl		The blitter to be freed.
 */

static void riscos_blitter_free(drawing *dr, blitter *bl)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	game_window_delete_blitter(window, bl);
}

/**
 * Save a copy of the game canvas on to a blitter.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param *bl		The blitter to be used.
 * \param x		The X coordinate of the area to copy.
 * \param y		The Y coordinate of the area to copy.
 */

static void riscos_blitter_save(drawing *dr, blitter *bl, int x, int y)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	game_window_save_blitter(window, bl, x, y);
}

/**
 * Draw the contents of a blitter on to the game canvas.
 *
 * \param *dr		The drawing structure referencing the target Game Window.
 * \param *bl		The blitter to be used.
 * \param x		The X coordinate of the area to write to.
 * \param y		The Y coordinate of the area to write to.
 */

static void riscos_blitter_load(drawing *dr, blitter *bl, int x, int y)
{
	struct game_window_block *window = GET_HANDLE_AS_TYPE(dr, struct game_window_block);

	game_windoow_load_blitter(window, bl, x, y);
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

	// debug_printf"Get Random Seed");

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

	// debug_printf"Returned seed.");
}

/**
 * Activate periodic callbacks to the midend.
 *
 * \param *fe			The frontend handle.
 */

void activate_timer(frontend *fe)
{
	// debug_printf"\\oActivate Timer");

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
	// debug_printf"\\oDeactivate Timer");

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

	// debug_printf"\\OFrontend Default Colour");

	output[0] = output[1] = output[2] = 1.0f;

	error = xwimp_read_palette(palette);
	if (error != NULL)
		return;

	output[0] = ((float) ((palette->entries[1] >>  8) & 0xff)) / 255.0f;
	output[1] = ((float) ((palette->entries[1] >> 16) & 0xff)) / 255.0f;
	output[2] = ((float) ((palette->entries[1] >> 24) & 0xff)) / 255.0f;
}
