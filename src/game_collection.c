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
 * \file: game_collection.c
 *
 * Active game collection implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/errors.h"

/* Application header files */

#include "game_collection.h"

#include "game_window.h"

/* The game collection data structure. */

struct game_collection_block {
	int x_size;				/**< The X size of the window, in game pixels.	*/
	int y_size;				/**< The Y size of the window, in game pixels.	*/

	struct game_window_block *window;	/**< The associated game window instance.	*/

	struct game_collection_block *next;	/**< The next game in the list, or null.	*/
};

/* Global variables. */

/**
 * The list of active game windows.
 */

static struct game_collection_block *game_collection_list = NULL;

/**
 * Initialise a new game and open its window.
 */

void game_collection_create_instance(void)
{
	struct game_collection_block *new;

	/* Allocate the memory for the instance from the static flex heap. */

	new = heap_alloc(sizeof(struct game_collection_block));
	if (new == NULL) {
		error_msgs_report_error("NoMemNewGame");
		return;
	}

	debug_printf("Creating a new game collection instance: block=0x%x", new);

	/* Link the game into the list, and initialise critical data. */

	new->next = game_collection_list;
	game_collection_list = new;

	new->window = NULL;

	new->x_size = 200;
	new->y_size = 200;

	/* Create the game window. */

	new->window = game_window_create_instance(new);
	if (new->window == NULL) {
		game_collection_delete_instance(new);
		return;
	}
}

/**
 * Delete a game instance.
 *
 * \param *instance	The instance to be deleted.
 */

void game_collection_delete_instance(struct game_collection_block *instance)
{
	struct game_collection_block **list;

	if (instance == NULL)
		return;

	debug_printf("Deleting a game instance: block=0x%x", instance);

	/* Delink the instance from the list. */

	list = &game_collection_list;

	while (*list != NULL && *list != instance)
		list = &((*list)->next);

	if (*list != NULL)
		*list = instance->next;

	/* Delete the window. */

	if (instance->window != NULL)
		game_window_delete_instance(instance->window);

	/* Deallocate the instance block. */

	heap_free(instance);
}
