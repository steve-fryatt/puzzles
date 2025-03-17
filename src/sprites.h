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
 * \file: iconbar.h
 *
 * Application Sprites Database external interface.
 */

#ifndef PUZZLES_SPRITES
#define PUZZLES_SPRITES

#include "oslib/osspriteop.h"

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
 * Test whether a given sprite exists.
 *
 * \param *name		The name of the sprite to test for.
 * \return		TRUE if the sprite exists; otherwise FALSE.
 */

osbool sprites_test_sprite(char *name);

#endif

