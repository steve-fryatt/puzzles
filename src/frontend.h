/* Copyright 2024-2025, Stephen Fryatt
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
 * Return codes from GUI events.
 */

enum frontend_event_outcome {
	FRONTEND_EVENT_UNKNOWN,
	FRONTEND_EVENT_ACCEPTED,
	FRONTEND_EVENT_REJECTED,
	FRONTEND_EVENT_EXIT
};

/**
 * Actions which can be carried out by the frontend.
 */

enum frontend_action {
	FRONTEND_ACTION_NONE,
	FRONTEND_ACTION_SIMPLE_NEW,
	FRONTEND_ACTION_SOLVE,
	FRONTEND_ACTION_RESTART,
	FRONTEND_ACTION_HELP
};

/**
 * A game collection instance.
 */

struct frontend;

/**
 * Initialise the front-end.
 */

void frontend_initialise(void);

/**
 * Load a game file into a new game instance, and open its window.
 *
 * \param *filename	The filename of the game to be loaded.
 */
void frontend_load_game_file(char *filename);

/**
 * Initialise a new game and open its window.
 *
 * \param game_index	The index into gamelist[] of the required game.
 * \param *pointer	The pointer at which to open the game.
 * \param *file		A file from which to load the game state, or NULL
 *			to create a new game from scratch.
 */

void frontend_create_instance(int game_index, wimp_pointer *pointer, FILE *file);

/**
 * Delete a frontend instance.
 *
 * \param *fe	The instance to be deleted.
 */

void frontend_delete_instance(struct frontend *fe);

/**
 * Perform an action through the frontend.
 *
 * \param *fe		The instance to which the action relates.
 * \param action	The action to carry out.
 * \return		The outcome of the action.
 */

enum frontend_event_outcome frontend_perform_action(struct frontend *fe, enum frontend_action action);

/**
 * Start a new game from the supplied parameters.
 *
 * \param *fe		The instance to which the action relates.
 * \param *params	The parameters to use for the new game.
 */

void frontend_start_new_game_from_parameters(struct frontend *fe, struct game_params *params);

/**
 * Process key events from the game window. These are any
 * mouse click or keypress events handled by the midend.
 *
 * \param *fe		The instance to which the event relates.
 * \param x		The X coordinate of the event.
 * \param y		The Y coordinate of the event.
 * \param button	The button details for the event.
 * \return		The outcome of the event.
 */

enum frontend_event_outcome frontend_handle_key_event(struct frontend *fe, int x, int y, int button);

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
 * \param *can_configure	Pointer to variable in which to return
 *				the configure state of the midend.
 * \param *can_undo		Pointer to variable in which to return
 *				the undo state of the midend.
 * \param *can_redo		Pointer to variable in which to return
 *				the redo state of the midend.
 * \param *can_solve		Pointer to variable in which to return
 *				the solve state of the midend.
 */

void frontend_get_menu_info(struct frontend *fe, struct preset_menu **presets, int *limit,
		int *current_preset, osbool *can_configure, osbool *can_undo, osbool *can_redo, osbool *can_solve);

/**
 * Return details of a configuration set from the midend.
 *
 * \param *fe			The frontend handle.
 * \param type			The configuration type to be returned.
 * \param **config_data		Pointer to a variable in which to return
 *				a pointer to the configuration data.
 * \param **window_title	Pointer to a variable in which to return
 *				a pointer to the proposed window title.
 */

void frontend_get_config_info(struct frontend *fe, int type, config_item **config_data, char **window_title);

/**
 * Update details of a configuration set to the midend.
 *
 * \param *fe			The frontend handle.
 * \param type			The configuration type being supplied.
 * \param *config_data		Pointer to the updated configuration data.
 * \param save			Should the new config be saved, if applicable?
 * \return			TRUE if successful; FALSE on failure.
 */

osbool frontend_set_config_info(struct frontend *fe, int type, config_item *config_data, osbool save);

/**
 * Save a game to disc as a Puzzle file.
 *
 * \param *fe			The frontend handle.
 * \param *filename		The filename of the target file.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool frontend_save_game_file(struct frontend *fe, char *filename);

#endif
