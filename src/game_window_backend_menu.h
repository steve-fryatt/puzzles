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
 * \file: game_window_backend_menu.h
 *
 * External interface to the code which takes the presets menu
 * structure from the backend and uses it to build a RISC OS
 * menu tree.
 */

#ifndef PUZZLES_GAME_WINDOW_BACKEND_MENU
#define PUZZLES_GAME_WINDOW_BACKEND_MENU

#include "core/puzzles.h"
#include "oslib/wimp.h"

/**
 * Build a new backend submenu, using a definition supplied by the
 * backend, and return a pointer to it.
 * 
 * This menu will remain defined until the corresponding _destroy()
 * function is called.
 * 
 * \param *source	The menu definition provided by the backend.
 * \param size		The number of items in the menu definition.
 * \return		A pointer to the menu, or NULL on failure.
 */

wimp_menu *game_window_backend_menu_create(struct preset_menu *source, int size);

/**
 * Update the state of the current backend menu, to reflect
 * the currently-active ID supplied.
 *
 * \param id		The currently-active game ID.
 */

void game_window_backend_menu_update_state(int id);

/**
 * Destroy any backend menu which is currently defined, and free
 * any RISC OS-specific memory which it was using.
 */

void game_window_backend_menu_destroy(void);

#endif
