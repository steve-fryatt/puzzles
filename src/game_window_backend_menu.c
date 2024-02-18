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
 * \file: game_window_backend_menu.c
 *
 * Implementation of the code which takes the presets menu
 * structure from the backend and uses it to build a RISC OS
 * menu tree.
 */

/* ANSI C header files */

#include "stddef.h"
#include "string.h"

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"

/* Application header files */

#include "game_window_backend_menu.h"

#include "core/puzzles.h"
#include "frontend.h"

/**
 * The maximum size allowed for looking up menu entry texts.
 */

#define GAME_WINDOW_BACKEND_MENU_ENTRY_LEN 64

/**
 * The root of the game window backend menu.
 */

static wimp_menu *game_window_backend_menu_root = NULL;

/**
 * The menu definition supplied by the backend.
 */

static struct preset_menu *game_window_backend_menu_definition = NULL;

/**
 * Has the current menu got a Custom... entry?
 */

static osbool game_window_backend_menu_can_configure = FALSE;

/**
 * The menu title text.
 */

static char *game_window_backend_menu_title = NULL;

/**
 * The custom menu entry text.
 */

static char *game_window_backend_menu_custom = NULL;

/* Static function prototypes. */

static wimp_menu *game_window_backend_menu_build_submenu(struct preset_menu* definition, osbool can_configure, osbool root);
static void game_window_backend_menu_build_entry(wimp_menu_entry *entry, char *title, int *menu_width);
static void game_window_backend_menu_update_submenu_state(wimp_menu *menu, struct preset_menu *definition, int id, osbool custom_active, osbool root);
static struct game_params *game_window_backend_menu_decode_submenu(wimp_selection *selection, struct preset_menu *definition, int index, osbool root);
static void game_window_backend_menu_destroy_submenu(wimp_menu *menu);

/**
 * Initialise the backend menu.
 */

void game_window_backend_menu_initialise(void)
{
	char buffer[GAME_WINDOW_BACKEND_MENU_ENTRY_LEN], *entry;

	/* Load the menu title from the messages file. */

	entry = msgs_lookup("TypeTitle:Type", buffer, GAME_WINDOW_BACKEND_MENU_ENTRY_LEN);
	if (entry == NULL)
		error_msgs_report_fatal("LookupFailedGMenu");
	
	game_window_backend_menu_title = strdup(entry);

	/* Load the Custom... entry from the messages file. */

	entry = msgs_lookup("TypeCustom:Custom...", buffer, GAME_WINDOW_BACKEND_MENU_ENTRY_LEN);
	if (entry == NULL)
		error_msgs_report_fatal("LookupFailedGMenu");
	
	game_window_backend_menu_custom = strdup(entry);

	/* Check that we got both items. */

	if (game_window_backend_menu_title == NULL || game_window_backend_menu_custom == NULL)
		error_msgs_report_fatal("NoMemInitGMenu");
}

/**
 * Build a new backend submenu, using a definition supplied by the
 * backend, and return a pointer to it.
 * 
 * This menu will remain defined until the corresponding _destroy()
 * function is called.
 * 
 * \param *source	The menu definition provided by the backend.
 * \param size		The number of items in the menu definition.
 * \param can_configure	Should the menu have a Custom... entry?
 * \return		A pointer to the menu, or NULL on failure.
 */

wimp_menu *game_window_backend_menu_create(struct preset_menu *source, int size, osbool can_configure)
{
	game_window_backend_menu_can_configure = can_configure;
	game_window_backend_menu_root = game_window_backend_menu_build_submenu(source, can_configure, TRUE);
	game_window_backend_menu_definition = source;

	return game_window_backend_menu_root;
}

/**
 * Build a single menu structure from within a backend submenu,
 * returning a pointer to it or NULL on failure. Any submenus
 * from within the menu structure will be processed recursively.
 * 
 * \param *definition	The menu definition from the backend.
 * \param can_configure	Should the menu have a Custom... entry?
 * \param root		True if this is the first submenu in
 *			the structure; False for subsequent calls.
 * \return		A pointer to the menu, or NULL on failure.
 */

static wimp_menu *game_window_backend_menu_build_submenu(struct preset_menu* definition, osbool can_configure, osbool root)
{
	wimp_menu *menu = NULL;
	int i, entries, len, width;

	/* The Wimp doesn't like zero-length menus... */

	if (definition == NULL || definition->n_entries == 0)
		return NULL;

	/* Add space for the Custom... entry and allocate a block. */

	entries = definition->n_entries;
	if (root == TRUE && can_configure == TRUE)
		entries++;

	menu = malloc(wimp_SIZEOF_MENU(entries));
	if (menu == NULL)
		return NULL;

	/* The menu header. */

	menu->title_fg = wimp_COLOUR_BLACK;
	menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	menu->work_fg = wimp_COLOUR_BLACK;
	menu->work_bg = wimp_COLOUR_WHITE;
	menu->width = 16;
	menu->height = wimp_MENU_ITEM_HEIGHT;
	menu->gap = wimp_MENU_ITEM_GAP;

	/* The menu title. */

	len = strlen(game_window_backend_menu_title);
	menu->title_data.indirected_text.text = game_window_backend_menu_title;

	width = (16 * len) + 16;
	if (width > menu->width)
		menu->width = width;

	/* The menu entries from the preset menu definitions. */

	for (i = 0; i < definition->n_entries; i++) {
		game_window_backend_menu_build_entry(&(menu->entries[i]), definition->entries[i].title, &(menu->width));

		if (definition->entries[i].submenu != NULL)
			menu->entries[i].sub_menu = game_window_backend_menu_build_submenu(definition->entries[i].submenu, can_configure, FALSE);
	}

	/* The Custom... entry if this is the root. */

	if (root == TRUE && can_configure == TRUE) {
		menu->entries[i - 1].menu_flags |= wimp_MENU_SEPARATE;

		game_window_backend_menu_build_entry(&(menu->entries[i++]), game_window_backend_menu_custom, &(menu->width));
	}

	/* Update the first and last entries' flags. */

	menu->entries[0].menu_flags |= wimp_MENU_TITLE_INDIRECTED;
	menu->entries[i - 1].menu_flags |= wimp_MENU_LAST;

	return menu;
}

/**
 * Construct a menu entry.
 * 
 * \param *entry	Pointer to the entry to be constructed.
 * \param *title	Pointer to a title string which can be used
 *			in place.
 * \param *menu_width	Pointer to a variable holding the current width
 *			of the menu in OS units.
 */

static void game_window_backend_menu_build_entry(wimp_menu_entry *entry, char *title, int *menu_width)
{
	int len, width;

	/* The menu entry. */

	entry->menu_flags = 0;
	entry->icon_flags = wimp_ICON_TEXT | wimp_ICON_INDIRECTED |
			wimp_ICON_FILLED |
			(wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
			(wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT);

	entry->sub_menu = NULL;

	/* The menu text. */

	len = strlen(title);

	entry->data.indirected_text.text = title;
	entry->data.indirected_text.size = len + 1;
	entry->data.indirected_text.validation = NULL;

	width = (16 * len) + 16;
	if (width > *menu_width)
		*menu_width = width;
}

/**
 * Update the state of the current backend menu, to reflect
 * the currently-active ID supplied.
 *
 * \param id		The currently-active game ID.
 * \param custom_active	Should any Custom... entry in the menu be
 *			active?
 */

void game_window_backend_menu_update_state(int id, osbool custom_active)
{
	game_window_backend_menu_update_submenu_state(game_window_backend_menu_root,
			game_window_backend_menu_definition, id, custom_active, TRUE);
}

/**
 * Update the state of a backend submenu to reflect the
 * currently-active ID supplied, recursively updating any
 * submenus which are found.
 * 
 * \param *menu		The submenu to be updated.
 * \param *definition	The menu definition from the backend.
 * \param id		The current preset from the midend.
 * \param custom_active	Should any Custom... entry in the menu be
 *			active?
 * \param root		True if this is the first submenu in
 *			the structure; False for subsequent calls.
 */

static void game_window_backend_menu_update_submenu_state(wimp_menu *menu, struct preset_menu *definition, int id, osbool custom_active, osbool root)
{
	int i;

	if (menu == NULL || definition == NULL)
		return;

	/* Process the standard menu entries from the definitions. */

	for (i = 0; i < definition->n_entries; i++) {
		menus_tick_entry(menu, i, (definition->entries[i].id == id) ? TRUE : FALSE);

		if (menu->entries[i].sub_menu != NULL && definition->entries[i].submenu != NULL)
			game_window_backend_menu_update_submenu_state(menu->entries[i].sub_menu, definition->entries[i].submenu, id, custom_active, FALSE);
	}

	/* Process the Custom... entry, if there is one. */

	if (game_window_backend_menu_can_configure == TRUE && root == TRUE) {
		menus_tick_entry(menu, i, (id == -1) ? TRUE : FALSE);
		menus_shade_entry(menu, i, !custom_active);
	}
}

/**
 * Decode a selection from the backend submenu, returning
 * a pointer to a set of game parameters for the midend.
 * 
 * \param *selection	The menu selection details.
 * \param index		The index into the selection at which the
 *			submenu starts.
 * \param *custom	Pointer to a variable to return whether the
 *			custom menue entry was selected.
 * \return		Pointer to a set of midend game parameters.
 */

struct game_params *game_window_backend_menu_decode(wimp_selection *selection, int index, osbool *custom)
{
	struct game_params *params;
	
	/* If the client wants to know about the Custom... entry,
	 * process that now.
	 */

	if (custom != NULL) {
		*custom = FALSE;

		if (game_window_backend_menu_can_configure == TRUE &&
				selection->items[index] == game_window_backend_menu_definition->n_entries) {
			*custom = TRUE;
			return NULL;
		}
	}

	/* Scan the rest of the menu, looking for an ID match. */

	params = game_window_backend_menu_decode_submenu(selection, game_window_backend_menu_definition, index, TRUE);
	if (params != NULL)
		return params;

	return NULL;
}

/**
 * Decode a selection from the backend submenu, returning
 * a pointer to a set of game parameters for the midend.
 * 
 * \param *selection	The menu selection details.
 * \param *definition	The menu definition from the backend.
 * \param index		The index into the selection at which the
 *			submenu starts.
 * \param root		True if this is the first submenu in
 *			the structure; False for subsequent calls.
 * \return		Pointer to a set of midend game parameters.
 */

static struct game_params *game_window_backend_menu_decode_submenu(wimp_selection *selection, struct preset_menu *definition, int index, osbool root)
{
	int selected_item;

	if (selection == NULL || definition == NULL)
		return NULL;

	selected_item = selection->items[index];

	/* If the selection index is outside the bounds of the current
	 * submenu, give up.
	 */

	if ((selected_item < 0) || (selected_item >= definition->n_entries))
		return NULL;

	/* If we're not at the end of the selection, try to step down
	 * the menu tree another level.
	 */

	if (index < 8 && selection->items[index + 1] > -1) {
		if (definition->entries[selected_item].submenu != NULL)
			return game_window_backend_menu_decode_submenu(selection, definition->entries[selected_item].submenu, index + 1, FALSE);
		else
			return NULL;
	}

	/* This must be the selected item. */

	return definition->entries[selected_item].params;
}

/**
 * Destroy any backend menu which is currently defined, and free
 * any RISC OS-specific memory which it was using.
 */

void game_window_backend_menu_destroy(void)
{
	game_window_backend_menu_destroy_submenu(game_window_backend_menu_root);

	game_window_backend_menu_root = NULL;
	game_window_backend_menu_definition = NULL;
	game_window_backend_menu_can_configure = FALSE;
}

/**
 * Destroy a backend submenu, recursively destroying any further
 * submenus linked from it and then freeing the memory used.
 * 
 * \param *menu		Pointer to the submenu block to be freed.
 */

static void game_window_backend_menu_destroy_submenu(wimp_menu *menu)
{
	wimp_menu_entry *entry = NULL;

	if (menu == NULL)
		return;

	entry = menu->entries;

	do {
		if (entry->sub_menu != NULL)
			game_window_backend_menu_destroy_submenu(entry->sub_menu);

		if (!(entry->menu_flags & wimp_MENU_LAST))
			entry++;
	} while (!(entry->menu_flags & wimp_MENU_LAST));

	free(menu);
}
