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

#include "sflib/debug.h"
//#include "sflib/errors.h"
//#include "sflib/string.h"
//#include "sflib/ihelp.h"
#include "sflib/menus.h"

/* Application header files */

#include "game_window_backend_menu.h"

#include "core/puzzles.h"
#include "frontend.h"
#include "game_draw.h"

/**
 * The root of the game window backend menu.
 */

static wimp_menu *game_window_backend_menu_root = NULL;

/**
 * The menu definition supplied by the backend.
 */

static struct preset_menu *game_window_backend_menu_definition = NULL;

/* Static function presets. */

static wimp_menu *game_window_backend_menu_build_submenu(struct preset_menu* definition, osbool root);
static void game_window_backend_menu_update_submenu_state(wimp_menu *menu, struct preset_menu *definition, int id, osbool root);
static int game_window_backend_menu_decode_submenu(wimp_selection *selection, struct preset_menu *definition, int index, osbool root);
static void game_window_backend_menu_destroy_submenu(wimp_menu *menu);

/**
 * Build a new backend submenu, using a definition supplied by the
 * backend, and return a pointer to it.
 * 
 * This menu will remain defined until the corresponding _destroy()
 * function is called.
 * 
 * \param *source	The menu definition provided by the backend.
 * \param size		The number of items in the menu definition.
 * \return		A pointer to the menu, or NULL on failure.
 */

wimp_menu *game_window_backend_menu_create(struct preset_menu *source, int size)
{
	game_window_backend_menu_root = game_window_backend_menu_build_submenu(source, TRUE);
	game_window_backend_menu_definition = source;

	return game_window_backend_menu_root;
}

/**
 * Build a single menu structure from within a backend submenu,
 * returning a pointer to it or NULL on failure. Any submenus
 * from within the menu structure will be processed recursively.
 * 
 * \param *definition	The menu definition from the backend.
 * \param root		True if this is the first submenu in
 *			the structure; False for subsequent calls.
 * \return		A pointer to the menu, or NULL on failure.
 */

static wimp_menu *game_window_backend_menu_build_submenu(struct preset_menu* definition, osbool root)
{
	wimp_menu *menu = NULL;
	int i = 0;

	menu = malloc(wimp_SIZEOF_MENU(definition->n_entries));
	if (menu == NULL)
		return NULL;

	strncpy(menu->title_data.text, "Type", 12); // TODO -- Set the title properly!!

	menu->title_fg = wimp_COLOUR_BLACK;
	menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	menu->work_fg = wimp_COLOUR_BLACK;
	menu->work_bg = wimp_COLOUR_WHITE;
	menu->width = 16;
	menu->height = wimp_MENU_ITEM_HEIGHT;
	menu->gap = wimp_MENU_ITEM_GAP;

	for (i = 0; i < definition->n_entries; i++) {
		menu->entries[i].menu_flags = ((i + 1) < definition->n_entries) ? 0 : wimp_MENU_LAST;
		menu->entries[i].icon_flags = wimp_ICON_TEXT | wimp_ICON_INDIRECTED |
				wimp_ICON_FILLED |
				(wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT) |
				(wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT);

		menu->entries[i].data.indirected_text.text = definition->entries[i].title;
		menu->entries[i].data.indirected_text.size = strlen(definition->entries[i].title) + 1;
		menu->entries[i].data.indirected_text.validation = NULL;

		if (definition->entries[i].submenu != NULL) {
			menu->entries[i].sub_menu = game_window_backend_menu_build_submenu(definition->entries[i].submenu, FALSE);
		} else {
			menu->entries[i].sub_menu = NULL;
		}
	}

	return menu;
}

/**
 * Update the state of the current backend menu, to reflect
 * the currently-active ID supplied.
 *
 * \param id		The currently-active game ID.
 */

void game_window_backend_menu_update_state(int id)
{
	game_window_backend_menu_update_submenu_state(game_window_backend_menu_root,
			game_window_backend_menu_definition, id, TRUE);
}

/**
 * Update the state of a backend submenu to reflect the
 * currently-active ID supplied, recursively updating any
 * submenus which are found.
 * 
 * \param *menu		The submenu to be updated.
 * \param *definition	The menu definition from the backend.
 * \param id		The current preset from the midend.
 * \param root		True if this is the first submenu in
 *			the structure; False for subsequent calls.
 */

static void game_window_backend_menu_update_submenu_state(wimp_menu *menu, struct preset_menu *definition, int id, osbool root)
{
	int i;

	if (menu == NULL || definition == NULL)
		return;

	for (i = 0; i < definition->n_entries; i++) {
		debug_printf("Compare %d and %d for entry %d", definition->entries[i].id, id, i);
		menus_tick_entry(menu, i, (definition->entries[i].id == id) ? TRUE : FALSE);

		if (menu->entries[i].sub_menu != NULL && definition->entries[i].submenu != NULL)
			game_window_backend_menu_update_submenu_state(menu->entries[i].sub_menu, definition->entries[i].submenu, id, FALSE);
	}
}

/**
 * Decode a selection from the backend submenu, returning
 * a Preset ID value for the midend.
 * 
 * \param *selection	The menu selection details.
 * \param index		The index into the selection at which the
 *			submenu starts.
 * \return		A midend preset Id value.
 */

int game_window_backend_menu_decode(wimp_selection *selection, int index)
{
	return game_window_backend_menu_decode_submenu(selection, game_window_backend_menu_definition, index, TRUE);
}

/**
 * Decode a selection from the backend submenu, returning
 * a Preset ID value for the midend.
 * 
 * \param *selection	The menu selection details.
 * \param *definition	The menu definition from the backend.
 * \param index		The index into the selection at which the
 *			submenu starts.
 * \param root		True if this is the first submenu in
 *			the structure; False for subsequent calls.
 * \return		A midend preset Id value.
 */

static int game_window_backend_menu_decode_submenu(wimp_selection *selection, struct preset_menu *definition, int index, osbool root)
{
	int selected_item;

	if (selection == NULL || definition == NULL)
		return -1;

	selected_item = selection->items[index];

	/* If the selection index is outside the bounds of the current
	 * submenu, give up.
	 */

	if ((selected_item < 0) || (selected_item >= definition->n_entries))
		return -1;

	/* If we're not at the end of the selection, try to step down
	 * the menu tree another level.
	 */

	if (index < 8 && selection->items[index + 1] > -1) {
		if (definition->entries[selected_item].submenu != NULL)
			return game_window_backend_menu_decode_submenu(selection, definition->entries[selected_item].submenu, index + 1, FALSE);
		else
			return -1;
	}

	/* This must be the selected item. */

	return definition->entries[selected_item].id;
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

	debug_printf("Freeing 0x%x from menu", menu);

	free(menu);
}
