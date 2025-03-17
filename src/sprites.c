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

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osspriteop.h"

/* SF-Lib header files. */

/* Application header files */

#include "sprites.h"


/* Global variables. */

/**
 * The sprite area.
 */

static osspriteop_area	*sprites_area = NULL;


/**
 * Initialise the application sprites database.
 *
 * \param *sprites	The sprite area to use for the application.
 */

void sprites_initialise(osspriteop_area *sprites)
{
	sprites_area = sprites;
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
 * Test whether a given sprite exists.
 *
 * \param *name		The name of the sprite to test for.
 * \return		TRUE if the sprite exists; otherwise FALSE.
 */

osbool sprites_test_sprite(char *name)
{
	if (name == NULL)
		return FALSE;

	return xosspriteop_read_sprite_info(osspriteop_USER_AREA, sprites_area,
			(osspriteop_id) name, NULL, NULL, NULL, NULL) == NULL;
}