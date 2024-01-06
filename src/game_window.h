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
 * \file: game_window.h
 *
 * Game window external interface.
 */

#ifndef PUZZLES_GAME_WINDOW
#define PUZZLES_GAME_WINDOW

#include "game_collection.h"

/**
 * Initialise the game windows and their associated menus and dialogues.
 */

void game_window_initialise(void);

/**
 * Initialise and open a new game window.
 *
 * \param *parent	The parent game collection instance.
 * \return		Pointer to the new window instance, or NULL.
 */

struct game_window_block *game_window_create_instance(struct game_collection_block *parent);

/**
 * Delete a game window instance and the associated window.
 *
 * \param *instance	The instance to be deleted.
 */

void game_window_delete_instance(struct game_window_block *instance);

#endif
