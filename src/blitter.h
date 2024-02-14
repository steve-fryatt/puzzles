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
 * \file: blitter.h
 *
 * Blitters external interface.
 */

#ifndef PUZZLES_BLITTER
#define PUZZLES_BLITTER

#include "oslib/types.h"

/**
 * A blitter set instance.
 */

struct blitter_set_block;

/**
 * A blitter instance.
 */

struct blitter_block;

/**
 * Create a new blitter set, for holding a collection of related
 * blitters.
 * 
 * \return		Pointer to the new set, or NULL.
 */

struct blitter_set_block *blitter_create_set(void);

/**
 * Delete a blitter set, including all of the blitters contained
 * within it.
 * 
 * \param *set		Pointer to the set to be deleted.
 */

void blitter_delete_set(struct blitter_set_block *set);

/**
 * Create a new blitter within a set.
 * 
 * \param *set		Pointer to the set to hold the blitter.
 * \param width		The width of the blitter, in pixels.
 * \param height	The height of the blitter, in pixels.
 * \return		Pointer to the new blitter, or NULL.
 */

struct blitter_block *blitter_create(struct blitter_set_block *set, int width, int height);

/**
 * Delete a blitter from within a set.
 * 
 * \param *set		Pointer to the set containing the blitter.
 * \param *blitter	Pointer to the blitter to be deleted.
 * \return		TRUE if successful; else FALSE.
 */

osbool blitter_delete(struct blitter_set_block *set, struct blitter_block *blitter);

/**
 * Use a blitter to save an area from the current screen or canvas.
 * 
 * \param *blitter	Pointer to the blitter to be used.
 * \param x		The X coordinate of the area to save, in OS units.
 * \param y		The Y coordinate of the area to save, in OS units.
 * \return		TRUE if successful; else FALSE.
*/

osbool blitter_store_from_canvas(struct blitter_block *blitter, int x, int y);

/**
 * Paint the contents of a blitter to the current screen or canvas.
 * If a coordinate is -1, the stored coordinate will be used instead.
 * 
 * \param *blitter	Pointer to the blitter to be used.
 * \param x		The X coordinate of the area to write to, in OS units.
 * \param y		The Y coordinate of the area to write to, in OS units.
 * \return		TRUE if successful; else FALSE.
 */

osbool blitter_paint_to_canvas(struct blitter_block *blitter, int x, int y);

#endif

