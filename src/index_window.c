/* Copyright 2024-2025, Stephen Fryatt
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
 * \file: index_window.c
 *
 * Index window implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/wimpextend.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/errors.h"
#include "sflib/general.h"
#include "sflib/ihelp.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "index_window.h"

#include "core/puzzles.h"
#include "frontend.h"
#include "sprites.h"

/**
 * The margin around the edge of the window, in OS units.
 */

#define LIST_WINDOW_MARGIN 16

/**
 * The icon guttering.
 */

#define INDEX_WINDOW_ICON_GUTTER 16

/**
 * The number of rows to show in a newly-opened window.
 */

#define INDEX_WINDOW_INITIAL_MAX_ROWS 3

/**
 * The number of columns to show in a newly-opened window.
 */

 #define INDEX_WINDOW_INITIAL_MAX_COLUMNS 4

/**
 * The additional padding in OS units to add to the horizontal dimension of a
 * large icon to account for text margins left and right.
 */

#define INDEX_WINDOW_LARGE_ICON_PADDING 16

/**
 * The additional padding in OS units to add to the horizontal dimension of a
 * small icon to account for text margins, left and right, and icon.
 */

#define INDEX_WINDOW_SMALL_ICON_PADDING 50

/**
 * The length of the buffer used for icon redraw.
 *
 * This MUST be at least as long as INDEX_WINDOW_VALIDATION_LENGTH.
 */

#define INDEX_WINDOW_BUFFER_LENGTH 64

/**
 * The length of a validation string containg just a sprite name.
 *
 * This must not be longer than INDEX_WINDOW_BUFFER_LENGTH.
 */

#define INDEX_WINDOW_VALIDATION_LENGTH 14

/**
 * No game matched from the window coordinates.
 */

#define INDEX_WINDOW_NO_GAME ((int) -1)

/**
 * The definition icon handles.
 */

#define INDEX_WINDOW_ICON_LARGE 0
#define INDEX_WINDOW_ICON_SMALL 1

#define INDEX_WINDOW_ICON_COUNT 2	/**< The number of icons. */

/* Static function prototypes. */

static osbool main_message_font_changed(wimp_message *message);
static void index_window_open_handler(wimp_open *open);
static void index_window_click_handler(wimp_pointer *pointer);
static void index_window_redraw_handler(wimp_draw *redraw);
static void index_window_scroll_event_handler(wimp_scroll *scroll);
static void index_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons);
static void index_window_recalculate_icon_dimensions(void);
static osbool index_window_recalculate_rows_and_columns(wimp_open *open);
static int index_window_find_game_from_pointer(wimp_w w, os_coord pos);
static int index_window_read_horizontal_border_width(wimp_w w);

/* Global variables. */

static wimp_window	*index_window_def = NULL;

/**
 * The index window handle.
 */

static wimp_w		index_window_handle = NULL;

/**
 * The width of an icon in the index window.
 */

static int		index_window_icon_width = 0;

/**
 * The height of an icon in the index window.
 */

static int		index_window_icon_height = 0;

/**
 * The current number of rows in the index window layout.
 */

static int		index_window_rows = 10;

/**
 * The current number of columns in the index window layout.
 */

static int		index_window_columns = 4;

/**
 * The active icon in the index window. This MUST match one of the template
 * icons defined in the window template.
 */

static wimp_i		index_window_active_icon = INDEX_WINDOW_ICON_LARGE;

/**
 * The icon widths as defined in the template.
 */

static int		index_window_starting_icon_width[INDEX_WINDOW_ICON_COUNT];

/* Line position calculations.
 *
 * NB: These can be called with lines < 0 to give lines off the top of the window!
 */

#define LINE_BASE(x) (-((x)+1) * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER) - LIST_WINDOW_MARGIN)
#define LINE_Y0(x) (LINE_BASE(x) + INDEX_WINDOW_ICON_GUTTER)
#define LINE_Y1(x) (LINE_BASE(x) + INDEX_WINDOW_ICON_GUTTER + index_window_icon_height)

#define COLUMN_SIDE(x) ((x) * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER) + LIST_WINDOW_MARGIN - INDEX_WINDOW_ICON_GUTTER)
#define COLUMN_X0(x) (COLUMN_SIDE(x) + INDEX_WINDOW_ICON_GUTTER)
#define COLUMN_X1(x) (COLUMN_SIDE(x) + INDEX_WINDOW_ICON_GUTTER + index_window_icon_width)

/* Row calculations: taking a positive offset from the top of the window, return
 * the raw row number and the position within a row.
 */

#define ROW(y) (((-(y)) - LIST_WINDOW_MARGIN) / (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER))
#define ROW_Y_POS(y) (((-(y)) - LIST_WINDOW_MARGIN) % (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER))

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

#define ROW_ABOVE(y) ((y) < ((index_window_icon_height + INDEX_WINDOW_ICON_GUTTER) - (INDEX_WINDOW_ICON_GUTTER + index_window_icon_height)))
#define ROW_BELOW(y) ((y) > ((index_window_icon_height + INDEX_WINDOW_ICON_GUTTER) - INDEX_WINDOW_ICON_GUTTER))
 

/**
 * Initialise the index window and its associated menus and dialogues.
 */

void index_window_initialise(void)
{
	int		columns, rows;
	os_error	*error;

	index_window_def = templates_load_window("Index");
	if (index_window_def == NULL)
		return;

	/* There should be the expected number icons defined in the window, which are our templates. */

	if (index_window_def->icon_count != INDEX_WINDOW_ICON_COUNT) {
		error_msgs_param_report_fatal("MissingIcon", "Index", NULL, NULL, NULL);
		return;
	}

	/* Work out the size of the window. */

	index_window_columns = INDEX_WINDOW_INITIAL_MAX_COLUMNS;

	index_window_rows = (gamecount + 1) / index_window_columns;

	/* Get the icon details and then hide them. */

	for (int i = 0; i < INDEX_WINDOW_ICON_COUNT; i++) {
		index_window_icon_width = index_window_def->icons[i].extent.x1 -
				index_window_def->icons[i].extent.x0;
	}

	index_window_def->icon_count = 0;

	/* Calculate the icon dimensions. */

	index_window_recalculate_icon_dimensions();

	/* Set the default visible size of the window.*/

	index_window_def->visible.x1 = index_window_def->visible.x0 + (2 * LIST_WINDOW_MARGIN) - INDEX_WINDOW_ICON_GUTTER +
			(INDEX_WINDOW_INITIAL_MAX_COLUMNS * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER));

	index_window_def->visible.y0 = index_window_def->visible.y1 - (2 * LIST_WINDOW_MARGIN) + INDEX_WINDOW_ICON_GUTTER +
			(INDEX_WINDOW_INITIAL_MAX_ROWS * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER));

	/* Set the default extent of the window. */

	columns = ((index_window_def->visible.x1 - index_window_def->visible.x0) - (2 * LIST_WINDOW_MARGIN) + INDEX_WINDOW_ICON_GUTTER) /
			(index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);

	rows = (gamecount + columns - 1) / columns;

	debug_printf("Initialising to %d columns, %d rows.", columns, rows);

	index_window_def->extent.x1 = index_window_def->extent.x0 + (2 * LIST_WINDOW_MARGIN) - INDEX_WINDOW_ICON_GUTTER +
			(gamecount * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER));

	index_window_def->extent.y0 = index_window_def->extent.y1 - (2 * LIST_WINDOW_MARGIN) + INDEX_WINDOW_ICON_GUTTER -
			(rows * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER));

	/* Set up the sprite area. */

	index_window_def->sprite_area = sprites_get_area();

	error = xwimp_create_window(index_window_def, &index_window_handle);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		index_window_handle = NULL;
		return;
	}

	ihelp_add_window(index_window_handle, "Index", index_window_decode_help);

	event_add_window_redraw_event(index_window_handle, index_window_redraw_handler);
	event_add_window_open_event(index_window_handle, index_window_open_handler);
	event_add_window_mouse_event(index_window_handle, index_window_click_handler);
	event_add_window_scroll_event(index_window_handle, index_window_scroll_event_handler);

	event_add_message_handler(message_FONT_CHANGED, EVENT_MESSAGE_INCOMING, main_message_font_changed);
}

/**
 * Handle incoming Message_FontChanged.
 */

static osbool main_message_font_changed(wimp_message *message)
{
	wimp_window_state state;
	os_error *error;

	state.w = index_window_handle;
	error = xwimp_get_window_state(&state);
	if (error != NULL)
		error_report_program(error);

	index_window_recalculate_icon_dimensions();
	if ((state.flags & wimp_WINDOW_OPEN) && index_window_recalculate_rows_and_columns((wimp_open *) &state))
		windows_redraw(state.w);

	return TRUE;
}
 
/**
 * (Re-)open the Index window on screen. 
 */

void index_window_open(void)
{
	wimp_window_state state;

	if (index_window_handle == NULL)
		return;

	state.w = index_window_handle;
	wimp_get_window_state(&state);

	if ((state.flags & wimp_WINDOW_OPEN) == 0) {
		debug_printf("Resetting window size...");

		state.visible.x1 = state.visible.x0 + (2 * LIST_WINDOW_MARGIN) - INDEX_WINDOW_ICON_GUTTER +
				(INDEX_WINDOW_INITIAL_MAX_COLUMNS * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER));

		state.visible.y0 = state.visible.y1 - (2 * LIST_WINDOW_MARGIN) + INDEX_WINDOW_ICON_GUTTER -
				(INDEX_WINDOW_INITIAL_MAX_ROWS * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER));
	
		index_window_recalculate_rows_and_columns((wimp_open *) &state);
	}

	windows_open_state_centred_on_screen(&state);
}

/**
 * Handle Open events on the index window, to adjust the extent.
 *
 * \param *open			The Wimp Open data block.
 */

static void index_window_open_handler(wimp_open *open)
{
	wimp_window_state state;

	if (open == NULL)
		return;

	if (index_window_recalculate_rows_and_columns(open))
		windows_redraw(open->w);

	state.w = open->w;
	wimp_get_window_state(&state);

	if (state.flags & wimp_WINDOW_TOGGLED)
		debug_printf("Click on toggle size icon");

	if (state.flags & wimp_WINDOW_FULL_SIZE)
		debug_printf("Window at full size");

	wimp_open_window(open);
}

/**
 * Handle mouse clicks in the index window.
 *
 * \param *pointer		The Wimp mouse click event data.
 */

static void index_window_click_handler(wimp_pointer *pointer)
{
	int game;

	if (pointer == NULL || pointer->w != index_window_handle)
		return;

	game = index_window_find_game_from_pointer(pointer->w, pointer->pos);
	if (game == INDEX_WINDOW_NO_GAME)
		return;

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
	case wimp_CLICK_ADJUST:
		frontend_create_instance(game, pointer);

		if (pointer->buttons == wimp_CLICK_ADJUST)
			wimp_close_window(pointer->w);
		break;
	}
}

/**
 * Callback to handle redraw events on the index window.
 *
 * \param  *redraw		The Wimp redraw event block.
 */

static void index_window_redraw_handler(wimp_draw *redraw)
{
	int			ox, left, right, x, oy, top, bottom, y, i;
	osbool			more;
	wimp_icon		*icon;
	enum sprites_size	target_size = SPRITES_SIZE_LARGE, sprite_size;
	char			buffer[INDEX_WINDOW_BUFFER_LENGTH], validation[INDEX_WINDOW_VALIDATION_LENGTH];


	icon = index_window_def->icons;

	/* Set up the buffers for the icons. */

	icon[index_window_active_icon].data.indirected_text_and_sprite.text = buffer;
	icon[index_window_active_icon].data.indirected_text_and_sprite.size = INDEX_WINDOW_BUFFER_LENGTH;
	icon[index_window_active_icon].data.indirected_text_and_sprite.validation = validation;

	switch (index_window_active_icon) {
	case INDEX_WINDOW_ICON_SMALL:
		target_size = SPRITES_SIZE_SMALL;
		break;
	case INDEX_WINDOW_ICON_LARGE:
	default:
		target_size = SPRITES_SIZE_LARGE;
		break;
	}

	/* Redraw the window. */

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		left = (redraw->clip.x0 - ox) / (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
		if (left < 0)
			left = 0;

		right = ((index_window_icon_width * 1.5) + redraw->clip.x1 - ox) / (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
		if (right > index_window_columns)
			right = index_window_columns;
	
		top = (oy - redraw->clip.y1) / (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		if (top < 0)
			top = 0;

		bottom = ((index_window_icon_height * 1.5) + oy - redraw->clip.y0) / (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		if (bottom > index_window_rows)
			bottom = index_window_rows;

		for (y = top; y < bottom; y++) {
			for (x = left; x < right; x++) {
				i = (y * index_window_columns) + x;
				if (i >= gamecount)
					continue;

				icon[index_window_active_icon].extent.x0 = COLUMN_X0(x);
				icon[index_window_active_icon].extent.x1 = COLUMN_X1(x);
				icon[index_window_active_icon].extent.y0 = LINE_Y0(y);
				icon[index_window_active_icon].extent.y1 = LINE_Y1(y);

				/* Copy the game name. */

				string_copy(buffer, (char *) gamelist[i]->name, INDEX_WINDOW_BUFFER_LENGTH);

				/* Find a suitable sprite. */

				sprite_size = sprites_find_sprite_validation((char *) gamelist[i]->name,
						target_size, validation, INDEX_WINDOW_VALIDATION_LENGTH);

				if (target_size == SPRITES_SIZE_SMALL && sprite_size == SPRITES_SIZE_LARGE)
					icon[index_window_active_icon].flags |= wimp_ICON_HALF_SIZE;
				else
					icon[index_window_active_icon].flags &= ~wimp_ICON_HALF_SIZE;

				/* Plot the icon. */

				wimp_plot_icon(&(icon[index_window_active_icon]));
			}
		}

		more = wimp_get_rectangle(redraw);
	}
}

/**
 * Handle scroll events in the index window.
 *
 * \param *scroll		The scroll event data to be processed.
 */

static void index_window_scroll_event_handler(wimp_scroll *scroll)
{
	int width, height, error, distance;

	/* Add in the X scroll offset. */

	width = scroll->visible.x1 - scroll->visible.x0;
	distance = 0;

	switch (scroll->xmin) {
	case wimp_SCROLL_COLUMN_LEFT:
		distance = -(index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
		break;

	case wimp_SCROLL_COLUMN_RIGHT:
		distance = +(index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
		break;

	case wimp_SCROLL_PAGE_LEFT:
		distance = -width;
		break;

	case wimp_SCROLL_PAGE_RIGHT:
		distance = +width;
		break;

	case wimp_SCROLL_AUTO_LEFT:
	case wimp_SCROLL_AUTO_RIGHT:
		/* We don't support Auto Scroll. */
		break;

	default: /* Extended Scroll */
		if (scroll->xmin > 0)
			distance = (scroll->xmin >> 2) * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
		else if (scroll->xmin < 0)
			distance = -((-scroll->xmin) >> 2) * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
		break;
	}

	/* Align to an icon boundary in the direction of scroll. */

	scroll->xscroll += distance;
	if ((error = ((scroll->xscroll - ((distance > 0) ? 0 : width)) % (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER))))
		scroll->xscroll -= ((distance > 0) ? (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER) : INDEX_WINDOW_ICON_GUTTER) + error;


	/* Add in the Y scroll offset. */

	height = scroll->visible.y1 - scroll->visible.y0;
	distance = 0;

	switch (scroll->ymin) {
	case wimp_SCROLL_LINE_UP:
		distance = +(index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		break;

	case wimp_SCROLL_LINE_DOWN:
		distance = -(index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		break;

	case wimp_SCROLL_PAGE_UP:
		distance = +height;
		break;

	case wimp_SCROLL_PAGE_DOWN:
		distance = -height;
		break;
	
	case wimp_SCROLL_AUTO_UP:
	case wimp_SCROLL_AUTO_DOWN:
		/* We don't support Auto Scroll. */
		break;

	default: /* Extended Scroll */
		if (scroll->ymin > 0)
			distance = +(scroll->ymin >> 2) * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		else if (scroll->ymin < 0)
			distance = -((-scroll->ymin) >> 2) * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		break;
	}

	/* Align to an icon boundary in the direction of scroll. */

	scroll->yscroll += distance;
	if ((error = ((scroll->yscroll - ((distance > 0) ? 0 : height)) % (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER))))
		scroll->yscroll -= ((distance > 0) ? (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER) : INDEX_WINDOW_ICON_GUTTER) + error;

	/* Apply the new scroll offsets. */

	wimp_open_window((wimp_open *) scroll);
}

/**
 * Turn a mouse position over the index window into an interactive
 * help token.
 *
 * \param *buffer		A buffer to take the generated token.
 * \param w			The window under the pointer.
 * \param i			The icon under the pointer.
 * \param pos			The current mouse position.
 * \param buttons		The current mouse button state.
 */

static void index_window_decode_help(char *buffer, wimp_w w, wimp_i i, os_coord pos, wimp_mouse_state buttons)
{
	int game = INDEX_WINDOW_NO_GAME;

	*buffer = '\0';

	game = index_window_find_game_from_pointer(w, pos);
	if (game == INDEX_WINDOW_NO_GAME)
		return;

	string_printf(buffer, IHELP_INAME_LEN, "%s", gamelist[game]->htmlhelp_topic);
}
 
/**
 * Recalculate the icon dimensions for the index window, following a layout or
 * desktop font change.
 *
 * On exit, index_window_icon_width and index_window_icon_height will reflect
 * the required dimensions for the icons, in OS units.
 */

static void index_window_recalculate_icon_dimensions(void)
{
	int width, max_width = 0;
	wimp_icon *icon_def;
	os_error *error;

	icon_def = &(index_window_def->icons[index_window_active_icon]);

	max_width = index_window_starting_icon_width[index_window_active_icon];

	for (int i = 0; i < gamecount; i++) {
		error = xwimptextop_string_width(gamelist[i]->name, 0, &width);
		if (error != NULL)
			error_report_program(error);

		if (width > max_width)
			max_width = width;
	}

	switch (index_window_active_icon) {
	case INDEX_WINDOW_ICON_LARGE:
		max_width += INDEX_WINDOW_LARGE_ICON_PADDING;
		break;
	case INDEX_WINDOW_ICON_SMALL:
		max_width += INDEX_WINDOW_SMALL_ICON_PADDING;
		break; 
	}

	index_window_icon_width = max_width;
	index_window_icon_height = icon_def->extent.y1 - icon_def->extent.y0;
}

/**
 * Recalculate the rows and colums of the index window, based on a new
 * Open Window request.
 *
 * On exit, index_window_rows and index_window_columns will have been updated
 * to reflect the new requirements.
 *
 * \param *open		The Wimp_OpenWindow block on which to work.
 * \return		TRUE if the rows and columns changed; FALSE if the
 *			layout in the window remained the same.
 */

static osbool index_window_recalculate_rows_and_columns(wimp_open *open)
{
	int columns, rows, max_columns, screen_width, visible_height, new_height;
	os_box extent;


	if (open == NULL || open->w == NULL)
		return FALSE;

	/* How many columns can we actually fit on the screen? */

	screen_width = general_mode_width() - index_window_read_horizontal_border_width(open->w);

	max_columns = (screen_width - (2 * LIST_WINDOW_MARGIN) + INDEX_WINDOW_ICON_GUTTER) /
			(index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);

	debug_printf("Max columns = %d", max_columns);

	/* How many rows and columns are we being asked to fit? */

	columns = ((open->visible.x1 - open->visible.x0) - (2 * LIST_WINDOW_MARGIN) + INDEX_WINDOW_ICON_GUTTER) /
			(index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);

	rows = (gamecount + columns - 1) / columns;

	debug_printf("New window size is: columns=%d, rows=%d", columns, rows);

	/* If the request to open the window is wider than the screen, resize.  */

	if (columns > max_columns) {
		columns = max_columns;
		rows = (gamecount + columns - 1) / columns;

		debug_printf("Too big! Resize to: columns=%d, rows=%d", columns, rows);
	}

	/* If the rows and columns haven't changed, we're done. */

	if (columns == index_window_columns && rows == index_window_rows)
		return FALSE;

	/* Update the row and column counts. */

	index_window_rows = rows;
	index_window_columns = columns;
	
	debug_printf("We're changing the window layout...");

	/* Work out and set the new extent of the window. */

	new_height = (rows * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER)) -
			INDEX_WINDOW_ICON_GUTTER + (2 * LIST_WINDOW_MARGIN);

	visible_height = open->yscroll + (open->visible.y0 - open->visible.y1);

	if (new_height > visible_height) {
		int new_scroll = new_height - (open->visible.y0 - open->visible.y1);

		if (new_scroll > 0) {
			open->visible.y0 += new_scroll;
			open->yscroll = 0;
		} else {
			open->yscroll = new_scroll;
		}

		wimp_open_window(open);
	}

	extent.x0 = 0;
	extent.x1 = extent.x0 + (2 * LIST_WINDOW_MARGIN) - INDEX_WINDOW_ICON_GUTTER +
			(max_columns * (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER));

	extent.y0 = -new_height;
	extent.y1 = 0;

	wimp_set_extent(open->w, &extent);

	return TRUE;
}

/**
 * Given a window handle and a screen pointer position, decode the details into
 * a game icon number from the index window.
 * 
 * \param w		The handle of the index window.
 * \param pos		The screen coordinates.
 * \return		The index of the game into the gamelist array, or
 *			INDEX_WINDOW_NO_GAME on failure.
 */

static int index_window_find_game_from_pointer(wimp_w w, os_coord pos) {
	wimp_window_state window;
	int xpos, ypos, row, column, icon;

	/* Calculate the work area coordinates. */

	window.w = w;
	wimp_get_window_state(&window);

	xpos = (pos.x - window.visible.x0) + window.xscroll;
	ypos = (pos.y - window.visible.y1) + window.yscroll;

	/* Calculate the row and column. */

	row = (-ypos - LIST_WINDOW_MARGIN) / (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
	column = (xpos - LIST_WINDOW_MARGIN) / (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);

	/* Calculate the position within the icon cell. */

	xpos = (xpos - LIST_WINDOW_MARGIN) % (index_window_icon_width + INDEX_WINDOW_ICON_GUTTER);
	ypos = (-ypos - LIST_WINDOW_MARGIN) % (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);

	/* If the row or column are out of range. */

	if (row < 0 || row >= index_window_rows)
		return INDEX_WINDOW_NO_GAME;

	if (column < 0 || column >= index_window_columns)
		return INDEX_WINDOW_NO_GAME;

	/* If the click was outside the icon bounds. */

	if (xpos < 0 || xpos > index_window_icon_width)
		return INDEX_WINDOW_NO_GAME;
	
	if (ypos < 0 || ypos >= index_window_icon_height)
		return INDEX_WINDOW_NO_GAME;

	icon = (row * index_window_columns) + column;
	if (icon < 0 || icon >= gamecount)
		return INDEX_WINDOW_NO_GAME;
	
	return icon;
}

/**
 * Read the size of the vertical scroll bar and window borders for a given
 * window.
 * 
 * \param w		The window handle of interest.
 * \return		The combined size of the borders, in OS units.
 */

static int index_window_read_horizontal_border_width(wimp_w w)
{
	wimpextend_furniture_sizes sizes;
	os_error *error;

	sizes.w = w;

	/* Zero the sizes we're interested in, so that if the values on exit are
	 * still zero we can insert a vaguely sensible default fallback value.
	 */

	sizes.border_widths.x0 = 0;
	sizes.border_widths.x1 = 0;

	error = xwimpextend_get_furniture_sizes(&sizes);
	if (error != NULL)
		error_report_program(error);

	/* Default to 100, which is probably safe-ish. Standard OS 5 themes
	 * have this at 44 in EX1 EY1, and probably 88 in EX0 EY0.
	 */

	if (sizes.border_widths.x0 == 0 && sizes.border_widths.x1 == 0)
		return 100;

	return sizes.border_widths.x0 + sizes.border_widths.x1;	
}
