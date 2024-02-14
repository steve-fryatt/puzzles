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

#include "canvas.h"

/* Static function prototypes. */

/**
 * A collection of blitters belonging to a single Game Window.
 */

struct blitter_set_block {
	/**
	 * A linked list of the blitters contained in this set.
	 */
	struct blitter_block *blitters;
};

/**
 * An individual blitter instance.
 */

struct blitter_block {
	/**
	 * The canvas to hold the blitter contents.
	 */
	struct canvas_block *canvas;

	/**
	 * The position from which the blitter was last captured.
	 */
	os_coord position;

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
 * \param width		The width of the blitter, in pixels.
 * \param height	The height of the blitter, in pixels.
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

	/* Initialise the canvas. */

	new->canvas = canvas_create_instance();
	if (new->canvas == NULL) {
		blitter_delete(set, new);
		return NULL;
	}

	canvas_configure_area(new->canvas, width, height, FALSE);

	/* Zero the coordinates. */

	new->position.x = 0;
	new->position.y = 0;

	debug_printf("\\LCreated new blitter 0x%x in set 0x%x; width=%d, height=%d", new, set, width, height);

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

	if (blitter->canvas != NULL)
		canvas_delete_instance(blitter->canvas);

	free(blitter);

	return TRUE;
}

/**
 * Use a blitter to save an area from the current screen or canvas.
 * 
 * \param *blitter	Pointer to the blitter to be used.
 * \param x		The X coordinate of the area to save, in OS units.
 * \param y		The Y coordinate of the area to save, in OS units.
 * \return		TRUE if successful; else FALSE.
*/

osbool blitter_store_from_canvas(struct blitter_block *blitter, int x, int y)
{
	if (blitter == NULL || blitter->canvas == NULL)
		return FALSE;

	blitter->position.x = x;
	blitter->position.y = y;

	return canvas_get_sprite(blitter->canvas, x, y);
}

/**
 * Paint the contents of a blitter to the current screen or canvas.
 * If a coordinate is -1, the stored coordinate will be used instead.
 * 
 * \param *blitter	Pointer to the blitter to be used.
 * \param x		The X coordinate of the area to write to, in OS units.
 * \param y		The Y coordinate of the area to write to, in OS units.
 * \return		TRUE if successful; else FALSE.
 */

osbool blitter_paint_to_canvas(struct blitter_block *blitter, int x, int y)
{
	if (blitter == NULL || blitter->canvas == NULL)
		return FALSE;

	if (x == -1)
		x = blitter->position.x;
	
	if (y == -1)
		y = blitter->position.y;

	return canvas_put_sprite(blitter->canvas, x, y);
}
