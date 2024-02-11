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
 * \file: sprite_support.h
 *
 * Sprite Support external interface.
 */

#ifndef PUZZLES_SPRITE_SUPPORT
#define PUZZLES_SPRITE_SUPPORT

#include "oslib/osspriteop.h"

/**
 * A sprite support instance, containing a sprite and
 * associated redirection details.
 */

//struct sprite_support_block;

// TODO -- Delete this!
struct sprite_support_block {
	osspriteop_area *sprite_area;
	osspriteop_save_area *save_area;
};

/**
 * Initialise a new sprite support instance.
 *
 * \return			A pointer to the new instance, or NULL.
 */

struct sprite_support_block *sprite_support_create_instance(void);


/**
 * Delete a sprite support instance.
 * 
 * \param *instance		Pointer to the instance to delete.
 */

void sprite_support_delete_instance(struct sprite_support_block *instance);

/**
 * Configure a sprite area and its single sprite, allocating
 * memory and initialising the sprite.
 * 
 * \param **area		Pointer to the area pointer for the
 *				sprite area to act upon; updated on
 *				exit with the correct pointer to the area.
 * \param width
 * \param height
 * \param include_palette
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sprite_support_configure_area(osspriteop_area **area, int width, int height, osbool include_palette);

/**
 * Add a 256 colour sprite to the first sprite in a sprite area.
 * 
 * This is done by hand, using the details provided on page 1-833
 * of the PRM. This should be a compatible with RISC OS 3.1 onwards!
 * 
 * The sprite is assumed to be unused at this point: no attempt is
 * made to shift the bitmap data up to allow space for the palette
 * to be inserted.
 *
 * \param *area			Pointer to the sprite area to act on.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sprite_support_insert_265_palette(osspriteop_area *area);

/**
 * Set the palette for the first sprite in an area to the
 * colours requested by a game.
 * 
 * \param *area			Pointer to the sprite area to act on.
 * \param *colours		An array of colours as supplied by the midend.
 * \param number_of_colours	The number of colours defined in the aray.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sprite_support_set_game_colours(osspriteop_area *area, float *colours, int number_of_colours);

#endif

