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

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
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

#include "game_collection.h"

/* The game window data structure. */

struct game_window_block {
	struct game_collection_block *parent;	/**< The parent game collection instance.	*/

	wimp_w handle;				/**< The handle of the game window.		*/

	int x_size;				/**< The X size of the window, in game pixels.	*/
	int y_size;				/**< The Y size of the window, in game pixels.	*/
};

/* Static function prototypes. */

static void game_window_close_handler(wimp_close *close);

/**
 * Initialise the game windows and their associated menus and dialogues.
 */

void game_window_initialise(void)
{
}

/**
 * Initialise and open a new game window.
 *
 * \param *parent	The parent game collection instance.
 * \return		Pointer to the new window instance, or NULL.
 */

struct game_window_block *game_window_create_instance(struct game_collection_block *parent)
{
	wimp_window window_definition;
	struct game_window_block *new;
	os_error *error;

	/* Allocate the memory for the instance from the static flex heap. */

	new = heap_alloc(sizeof(struct game_window_block));
	if (new == NULL)
		return NULL;

	debug_printf("Creating a new game window instance: block=0x%x", new);

	new->parent = parent;

	/* Create the new window. */

	window_definition.visible.x0 = 200;
	window_definition.visible.y0 = 200;
	window_definition.visible.x1 = 600;
	window_definition.visible.y1 = 600;
	window_definition.xscroll = 0;
	window_definition.yscroll = 0;
	window_definition.next = wimp_TOP;
	window_definition.flags = wimp_WINDOW_NEW_FORMAT |
			wimp_WINDOW_MOVEABLE | wimp_WINDOW_AUTO_REDRAW |
			wimp_WINDOW_BOUNDED_ONCE | wimp_WINDOW_BACK_ICON |
			wimp_WINDOW_CLOSE_ICON | wimp_WINDOW_TITLE_ICON |
			wimp_WINDOW_TOGGLE_ICON | wimp_WINDOW_VSCROLL |
			wimp_WINDOW_SIZE_ICON | wimp_WINDOW_HSCROLL;
	window_definition.title_fg = wimp_COLOUR_BLACK;
	window_definition.title_bg = wimp_COLOUR_LIGHT_GREY;
	window_definition.work_fg = wimp_COLOUR_BLACK;
	window_definition.work_bg = wimp_COLOUR_VERY_LIGHT_GREY;
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

	heap_free(instance);
}

/**
 * Handle Close events on game windows, deleting the window and the
 * associated instance data.
 *
 * \param *close		The Wimp Close data block.
 */

static void game_window_close_handler(wimp_close *close)
{
	struct game_window_block	*instance;

	debug_printf("\\RClosing game window");

	instance = event_get_window_user_data(close->w);
	if (instance == NULL)
		return;

	/* Delete the parent game instance. */

	game_collection_delete_instance(instance->parent);
}
