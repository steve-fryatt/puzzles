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

#include "frontend.h"

/**
 * Initialise the game windows and their associated menus and dialogues.
 */

void game_window_initialise(void);

/**
 * Initialise a new game window instance.
 *
 * \param *fe		The parent game frontend instance.
 * \param *title	The title of the window.
 * \return		Pointer to the new window instance, or NULL.
 */

struct game_window_block *game_window_create_instance(struct frontend *fe, const char *title);

/**
 * Delete a game window instance and the associated window.
 *
 * \param *instance	The instance to be deleted.
 */

void game_window_delete_instance(struct game_window_block *instance);

/**
 * Create and open the game window at the specified location.
 * 
 * \param *instance	The instance to open the window on.
 * \param status_bar	TRUE if the window should have a status bar.
 * \param *pointer	The pointer at which to open the window.
 */

void game_window_open(struct game_window_block *instance, osbool status_bar, wimp_pointer *pointer);

/**
 * Create or update the drawing canvas associated with a window
 * instance.
 * 
 * After an update, the canvas will be cleared and it will be
 * necessary to request that the client redraws any graphics which
 * had previously been present.
 * 
 * \param *instance		The instance to update.
 * \param x			The required horizontal dimension.
 * \param y			The required vertical dimension.
 * \param *colours		An array of colours required by the game.
 * \param number_of_colours	The number of colourd defined in the array.
 * \return			TRUE if successful; else FALSE.
 */

osbool game_window_create_canvas(struct game_window_block *instance, int x, int y, float *colours, int number_of_colours);

/**
 * Update the text in the status bar.
 * 
 * \param *instance		The instance to update.
 * \param *text			The new status bar text.
 * \return			TRUE if successful; otherwise FALSE.
 */

osbool game_window_set_status_text(struct game_window_block *instance, const char *text);

/**
 * Start regular 20ms callbacks to the frontend, which can be passed
 * on to the midend.
 *
 * \param *instance		The instance to update.
 * \return			TRUE if successful; else FALSE.
 */

osbool game_window_start_timer(struct game_window_block *instance);

/**
 * Cancel any regular 20ms callbacks to the frontend which are in progress.
 *
 * \param *instance		The instance to update.
 */

void game_window_stop_timer(struct game_window_block *instance);

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
 * Request a forced redraw of part of the canvas as the next available
 * opportunity.
 * 
 * \param *instance	The instance to plot to.
 * \param x0		The X coordinate of the top left corner of
 *			the area to be redrawn (inclusive).
 * \param y0		The Y coordinate of the top left corner of
 *			the area to be redrawn (inclusive).
 * \param x1		The X coordinate of the bottom right corner
 *			of the area to be redrawn (exclusive).
 * \param y1		The Y coordinate of the bottom right corner
 *			of the area to be redrawn (exclusive).
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_force_redraw(struct game_window_block *instance, int x0, int y0, int x1, int y1);

/**
 * Set the plotting colour in a game window.
 *
 * \param *instance	The instance to plot to.
 * \param colour	The colour, as an index into the game's list.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_set_colour(struct game_window_block *instance, int colour);

/**
 * Set a graphics clipping window, to affect all future
 * operations on the canvas.
 * 
 * \param *instance	The instance to plot to.
 * \param x0		The X coordinate of the top left corner of
 *			the window (inclusive).
 * \param y0		The Y coordinate of the top left corner of
 *			the window (inclusive).
 * \param x1		The X coordinate of the bottom right corner
 *			of the window (exclusive).
 * \param y1		The Y coordinate of the bottom right corner
 *			of the window (exclusive).
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_set_clip(struct game_window_block *instance, int x0, int y0, int x1, int y1);

/**
 * Clear the clipping window set by set_clip()
 * 
 * \param *instance	The instance to plot to.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_clear_clip(struct game_window_block *instance);

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

/**
 * Start a closed polygon path in a game window.
 * 
 * \param *instance	The instance to plot to.
 * \param x		The X coordinate of the start.
 * \param y		The Y coordinate of the start.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_start_path(struct game_window_block *instance, int x, int y);

/**
 * Add a segment to a closed polygon path in a game window.
 * 
 * \param *instance	The instance to plot to.
 * \param x		The X coordinate of the end of the segment.
 * \param y		The Y coordinate of the end of the segment.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_add_segment(struct game_window_block *instance, int x, int y);

/**
 * End a polygon path in a game window, drawing either an outline, filled
 * shape, or both
 * 
 * \param *instance	The instance to plot to.
 * \param closed	TRUE if the path should be closed; else FALSE
 * \param width		The width of the path outline.
 * \param outline	The required outline colour, or -1 for none.
 * \param fill		The required fill colour, or -1 for none.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_end_path(struct game_window_block *instance, osbool closed, int width, int outline, int fill);

/**
 * Write a line of text in a game window.
 * 
 * \param *instance	The instance to write to.
 * \param x		The X coordinate at which to write the text.
 * \param y		The Y coordinate at which to write the text.
 * \param size		The size of the text in pixels.
 * \param align		The alignment of the text around the coordinates.
 * \param colour	The colour of the text.
 * \param monospaced	TRUE to use a monospaced font.
 * \param *text		The text to write.
 * \return		TRUE if successful; else FALSE.
 */

osbool game_window_write_text(struct game_window_block *instance, int x, int y, int size, int align, int colour, osbool monospaced, const char *text);

#endif
