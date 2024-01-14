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

/* Static function prototypes. */

static void	index_window_click_handler(wimp_pointer *pointer);

/* Global variables. */

/**
 * The index window handle.
 */

static wimp_w		index_window_handle = NULL;


/**
 * Initialise the index window and its associated menus and dialogues.
 */

void index_window_initialise(void)
{
	int			i, height;
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

	height = icon_def->icon.extent.y1 - icon_def->icon.extent.y0;

	window_def->extent.y0 = window_def->extent.y1 -
			((gamecount * (height + INDEX_WINDOW_ICON_GUTTER)) + INDEX_WINDOW_ICON_GUTTER);

	window_def->visible.y0 = window_def->visible.y1 -
			((((gamecount > INDEX_WINDOW_INITIAL_MAX_ROWS) ? INDEX_WINDOW_INITIAL_MAX_ROWS : gamecount) *
					(height + INDEX_WINDOW_ICON_GUTTER)) + INDEX_WINDOW_ICON_GUTTER);

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
		icon_def->icon.extent.y0 = -(i + 1) * (height + INDEX_WINDOW_ICON_GUTTER);
		icon_def->icon.extent.y1 = icon_def->icon.extent.y0 + height;

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
 * Handle mouse clicks on the iconbar icon.
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
		frontend_create_instance(pointer->i);

		if (pointer->buttons == wimp_CLICK_ADJUST)
			wimp_close_window(pointer->w);
		break;
	}
}
