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
 * \file: frontend.h
 *
 * Frontend collection external (to the RISC OS code) interface.
 * 
 * This header defines the interface exposed to the rest of the
 * RISC OS application. The interface facing the midend is defined
 * in core/puzzles.h
 */

#ifndef PUZZLES_FRONTEND
#define PUZZLES_FRONTEND

#include "core/puzzles.h"

/**
 * A game collection instance.
 */

struct frontend;

/**
 * Initialise a new game and open its window.
 * 
 * \param game_index	The index into gamelist[] of the required game.
 * \param *pointer	The pointer at which to open the game.
 */

void frontend_create_instance(int game_index, wimp_pointer *pointer);

/**
 * Delete a frontend instance.
 *
 * \param *fe	The instance to be deleted.
 */

void frontend_delete_instance(struct frontend *fe);

/**
 * Process key events from the game window. These are any
 * mouse click or keypress events handled by the midend.
 *
 * \param *fe		The instance to which the event relates.
 * \param x		The X coordinate of the event.
 * \param y		The Y coordinate of the event.
 * \param button	The button details for the event.
 * \return		TRUE if the event was accepted; otherwise FALSE.
 */

osbool frontend_handle_key_event(struct frontend *fe, int x, int y, int button);

/**
 * Process a periodic callback from the game window, passing it on
 * to the midend.
 * 
 * \param *fe			The frontend handle.
 * \param tplus			The time in seconds since the last
 *				callback event.
 */

void frontend_timer_callback(struct frontend *fe, float tplus);

/**
 * Return details that the game window might need in order to open
 * a window menu.
 *
 * \param *fe			The frontend handle.
 * \param **presets		Pointer to variable in which to return
 *				a pointer to the midend presets menu.
 * \param *limit		Pointer to a variable in which to return
 *				the number of entries in the presets menu.
 * \param *current_preset	Pointer to a variable in which to return
 *				the currently-active preset.
 * \param *can_undo		Pointer to variable in which to return
 *				the undo state of the midend.
 * \param *can_redo		Pointer to variable in which to return
 *				the redo state of the midend.
 */

void frontend_get_menu_info(struct frontend *fe, struct preset_menu **presets, int *limit, int * current_preset, osbool *can_undo, osbool *can_redo);

#endif
