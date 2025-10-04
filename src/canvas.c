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
 * \file: cancas.c
 *
 * Drawing Canvas implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdint.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/types.h"
#include "oslib/os.h"
#include "oslib/colourtrans.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"

/* Application header files */

#include "canvas.h"

/* The name of the canvas sprite. */

#define CANVAS_SPRITE_NAME "Canvas"
#define CANVAS_SPRITE_ID (osspriteop_id) CANVAS_SPRITE_NAME

/**
 * The size of a sprite area header block, in bytes.
 */

#define CANVAS_AREA_HEADER_SIZE 16

/**
 * The size of a sprite header block, in bytes.
 */

#define CANVAS_SPRITE_HEADER_SIZE 44

/**
 * The size of the palette that we use in sprites, in entries.
 */

#define CANVAS_MAX_PALETTE_ENTRIES 256

/**
 * The maximum error allowable between a colour and an existing palette
 * entry before the colour will be added.
 */

#define CANVAS_MAX_PALETTE_ERROR 5

/**
 * The number of intermediate colours to include in colour gradients.
 */

#define CANVAS_GRADIENT_LENGTH 5

/**
 * The size of a palette in bytes.
 *
 * There are 4 bytes per colour entry, and two colour entries
 * (flash 1 and flash 2) in each palette entry.
 */

#define CANVAS_PALETTE_SIZE (CANVAS_MAX_PALETTE_ENTRIES * 4 * 2)

/**
 * Locate the first sprite in a sprite area.
 */

#define canvas_get_first_sprite(area) ((osspriteop_header *) (((byte *) (area)) + (area)->first))

/**
 * Locate the palette in a sprite.
 */

#define canvas_get_palette(sprite) ((os_sprite_palette *) (((byte *) (sprite)) + CANVAS_SPRITE_HEADER_SIZE))

/**
 * Test to see if a sprite area is empty.
 */

#define canvas_does_sprite_exist(area) (((area)->first == (area)->used) ? FALSE : TRUE)

/**
 * Test to see if a sprite has a palette.
 */

#define canvas_does_palette_exist(sprite) (((sprite)->image == CANVAS_SPRITE_HEADER_SIZE) ? FALSE : TRUE)

/**
 * Assemble a set of RGB values in the range 0 to 255 into an OS Colour value.
 */

#define canvas_make_os_colour(r, g, b) (((r) << 8) | ((g) << 16) | ((b) << 24))

/**
 * Split the red component out of an OS COlour value, as a value in the range 0 to 255.
 */

#define canvas_get_os_colour_red(colour) (((colour) >> 8) & 0xff)

/**
 * Split the green component out of an OS COlour value, as a value in the range 0 to 255.
 */

 #define canvas_get_os_colour_green(colour) (((colour) >> 16) & 0xff)

/**
 * Split the blue component out of an OS COlour value, as a value in the range 0 to 255.
 */

 #define canvas_get_os_colour_blue(colour) (((colour) >> 24) & 0xff)


/**
 * A Canvas instance block.
 */

struct canvas_block {
	os_coord size;				/**< The size of the canvas area, in pixels.			*/

	osspriteop_area *sprite_area;		/**< The sprite area holding the canvas sprite.			*/
	osspriteop_save_area *save_area;	/**< The save area for redirecting VDU output.			*/

	osbool redirection_active;		/**< TRUE if VDU redirection to the canvas sprite is active.	*/

	int saved_context0;			/**< The first byte of context for sprite redirection.		*/
	int saved_context1;			/**< The second byte of context for sprite redirection.		*/
	int saved_context2;			/**< The third byte of context for sprite redirection.		*/
	int saved_context3;			/**< The fourth byte of context for sprite redirection.		*/
};

/* Static function prototypes. */

static osbool canvas_insert_265_palette(osspriteop_area *area);
static int canvas_set_palette_game_colours(os_sprite_palette *palette, int palette_entries, float *colours, int number_of_colours);
static int canvas_set_pallete_build_gradient(os_sprite_palette *palette, int palette_entries, os_colour start, os_colour end, int points);
static int canvas_set_palette_fill_unused(os_sprite_palette *palette, int palette_entries);
static void canvas_set_palette_entry(os_sprite_palette *palette, int entry, os_colour colour);

/**
 * Initialise a new canvas instance.
 *
 * \return			A pointer to the new instance, or NULL.
 */

struct canvas_block *canvas_create_instance(void)
{
	struct canvas_block *new = NULL;

	new = malloc(sizeof(struct canvas_block));
	if (new == NULL)
		return NULL;

	new->sprite_area = NULL;
	new->save_area = NULL;

	new->size.x = 0;
	new->size.y = 0;

	new->redirection_active = FALSE;

	// debug_printf"Canvas instance 0x%x created", new);

	return new;
}

/**
 * Delete a canvas instance.
 *
 * \param *instance		Pointer to the instance to delete.
 */

void canvas_delete_instance(struct canvas_block *instance)
{
	if (instance == NULL)
		return;

	// debug_printf"Deleting canvas instance 0x%x", instance);

	if (instance->sprite_area != NULL)
		free(instance->sprite_area);

	if (instance->save_area != NULL)
		free(instance->save_area);
}

/**
 * Configure a canvas to a given dimension, and set up its sprite
 *
 * \param *instance		The canvas instance to be updated.
 * \param width			The required canvas width.
 * \param height		The required canvas height.
 * \param include_palette	TRUE if a palette should be included in the sprite.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool canvas_configure_area(struct canvas_block *instance, int width, int height, osbool include_palette)
{
	size_t area_size = 0;
	os_error *error;

	if (instance == NULL)
		return FALSE;

	/* Zero the canvas size. */

	instance->size.x = 0;
	instance->size.y = 0;

	/* If there's already a save area, zero its first word to reset it. */

	if (instance->save_area != NULL)
		*((int32_t *) instance->save_area) = 0;

	/* Calculate the size of the area, in bytes.
	 *
	 * We require the sprite area header and the sprite header, plus the required
	 * number of rows with each rounded up to a full number of words (+3) and an
	 * extra three bytes added on for copying to blitters at non-aligned addresses
	 * at the start of the row (+3). If there's to be a palette, we add in space
	 * for that, too.
	 *
	 * We're assuming that we will only work in 256 colour sprites, so one pixel
	 * is one byte.
	 */

	if (width > 0 && height > 0)
		area_size = CANVAS_AREA_HEADER_SIZE + CANVAS_SPRITE_HEADER_SIZE + (((width + 6) & 0xfffffffc) * height);
	else
		area_size = CANVAS_SPRITE_HEADER_SIZE;

	if (include_palette)
		area_size += CANVAS_PALETTE_SIZE;

	/* Allocate, or adjust, the required area. */

	if (instance->sprite_area == NULL)
		instance->sprite_area = malloc(area_size);
	else
		instance->sprite_area = realloc(instance->sprite_area, area_size);

	if (instance->sprite_area == NULL)
		return FALSE;

	/* Initialise the area. */

	instance->sprite_area->size = area_size;
	instance->sprite_area->first = CANVAS_AREA_HEADER_SIZE;

	error = xosspriteop_clear_sprites(osspriteop_USER_AREA, instance->sprite_area);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		free(instance->sprite_area);
		instance->sprite_area = NULL;
		return FALSE;
	}

	if (area_size == CANVAS_AREA_HEADER_SIZE) {
		error_msgs_report_error("SpriteBadDims");
		return FALSE;
	}

	/* Create the sprite. */

	error = xosspriteop_create_sprite(osspriteop_USER_AREA, instance->sprite_area, CANVAS_SPRITE_NAME, FALSE, width, height, (os_mode) 21);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		free(instance->sprite_area);
		instance->sprite_area = NULL;
		return FALSE;
	}

	instance->size.x = width;
	instance->size.y = height;

	/* Add the palette if required. */

	if (include_palette == TRUE && canvas_insert_265_palette(instance->sprite_area) == FALSE)
		return FALSE;

	return TRUE;
}

/**
 * Configure the save area for a canvas to suit the
 * current sprite.
 *
 * \param *instance		The canvas instance to be updated.
 * \return			TRUE if successful; otherwise FALSE.
 */

osbool canvas_configure_save_area(struct canvas_block *instance)
{
	int area_size = 0;
	os_error *error;

	if (instance == NULL || instance->sprite_area == NULL)
		return FALSE;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	/* Identify how much space we require. */

	error = xosspriteop_read_save_area_size(osspriteop_USER_AREA, instance->sprite_area,
			CANVAS_SPRITE_ID, &area_size);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Allocate, or adjust, the required save area. */

	if (instance->save_area == NULL)
		instance->save_area = malloc(area_size);
	else
		instance->save_area = realloc(instance->save_area, area_size);

 	if (instance->save_area == NULL)
		return FALSE;

	*((int32_t *) instance->save_area) = 0;

	return TRUE;
}

/**
 * Add a 256 colour sprite to the first sprite in a sprite area.
 *
 * This is done by hand, using the details provided on page 1-833
 * of the PRM. This should be a compatible with RISC OS 3.1 onwards!
 *
 * The sprite is assumed to be unused at this point: no attempt is
 * made to shift the bitmap data up to allow space for the palette
 * to be inserted.
 *
 * \param *area			Pointer to the sprite area to act on.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool canvas_insert_265_palette(osspriteop_area *area)
{
	osspriteop_header *sprite = NULL;

	if (area == NULL || canvas_does_sprite_exist(area) == FALSE)
		return FALSE;

	sprite = canvas_get_first_sprite(area);

	/* Check that the palette doesn't already exist. */

	if (canvas_does_palette_exist(sprite) == TRUE)
		return FALSE;

	/* Check that this is the only sprite in the area. */

	if (area->used != area->first + sprite->size) {
		error_msgs_report_error("SpriteBadArea");
		return FALSE;
	}

	/* Check that there is enough free space. */

	if (area->size - area->used < CANVAS_PALETTE_SIZE) {
		error_msgs_report_error("SpriteNoSpaceForPalette");
		return FALSE;
	}

	/* Increase the space used in the sprite area. */

	area->used += CANVAS_PALETTE_SIZE;

	/* Insert the palette into the sprite. */

	sprite->size += CANVAS_PALETTE_SIZE;
	sprite->image += CANVAS_PALETTE_SIZE;
	sprite->mask += CANVAS_PALETTE_SIZE;

	// debug_printf"\\VAdded Sprite Palette");

	return TRUE;
}

/**
 * Set the palette for the sprite within a canvas to the colours requested
 * by a game.
 *
 * \param *instance		The canvas instance to be updated.
 * \param *colours		An array of colours as supplied by the midend.
 * \param number_of_colours	The number of colours defined in the aray.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool canvas_set_game_colours(struct canvas_block *instance, float *colours, int number_of_colours)
{
	osspriteop_header *sprite = NULL;
	os_sprite_palette *palette = NULL;
	int palette_entries = 0;

	if (instance == NULL || instance->sprite_area == NULL || colours == NULL)
		return FALSE;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	sprite = canvas_get_first_sprite(instance->sprite_area);

	if (canvas_does_palette_exist(sprite) == FALSE)
		return FALSE;

	palette = canvas_get_palette(sprite);
	if (palette == NULL)
		return FALSE;

	/* Add the backend	 game colours. */

	palette_entries = canvas_set_palette_game_colours(palette, palette_entries, colours, number_of_colours);

	/* Generate some intermediate colours for antialiasing. */

	palette_entries = canvas_set_pallete_build_gradient(palette, palette_entries, os_COLOUR_BLACK, os_COLOUR_WHITE, 10);

	for (int start = 0; start < (number_of_colours - 1); start++) {
		for (int end = start + 1; end < number_of_colours; end++) {
			palette_entries = canvas_set_pallete_build_gradient(palette, palette_entries,
					palette->entries[start].on, palette->entries[end].on, CANVAS_GRADIENT_LENGTH);
		}
	}

	/* Fill any unused space. */

	palette_entries = canvas_set_palette_fill_unused(palette, palette_entries);

	/* There should be no space left in the palette. */

	return (palette_entries == CANVAS_MAX_PALETTE_ENTRIES) ? TRUE : FALSE;
}

/**
 * Set a palette to the colours required by a game.
 *
 * \param *palette		The palette to be updated.
 * \param palette_entries	The number of defined colours currently in the
 *				palette before the operation.
 * \param *colours		An array of colours as supplied by the midend.
 * \param number_of_colours	The number of colours defined in the aray.
 * \return			The number of defined colours in the palette
 *				after the update has completed.
 */

static int canvas_set_palette_game_colours(os_sprite_palette *palette, int palette_entries, float *colours, int number_of_colours)
{
	if (palette == NULL || colours == NULL)
		return palette_entries;

	/* There must be a valid number of colours. */

	if (palette_entries >= CANVAS_MAX_PALETTE_ENTRIES)
		return palette_entries;

	if (number_of_colours < 0 || number_of_colours >= (CANVAS_MAX_PALETTE_ENTRIES - palette_entries))
		return palette_entries;

	/* Copy each of the game colours into the palette. */

	for (int entry = 0; entry < number_of_colours; entry++) {
		canvas_set_palette_entry(palette, palette_entries++, canvas_make_os_colour(
				(int) (colours[entry * 3] * 0xff),
				(int) (colours[entry * 3 + 1] * 0xff),
				(int) (colours[entry * 3 + 2] * 0xff)));
	}

	return palette_entries;
}
/**
 * Add a gradient of colours to a palette. The gradient is defined to run from
 * start to end, but does *not* include the start and end colours. This is
 * because it is assumed that it will be used to generate colour gradients
 * between defined game colours.
 *
 * \param *palette		The palette to be updated.
 * \param palette_entries	The number of defined colours currently in the
 *				palette before the operation.
 * \param start			The colour for the start of the gradient.
 * \param end			The colour for the end of the gradient.
 * \param points		The number of points to include in the gradient,
 *				not including the start and finish.
 * \return			The number of defined colours in the palette
 *				after the update has completed.
 */

static int canvas_set_pallete_build_gradient(os_sprite_palette *palette, int palette_entries, os_colour start, os_colour end, int points)
{
	int r_start, g_start, b_start, r_end, g_end, b_end;

	if (palette == NULL)
		return palette_entries;

	/* There must be a valid number of colours. */

	if (palette_entries >= CANVAS_MAX_PALETTE_ENTRIES)
		return palette_entries;

	if (points < 1 || points >= (CANVAS_MAX_PALETTE_ENTRIES - palette_entries))
		return palette_entries;

	/* Calculate the starting colour of the gradient. */

	r_start = canvas_get_os_colour_red(start);
	g_start = canvas_get_os_colour_green(start);
	b_start = canvas_get_os_colour_blue(start);

	/* Calculate the finishing colour of the gradient. */

	r_end = canvas_get_os_colour_red(end);
	g_end = canvas_get_os_colour_green(end);
	b_end = canvas_get_os_colour_blue(end);

	/* Build the colour gradients. */

	for (int step = 1; step <= points; step++) {
		int r, g, b;
		osbool include = TRUE;

		/* Calculate the step colour. */

		r = (((r_end - r_start) * step / points) + r_start) & 0xff;
		g = (((g_end - g_start) * step / points) + g_start) & 0xff;
		b = (((b_end - b_start) * step / points) + b_start) & 0xff;

		/* Search the rest of the palette for a similar entry. */

		for (int search = 0; search < palette_entries; search++) {
			int r1, g1, b1, er, eg, eb;

			r1 = canvas_get_os_colour_red(palette->entries[search].on);
			g1 = canvas_get_os_colour_green(palette->entries[search].on);
			b1 = canvas_get_os_colour_blue(palette->entries[search].on);

			/* If the existing entry is close enough to the new colour
			 * on red, green and blue, don't bother including the new one.
			 */

			er = 100 * abs(r - r1) / r;
			eg = 100 * abs(g - g1) / g;
			eb = 100 * abs(b - b1) / b;

			if (er < CANVAS_MAX_PALETTE_ERROR &&
					eg < CANVAS_MAX_PALETTE_ERROR &&
					eb < CANVAS_MAX_PALETTE_ERROR) {
				include = FALSE;
				break;
			}
		}

		/* Include the palette entry if a close enough match wasn't found. */

		if (include == TRUE)
			canvas_set_palette_entry(palette, palette_entries++, canvas_make_os_colour(r, g, b));
	}

	return palette_entries;
}

/**
 * Fill unused entries in a palette with white. On exit, the function should
 * return CANVAS_MAX_PALETTE_ENTRIES.
 *
 * \param *palette		The palette to be updated.
 * \param palette_entries	The number of defined colours currently in the
 *				palette before the operation.
 * \return			The number of defined colours in the palette
 *				after the update has completed.
 */

static int canvas_set_palette_fill_unused(os_sprite_palette *palette, int palette_entries)
{
	if (palette == NULL)
		return palette_entries;

	/* There must be a valid number of colours. */

	if (palette_entries >= CANVAS_MAX_PALETTE_ENTRIES)
		return palette_entries;

	/* Fill the rest of the palette entries with white. */

	for (; palette_entries < CANVAS_MAX_PALETTE_ENTRIES; palette_entries++)
		canvas_set_palette_entry(palette, palette_entries, os_COLOUR_WHITE);

	return palette_entries;
}

/**
 * Find an entry from the canvas palette. If the request isn't
 * valid, the colour black is returned.
 *
 * \param *instance	The canvas instance to query.
 * \param entry		The palette entry to be returned.
 * \return		The requested colour, or black.
 */

os_colour canvas_get_palette_entry(struct canvas_block *instance, int entry)
{
	osspriteop_header *sprite = NULL;
	os_sprite_palette *palette = NULL;

	if (instance == NULL || instance->sprite_area == NULL)
		return os_COLOUR_BLACK;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return os_COLOUR_BLACK;

	if (entry < 0 || entry >= CANVAS_MAX_PALETTE_ENTRIES)
		return os_COLOUR_BLACK;

	sprite = canvas_get_first_sprite(instance->sprite_area);

	if (canvas_does_palette_exist(sprite) == FALSE)
		return os_COLOUR_BLACK;

	palette = canvas_get_palette(sprite);

	return palette->entries[entry].on;
}

/**
 * Request details of the size of a canvas sprite.
 *
 * \param *instance	The canvas instance to query.
 * \param *size		Pointer to an os_coord object to take the size.
 * \return		TRUE if successful; otherwise FALSE.
 */

osbool canvas_get_size(struct canvas_block *instance, os_coord *size)
{
	if (instance == NULL)
		return FALSE;

	if (size != NULL) {
		size->x = instance->size.x;
		size->y = instance->size.y;
	}

	return TRUE;
}

/**
 * Start VDU redirection to a canvas sprite.
 *
 * \param *instance	The canvas instance to update.
 * \return		TRUE if successful; otherwise FALSE.
 */

osbool canvas_start_redirection(struct canvas_block *instance)
{
	os_error *error;

	if (instance == NULL || instance->sprite_area == NULL || instance->save_area == NULL)
		return FALSE;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	/* We can't start redirection if it's already active! */

	if (instance->redirection_active == TRUE)
		return FALSE;

	/* Switch VDU output to the canvas sprite. */

	error = xosspriteop_switch_output_to_sprite(osspriteop_USER_AREA, instance->sprite_area,
			CANVAS_SPRITE_ID, instance->save_area,
			&(instance->saved_context0), &(instance->saved_context1), &(instance->saved_context2), &(instance->saved_context3));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	instance->redirection_active = TRUE;

	// debug_printf"VDU Redirection Active");

	return TRUE;
}

/**
 * Stop VDU redirection to a canvas sprite.
 *
 * \param *instance	The canvas instance to update.
 * \return		TRUE if successful; otherwise FALSE.
 */

osbool canvas_stop_redirection(struct canvas_block *instance)
{
	os_error *error;

	if (instance == NULL || instance->sprite_area == NULL || instance->save_area == NULL)
		return FALSE;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	/* We can't stop redirection if it isn't active! */

	if (instance->redirection_active == FALSE)
		return FALSE;

	/* Restore the graphics context. */

	error = xosspriteop_switch_output_to_sprite(instance->saved_context0, (osspriteop_area *) instance->saved_context1,
			(osspriteop_id) instance->saved_context2, (osspriteop_save_area *) instance->saved_context3, NULL, NULL, NULL, NULL);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	instance->redirection_active = FALSE;

	// debug_printf"VDU Redirection Inactive");

	return TRUE;

}

/**
 * Test to see if VDU redirection is active for a canvas.
 *
 * \param *instance	The canvas instance to test.
 * \return		TRUE if redirection is active; else FALSE.
 */

osbool canvas_is_redirection_active(struct canvas_block *instance)
{
	return (instance != NULL && instance->sprite_area != NULL && instance->save_area != NULL &&
			instance->redirection_active == TRUE) ? TRUE : FALSE;
}

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

osbool canvas_prepare_redraw(struct canvas_block *instance, os_factors *factors, osspriteop_trans_tab *translation_table)
{
	if (instance == NULL || instance->sprite_area == NULL || factors == NULL || translation_table == NULL)
		return FALSE;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	if (xwimp_read_pix_trans(osspriteop_USER_AREA, instance->sprite_area, CANVAS_SPRITE_ID, factors, NULL) != NULL)
		return FALSE;

	if (xcolourtrans_select_table_for_sprite(instance->sprite_area, CANVAS_SPRITE_ID, os_CURRENT_MODE,
			(os_palette *) -1, translation_table, 0) != NULL)
		return FALSE;

	return TRUE;
}

/**
 * Plot the canvas sprite to the screen for a redraw operation,
 * using the palette and all of the necessary translation tables.
 *
 * Any errors which occur will be quietly dropped.
 *
 * \param *instance		The canvas instance to be plotted.
 * \param x			The X coordinate of the bottom left corner of
 *				the location at which to plot the sprite.
 * \param y			The Y coordinate of the bottom left corner of
 *				the location at which to plot the sprite.
 * \param *factors		The pixel translation table to use.
 * \param *translation_table	The colour translation table to use.
 */

void canvas_redraw_sprite(struct canvas_block *instance, int x, int y, os_factors *factors, osspriteop_trans_tab *translation_table)
{
	if (instance != NULL && instance->sprite_area != NULL && canvas_does_sprite_exist(instance->sprite_area) == TRUE) {
		xosspriteop_put_sprite_scaled(osspriteop_USER_AREA, instance->sprite_area, CANVAS_SPRITE_ID,
				x, y, (osspriteop_action) 0, factors, translation_table);
	}
}

/**
 * Capture the canvas from screen.
 *
 * To avoid passing canvas sizes back and forth between the client
 * and the canvas, we specify the coordinates from the TOP left of
 * the sprite area, which suits the Blitter's information.
 *
 * \param *instance		The canvas instance to be plotted.
 * \param x			The X coordinate of the top left corner of
 *				the location from which to capture the sprite.
 * \param y			The Y coordinate of the top left corner of
 *				the location from which to capture the sprite.
 * \return			TRUE if successful; otherwise FALSE.
 */

osbool canvas_get_sprite(struct canvas_block *instance, int x, int y)
{
	if (instance == NULL || instance->sprite_area == NULL || canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	return (xosspriteop_get_sprite_user_coords(osspriteop_USER_AREA, instance->sprite_area, CANVAS_SPRITE_NAME, FALSE,
			x, y - CANVAS_PIXEL_SIZE * (instance->size.y - 1),
			x + CANVAS_PIXEL_SIZE * (instance->size.x - 1), y) == NULL) ? TRUE : FALSE;
}

/**
 * Plot the canvas sprite to the screen without palette or translation
 * tables.
 *
 * To avoid passing canvas sizes back and forth between the client
 * and the canvas, we specify the coordinates from the TOP left of
 * the sprite area, which suits the Blitter's information.
 *
 * \param *instance		The canvas instance to be plotted.
 * \param x			The X coordinate of the top left corner of
 *				the location at which to plot the sprite.
 * \param y			The Y coordinate of the top left corner of
 *				the location at which to plot the sprite.
 * \return			TRUE if successful; otherwise FALSE.
 */

osbool canvas_put_sprite(struct canvas_block *instance, int x, int y)
{
	if (instance == NULL || instance->sprite_area == NULL || canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return FALSE;

	return (xosspriteop_put_sprite_user_coords(osspriteop_USER_AREA, instance->sprite_area, CANVAS_SPRITE_ID,
				x, y - CANVAS_PIXEL_SIZE * (instance->size.y - 1), os_ACTION_OVERWRITE) == NULL) ? TRUE : FALSE;
}

/**
 * Save the canvas sprite area to disc.
 *
 * \param *instance	The canvas instance to save from.
 * \param *filename	A filename to save the area to.
 */

void canvas_save_sprite(struct canvas_block *instance, char *filename)
{
	os_error *error;

	if (instance == NULL || instance->sprite_area == NULL || filename == NULL)
		return;

	if (canvas_does_sprite_exist(instance->sprite_area) == FALSE)
		return;

	error = xosspriteop_save_sprite_file(osspriteop_USER_AREA, instance->sprite_area, filename);
	debug_printf("Saved sprites: outcome=0x%x", error);
	if (error != NULL)
		debug_printf("\\RFailed to save: %s", error->errmess);
}

/**
 * Set an entry in the palette.
 *
 * \param *palette	Pointer to the palette to be updated.
 * \param entry		The index of the entry to be updated in the palette.
 * \param colour	The new value to apply to that index.
 */

static void canvas_set_palette_entry(os_sprite_palette *palette, int entry, os_colour colour)
{
	if (palette != NULL && entry >= 0 && entry < CANVAS_MAX_PALETTE_ENTRIES) {
		palette->entries[entry].on = colour;
		palette->entries[entry].off = colour;
	}
}
