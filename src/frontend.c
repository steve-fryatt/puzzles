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
 * \file: frontend.c
 *
 * Frontend collection implementation.
 */

/* ANSI C header files */

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"

/* Application header files */

#include "frontend.h"

#include "core/puzzles.h"

#include "game_window.h"

/* The game collection data structure. */

struct frontend {
	int x_size;				/**< The X size of the window, in game pixels.	*/
	int y_size;				/**< The Y size of the window, in game pixels.	*/

	struct game_window_block *window;	/**< The associated game window instance.	*/

	struct frontend *next;	/**< The next game in the list, or null.	*/
};

/* Global variables. */

/**
 * The list of active game windows.
 */

static struct frontend *frontend_list = NULL;

/**
 * Initialise a new game and open its window.
 */

void frontend_create_instance(void)
{
	struct frontend *new;

	/* Allocate the memory for the instance from the heap. */

	new = malloc(sizeof(struct frontend));
	if (new == NULL) {
		error_msgs_report_error("NoMemNewGame");
		return;
	}

	debug_printf("Creating a new game collection instance: block=0x%x", new);

	/* Link the game into the list, and initialise critical data. */

	new->next = frontend_list;
	frontend_list = new;

	new->window = NULL;

	new->x_size = 200;
	new->y_size = 200;

	/* Create the game window. */

	new->window = game_window_create_instance(new);
	if (new->window == NULL) {
		frontend_delete_instance(new);
		return;
	}
}

/**
 * Delete a frontend instance.
 *
 * \param *fe	The instance to be deleted.
 */

void frontend_delete_instance(struct frontend *fe)
{
	struct frontend **list;

	if (fe == NULL)
		return;

	debug_printf("Deleting a game instance: block=0x%x", fe);

	/* Delink the instance from the list. */

	list = &frontend_list;

	while (*list != NULL && *list != fe)
		list = &((*list)->next);

	if (*list != NULL)
		*list = fe->next;

	/* Delete the window. */

	if (fe->window != NULL)
		game_window_delete_instance(fe->window);

	/* Deallocate the instance block. */

	free(fe);
}


void get_random_seed(void **randseed, int *randseedsize)
{
}

void activate_timer(frontend *fe)
{

}

void deactivate_timer(frontend *fe)
{

}

void fatal(const char *fmt, ...)
{

}

void frontend_default_colour(frontend *fe, float *output)
{

}
