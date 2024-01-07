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

/**
 * Start a drawing operation on the game window canvas, redirecting
 * VDU output to the canvas sprite.
 * 
 * \param *instance	The instance to start drawing in.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_start_draw(struct game_window_block *instance);

/**
 * End a drawing operation on the game window canvas, restoring
 * VDU output back to the previous context.
 * 
 * \param *instance	The instance to finish drawing in.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_end_draw(struct game_window_block *instance);
/**
 * Perform an OS_Plot operation in a game window.
 *
 * A redraw operation must be in progress when this call is used.
 *
 * \param *instance	The instance to plot to.
 * \param plot_code	The OS_Plot operation code.
 * \param x		The X coordinate.
 * \param y		The Y coordinate.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_plot(struct game_window_block *instance, os_plot_code plot_code, int x, int y);

#endif
