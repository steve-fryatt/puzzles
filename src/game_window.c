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
#include "oslib/osbyte.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/string.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
//#include "sflib/msgs.h"
#include "sflib/templates.h"
//#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files */

#include "game_window.h"

#include "core/puzzles.h"
#include "frontend.h"
#include "blitter.h"
#include "canvas.h"
#include "game_draw.h"
#include "game_config.h"
#include "game_window_backend_menu.h"
#include "index_window.h"

/* Game Window menu */

#define GAME_WINDOW_MENU_PRESETS 0
#define GAME_WINDOW_MENU_RESTART 1
#define GAME_WINDOW_MENU_NEW 2
#define GAME_WINDOW_MENU_SPECIFIC 3
#define GAME_WINDOW_MENU_RANDOM_SEED 4
#define GAME_WINDOW_MENU_SOLVE 5
#define GAME_WINDOW_MENU_HELP 6
#define GAME_WINDOW_MENU_UNDO 7
#define GAME_WINDOW_MENU_REDO 8
#define GAME_WINDOW_MENU_PREFERENCES 9

/* The height of the status bar. */

#define GAME_WINDOW_STATUS_BAR_HEIGHT 52

/* The length of the status bar text. */

#define GAME_WINDOW_STATUS_BAR_LENGTH 128

/* The autoscroll border. */

#define GAME_WINDOW_AUTOSCROLL_BORDER 80

/* Conversion from midend to RISC OS units. */

#define game_window_convert_x_coordinate_to_canvas(canvas_x, x) (CANVAS_PIXEL_SIZE * (x))
#define game_window_convert_y_coordinate_to_canvas(canvas_y, y) (CANVAS_PIXEL_SIZE * ((canvas_y) - ((y) + 1)))

/* Conversion from desktop to window coordinates. */

#define game_window_convert_to_window_x_coordinate(window, x) (((x) - (window)->visible.x0 + (window)->xscroll) / CANVAS_PIXEL_SIZE)
#define game_window_convert_to_window_y_coordinate(window, y) (-(((y) - (window)->visible.y1 + (window)->yscroll) / CANVAS_PIXEL_SIZE))

/* Workspace for calculating string sizes. */

static font_scan_block game_window_fonts_scan_block;

enum game_window_drag_type {
	GAME_WINDOW_DRAG_NONE,
	GAME_WINDOW_DRAG_SELECT,
	GAME_WINDOW_DRAG_MENU,
	GAME_WINDOW_DRAG_ADJUST
};

/* The game window data structure. */

struct game_window_block {
	struct frontend *fe;					/**< The parent frontend instance.		*/

	const char *title;					/**< The title of the game.			*/

	wimp_w handle;						/**< The handle of the game window.		*/
	wimp_w status_bar;					/**< The handle of the status bar.		*/
	wimp_i status_icon;					/**< The handle of the status bar icon.		*/

	struct blitter_set_block *blitters;			/**< The list of associated blitters.		*/

	struct canvas_block *canvas;				/**< The details for the window canvas.		*/

	struct game_config_block *specific;			/**< The config window for the specific code.	*/
	struct game_config_block *random_seed;			/**< The config window for the random seed.	*/
	struct game_config_block *preferences;			/**< The config window for the preferences.	*/
	struct game_config_block *custom;			/**< The config window for the custom game.	*/

	char status_text[GAME_WINDOW_STATUS_BAR_LENGTH];	/**< The status bar text.			*/

	os_coord window_size;					/**< The size of the window, in pixels.		*/

	int number_of_colours;					/**< The number of colours defined.		*/

	osbool callback_timer_active;				/**< Is the callback timer currently active?	*/
	os_t last_callback;					/**< The time of the last frontend callback.	*/

	enum game_window_drag_type drag_type;			/**< The current drag type.			*/
};

/* The Game Window menu. */

static wimp_menu *game_window_menu = NULL;

/* Static function prototypes. */

static void game_window_close_handler(wimp_close *close);
static void game_window_click_handler(wimp_pointer *pointer);
static int game_window_click_and_release(struct game_window_block *instance, wimp_pointer *pointer, wimp_window_state *state);
static int game_window_start_drag(struct game_window_block *instance, wimp_pointer *pointer, wimp_window_state *state);
static osbool game_window_drag_in_progress(void *data);
static void game_window_drag_end(wimp_dragged *drag, void *data);
static osbool game_window_keypress_handler(wimp_key *key);
static void game_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool game_window_config_complete(int type, config_item *config_data, enum game_config_outcome outcome, void *data);
static void game_window_redraw_handler(wimp_draw *redraw);
static void game_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void game_window_menu_close_handler(wimp_w w, wimp_menu *menu);
static osbool game_window_timer_callback(os_t time, void *data);

/**
 * Initialise the game windows and their associated menus and dialogues.
 */

void game_window_initialise(void)
{
	/* Font_ScanString block set-up. */

	game_window_fonts_scan_block.space.x = 0;
	game_window_fonts_scan_block.space.y = 0;

	game_window_fonts_scan_block.letter.x = 0;
	game_window_fonts_scan_block.letter.y = 0;

	game_window_fonts_scan_block.split_char = -1;

	/* The window menu. */

	game_window_menu = templates_get_menu("GameWindowMenu");
	ihelp_add_menu(game_window_menu, "GameMenu");
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

	// debug_printf"Creating a new game window instance: block=0x%x, fe=0x%x", new, fe);

	/* Initialise critical fields in the struct. */

	new->fe = fe;
	new->title = title;

	new->handle = NULL;
	new->status_bar = NULL;

	*(new->status_text) = '\0';

	new->window_size.x = 0;
	new->window_size.y = 0;

	new->number_of_colours = 0;

	new->callback_timer_active = FALSE;
	new->drag_type = GAME_WINDOW_DRAG_NONE;

	new->canvas = canvas_create_instance();
	new->blitters = blitter_create_set();

	new->specific = NULL;
	new->random_seed = NULL;
	new->preferences = NULL;
	new->custom = NULL;

	if (new->canvas == NULL || new->blitters == NULL) {
		game_window_delete_instance(new);
		return NULL;
	}

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

	// debug_printf"Deleting a game window instance: block=0x%x", instance);

	/* Delete the window. */

	if (instance->handle != NULL) {
		ihelp_remove_window(instance->handle);
		event_delete_window(instance->handle);
		wimp_delete_window(instance->handle);
	}

	if (instance->status_bar != NULL) {
		ihelp_remove_window(instance->status_bar);
		event_delete_window(instance->status_bar);
		wimp_delete_window(instance->status_bar);
	}

	/* Deallocate the instance block. */

	if (instance->canvas != NULL)
		canvas_delete_instance(instance->canvas);

	if (instance->blitters != NULL)
		blitter_delete_set(instance->blitters);

	if (instance->specific != NULL)
		game_config_delete_instance(instance->specific);

	if (instance->random_seed != NULL)
		game_config_delete_instance(instance->random_seed);

	if (instance->preferences != NULL)
		game_config_delete_instance(instance->preferences);

	if (instance->custom != NULL)
		game_config_delete_instance(instance->custom);

	free(instance);
}

/**
 * Create and open the game window at the specified location.
 * 
 * \param *instance	The instance to open the window on.
 * \param status_bar	TRUE if the window should have a status bar.
 * \param *pointer	The pointer at which to open the window.
 */

void game_window_open(struct game_window_block *instance, osbool status_bar, wimp_pointer *pointer)
{
	wimp_window window_definition;
	wimp_icon_create icon;
	os_error *error;

	if (instance == NULL || instance->handle != NULL)
		return;

	/* Create the main window. */

	window_definition.visible.x0 = 200; // The location will be updated when we open the window
	window_definition.visible.y0 = 200; // at the pointer, so only width and height matter!
	window_definition.visible.x1 = window_definition.visible.x0 + instance->window_size.x;
	window_definition.visible.y1 = window_definition.visible.y0 + instance->window_size.y +
			((status_bar == TRUE) ? GAME_WINDOW_STATUS_BAR_HEIGHT : 0);

	window_definition.xscroll = 0;
	window_definition.yscroll = 0;
	window_definition.next = wimp_TOP;
	window_definition.flags =
			wimp_WINDOW_NEW_FORMAT |
			wimp_WINDOW_MOVEABLE |
			wimp_WINDOW_BOUNDED_ONCE |
			wimp_WINDOW_BACK_ICON |
			wimp_WINDOW_CLOSE_ICON |
			wimp_WINDOW_TITLE_ICON |
			wimp_WINDOW_TOGGLE_ICON |
			wimp_WINDOW_VSCROLL |
			wimp_WINDOW_SIZE_ICON |
			wimp_WINDOW_HSCROLL;
	window_definition.title_fg = wimp_COLOUR_BLACK;
	window_definition.title_bg = wimp_COLOUR_LIGHT_GREY;
	window_definition.work_fg = wimp_COLOUR_BLACK;
	window_definition.work_bg = wimp_COLOUR_TRANSPARENT;
	window_definition.scroll_outer = wimp_COLOUR_MID_LIGHT_GREY;
	window_definition.scroll_inner = wimp_COLOUR_VERY_LIGHT_GREY;
	window_definition.highlight_bg = wimp_COLOUR_CREAM;
	window_definition.extra_flags = 0;
	window_definition.extent.x0 = 0;
	window_definition.extent.y0 = -(instance->window_size.y +
			((status_bar == TRUE) ? GAME_WINDOW_STATUS_BAR_HEIGHT : 0));
	window_definition.extent.x1 = instance->window_size.x;
	window_definition.extent.y1 = 0;
	window_definition.title_flags =
			wimp_ICON_TEXT |
			wimp_ICON_INDIRECTED |
			wimp_ICON_BORDER |
			wimp_ICON_HCENTRED |
			wimp_ICON_VCENTRED |
			wimp_ICON_FILLED;
	window_definition.work_flags = wimp_BUTTON_RELEASE_DRAG << wimp_ICON_BUTTON_TYPE_SHIFT;
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

	ihelp_add_window(instance->handle, "Game", NULL);
	event_add_window_menu(instance->handle, game_window_menu);
	event_add_window_menu_prepare(instance->handle, game_window_menu_prepare_handler);
	event_add_window_menu_close(instance->handle, game_window_menu_close_handler);
	event_add_window_menu_selection(instance->handle, game_window_menu_selection_handler);

	event_add_window_user_data(instance->handle, instance);
	event_add_window_close_event(instance->handle, game_window_close_handler);
	event_add_window_redraw_event(instance->handle, game_window_redraw_handler);
	event_add_window_mouse_event(instance->handle, game_window_click_handler);
	event_add_window_key_event(instance->handle, game_window_keypress_handler);

	/* Create the status bar. */

	if (status_bar == TRUE) {
		window_definition.flags =
				wimp_WINDOW_NEW_FORMAT |
				wimp_WINDOW_AUTO_REDRAW | 
				wimp_WINDOW_MOVEABLE |
				wimp_WINDOW_BOUNDED_ONCE;
		window_definition.extent.y0 = -GAME_WINDOW_STATUS_BAR_HEIGHT;
		window_definition.work_bg = wimp_COLOUR_VERY_LIGHT_GREY;
		window_definition.title_flags =
				wimp_ICON_TEXT |
				wimp_ICON_BORDER |
				wimp_ICON_HCENTRED |
				wimp_ICON_VCENTRED |
				wimp_ICON_FILLED;
		strncpy(window_definition.title_data.text, "Status Bar", 12);

		error = xwimp_create_window(&window_definition, &(instance->status_bar));
		if (error != NULL) {
			game_window_delete_instance(instance);
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return;
		}

		icon.w = instance->status_bar;
		icon.icon.extent.x0 = window_definition.extent.x0;
		icon.icon.extent.y0 = window_definition.extent.y0;
		icon.icon.extent.x1 = window_definition.extent.x1;
		icon.icon.extent.y1 = window_definition.extent.y1;
		icon.icon.flags = 
				wimp_ICON_TEXT |
				wimp_ICON_INDIRECTED |
				wimp_ICON_VCENTRED |
				(wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
				(wimp_COLOUR_VERY_LIGHT_GREY << wimp_ICON_BG_COLOUR_SHIFT);
		icon.icon.data.indirected_text.text = instance->status_text;
		icon.icon.data.indirected_text.size = GAME_WINDOW_STATUS_BAR_LENGTH;
		icon.icon.data.indirected_text.validation = NULL;
		error = xwimp_create_icon(&icon, &(instance->status_icon));
		if (error != NULL) {
			game_window_delete_instance(instance);
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return;
		}

		/* Register the window events. */

		ihelp_add_window(instance->status_bar, "Game", NULL);

		event_add_window_menu(instance->status_bar, game_window_menu);
		event_add_window_menu_prepare(instance->status_bar, game_window_menu_prepare_handler);
		event_add_window_menu_close(instance->status_bar, game_window_menu_close_handler);
		event_add_window_menu_selection(instance->status_bar, game_window_menu_selection_handler);
	}

	/* Open the window. */

	windows_open_centred_at_pointer(instance->handle, pointer);
	if (instance->status_bar != NULL)
		windows_open_nested_as_footer(instance->status_bar, instance->handle, GAME_WINDOW_STATUS_BAR_HEIGHT, TRUE);

	wimp_set_caret_position(instance->handle, wimp_ICON_WINDOW, 0, 0, -1, -1);
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
	wimp_pointer			pointer;

	// debug_printf"\\RClosing game window");

	instance = event_get_window_user_data(close->w);
	if (instance == NULL)
		return;

	/* If Adjust was clicked, open the index window. */

	wimp_get_pointer_info(&pointer);

	if (pointer.buttons == wimp_CLICK_ADJUST)
		index_window_open();

	/* Save the sprite for analysis.
	 *
	 * TODO -- Remove!
	 */

//	canvas_save_sprite(instance->canvas, "RAM::RamDisc0.$.Sprites");

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
	enum frontend_event_outcome	outcome = FRONTEND_EVENT_UNKNOWN;

	instance = event_get_window_user_data(pointer->w);
	if (instance == NULL)
		return;

	window.w = pointer->w;
	wimp_get_window_state(&window);

	/* Process the click event. We're collecting clicks on release, to
	 * allow us to spot and handle drags correctly. If a click comes
	 * in while a drag is flagged as active, we've already supplied
	 * the *_RELEASE on the Drag_End event and can simply reset the
	 * dragging flag and discard the event.
	 */

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
	case wimp_CLICK_ADJUST:
		if (instance->drag_type == GAME_WINDOW_DRAG_NONE)
			outcome = game_window_click_and_release(instance, pointer, &window);
		else
			instance->drag_type = GAME_WINDOW_DRAG_NONE;
		break;
	case wimp_DRAG_SELECT:
	case wimp_DRAG_ADJUST:
		outcome = game_window_start_drag(instance, pointer, &window);
		break;
	}

	/* If the event outcome was "Quit", just exit now. Otherwise, set focus to our window */
	
	if (outcome == FRONTEND_EVENT_EXIT)
		frontend_delete_instance(instance->fe);
	else
		wimp_set_caret_position(instance->handle, wimp_ICON_WINDOW, 0, 0, -1, -1);
}

/**
 * Handle a simple mouse click (and release) event.
 * 
 * \param *instance	The instance from which the event originates.
 * \param *pointer	The pointer details.
 * \param *state	The window state for the window.
 * \return		The outcome of passing the event to the frontend.
 */

static int game_window_click_and_release(struct game_window_block *instance, wimp_pointer *pointer, wimp_window_state *state)
{
	int x, y, up, down, outcome;

	if (instance == NULL || instance->fe == NULL)
		return FRONTEND_EVENT_REJECTED;

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
		/* Some things appear to use middle click, so Ctrl-Select emulates Menu. */

		if (osbyte1(osbyte_IN_KEY, 0xfb, 0xff) == 0xff || osbyte1(osbyte_IN_KEY, 0xf8, 0xff) == 0xff) {
			down = MIDDLE_BUTTON;
			up = MIDDLE_RELEASE;
		} else {
			down = LEFT_BUTTON;
			up = LEFT_RELEASE;
		}
		break;
	case wimp_CLICK_ADJUST:
		down = RIGHT_BUTTON;
		up = RIGHT_RELEASE;
		break;
	default:
		return FRONTEND_EVENT_REJECTED;
	}

	x = game_window_convert_to_window_x_coordinate(state, pointer->pos.x);
	y = game_window_convert_to_window_y_coordinate(state, pointer->pos.y);

	outcome = frontend_handle_key_event(instance->fe, x, y, down);
	if (outcome == FRONTEND_EVENT_EXIT)
		return outcome;

	return frontend_handle_key_event(instance->fe, x, y, up);
}

/**
 * Handle the start of a drag event.
 * 
 * \param *instance	The instance from which the event originates.
 * \param *pointer	The pointer details.
 * \param *state	The window state for the window.
 * \return		The outcome of passing the event to the frontend.
 */

static int game_window_start_drag(struct game_window_block *instance, wimp_pointer *pointer, wimp_window_state *state)
{
	wimp_drag drag;
	wimp_auto_scroll_info scroll;
	int x, y, down;

	if (instance == NULL || instance->fe == NULL || instance->drag_type != GAME_WINDOW_DRAG_NONE)
		return FRONTEND_EVENT_REJECTED;

	drag.w = instance->handle;
	drag.type = wimp_DRAG_USER_POINT;

	drag.initial.x0 = pointer->pos.x;
	drag.initial.y0 = pointer->pos.y;
	drag.initial.x1 = pointer->pos.x;
	drag.initial.y1 = pointer->pos.y;

	drag.bbox.x0 = state->visible.x0;
	drag.bbox.y0 = state->visible.y0 +
			((instance->status_bar == NULL) ? 0 : GAME_WINDOW_STATUS_BAR_HEIGHT);
	drag.bbox.x1 = state->visible.x1;
	drag.bbox.y1 = state->visible.y1;

	scroll.w = instance->handle;

	scroll.pause_zone_sizes.x0 = GAME_WINDOW_AUTOSCROLL_BORDER;
	scroll.pause_zone_sizes.y0 = GAME_WINDOW_AUTOSCROLL_BORDER +
			((instance->status_bar == NULL) ? 0 : GAME_WINDOW_STATUS_BAR_HEIGHT);
	scroll.pause_zone_sizes.x1 = GAME_WINDOW_AUTOSCROLL_BORDER;
	scroll.pause_zone_sizes.y1 = GAME_WINDOW_AUTOSCROLL_BORDER;

	scroll.pause_duration = 0;
	scroll.state_change = wimp_AUTO_SCROLL_DEFAULT_HANDLER;

	wimp_drag_box(&drag);
	wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_HORIZONTAL | wimp_AUTO_SCROLL_ENABLE_VERTICAL, &scroll);
	event_set_drag_handler(game_window_drag_end, game_window_drag_in_progress, instance);

	switch (pointer->buttons) {
	case wimp_DRAG_SELECT:
		/* Some things appear to use middle drag, so Ctrl-Select emulates Menu. */

		if (osbyte1(osbyte_IN_KEY, 0xfb, 0xff) == 0xff || osbyte1(osbyte_IN_KEY, 0xf8, 0xff) == 0xff) {
			instance->drag_type = GAME_WINDOW_DRAG_MENU;
			down = MIDDLE_BUTTON;
		} else {
			instance->drag_type = GAME_WINDOW_DRAG_SELECT;
			down = LEFT_BUTTON;
		}
		break;
	case wimp_DRAG_ADJUST:
		instance->drag_type = GAME_WINDOW_DRAG_ADJUST;
		down = RIGHT_BUTTON;
		break;
	default:
		return FRONTEND_EVENT_REJECTED;
	}

	x = game_window_convert_to_window_x_coordinate(state, pointer->pos.x);
	y = game_window_convert_to_window_y_coordinate(state, pointer->pos.y);

	return frontend_handle_key_event(instance->fe, x, y, down);
}

/**
 * Handle Null Events from the Wimp during a drag operation.
 * 
 * \param *data		Pointer to the instance from which the event originates.
 * \return		TRUE if the event was handled.
 */

static osbool game_window_drag_in_progress(void *data)
{
	struct game_window_block *instance = data;
	wimp_pointer pointer;
	wimp_window_state window;
	int x, y, drag;

	if (instance == NULL || instance->fe == NULL)
		return TRUE;

	switch (instance->drag_type) {
	case GAME_WINDOW_DRAG_SELECT:
		drag = LEFT_DRAG;
		break;
	case GAME_WINDOW_DRAG_MENU:
		drag = MIDDLE_DRAG;
		break;
	case GAME_WINDOW_DRAG_ADJUST:
		drag = RIGHT_DRAG;
		break;
	default:
		return TRUE;
	}

	if (xwimp_get_pointer_info(&pointer) != NULL)
		return TRUE;

	window.w = instance->handle;
	if (xwimp_get_window_state(&window) != NULL)
		return TRUE;

	x = game_window_convert_to_window_x_coordinate(&window, pointer.pos.x);
	y = game_window_convert_to_window_y_coordinate(&window, pointer.pos.y);

	// debug_printf"Drag in progress: x=%d, y=%d", pointer.pos.x, pointer.pos.y);

	frontend_handle_key_event(instance->fe, x, y, drag);

	return TRUE;
}

/**
 * Handle the Drag End event from the Wimp at the end of a drag operation.
 * 
 * \param *drag		The Wimp event data block.
 * \param *data		Pointer to the instance from which the event originates.
 */

static void game_window_drag_end(wimp_dragged *drag, void *data)
{
	struct game_window_block *instance = data;
	wimp_window_state window;
	int x, y, release;

	if (instance == NULL || instance->fe == NULL)
		return;

	/* Terminate the scroll process. */

	if (xwimp_auto_scroll(NONE, NULL, NULL) != NULL)
		return;

	switch (instance->drag_type) {
	case GAME_WINDOW_DRAG_SELECT:
		release = LEFT_RELEASE;
		break;
	case GAME_WINDOW_DRAG_MENU:
		release = MIDDLE_RELEASE;
		break;
	case GAME_WINDOW_DRAG_ADJUST:
		release = RIGHT_RELEASE;
		break;
	default:
		return;
	}

	window.w = instance->handle;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	x = game_window_convert_to_window_x_coordinate(&window, drag->final.x0);
	y = game_window_convert_to_window_y_coordinate(&window, drag->final.y0);

	frontend_handle_key_event(instance->fe, x, y, release);
}

/**
 * Handle keypress events in game windows.
 *
 * \param *key		The Wimp Key event data block.
 * \return		TRUE if the event was handled; otherwise FALSE.
 */

static osbool game_window_keypress_handler(wimp_key *key)
{
	struct game_window_block	*instance;
	enum frontend_event_outcome	outcome = FRONTEND_EVENT_UNKNOWN;
	int				button = -1, numpad = -1;

	instance = event_get_window_user_data(key->w);
	if (instance == NULL)
		return FALSE;

	/* Convert RISC OS-specifc key codes.
	 *
	 * The handling of the numeric keypad is probably not compatible
	 * with pre-RiscPC hardware (and possibly non-UK keyboard layouts).
	 * This may need revisiting in the future!
	 */

	switch (key->c) {
	case wimp_KEY_F8:
		frontend_handle_key_event(instance->fe, 0, 0, UI_UNDO);
		break;
	case wimp_KEY_F9:
		frontend_handle_key_event(instance->fe, 0, 0, UI_REDO);
		break;
	case wimp_KEY_LEFT:
		button = CURSOR_LEFT;
		break;
	case wimp_KEY_LEFT | wimp_KEY_SHIFT:
		button = CURSOR_LEFT | MOD_SHFT;
		break;
	case wimp_KEY_LEFT | wimp_KEY_CONTROL:
		button = CURSOR_LEFT | MOD_CTRL;
		break;
	case wimp_KEY_LEFT | wimp_KEY_SHIFT | wimp_KEY_CONTROL:
		button = CURSOR_LEFT | MOD_SHFT | MOD_CTRL;
		break;
	case wimp_KEY_RIGHT:
		button = CURSOR_RIGHT;
		break;
	case wimp_KEY_RIGHT | wimp_KEY_SHIFT:
		button = CURSOR_RIGHT | MOD_SHFT;
		break;
	case wimp_KEY_RIGHT | wimp_KEY_CONTROL:
		button = CURSOR_RIGHT | MOD_CTRL;
		break;
	case wimp_KEY_RIGHT | wimp_KEY_SHIFT | wimp_KEY_CONTROL:
		button = CURSOR_RIGHT | MOD_SHFT | MOD_CTRL;
		break;
	case wimp_KEY_UP:
		button = CURSOR_UP;
		break;
	case wimp_KEY_UP | wimp_KEY_SHIFT:
		button = CURSOR_UP | MOD_SHFT;
		break;
	case wimp_KEY_UP | wimp_KEY_CONTROL:
		button = CURSOR_UP | MOD_CTRL;
		break;
	case wimp_KEY_UP | wimp_KEY_SHIFT | wimp_KEY_CONTROL:
		button = CURSOR_UP | MOD_SHFT | MOD_CTRL;
		break;
	case wimp_KEY_DOWN:
		button = CURSOR_DOWN;
		break;
	case wimp_KEY_DOWN | wimp_KEY_SHIFT:
		button = CURSOR_DOWN | MOD_SHFT;
		break;
	case wimp_KEY_DOWN | wimp_KEY_CONTROL:
		button = CURSOR_DOWN | MOD_CTRL;
		break;
	case wimp_KEY_DOWN | wimp_KEY_SHIFT | wimp_KEY_CONTROL:
		button = CURSOR_DOWN | MOD_SHFT | MOD_CTRL;
		break;
	case '1':
		button = key->c;
		numpad = 107;
		break;
	case '2':
		button = key->c;
		numpad = 124;
		break;
	case '3':
		button = key->c;
		numpad = 108;
		break;
	case '4':
		button = key->c;
		numpad = 122;
		break;
	case '5':
		button = key->c;
		numpad = 123;
		break;
	case '6':
		button = key->c;
		numpad = 26;
		break;
	case '7':
		button = key->c;
		numpad = 27;
		break;
	case '8':
		button = key->c;
		numpad = 42;
		break;
	case '9':
		button = key->c;
		numpad = 43;
		break;
	case '0':
		button = key->c;
		numpad = 106;
		break;
	case '/':
		button = key->c;
		numpad = 74;
		break;
	case '*':
		button = key->c;
		numpad = 91;
		break;
	case '-':
		button = key->c;
		numpad = 59;
		break;
	case '+':
		button = key->c;
		numpad = 58;
		break;
	case '.':
		button = key->c;
		numpad = 76;
		break;
	case '\r':
		button = key->c;
		numpad = 60;
		break;
	default:
		if (key->c >= 0 && key->c < 127)
			button = key->c;
		break;
	}

	/* Check for number pad keys being down, and flag them. */

	if (numpad >= 0) {
		if (osbyte1(osbyte_IN_KEY, numpad ^ 0xff, 0xff) == 0xff)
			button |= MOD_NUM_KEYPAD;
	}

	/* Process any code that we found. */

	if (button >= 0)
		outcome = frontend_handle_key_event(instance->fe, 0, 0, button);

	if (outcome == FRONTEND_EVENT_UNKNOWN)
		return FALSE;

	if (outcome == FRONTEND_EVENT_EXIT)
		frontend_delete_instance(instance->fe);

	return (outcome == FRONTEND_EVENT_REJECTED) ? FALSE : TRUE;
}

/**
 * Handle Menu Selection events from game windows.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *selection	The menu selection details.
 */

static void game_window_menu_selection_handler(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct game_window_block *instance;
	struct game_params *params;
	wimp_pointer pointer;
	config_item *config_data = NULL;
	char *window_title = NULL;
	osbool custom = FALSE;

	if (menu != game_window_menu)
		return;

	instance = event_get_window_user_data(w);
	if (instance == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]) {
	case GAME_WINDOW_MENU_NEW:
		frontend_perform_action(instance->fe, FRONTEND_ACTION_SIMPLE_NEW);
		break;
	case GAME_WINDOW_MENU_RESTART:
		frontend_perform_action(instance->fe, FRONTEND_ACTION_RESTART);
		break;
	case GAME_WINDOW_MENU_SOLVE:
		frontend_perform_action(instance->fe, FRONTEND_ACTION_SOLVE);
		break;
	case GAME_WINDOW_MENU_HELP:
		frontend_perform_action(instance->fe, FRONTEND_ACTION_HELP);
		break;
	case GAME_WINDOW_MENU_UNDO:
		frontend_handle_key_event(instance->fe, 0, 0, UI_UNDO);
		break;
	case GAME_WINDOW_MENU_REDO:
		frontend_handle_key_event(instance->fe, 0, 0, UI_REDO);
		break;
	case GAME_WINDOW_MENU_PRESETS:
		params = game_window_backend_menu_decode(selection, 1, &custom);
		if (params != NULL) {
			frontend_start_new_game_from_parameters(instance->fe, params);
		} else if (custom == TRUE && instance->custom == NULL) {
			frontend_get_config_info(instance->fe, CFG_SETTINGS, &config_data, &window_title);
			instance->custom = game_config_create_instance(CFG_SETTINGS, config_data, window_title, &pointer, game_window_config_complete, instance);
		}
		break;
	case GAME_WINDOW_MENU_SPECIFIC:
		if (instance->specific == NULL) {
			frontend_get_config_info(instance->fe, CFG_DESC, &config_data, &window_title);
			instance->specific = game_config_create_instance(CFG_DESC, config_data, window_title, &pointer, game_window_config_complete, instance);
		}
		break;
	case GAME_WINDOW_MENU_RANDOM_SEED:
		if (instance->random_seed == NULL) {
			frontend_get_config_info(instance->fe, CFG_SEED, &config_data, &window_title);
			instance->random_seed = game_config_create_instance(CFG_SEED, config_data, window_title, &pointer, game_window_config_complete, instance);
		}
		break;
	case GAME_WINDOW_MENU_PREFERENCES:
		if (instance->preferences == NULL) {
			frontend_get_config_info(instance->fe, CFG_PREFS, &config_data, &window_title);
			instance->preferences = game_config_create_instance(CFG_PREFS, config_data, window_title, &pointer, game_window_config_complete, instance);
		}
		break;
	}
}

/**
 * Handle user updates from the Game Config boxes.
 * 
 * \param type			The type of data being returned.
 * \param *config_data		Pointer to the config data structure.
 * \param outcome		The outcome from the dialogue box.
 * \param *data			Pointer to the associated Game Window instance.
 * \return			TRUE if the midend accepted the data;
 *				otherwise FALSE.
 */

static osbool game_window_config_complete(int type, config_item *config_data, enum game_config_outcome outcome, void *data)
{
	struct game_window_block *instance = data;
	osbool midend_response = TRUE;

	if (instance == NULL)
		return FALSE;

	/* If the choices were set, update the configuration. */

	if (outcome & GAME_CONFIG_SET)
		midend_response = frontend_set_config_info(instance->fe, type, config_data);

	/* Delete our reference to the dialogue unless Hold Open is requested. */

	if (!(outcome & GAME_CONFIG_HOLD_OPEN) && midend_response) {
		switch (type) {
		case CFG_DESC:
			instance->specific = NULL;
			break;
		case CFG_SEED:
			instance->random_seed = NULL;
			break;
		case CFG_PREFS:
			instance->preferences = NULL;
			break;
		case CFG_SETTINGS:
			instance->custom = NULL;
			break;
		}
	}

	/* If the choices were set, reflect them in a new game. */

	if (outcome & GAME_CONFIG_SET)
		frontend_perform_action(instance->fe, FRONTEND_ACTION_SIMPLE_NEW);

	return midend_response;
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
	osbool				canvas_ready = FALSE, more;

	translation_table = (osspriteop_trans_tab *) &table;

	instance = event_get_window_user_data(redraw->w);

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	if (instance != NULL)
		canvas_ready = canvas_prepare_redraw(instance->canvas, &factors, translation_table);

	while (more) {
		if (instance != NULL && canvas_ready == TRUE)
			canvas_redraw_sprite(instance->canvas, ox, oy - instance->window_size.y, &factors, translation_table);

		more = wimp_get_rectangle(redraw);
	}
}

/**
 * Handle Menu Prepare events from game windows
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *pointer	The pointer position, or NULL for a re-open.
 */

static void game_window_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct game_window_block *instance;
	struct preset_menu *presets = NULL;
	wimp_menu *presets_submenu = NULL;
	int presets_limit = 0, current_preset = 0;
	osbool can_configure = FALSE, can_undo = FALSE, can_redo = FALSE, can_solve = FALSE;

	if (menu != game_window_menu)
		return;

	instance = event_get_window_user_data(w);
	if (instance == NULL)
		return;

	frontend_get_menu_info(instance->fe, &presets, &presets_limit,
			&current_preset, &can_configure, &can_undo, &can_redo, &can_solve);

	/* The menu is being newly opened, so set up the one-off data. */

	if (pointer != NULL) {

		/* Set the menu title. */

		menu->title_data.indirected_text.text = (char *) instance->title;

		/* Build the presets submenus. */

		presets_submenu = game_window_backend_menu_create(presets, presets_limit, can_configure);

		menu->entries[GAME_WINDOW_MENU_PRESETS].sub_menu = presets_submenu;
		menus_shade_entry(menu, GAME_WINDOW_MENU_PRESETS, (presets_submenu == NULL) ? TRUE : FALSE);
	}

	/* Update the menu state. */

	game_window_backend_menu_update_state(current_preset, (instance->custom == NULL) ? TRUE : FALSE);

	menus_shade_entry(menu, GAME_WINDOW_MENU_UNDO, !can_undo);
	menus_shade_entry(menu, GAME_WINDOW_MENU_REDO, !can_redo);
	menus_shade_entry(menu, GAME_WINDOW_MENU_SOLVE, !can_solve);

	menus_shade_entry(menu, GAME_WINDOW_MENU_SPECIFIC, (instance->specific == NULL) ? FALSE : TRUE);
	menus_shade_entry(menu, GAME_WINDOW_MENU_RANDOM_SEED, (instance->random_seed == NULL) ? FALSE : TRUE);
	menus_shade_entry(menu, GAME_WINDOW_MENU_PREFERENCES, (instance->preferences == NULL) ? FALSE : TRUE);
}

/**
 * Handle Menu Close events from game windows
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 */

static void game_window_menu_close_handler(wimp_w w, wimp_menu *menu)
{
	if (menu != game_window_menu)
		return;

	game_window_backend_menu_destroy();
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
	int window_height;
	os_coord canvas_size, centre;
	wimp_window_state state;
	os_box extent;

	if (instance == NULL)
		return FALSE;

	/* Check to see if there's anything to do. */

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	if (canvas_size.x == x && canvas_size.y == y)
		return TRUE;

	instance->number_of_colours = 0;

	// debug_printf"Requesting canvas of x=%d, y=%d", x, y);

	/* Allocate, or adjust, the required area. */

	if (canvas_configure_area(instance->canvas, x, y, TRUE) == FALSE)
		return FALSE;

	/* Configure the game colours. */

	if (canvas_set_game_colours(instance->canvas, colours, number_of_colours) == FALSE)
		return FALSE;

	instance->number_of_colours = number_of_colours;

	/* Initialise the save area. */

	if (canvas_configure_save_area(instance->canvas) == FALSE)
		return FALSE;

	/* Set the window and status bar extent. */

	instance->window_size.x = 2 * x;
	instance->window_size.y = 2 * y;

	if (instance->handle != NULL) {
		extent.x0 = 0;
		extent.x1 = instance->window_size.x;
		extent.y0 = -(instance->window_size.y + ((instance->status_bar == NULL) ? 0 : GAME_WINDOW_STATUS_BAR_HEIGHT));
		extent.y1 = 0;
		wimp_set_extent(instance->handle, &extent);
	}

	if (instance->status_bar != NULL) {
		extent.x0 = 0;
		extent.x1 = instance->window_size.x;
		extent.y0 = -GAME_WINDOW_STATUS_BAR_HEIGHT;
		extent.y1 = 0;
		wimp_set_extent(instance->status_bar, &extent);
		windows_redraw(instance->status_bar);
	}

	/* Update the visible area. */

	if (instance->handle != NULL) {
		state.w = instance->handle;
		wimp_get_window_state(&state);

		centre.x = state.visible.x0 + ((state.visible.x1 - state.visible.x0) / 2);
		centre.y = state.visible.y0 + ((state.visible.y1 - state.visible.y0) / 2);

		window_height = instance->window_size.y + ((instance->status_bar == NULL) ? 0 : GAME_WINDOW_STATUS_BAR_HEIGHT);

		state.visible.x0 = centre.x - (instance->window_size.x / 2);
		state.visible.y0 = centre.y - (window_height / 2);

		state.visible.x1 = state.visible.x0 + instance->window_size.x;
		state.visible.y1 = state.visible.y0 + window_height;

		wimp_open_window((wimp_open *) &state);
	}

	return TRUE;
}

/**
 * Update the text in the status bar.
 * 
 * \param *instance		The instance to update.
 * \param *text			The new status bar text.
 * \return			TRUE if successful; otherwise FALSE.
 */

osbool game_window_set_status_text(struct game_window_block *instance, const char *text)
{
	if (instance == NULL)
		return false;

	string_copy(instance->status_text, (char *) text, GAME_WINDOW_STATUS_BAR_LENGTH);

	if (instance->status_bar != NULL)
		wimp_set_icon_state(instance->status_bar, instance->status_icon, 0, 0);

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
	if (instance == NULL || instance->canvas == NULL)
		return FALSE;

	return canvas_start_redirection(instance->canvas);
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
	if (instance == NULL || instance->canvas == NULL)
		return FALSE;

	/* Reset any graphics clip window that may have been left in force. */

	game_window_clear_clip(instance);

	return canvas_stop_redirection(instance->canvas);
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

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	/* There's no point queueing updates if the window isn't open. */

	if (instance->handle == NULL || windows_get_open(instance->handle) == FALSE)
		return TRUE;

	x0 = x0 * CANVAS_PIXEL_SIZE;
	y0 = -y0 * CANVAS_PIXEL_SIZE;

	x1 = (x1 + 1) * CANVAS_PIXEL_SIZE;
	y1 = -(y1 + 1) * CANVAS_PIXEL_SIZE;

	// debug_printf"Request a redraw: x0=%d, y0=%d, x1=%d, y1=%d", x0, y1, x1, y0);

	/* Queue the update. */

	error = xwimp_force_redraw(instance->handle, x0, y1, x1, y0);
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

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	if (colour < 0 || colour >= instance->number_of_colours)
		return TRUE;

	error = xos_set_colour(os_ACTION_OVERWRITE, colour);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	// debug_printf"\\lSelect colour %d", colour);

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
	os_coord canvas_size;
	os_error *error;

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	x0 = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x0);
	y0 = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y0);

	x1 = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x1);
	y1 = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y1);

	error = xos_writec(os_VDU_SET_GRAPHICS_WINDOW);

	if (error == NULL)
		error = xos_writec(x0 & 0xff);

	if (error == NULL)
		error = xos_writec((x0 >> 8) & 0xff);

	if (error == NULL)
		error = xos_writec(y1 & 0xff);

	if (error == NULL)
		error = xos_writec((y1 >> 8) & 0xff);

	if (error == NULL)
		error = xos_writec(x1 & 0xff);

	if (error == NULL)
		error = xos_writec((x1 >> 8) & 0xff);

	if (error == NULL)
		error = xos_writec(y0 & 0xff);

	if (error == NULL)
		error = xos_writec((y0 >> 8) & 0xff);

	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	// debug_printf"\\lSet clip to %d,%d -- %d,%d", x0, y1, x1, y0);

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

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	error = xos_writec(os_VDU_RESET_WINDOWS);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	// debug_printf"Reset clip");

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
	os_coord canvas_size;
	os_error *error;

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	x = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x);
	y = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y);

	// debug_printf"\\lPlotted 0x%x to %d, %d", plot_code, x, y);

	error = xos_plot(plot_code, x, y);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

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
	os_coord canvas_size;

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	game_draw_start_path();

	x = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x);
	y = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y);

	// debug_printf"\\lStart path from %d, %d", x, y);

	return game_draw_add_move(x, y);
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
	os_coord canvas_size;

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	x = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x);
	y = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y);

	// debug_printf"\\lContinue path to %d, %d", x, y);

	return game_draw_add_line(x, y);
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

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
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

	// debug_printf"\\lPath plotted");

	return TRUE;
}

/**
 * Write a line of text in a game window.
 * 
 * \param *instance	The instance to write to.
 * \param x		The X coordinate at which to write the text.
 * \param y		The Y coordinate at which to write the text.
 * \param size		The size of the text in pixels.
 * \param align		The alignment of the text around the coordinates.
 * \param colour	The colour of the text.
 * \param monospaced	TRUE to use a monospaced font.
 * \param *text		The text to write.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_write_text(struct game_window_block *instance, int x, int y, int size, int align, int colour, osbool monospaced, const char *text)
{
	font_f face;
	os_colour foreground, background;
	int xpt, ypt, width, height, xoffset = 0, yoffset = 0;
	os_coord canvas_size;
	os_error *error;

	if (instance == NULL || canvas_is_redirection_active(instance->canvas) == FALSE)
		return FALSE;

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	/* Transform the location coordinates. */

	x = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x);
	y = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y);

	size *= CANVAS_PIXEL_SIZE;

	// debug_printf"\\lPrint text at %d, %d (OS Units)", x, y);

	/* Convert the size in pixels into points. */

	error = xfont_converttopoints(size, size, &xpt, &ypt);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Use either Homerton or Corpus, depending on the monospace requirement. */

	error = xfont_find_font((monospaced) ? "Corpus.Bold" : "Homerton.Bold", xpt / 62.5, ypt / 62.5, 0, 0, &face, NULL, NULL);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Find the size of the supplied text. */

	error = xfont_scan_string(face, text, font_KERN | font_GIVEN_FONT | font_GIVEN_BLOCK | font_RETURN_BBOX,
			0x7fffffff, 0x7fffffff, &game_window_fonts_scan_block, NULL, 0, NULL, NULL, NULL, NULL);
	if (error != NULL) {
		font_lose_font(face);
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Align the text around the coordinates. */

	/* TODO -- The vertical alignment is oncorrect, as ALIGN_VCENTRE should
	 * align a standard line of capitals. We instead align each piece of
	 * text in isolation.
	 */

	width = game_window_fonts_scan_block.bbox.x1 - game_window_fonts_scan_block.bbox.x0;
	height = game_window_fonts_scan_block.bbox.y1 - game_window_fonts_scan_block.bbox.y0;

	if (align & ALIGN_HLEFT)
		xoffset = - game_window_fonts_scan_block.bbox.x0;
	else if (align & ALIGN_HCENTRE)
		xoffset = - (game_window_fonts_scan_block.bbox.x0 + (width / 2));
	else if (align & ALIGN_HRIGHT)
		xoffset = - (game_window_fonts_scan_block.bbox.x0 + width);

	if (align & ALIGN_VCENTRE)
		yoffset = - (game_window_fonts_scan_block.bbox.y0 + (height / 2));

	/* Convert the offsets back into OS units. */

	error = xfont_convertto_os(xoffset, yoffset, &xoffset, &yoffset);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		font_lose_font(face);
		return FALSE;
	}

	/* Set the colours and plot the text. */

	foreground = canvas_get_palette_entry(instance->canvas, colour);
	background = canvas_get_palette_entry(instance->canvas, 0);

	error = xcolourtrans_set_font_colours(face, background, foreground, 14, NULL, NULL, NULL);

	if (error == NULL)
		error = xfont_paint(face, text, font_OS_UNITS | font_KERN | font_GIVEN_FONT, x + xoffset, y + yoffset, NULL, NULL, 0);

	/* Free the fonts that were used. */

	font_lose_font(face);

	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	return TRUE;
}

/**
 * Create a new blitter within a game window.
 * 
 * \param *instance	The instance to take the blitter.
 * \param width		The width of the blitter, in pixels.
 * \param height	The height of the blitter, in pixels.
 * \return		Pointer to the new blitter.
 */

blitter *game_window_create_blitter(struct game_window_block *instance, int width, int height)
{
	if (instance == NULL)
		return NULL;

	// debug_printf"\\ORequesting new blitter: width=%d, height=%d", width, height);

	return (blitter *) blitter_create(instance->blitters, width, height);
}

/**
 * Delete a blitter from within a game window.
 * 
 * \param *instance	The instance containing the blitter.
 * \param *blitter	The blitter to be deleted.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_delete_blitter(struct game_window_block *instance, blitter *blitter)
{
	if (instance == NULL)
		return FALSE;

	// debug_printf"\\OBlitter Free");

	return blitter_delete(instance->blitters, (struct blitter_block *) blitter);
}

/**
 * Save a section of the game window to a blitter.
 *
 * \param *instance	The instance containing the blitter.
 * \param *blitter	The blitter to save to.
 * \param x		The X coordinate of the top-left of the save area.
 * \param y		The Y coordinate of the top-left of the save area.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_save_blitter(struct game_window_block *instance, blitter *blitter, int x, int y)
{
	os_coord canvas_size;

	if (instance == NULL || blitter == NULL)
		return FALSE;

	// debug_printf"\\OBlitter Save from x=%d, y=%d", x, y);

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	/* Transform the location coordinates. */

	x = game_window_convert_x_coordinate_to_canvas(canvas_size.x, x);
	y = game_window_convert_y_coordinate_to_canvas(canvas_size.y, y);

	return blitter_store_from_canvas((struct blitter_block *) blitter, x, y);
}

/**
 * Update a section of the game window with the contents of a blitter.
 * 
 * If the coordinates are supplied as BLITTER_FROMSAVED, the content
 * will be written back to the location from which it was saved.
 *
 * \param *instance	The instance containing the blitter.
 * \param *blitter	The blitter to save to.
 * \param x		The X coordinate of the top-left of the plot area.
 * \param y		The Y coordinate of the top-left of the plot area.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_windoow_load_blitter(struct game_window_block *instance, blitter *blitter, int x, int y)
{
	os_coord canvas_size;

	if (instance == NULL || blitter == NULL)
		return FALSE;

	// debug_printf"\\OBlitter Load to x=%d, y=%d", x, y);

	if (canvas_get_size(instance->canvas, &canvas_size) == FALSE)
		return FALSE;

	/* Transform the location coordinates. */

	x = (x == BLITTER_FROMSAVED) ? -1 : game_window_convert_x_coordinate_to_canvas(canvas_size.x, x);
	y = (y == BLITTER_FROMSAVED) ? -1 : game_window_convert_y_coordinate_to_canvas(canvas_size.y, y);

	return blitter_paint_to_canvas((struct blitter_block *) blitter, x, y);
}
