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

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/errors.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "index_window.h"

#include "core/puzzles.h"
#include "frontend.h"

/**
 * The icon guttering.
 */

#define INDEX_WINDOW_ICON_GUTTER 4

#define INDEX_WINDOW_INITIAL_MAX_ROWS 6

#define INDEX_WINDOW_HORIZONTAL_SCROLL 52

/* Static function prototypes. */

static void index_window_click_handler(wimp_pointer *pointer);
static void index_window_scroll_event_handler(wimp_scroll *scroll);

/* Global variables. */

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
 * Initialise the index window and its associated menus and dialogues.
 */

void index_window_initialise(void)
{
	int			i;
	wimp_window 		*window_def;
	wimp_icon_create	*icon_def;
	wimp_i			icon_handle;
	os_error		*error;

	window_def = templates_load_window("Index");
	if (window_def == NULL)
		return;

	/* There should be one icon defined in the window, which is our template. */

	if (window_def->icon_count == 0) {
		error_msgs_param_report_fatal("MissingIcon", "Index", NULL, NULL, NULL);
		return;
	}

	/* Hide the icon, set the extents, and create the window. */

	window_def->icon_count = 0;
	icon_def = (wimp_icon_create *) &(window_def->icon_count);

	index_window_icon_width = icon_def->icon.extent.x1 - icon_def->icon.extent.x0;
	index_window_icon_height = icon_def->icon.extent.y1 - icon_def->icon.extent.y0;

	window_def->extent.y0 = window_def->extent.y1 -
			((gamecount * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER)) + INDEX_WINDOW_ICON_GUTTER);

	window_def->visible.y0 = window_def->visible.y1 -
			((((gamecount > INDEX_WINDOW_INITIAL_MAX_ROWS) ? INDEX_WINDOW_INITIAL_MAX_ROWS : gamecount) *
					(index_window_icon_height + INDEX_WINDOW_ICON_GUTTER)) + INDEX_WINDOW_ICON_GUTTER);

	error = xwimp_create_window(window_def, &index_window_handle);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		index_window_handle = NULL;
		return;
	}

	/* Set up an icon definition.
	 *
	 * We're done with the window definition now, so we will just use the
	 * icon definition in situ and write over the icon count with the
	 * window handle for the icon definition block.
	 */

	icon_def->w = index_window_handle;

	icon_def->icon.extent.x0 = window_def->extent.x0 + INDEX_WINDOW_ICON_GUTTER;
	icon_def->icon.extent.x1 = window_def->extent.x1 - INDEX_WINDOW_ICON_GUTTER;

	for (i = 0; i < gamecount; i++) {
		icon_def->icon.extent.y0 = -(i + 1) * (index_window_icon_height + INDEX_WINDOW_ICON_GUTTER);
		icon_def->icon.extent.y1 = icon_def->icon.extent.y0 + index_window_icon_height;

		/* We're assuming that the names in the game definitions aren't going to move. */

		icon_def->icon.data.indirected_text_and_sprite.text = (char *) gamelist[i]->name;
		icon_def->icon.data.indirected_text_and_sprite.size = strlen(gamelist[i]->name) + 1;

		error = xwimp_create_icon(icon_def, &icon_handle);
		if (error != NULL) {
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
			return;
		}
	}

	free(window_def);

	ihelp_add_window(index_window_handle, "Index", NULL);

	event_add_window_mouse_event(index_window_handle, index_window_click_handler);
	event_add_window_scroll_event(index_window_handle, index_window_scroll_event_handler);
}

/**
 * (Re-)open the Index window on screen. 
 */

void index_window_open(void)
{
	if (index_window_handle != NULL)
		windows_open_centred_on_screen(index_window_handle);
}

/**
 * Handle mouse clicks in the index window.
 *
 * \param *pointer		The Wimp mouse click event data.
 */

static void index_window_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
	case wimp_CLICK_ADJUST:
		frontend_create_instance(pointer->i, pointer);

		if (pointer->buttons == wimp_CLICK_ADJUST)
			wimp_close_window(pointer->w);
		break;
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

	scroll->xscroll += distance;

	/* Add in the Y scroll offset. */

	height = (scroll->visible.y1 - scroll->visible.y0);
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
