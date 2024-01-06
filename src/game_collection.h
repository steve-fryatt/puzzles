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
 * \file: game_collection.h
 *
 * Active game collection external interface.
 */

#ifndef PUZZLES_GAME_COLLECTION
#define PUZZLES_GAME_COLLECTION

/**
 * A game collection instance.
 */

struct game_collection_block;

/**
 * Initialise a new game and open its window.
 */

void game_collection_create_instance(void);

/**
 * Delete a game instance.
 *
 * \param *instance	The instance to be deleted.
 */

void game_collection_delete_instance(struct game_collection_block *instance);

#endif
