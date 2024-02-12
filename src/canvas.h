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
 * \file: canvas.h
 *
 * Drawing Canvas external interface.
 */

#ifndef PUZZLES_CANVAS
#define PUZZLES_CANVAS

#include "oslib/osspriteop.h"

/**
 * A sprite support instance, containing a sprite and
 * associated redirection details.
 */

struct canvas_block;

/**
 * Initialise a new sprite support instance.
 *
 * \return			A pointer to the new instance, or NULL.
 */

struct canvas_block *canvas_create_instance(void);


/**
 * Delete a sprite support instance.
 * 
 * \param *instance		Pointer to the instance to delete.
 */

void canvas_delete_instance(struct canvas_block *instance);

/**
 * Configure a canvas to a given dimension, and set up its sprite
 * 
 * \param *instance		The canvas instance to be updated.
 * \param width			The required canvas width.
 * \param height		The required canvas height.
 * \param include_palette	TRUE if a palette should be included in the sprite.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool canvas_configure_area(struct canvas_block *instance, int width, int height, osbool include_palette);

/**
 * Configure the save area for a canvas to suit the
 * current sprite.
 * 
 * \param *instance		The canvas instance to be updated.
 * \return			TRUE if successful; otherwise FALSE.
 */

osbool canvas_configure_save_area(struct canvas_block *instance);

/**
 * Set the palette for the sprite within a canvas to the colours requested
 * by a game.
 * 
 * \param *instance		The canvas instance to be updated.
 * \param *colours		An array of colours as supplied by the midend.
 * \param number_of_colours	The number of colours defined in the aray.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool canvas_set_game_colours(struct canvas_block *instance, float *colours, int number_of_colours);

/**
 * Find an entry from the canvas palette. If the request isn't
 * valid, the colour black is returned.
 * 
 * \param *instance	The canvas instance to query.
 * \param entry		The palette entry to be returned.
 * \return		The requested colour, or black.
 */

os_colour canvas_get_palette_entry(struct canvas_block *instance, int entry);

/**
 * Request details of the size of a canvas sprite.
 * 
 * \param *instance	The canvas instance to query.
 * \param *size		Pointer to an os_coord object to take the size.
 * \return		TRUE if successful; otherwise FALSE.
 */

osbool canvas_get_size(struct canvas_block *instance, os_coord *size);

/**
 * Start VDU redirection to a canvas sprite.
 * 
 * \param *instance	The canvas instance to update.
 * \return		TRUE if successful; otherwise FALSE.
 */

osbool canvas_start_redirection(struct canvas_block *instance);

/**
 * Stop VDU redirection to a canvas sprite.
 * 
 * \param *instance	The canvas instance to update.
 * \return		TRUE if successful; otherwise FALSE.
 */

osbool canvas_stop_redirection(struct canvas_block *instance);

/**
 * Test to see if VDU redirection is active for a canvas.
 * 
 * \param *instance	The canvas instance to test.
 * \return		TRUE if redirection is active; else FALSE.
 */

osbool canvas_is_redirection_active(struct canvas_block *instance);

/**
 * Prepare the data required to be passed to a redraw operation.
 *
 * \param *instance		The canvas instance to prepare.
 * \param *factors		Pointer to an empty pixel translation table,
 *				to be returned filled.
 * \apram *translation_table	Pointer to an empty colour translation table,
 *				to be returned filled.
 * \return			TRUE if the preparation was successful;
 *				else FALSE.
 */

osbool canvas_prepare_redraw(struct canvas_block *instance, os_factors *factors, osspriteop_trans_tab *translation_table);

/**
 * Plot the canvas sprite to the screen.
 *
 * \param *instance		The canvas instance to be plotted.
 * \param x			The X coordinate at which to plot the sprite.
 * \param y			The Y coordinate at which to plot the sprite.
 * \param *factors		The pixel translation table to use.
 * \param *translation_table	The colour translation table to use.
 */

void canvas_redraw_sprite(struct canvas_block *instance, int x, int y, os_factors *factors, osspriteop_trans_tab *translation_table);

/**
 * Save the canvas sprite area to disc.
 *
 * \param *instance	The canvas instance to save from.
 * \param *filename	A filename to save the area to.
 */

void canvas_save_sprite(struct canvas_block *instance, char *filename);

#endif

