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
 * \file: blitter.c
 *
 * Blitters implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"

/* Application header files */

#include "blitter.h"

/* Static function prototypes. */


struct blitter_set_block {
	/**
	 * A linked list of the blitters contained in this set.
	 */
	struct blitter_block *blitters;
};


struct blitter_block {
	/**
	 * Pointer to the next blitter in the set.
	 */

	struct blitter_block *next;
};

/* Global variables. */


/**
 * Create a new blitter set, for holding a collection of related
 * blitters.
 * 
 * \return		Pointer to the new set, or NULL.
 */

struct blitter_set_block *blitter_create_set(void)
{
	struct blitter_set_block *new = NULL;

	new = malloc(sizeof(struct blitter_set_block));
	if (new == NULL)
		return NULL;

	new->blitters = NULL;

	debug_printf("\\VCreated new blitter set 0x%x", new);

	return new;
}

/**
 * Delete a blitter set, including all of the blitters contained
 * within it.
 * 
 * \param *set		Pointer to the set to be deleted.
 */

void blitter_delete_set(struct blitter_set_block *set)
{
	if (set == NULL)
		return;

	debug_printf("\\BDeleting blitter set 0x%x", set);

	/* Delete any blitters in the set. */

	while (set->blitters != NULL)
		blitter_delete(set, set->blitters);

	/* Deallocate the set instance block. */

	free(set);
}

/**
 * Create a new blitter within a set.
 * 
 * \param *set		Pointer to the set to hold the blitter.
 * \param width		The width of the blitter, in OS units.
 * \param height	The height of the blitter, in OS units.
 * \return		Pointer to the new blitter, or NULL.
 */

struct blitter_block *blitter_create(struct blitter_set_block *set, int width, int height)
{
	struct blitter_block *new = NULL;

	if (set == NULL)
		return NULL;

	/* Allocate the memory, and link it into the set. */

	new = malloc(sizeof(struct blitter_block));
	if (new == NULL)
		return NULL;

	new->next = set->blitters;
	set->blitters = new;

	debug_printf("\\LCreated new blitter 0x%x in set 0x%x", new, set);

	return new;
}

/**
 * Delete a blitter from within a set.
 * 
 * \param *set		Pointer to the set containing the blitter.
 * \param *blitter	Pointer to the blitter to be deleted.
 * \return		TRUE if successful; else FALSE.
 */

osbool blitter_delete(struct blitter_set_block *set, struct blitter_block *blitter)
{
	struct blitter_block **list;

	if (set == NULL || blitter == NULL)
		return FALSE;

	debug_printf("\\LDeleting blitter 0x%x from set 0x%x", blitter, set);

	/* Delink the blitter from its set. */

	list = &(set->blitters);

	while (*list != NULL && *list != blitter)
		list = &((*list)->next);

	if (*list == NULL) {
		error_msgs_report_error("BadBlitterSet");
		return FALSE;
	}
		
	*list = blitter->next;

	/* Deallocate the blitter block. */

	free(blitter);

	return TRUE;
}
