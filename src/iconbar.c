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
 * \file: iconbar.c
 *
 * IconBar icon implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/url.h"

/* Application header files */

#include "iconbar.h"

#include "frontend.h"
#include "help.h"
#include "index_window.h"
#include "main.h"


/* Iconbar menu */

#define ICONBAR_MENU_INFO 0
#define ICONBAR_MENU_HELP 1
#define ICONBAR_MENU_QUIT 2

/* Program Info Window */

#define ICON_PROGINFO_AUTHOR  4
#define ICON_PROGINFO_PORTER  6
#define ICON_PROGINFO_VERSION 8
#define ICON_PROGINFO_WEBSITE 10

/* Static function prototypes. */

static void	iconbar_click_handler(wimp_pointer *pointer);
static void	iconbar_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool	iconbar_proginfo_web_click(wimp_pointer *pointer);

/* Global variables. */

/**
 * The iconbar menu handle.
 */

static wimp_menu	*iconbar_menu = NULL;

/**
 * The iconbar menu program info window handle.
 */

static wimp_w		iconbar_info_window = NULL;


/**
 * Initialise the iconbar icon and its associated menus and dialogues.
 */

void iconbar_initialise(void)
{
	char*			date = BUILD_DATE;
	wimp_icon_create	icon_bar;

	iconbar_menu = templates_get_menu("IconBarMenu");
	ihelp_add_menu(iconbar_menu, "IconBarMenu");

	iconbar_info_window = templates_create_window("ProgInfo");
	templates_link_menu_dialogue("ProgInfo", iconbar_info_window);
	ihelp_add_window(iconbar_info_window, "ProgInfo", NULL);
	icons_msgs_param_lookup(iconbar_info_window, ICON_PROGINFO_VERSION, "Version",
			BUILD_VERSION, date, NULL, NULL);
	icons_printf(iconbar_info_window, ICON_PROGINFO_AUTHOR, "\xa9 Simon Tatham, 2004-%s", date + 7);
	icons_printf(iconbar_info_window, ICON_PROGINFO_PORTER, "\xa9 Stephen Fryatt, 2024-%s", date + 7);
	event_add_window_icon_click(iconbar_info_window, ICON_PROGINFO_WEBSITE, iconbar_proginfo_web_click);

	icon_bar.w = wimp_ICON_BAR_RIGHT;
	icon_bar.icon.extent.x0 = 0;
	icon_bar.icon.extent.x1 = 68;
	icon_bar.icon.extent.y0 = 0;
	icon_bar.icon.extent.y1 = 69;
	icon_bar.icon.flags = wimp_ICON_SPRITE | (wimp_BUTTON_CLICK << wimp_ICON_BUTTON_TYPE_SHIFT);
	msgs_lookup("TaskSpr", icon_bar.icon.data.sprite, osspriteop_NAME_LIMIT);
	wimp_create_icon(&icon_bar);

	event_add_window_mouse_event(wimp_ICON_BAR, iconbar_click_handler);
	event_add_window_menu(wimp_ICON_BAR, iconbar_menu);
	event_add_window_menu_selection(wimp_ICON_BAR, iconbar_menu_selection);
}


/**
 * Handle mouse clicks on the iconbar icon.
 *
 * \param *pointer		The Wimp mouse click event data.
 */

static void iconbar_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
		index_window_open();
		break;
	}
}


/**
 * Handle selections from the iconbar menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void iconbar_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	wimp_pointer	pointer;

	wimp_get_pointer_info(&pointer);

	switch(selection->items[0]) {
	case ICONBAR_MENU_HELP:
		help_launch(NULL);
		break;

	case ICONBAR_MENU_QUIT:
		main_quit_flag = TRUE;
		break;
	}
}


/**
 * Handle clicks on the Website action button in the program info window.
 *
 * \param *pointer	The Wimp Event message block for the click.
 * \return		TRUE if we handle the click; else FALSE.
 */

static osbool iconbar_proginfo_web_click(wimp_pointer *pointer)
{
	char	temp_buf[256];

	msgs_lookup("SupportURL:http://www.stevefryatt.org.uk/risc-os/games", temp_buf, sizeof(temp_buf));
	url_launch(temp_buf);

	if (pointer->buttons == wimp_CLICK_SELECT)
		wimp_create_menu((wimp_menu *) -1, 0, 0);

	return TRUE;
}

