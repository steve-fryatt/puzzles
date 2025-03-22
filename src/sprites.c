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
 * \file: sprites.c
 *
 * Application Sprites database implementation.
 */

/* ANSI C header files */

#include <stddef.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osspriteop.h"

/* SF-Lib header files. */

#include "sflib/msgs.h"
#include "sflib/string.h"

/* Application header files */

#include "sprites.h"

/* Constants */

/**
 * The length of a sprite name buffer (including terminator).
 */

#define SPRITES_NAME_BUFFER_LENGTH 13

/* Global variables. */

/**
 * The sprite area.
 */

static osspriteop_area	*sprites_area = NULL;

/**
 * The name of the base task sprite.
 */

static char sprites_task_name[SPRITES_NAME_BUFFER_LENGTH];


/**
 * Initialise the application sprites database.
 *
 * \param *sprites	The sprite area to use for the application.
 */

void sprites_initialise(osspriteop_area *sprites)
{
	sprites_area = sprites;
	msgs_lookup("TaskSpr", sprites_task_name, SPRITES_NAME_BUFFER_LENGTH);
}

/**
 * Return the sprite area pointer.
 * 
 * \return		Pointer to the sprite area, or NULL.
 */

osspriteop_area *sprites_get_area(void)
{
	return sprites_area;
}


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

enum sprites_size sprites_find_sprite_validation(char *name, enum sprites_size requirement, char *buffer, size_t length)
{
	char sprite_name[SPRITES_NAME_BUFFER_LENGTH];
	enum sprites_size found = SPRITES_SIZE_NONE;

	/* The situations where we really can't do anything.
	 * For other scenarios, we'll try to return something!
	 */

	if (name == NULL || buffer == NULL || length == 0)
		return SPRITES_SIZE_NONE;

	/* Make sure that our name is safely terminated, whatever happens. */

	*sprite_name = '\0';

	/* Try for a small game sprite if that's an option. */

	if (found == SPRITES_SIZE_NONE && requirement == SPRITES_SIZE_SMALL) {
		string_printf(sprite_name, SPRITES_NAME_BUFFER_LENGTH, "sm%s", name);

		if (xosspriteop_read_sprite_info(osspriteop_USER_AREA, sprites_area,
				(osspriteop_id) sprite_name, NULL, NULL, NULL, NULL) == NULL)
			found = SPRITES_SIZE_SMALL;
	}

	/* Now try for a large game sprite. */

	if (found == SPRITES_SIZE_NONE) {
		string_printf(sprite_name, SPRITES_NAME_BUFFER_LENGTH, "%s", name);

		if (xosspriteop_read_sprite_info(osspriteop_USER_AREA, sprites_area,
				(osspriteop_id) sprite_name, NULL, NULL, NULL, NULL) == NULL)
			found = SPRITES_SIZE_LARGE;
	}

	/* If the game matches have failed, settle for the task sprite. */

	if (found == SPRITES_SIZE_NONE) {
		switch (requirement) {
		case SPRITES_SIZE_SMALL:
			string_printf(sprite_name, SPRITES_NAME_BUFFER_LENGTH, "sm%s", sprites_task_name);
			found = SPRITES_SIZE_SMALL;
			break;
		case SPRITES_SIZE_LARGE:
		default:
			string_printf(sprite_name, SPRITES_NAME_BUFFER_LENGTH, "%s", sprites_task_name);
			found = SPRITES_SIZE_LARGE;
			break;
		}
	}

	/* Update the client's buffer and return. */

	string_printf(buffer, length, "S%s", sprite_name);

	return found;
}