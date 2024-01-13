/* Copyright 2024, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: game_draw.h
 *
 * Draw game objects to screen or paper interface.
 */

#ifndef PUZZLES_GAME_DRAW
#define PUZZLES_GAME_DRAW

#include "oslib/os.h"

/**
 * Draw a rectangle on screen.
 *
 * \param *outline		The rectangle outline, in absolute OS Units.
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_box(os_box *outline, int width);


/**
 * Draw a line on screen.
 *
 * \param x0			The start X coordinate, in OS Units.
 * \param y0			The start Y coordinate, in OS Units.
 * \param x1			The end X coordinate, in OS Units.
 * \param y1			The end Y coordinate, in OS Units.
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_line(int x0, int y0, int x1, int y1, int width);

/**
 * Start a new path.
 */

void game_draw_start_path(void);

/**
 * Add a move to the current Draw Path.
 *
 * \param x			The X coordinate to move to, in OS Units.
 * \param y			The Y coordinate to move to, in OS Units.
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_add_move(int x, int y);

/**
 * Add a line to the current Draw Path.
 *
 * \param x			The X coordinate to draw to, in OS Units.
 * \param y			The Y coordinate to draw to, in OS Units.
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_add_line(int x, int y);

/**
 * Close the current subpath in the Draw Path.
 *
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_close_subpath(void);

/**
 * End the current Draw Path.
 *
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_end_path(void);

/**
 * Plot the path in the buffer.
 *
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_plot_path(int width);

/**
 * Plot the path in the buffer as a filled shape.
 *
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_fill_path(int width);

#endif
