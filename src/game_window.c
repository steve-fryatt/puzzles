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
 * \file: game_window.c
 *
 * Game window implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/colourtrans.h"
#include "oslib/font.h"
#include "oslib/os.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/string.h"
//#include "sflib/icons.h"
//#include "sflib/ihelp.h"
//#include "sflib/menus.h"
//#include "sflib/msgs.h"
//#include "sflib/templates.h"
//#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files */

#include "game_window.h"

#include "core/puzzles.h"
#include "frontend.h"
#include "game_draw.h"

/* The name of the canvas sprite. */

#define GAME_WINDOW_SPRITE_NAME "Canvas"
#define GAME_WINDOW_SPRITE_ID (osspriteop_id) GAME_WINDOW_SPRITE_NAME

/* Workspace for calculating string sizes. */

static font_scan_block game_window_fonts_scan_block;

/* The game window data structure. */

struct game_window_block {
	struct frontend *fe;			/**< The parent frontend instance.		*/

	const char *title;			/**< The title of the game.			*/

	wimp_w handle;				/**< The handle of the game window.		*/

	osspriteop_area *sprite;		/**< The sprite area for the window canvas.	*/
	osspriteop_save_area *save_area;	/**< The save area for redirecting VDU output.	*/

	osbool vdu_redirection_active;
	int saved_context0;
	int saved_context1;
	int saved_context2;
	int saved_context3;

	os_coord canvas_size;			/**< The size of the drawing canvas, in pixels.	*/
	os_coord window_size;			/**< The size of the window, in pixels.		*/

	int number_of_colours;			/**< The number of colours defined.		*/

	osbool callback_timer_active;		/**< Is the callback timer currently active?	*/
	os_t last_callback;			/**< The time of the last frontend callback.	*/
};

/* Static function prototypes. */

static void game_window_close_handler(wimp_close *close);
static void game_window_click_handler(wimp_pointer *pointer);
static osbool game_window_keypress_handler(wimp_key *key);
static void game_window_redraw_handler(wimp_draw *redraw);
static osbool game_window_timer_callback(os_t time, void *data);

/**
 * Initialise the game windows and their associated menus and dialogues.
 */

void game_window_initialise(void)
{
	game_window_fonts_scan_block.space.x = 0;
	game_window_fonts_scan_block.space.y = 0;

	game_window_fonts_scan_block.letter.x = 0;
	game_window_fonts_scan_block.letter.y = 0;

	game_window_fonts_scan_block.split_char = -1;
}

/**
 * Initialise and a new game window instance.
 *
 * \param *fe		The parent game frontend instance.
 * \param *title	The title of the window.
 * \return		Pointer to the new window instance, or NULL.
 */

struct game_window_block *game_window_create_instance(struct frontend *fe, const char *title)
{
	struct game_window_block *new;

	/* Allocate the memory for the instance from the heap. */

	new = malloc(sizeof(struct game_window_block));
	if (new == NULL)
		return NULL;

	debug_printf("Creating a new game window instance: block=0x%x, fe=0x%x", new, fe);

	/* Initialise critical fields in the struct. */

	new->fe = fe;
	new->title = title;

	new->handle = NULL;
	new->sprite = NULL;
	new->save_area = NULL;
	new->vdu_redirection_active = FALSE;

	new->canvas_size.x = 0;
	new->canvas_size.y = 0;

	new->window_size.x = 0;
	new->window_size.y = 0;

	new->number_of_colours = 0;

	new->callback_timer_active = FALSE;

	/* Create the new window. */


	return new;
}

/**
 * Delete a game window instance and the associated window.
 *
 * \param *instance	The instance to be deleted.
 */

void game_window_delete_instance(struct game_window_block *instance)
{
	if (instance == NULL)
		return;

	debug_printf("Deleting a game window instance: block=0x%x", instance);

	/* Delete the window. */

	if (instance->handle != NULL) {
		wimp_delete_window(instance->handle);
	}

	/* Deallocate the instance block. */

	if (instance->sprite != NULL)
		free(instance->sprite);

	if (instance->save_area != NULL)
		free(instance->save_area);

	free(instance);
}

/**
 * Create and open the game window at the specified location.
 * 
 * \param *instance	The instance to open the window on.
 * \param *pointer	The pointer at which to open the window.
 */

void game_window_open(struct game_window_block *instance, wimp_pointer *pointer)
{
	wimp_window window_definition;
	os_error *error;

	if (instance == NULL || instance->handle != NULL)
		return;

	window_definition.visible.x0 = 200; // The location will be updated when we open the window
	window_definition.visible.y0 = 200; // at the pointer, so only width and height matter!
	window_definition.visible.x1 = window_definition.visible.x0 + instance->window_size.x;
	window_definition.visible.y1 = window_definition.visible.y0 + instance->window_size.y;
	window_definition.xscroll = 0;
	window_definition.yscroll = 0;
	window_definition.next = wimp_TOP;
	window_definition.flags = wimp_WINDOW_NEW_FORMAT |
			wimp_WINDOW_MOVEABLE |
			wimp_WINDOW_BOUNDED_ONCE | wimp_WINDOW_BACK_ICON |
			wimp_WINDOW_CLOSE_ICON | wimp_WINDOW_TITLE_ICON |
			wimp_WINDOW_TOGGLE_ICON | wimp_WINDOW_VSCROLL |
			wimp_WINDOW_SIZE_ICON | wimp_WINDOW_HSCROLL;
	window_definition.title_fg = wimp_COLOUR_BLACK;
	window_definition.title_bg = wimp_COLOUR_LIGHT_GREY;
	window_definition.work_fg = wimp_COLOUR_BLACK;
	window_definition.work_bg = wimp_COLOUR_TRANSPARENT;
	window_definition.scroll_outer = wimp_COLOUR_MID_LIGHT_GREY;
	window_definition.scroll_inner = wimp_COLOUR_VERY_LIGHT_GREY;
	window_definition.highlight_bg = wimp_COLOUR_CREAM;
	window_definition.extra_flags = 0;
	window_definition.extent.x0 = 0;
	window_definition.extent.y0 = -instance->window_size.y;
	window_definition.extent.x1 = instance->window_size.x;
	window_definition.extent.y1 = 0;
	window_definition.title_flags = wimp_ICON_TEXT | wimp_ICON_INDIRECTED |
			wimp_ICON_BORDER | wimp_ICON_HCENTRED |
			wimp_ICON_VCENTRED | wimp_ICON_FILLED;
	window_definition.work_flags = wimp_BUTTON_CLICK_DRAG << wimp_ICON_BUTTON_TYPE_SHIFT;
	window_definition.sprite_area = wimpspriteop_AREA;
	window_definition.xmin = 0;
	window_definition.ymin = 0;
	window_definition.title_data.indirected_text.text = (char *) instance->title;
	window_definition.title_data.indirected_text.size = strlen(instance->title) + 1;
	window_definition.title_data.indirected_text.validation = NULL;
	window_definition.icon_count = 0;

	error = xwimp_create_window(&window_definition, &(instance->handle));
	if (error != NULL) {
		game_window_delete_instance(instance);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	/* Register the window events. */

	event_add_window_user_data(instance->handle, instance);
	event_add_window_close_event(instance->handle, game_window_close_handler);
	event_add_window_redraw_event(instance->handle, game_window_redraw_handler);
	event_add_window_mouse_event(instance->handle, game_window_click_handler);
	event_add_window_key_event(instance->handle, game_window_keypress_handler);

	/* Open the window. */

	windows_open_centred_at_pointer(instance->handle, pointer);
}

/**
 * Handle Close events on game windows, deleting the window and the
 * associated instance data.
 *
 * \param *close	The Wimp Close data block.
 */

static void game_window_close_handler(wimp_close *close)
{
	struct game_window_block	*instance;

	debug_printf("\\RClosing game window");

	instance = event_get_window_user_data(close->w);
	if (instance == NULL)
		return;

	/* Delete the parent game instance. */

	frontend_delete_instance(instance->fe);
}

/**
 * Handle mouse click events in game windows.
 *
 * \param *pointer	The Wimp Pointer event data block.
 */

static void game_window_click_handler(wimp_pointer *pointer)
{
	struct game_window_block	*instance;
	wimp_window_state		window;
	int				x, y;

	instance = event_get_window_user_data(pointer->w);
	if (instance == NULL)
		return;

	window.w = pointer->w;
	wimp_get_window_state(&window);

	x = (pointer->pos.x - window.visible.x0 + window.xscroll) / 2;
	y = -((pointer->pos.y - window.visible.y1 + window.yscroll) / 2);

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
		frontend_handle_key_event(instance->fe, x, y, LEFT_BUTTON);
		frontend_handle_key_event(instance->fe, x, y, LEFT_RELEASE);
		break;
	case wimp_CLICK_ADJUST:
		frontend_handle_key_event(instance->fe, x, y, RIGHT_BUTTON);
		frontend_handle_key_event(instance->fe, x, y, RIGHT_RELEASE);
		break;
	}
}

/**
 * Handle keypress events in game windows.
 *
 * \param *key		The Wimp Key event data block.
 * \return		TRUE if the event was handled; otherwise FALSE.
 */

static osbool game_window_keypress_handler(wimp_key *key)
{
	struct game_window_block *instance;

	instance = event_get_window_user_data(key->w);
	if (instance == NULL)
		return FALSE;

	/* Pass ASCII codes directly to the front-end. */

	if (key->c >= 0 && key->c < 127)
		return frontend_handle_key_event(instance->fe, key->c, 0, 0);

	return FALSE;
}

/**
 * Handle Redraw events from game windows.
 *
 * \param *redraw	The Wimp Draw event data block.
 */

static void game_window_redraw_handler(wimp_draw *redraw)
{
	struct game_window_block	*instance;
	int				ox, oy;
	os_factors			factors;
	byte				table[256];
	osspriteop_trans_tab		*translation_table;
	osbool				more;
	os_error			*error;

	translation_table = (osspriteop_trans_tab *) &table;

	instance = event_get_window_user_data(redraw->w);

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	error = xwimp_read_pix_trans(osspriteop_USER_AREA, instance->sprite,
		GAME_WINDOW_SPRITE_ID, &factors, NULL);

	error = xcolourtrans_select_table_for_sprite(instance->sprite, GAME_WINDOW_SPRITE_ID, os_CURRENT_MODE, (os_palette *) -1, translation_table, 0);

	while (more) {
		if (instance != NULL && error == NULL) {
			xosspriteop_put_sprite_scaled(osspriteop_USER_AREA, instance->sprite,
				GAME_WINDOW_SPRITE_ID, ox, oy - instance->window_size.y,
				(osspriteop_action) 0, &factors, translation_table);
		}

		more = wimp_get_rectangle(redraw);
	}
}

/**
 * Create or update the drawing canvas associated with a window
 * instance.
 * 
 * After an update, the canvas will be cleared and it will be
 * necessary to request that the client redraws any graphics which
 * had previously been present.
 * 
 * \param *instance		The instance to update.
 * \param x			The required horizontal dimension.
 * \param y			The required vertical dimension.
 * \param *colours		An array of colours required by the game.
 * \param number_of_colours	The number of colourd defined in the array.
 * \return			TRUE if successful; else FALSE.
 */

osbool game_window_create_canvas(struct game_window_block *instance, int x, int y, float *colours, int number_of_colours)
{
	size_t area_size;
	int entry, save_area_size;
	os_error *error;
	os_box extent;
	osspriteop_header *sprite;
	os_sprite_palette *palette;


	if (instance == NULL)
		return FALSE;

	/* Check to see if there's anything to do. */

	if (instance->canvas_size.x == x && instance->canvas_size.y == y)
		return TRUE;

	instance->canvas_size.x = 0;
	instance->canvas_size.y = 0;

	/* The size of the area is 16 for the area header, 44 for the sprite
	 * header, (x * y) bytes for the sprite with the rows rounded up to,
	 * a full number of words, and 2048 for the 256 double-word
	 * palette entries that we're going to add in.
	 */

	area_size = 16 + 44 + 2048 + (((x + 3) & 0xfffffffc) * y);

	/* If there's already a save area, zero its first word to reset it. */

	if (instance->save_area != NULL)
		*((int32_t *) instance->save_area) = 0;

	/* Allocate, or adjust, the required area. */

	if (instance->sprite == NULL)
		instance->sprite = malloc(area_size);
	else
		instance->sprite = realloc(instance->sprite, area_size);

	if (instance->sprite == NULL)
		return FALSE;

	/* Initialise the area. */

	instance->sprite->size = area_size;
	instance->sprite->first = 16;

	error = xosspriteop_clear_sprites(osspriteop_USER_AREA, instance->sprite);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		instance->sprite = NULL;
		return FALSE;
	}

	error = xosspriteop_create_sprite(osspriteop_USER_AREA, instance->sprite, GAME_WINDOW_SPRITE_NAME, FALSE, x, y, (os_mode) 21);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		instance->sprite = NULL;
		return FALSE;
	}

	instance->canvas_size.x = x;
	instance->canvas_size.y = y;

	/* Now add a palette by hand (see page 1-833 of the PRM). */

	instance->sprite->used += 2048;

	sprite = (osspriteop_header *) ((byte *) instance->sprite + instance->sprite->first);

	debug_printf("Sprite at 0x%x, size=%d, image at %d", sprite, sprite->size, sprite->image);

	sprite->size += 2048;
	sprite->image += 2048;
	sprite->mask += 2048;

	palette = (os_sprite_palette *) ((byte *) sprite + 44);

	debug_printf("Palette at 0x%x, sprite at 0x%x, size=%d, image at %d", palette, sprite, sprite->size, sprite->image);

	for (entry = 0; entry < 256; entry++) {
		if (entry < number_of_colours) {
			palette->entries[entry].on = ((int) (colours[entry * 3] * 0xff) << 8) |
					((int) (colours[entry * 3 + 1] * 0xff) << 16) |
					((int) (colours[entry * 3 + 2] * 0xff) << 24);
		} else {
			palette->entries[entry].on = 0xffffff00;
		}

		palette->entries[entry].off = palette->entries[entry].on;
	}

	instance->number_of_colours = number_of_colours;

	/* Initialise the save area. */

	error = xosspriteop_read_save_area_size(osspriteop_USER_AREA, instance->sprite,
			GAME_WINDOW_SPRITE_ID, &save_area_size);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		instance->sprite = NULL;
		return FALSE;
	}

	/* Allocate, or adjust, the required save area. */

	if (instance->save_area == NULL)
		instance->save_area = malloc(save_area_size);
	else
		instance->save_area = realloc(instance->save_area, save_area_size);

 	if (instance->save_area == NULL) {
		instance->sprite = NULL;
		return FALSE;
	}

	*((int32_t *) instance->save_area) = 0;

	debug_printf("Set canvas: sprite area size=%d, area=0x%x, save size=%d, save=0x%x", area_size, instance->sprite, save_area_size, instance->save_area);

	/* Set the window extent. */

	instance->window_size.x = 2 * x;
	instance->window_size.y = 2 * y;

	if (instance->handle != NULL) {
		extent.x0 = 0;
		extent.x1 = 2 * x;
		extent.y0 = -2 * y;
		extent.y1 = 0;
		wimp_set_extent(instance->handle, &extent);
	}

	/* TODO -- Bodge to clear the screen. */
	/* The code from here down is debug to test the drawing routines.
	 * It should be removed once the development is complete!
	 */

	game_window_start_draw(instance);
	os_set_colour(os_ACTION_OVERWRITE | os_COLOUR_SET_BG, 0);
	os_writec(os_VDU_CLG);
	game_window_end_draw(instance);

	game_window_start_draw(instance);
	os_set_colour(os_ACTION_OVERWRITE, 1);
	game_window_plot(instance, os_PLOT_SOLID | os_MOVE_TO, 20, 20);
	game_window_plot(instance, os_PLOT_SOLID | os_PLOT_TO, 20, y - 20);
	game_window_plot(instance, os_PLOT_SOLID | os_PLOT_TO, x - 20, y - 20);
	game_window_plot(instance, os_PLOT_SOLID | os_PLOT_TO, x - 20, 20);
	game_window_plot(instance, os_PLOT_SOLID | os_PLOT_TO, 20, 20);
	game_window_end_draw(instance);

	game_window_start_draw(instance);
	game_window_set_colour(instance, 2);
	game_window_plot(instance, os_PLOT_SOLID | os_MOVE_TO, 100, 100);
	game_window_plot(instance, os_PLOT_RECTANGLE | os_PLOT_SOLID | os_PLOT_TO, x - 100, y - 100);
	game_window_end_draw(instance);

	game_window_start_draw(instance);
	game_window_set_colour(instance, 4);
	game_window_start_path(instance, 50, 50);
	game_window_add_segment(instance, x - 50, 50);
	game_window_add_segment(instance, x - 50, 50);
	game_window_add_segment(instance, 50, y - 50);

	game_window_end_path(instance, TRUE, 2, -1, 4);
	game_window_end_draw(instance);

	error = xosspriteop_save_sprite_file(osspriteop_USER_AREA, instance->sprite, "RAM::RamDisc0.$.Sprites");
	debug_printf("Saved sprites: outcome=0x%x", error);

	/* TODO -- Remove the code down to here! */

	return TRUE;
}

/**
 * Start regular 20ms callbacks to the frontend, which can be passed
 * on to the midend.
 *
 * \param *instance		The instance to update.
 * \return			TRUE if successful; else FALSE.
 */

osbool game_window_start_timer(struct game_window_block *instance)
{
	if (instance == NULL)
		return FALSE;

	if (instance->callback_timer_active == TRUE)
		return TRUE;

	instance->callback_timer_active = TRUE;
	instance->last_callback = os_read_monotonic_time();

	return event_add_regular_callback(instance->handle, 0, 2, game_window_timer_callback, instance);
}

/**
 * Cancel any regular 20ms callbacks to the frontend which are in progress.
 *
 * \param *instance		The instance to update.
 */

void game_window_stop_timer(struct game_window_block *instance)
{
	if (instance == NULL)
		return;

	event_delete_callback_by_data(game_window_timer_callback, instance);
	instance->callback_timer_active = FALSE;
}

/**
 * The callback routine for the 20ms tick events.
 * 
 * \param time			The time that the callback occurred.
 * \param *data			The game window instance owning the callback.
 * \return			TRUE if the callback is to be claimed.
 */

static osbool game_window_timer_callback(os_t time, void *data)
{
	struct game_window_block *instance = data;
	float interval;

	if (instance == NULL)
		return TRUE;

	interval = ((float) (time - instance->last_callback)) / 100.0f;
	instance->last_callback = time;

	frontend_timer_callback(instance->fe, interval);

	return TRUE;
}

/**
 * Start a drawing operation on the game window canvas, redirecting
 * VDU output to the canvas sprite.
 * 
 * \param *instance	The instance to start drawing in.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_start_draw(struct game_window_block *instance)
{
	os_error *error;

	if (instance == NULL || instance->sprite == NULL || instance->save_area == NULL ||
			instance->vdu_redirection_active == TRUE)
		return FALSE;

	error = xosspriteop_switch_output_to_sprite(osspriteop_USER_AREA, instance->sprite,
			GAME_WINDOW_SPRITE_ID, instance->save_area,
			&(instance->saved_context0), &(instance->saved_context1), &(instance->saved_context2), &(instance->saved_context3));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	instance->vdu_redirection_active = TRUE;

	debug_printf("VDU Redirection Active");

	return TRUE;
}

/**
 * End a drawing operation on the game window canvas, restoring
 * VDU output back to the previous context.
 * 
 * \param *instance	The instance to finish drawing in.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_end_draw(struct game_window_block *instance)
{
	os_error *error;

	if (instance == NULL || instance->sprite == NULL || instance->save_area == NULL ||
			instance->vdu_redirection_active == FALSE)
		return FALSE;

	/* Reset any graphics clip window that may have been left in force. */

	game_window_clear_clip(instance);

	/* Restore the graphics context. */

	error = xosspriteop_switch_output_to_sprite(instance->saved_context0, (osspriteop_area *) instance->saved_context1,
			(osspriteop_id) instance->saved_context2, (osspriteop_save_area *)instance->saved_context3, NULL, NULL, NULL, NULL);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	instance->vdu_redirection_active = FALSE;

	debug_printf("VDU Redirection Inactive");

	return TRUE;
}

/**
 * Request a forced redraw of part of the canvas as the next available
 * opportunity.
 * 
 * \param *instance	The instance to plot to.
 * \param x0		The X coordinate of the top left corner of
 *			the area to be redrawn (inclusive).
 * \param y0		The Y coordinate of the top left corner of
 *			the area to be redrawn (inclusive).
 * \param x1		The X coordinate of the bottom right corner
 *			of the area to be redrawn (inclusive).
 * \param y1		The Y coordinate of the bottom right corner
 *			of the area to be redrawn (inclusive).
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_force_redraw(struct game_window_block *instance, int x0, int y0, int x1, int y1)
{
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	/* There's no point queueing updates if the window isn't open. */

	if (instance->handle == NULL || windows_get_open(instance->handle) == FALSE)
		return TRUE;

	debug_printf("Request a redraw: x0=%d, y0=%d, x1=%d, y1=%d",
			2 * x0, -2 * y1, 2 * (x1 + 1), -2 * (y0 - 1));

	/* Queue the update. */

	error = xwimp_force_redraw(instance->handle, 2 * x0, -2 * y1, 2 * (x1 + 1), -2 * (y0 - 1));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	return TRUE;
}

/**
 * Set the plotting colour in a game window.
 *
 * \param *instance	The instance to plot to.
 * \param colour	The colour, as an index into the game's list.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_set_colour(struct game_window_block *instance, int colour)
{
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	if (colour < 0 || colour >= instance->number_of_colours)
		return TRUE;

	error = xos_set_colour(os_ACTION_OVERWRITE, colour);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	debug_printf("Select colour %d", colour);

	return TRUE;
}

/**
 * Set a graphics clipping window, to affect all future
 * operations on the canvas.
 * 
 * \param *instance	The instance to plot to.
 * \param x0		The X coordinate of the top left corner of
 *			the window (inclusive).
 * \param y0		The Y coordinate of the top left corner of
 *			the window (inclusive).
 * \param x1		The X coordinate of the bottom right corner
 *			of the window (exclusive).
 * \param y1		The Y coordinate of the bottom right corner
 *			of the window (exclusive).
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_set_clip(struct game_window_block *instance, int x0, int y0, int x1, int y1)
{
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	x0 = 2 * x0;
	y0 = 2 * (instance->canvas_size.y - y0);
	
	x1 = 2 * x1;
	y1 = 2 * (instance->canvas_size.y - y1);

	error = xos_writec(os_VDU_SET_GRAPHICS_WINDOW);

	if (error == NULL)
		error = xos_writec(x0 & 0xff);

	if (error == NULL)
		error = xos_writec((x0 >> 8) & 0xff);

	if (error == NULL)
		error = xos_writec(y0 & 0xff);

	if (error == NULL)
		error = xos_writec((y0 >> 8) & 0xff);

	if (error == NULL)
		error = xos_writec(x1 & 0xff);

	if (error == NULL)
		error = xos_writec((x1 >> 8) & 0xff);

	if (error == NULL)
		error = xos_writec(y1 & 0xff);

	if (error == NULL)
		error = xos_writec((y1 >> 8) & 0xff);

	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	debug_printf("Set clip to %d,%d -- %d,%d", x0, y0, x1, y1);

	return TRUE;
}

/**
 * Clear the clipping window set by set_clip()
 * 
 * \param *instance	The instance to plot to.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_clear_clip(struct game_window_block *instance)
{
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	error = xos_writec(os_VDU_RESET_WINDOWS);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	debug_printf("Reset clip");

	return TRUE;
}

/**
 * Perform an OS_Plot operation in a game window.
 *
 * A redraw operation must be in progress when this call is used.
 *
 * \param *instance	The instance to plot to.
 * \param plot_code	The OS_Plot operation code.
 * \param x		The X coordinate.
 * \param y		The Y coordinate.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_plot(struct game_window_block *instance, os_plot_code plot_code, int x, int y)
{
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	error = xos_plot(plot_code, 2 * x, 2 * (instance->canvas_size.y - y));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	debug_printf("Plotted 0x%x to %d, %d (calculated as %d, %d - %d = %d)",
			plot_code, x, y, 2*x, instance->canvas_size.y, y, instance->canvas_size.y - y);

	return TRUE;
}

/**
 * Start a polygon path in a game window.
 * 
 * \param *instance	The instance to plot to.
 * \param x		The X coordinate of the start.
 * \param y		The Y coordinate of the start.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_start_path(struct game_window_block *instance, int x, int y)
{
	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	game_draw_start_path();

	debug_printf("Start path from %d, %d", 2 * x, 2 * (instance->canvas_size.y - y));

	return game_draw_add_move(2 * x, 2 * (instance->canvas_size.y - y));
}

/**
 * Add a segment to a polygon path in a game window.
 * 
 * \param *instance	The instance to plot to.
 * \param x		The X coordinate of the end of the segment.
 * \param y		The Y coordinate of the end of the segment.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_add_segment(struct game_window_block *instance, int x, int y)
{
	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	debug_printf("Continue path to %d, %d", 2 * x, 2 * (instance->canvas_size.y - y));

	return game_draw_add_line(2 * x, 2 * (instance->canvas_size.y - y));
}

/**
 * End a polygon path in a game window, drawing either an outline, filled
 * shape, or both
 * 
 * \param *instance	The instance to plot to.
 * \param closed	TRUE if the path should be closed; else FALSE
 * \param width		The width of the path outline.
 * \param outline	The required outline colour, or -1 for none.
 * \param fill		The required fill colour, or -1 for none.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_end_path(struct game_window_block *instance, osbool closed, int width, int outline, int fill)
{
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	if (closed && !game_draw_close_subpath())
		return FALSE;

	if (!game_draw_end_path())
		return FALSE;

	if (fill != -1) {
		game_window_set_colour(instance, fill);

		error = game_draw_fill_path(width);
		if (error != NULL) {
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return FALSE;
		}
	}

	if (outline != -1) {
		game_window_set_colour(instance, outline);

		error = game_draw_plot_path(width);
		if (error != NULL) {
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return FALSE;
		}
	}

	debug_printf("Path plotted");

	return TRUE;
}

osbool game_window_write_text(struct game_window_block *instance, int x, int y, int size, int horizontal, int vertical, int colour, osbool monospaced, const char *text)
{
	font_f face;
	int xpt, ypt, xos, yos;
	os_error *error;

	if (instance == NULL || instance->vdu_redirection_active == FALSE)
		return FALSE;

	/* Transform the location coordinates. */

	x = 2 * x;
	y = 2 * (instance->canvas_size.y - y);

	debug_printf("Print text at %d, %d (OS Units)", x, y);

	game_window_set_colour(instance, colour);
	xos_plot(os_MOVE_TO, x - 6, y);
	xos_plot(os_PLOT_BY | os_PLOT_SOLID, 12, 0);
	xos_plot(os_MOVE_BY, -6, -6);
	xos_plot(os_PLOT_BY | os_PLOT_SOLID, 0, 12);


	/* Convert the size in pixels into points. */

	error = xfont_converttopoints(size, size, &xpt, &ypt);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	debug_printf("Requested size %d pixels: that's %d, %d points", size, xpt, ypt);

	/* Use either Homerton or Corpus, depending on the monospace requirement. */

	error = xfont_find_font((monospaced) ? "Corpus.Medium" : "Homerton.Medium", xpt / 30, ypt / 30, 0, 0, &face, NULL, NULL);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Find the size of the supplies text. */

	error = xfont_scan_string(face, text, font_KERN | font_GIVEN_FONT | font_GIVEN_BLOCK | font_RETURN_BBOX,
			0x7fffffff, 0x7fffffff, &game_window_fonts_scan_block, NULL, 0, NULL, NULL, NULL, NULL);
	if (error != NULL) {
		font_lose_font(face);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Convert the bounding box back into OS units. */

	error = xfont_convertto_os(game_window_fonts_scan_block.bbox.x1 - game_window_fonts_scan_block.bbox.x0,
			game_window_fonts_scan_block.bbox.y1 - game_window_fonts_scan_block.bbox.y0, &xos, &yos);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		font_lose_font(face);
		return FALSE;
	}

	debug_printf("Bounding box for '%s' is x0=%d, y0=%d, x1=%d, y1=%d", text,
			game_window_fonts_scan_block.bbox.x0,
			game_window_fonts_scan_block.bbox.y0,
			game_window_fonts_scan_block.bbox.x1,
			game_window_fonts_scan_block.bbox.y1);

	debug_printf("Bounding box is %d, %d OS Units", xos, yos);

	if (horizontal < 0)
		x -= xos;
	else if (horizontal == 0)
		x -= (xos / 2);

	if (vertical < 0)
		y += yos;
	else if (vertical == 0)
		y += (yos / 2);

	error = xcolourtrans_set_font_colours(face, 0xffffffff, 0x00000000, 14, NULL, NULL, NULL);

	if (error == NULL)
		error = xfont_paint(face, text, font_OS_UNITS | font_KERN | font_GIVEN_FONT, x, y, NULL, NULL, 0);

	/* Free the fonts that were used. */

	font_lose_font(face);

	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	return TRUE;
}
