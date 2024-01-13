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
};

/* Static function prototypes. */

static void game_window_close_handler(wimp_close *close);
static void game_window_redraw_handler(wimp_draw *redraw);

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
 * Initialise and open a new game window.
 *
 * \param *parent	The parent game collection instance.
 * \return		Pointer to the new window instance, or NULL.
 */

struct game_window_block *game_window_create_instance(struct frontend *fe)
{
	wimp_window window_definition;
	struct game_window_block *new;
	os_error *error;

	/* Allocate the memory for the instance from the heap. */

	new = malloc(sizeof(struct game_window_block));
	if (new == NULL)
		return NULL;

	debug_printf("Creating a new game window instance: block=0x%x, fe=0x%x", new, fe);

	/* Initialise critical fields in the struct. */

	new->fe = fe;

	new->handle = NULL;
	new->sprite = NULL;
	new->save_area = NULL;
	new->vdu_redirection_active = FALSE;

	new->canvas_size.x = 0;
	new->canvas_size.y = 0;

	new->window_size.x = 0;
	new->window_size.y = 0;

	new->number_of_colours = 0;

	/* Create the new window. */

	window_definition.visible.x0 = 200;
	window_definition.visible.y0 = 200;
	window_definition.visible.x1 = 600;
	window_definition.visible.y1 = 600;
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
	window_definition.extent.y0 = -1200;
	window_definition.extent.x1 = 1200;
	window_definition.extent.y1 = 0;
	window_definition.title_flags = wimp_ICON_TEXT |
			wimp_ICON_BORDER | wimp_ICON_HCENTRED |
			wimp_ICON_VCENTRED | wimp_ICON_FILLED;
	window_definition.work_flags = wimp_BUTTON_NEVER << wimp_ICON_BUTTON_TYPE_SHIFT;
	window_definition.sprite_area = wimpspriteop_AREA;
	window_definition.xmin = 0;
	window_definition.ymin = 0;
	string_copy(window_definition.title_data.text, "Hello World!", 12);
	window_definition.icon_count = 0;

	error = xwimp_create_window(&window_definition, &(new->handle));
	if (error != NULL) {
		game_window_delete_instance(new);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return NULL;
	}

	/* Register the window events. */

	event_add_window_user_data(new->handle, new);
	event_add_window_close_event(new->handle, game_window_close_handler);
	event_add_window_redraw_event(new->handle, game_window_redraw_handler);

	/* Open the window. */

	windows_open(new->handle);

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

	if (instance->handle != NULL) {
		extent.x0 = 0;
		extent.x1 = 2 * x;
		extent.y0 = -2 * y;
		extent.y1 = 0;
		wimp_set_extent(instance->handle, &extent);

		instance->window_size.x = 2 * x;
		instance->window_size.y = 2 * y;
	}

	/* TODO -- Bodge to clear the screen. */

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
 *			the window.
 * \param y0		The Y coordinate of the top left corner of
 *			the window.
 * \param x1		The X coordinate of the bottom right corner
 *			of the window.
 * \param y1		The Y coordinate of the bottom right corner
 *			of the window.
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
