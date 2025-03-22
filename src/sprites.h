/* Copyright 2025, Stephen Fryatt
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
 * \file: iconbar.h
 *
 * Application Sprites Database external interface.
 */

#ifndef PUZZLES_SPRITES
#define PUZZLES_SPRITES

#include <stddef.h>
#include "oslib/osspriteop.h"

/**
 * A set of the available sprite sizes.
 */

enum sprites_size {
	SPRITES_SIZE_NONE,	/**< No sprite -- possibly an error?	*/
	SPRITES_SIZE_LARGE,	/**< A large sprite.			*/
	SPRITES_SIZE_SMALL	/**< A small sprite.			*/
};

/**
 * Initialise the application sprites database.
 *
 * \param *sprites	The sprite area to use for the application.
 */

void sprites_initialise(osspriteop_area *sprites);

/**
 * Return the sprite area pointer.
 * 
 * \return		Pointer to the sprite area, or NULL.
 */

osspriteop_area *sprites_get_area(void);

/**
 * Find a suitable sprite for a given game name. This will attempt to find a
 * suitable name in the application sprite area, then fall back to the base
 * Puzzles application sprite.
 *
 * If SPRITES_SIZE_NONE is returned, the buffer state is undefined.
 *
 * \param *name		The name of the sprite to test for.
 * \param requirement	The requirement for a large or small sprite.
 * \param *buffer	Pointer to a buffer to hold the sprite name, as a
 *			validation string.
 * \param length	The length of the supplied buffer.
 * \return		The type of sprite found.
 */

enum sprites_size sprites_find_sprite_validation(char *name, enum sprites_size requirement, char *buffer, size_t length);

#endif

