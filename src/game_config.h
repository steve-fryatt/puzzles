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
 * \file: game_config.h
 *
 * External interface to the code which creates and manages
 * the configuration dialogues presented on behalf of the
 * midend.
 */

#ifndef PUZZLES_GAME_CONFIG
#define PUZZLES_GAME_CONFIG

#include "core/puzzles.h"

/**
 * The possible outcomes of a Game Config operation.
 */

enum game_config_outcome {
	GAME_CONFIG_CANCEL = 0x00,
	GAME_CONFIG_SET = 0x01,
	GAME_CONFIG_SAVE = 0x02,
	GAME_CONFIG_HOLD_OPEN = 0x10
};

/**
 * A Game Config instance.
 */

struct game_config_block;

/**
 * Initialise the game config module.
 */

void game_config_initialise(void);

/**
 * Create a new Game Config instance and initialise its window.
 *
 * \param type		The type of config data being displayed.
 * \param *config	Pointer to the config data to be displayed.
 * \param *title	Pointer to a title for the config window.
 * \param *pointer	The Wimp pointer location at which to open the window.
 * \param *callback	The function to be called on user completion.
 * \param *data		The client data to be passed to the callback.
 * \return		Pointer to the new instance, or NULL.
 */

struct game_config_block *game_config_create_instance(int type, config_item *config, char *title,
		wimp_pointer *pointer, osbool (*callback)(int, config_item *, enum game_config_outcome, void *), void *data);

/**
 * Close a Game Config window and delete its instance.
 *
 * \param *instance	The instance to be deleted.
 */

void game_config_delete_instance(struct game_config_block *instance);

#endif
