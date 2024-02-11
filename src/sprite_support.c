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
 * \file: sprite_support.c
 *
 * Sprite Support implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/types.h"
#include "oslib/os.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"

/* Application header files */

#include "sprite_support.h"

/* The name of the canvas sprite. */

#define SPRITE_SUPPORT_SPRITE_NAME "Canvas"
#define SPRITE_SUPPORT_SPRITE_ID (osspriteop_id) SPRITE_SUPPORT_SPRITE_NAME

/**
 * The size of a sprite area header block, in bytes.
 */

#define SPRITE_SUPPORT_AREA_HEADER_SIZE 16

/**
 * The size of a sprite header block, in bytes.
 */

#define SPRITE_SUPPORT_SPRITE_HEADER_SIZE 44

/**
 * The size of the palette that we use in sprites, in entries.
 */

#define SPRITE_SUPPORT_MAX_PALETTE_ENTRIES 256

/**
 * The size of a palette in bytes.
 * 
 * There are 4 bytes per colour entry, and two colour entries
 * (flash 1 and flash 2) in each palette entry.
 */

#define SPRITE_SUPPORT_PALETTE_SIZE (SPRITE_SUPPORT_MAX_PALETTE_ENTRIES * 4 * 2)

/**
 * Locate the first sprite in a sprite area.
 */

#define sprite_support_get_first_sprite(area) ((osspriteop_header *) (((byte *) (area)) + (area)->first))

/**
 * Locate the palette in a sprite.
 */

#define sprite_support_get_palette(sprite) ((os_sprite_palette *) (((byte *) (sprite)) + SPRITE_SUPPORT_SPRITE_HEADER_SIZE))

/**
 * A Sprite Support instance block.
 */
/*
struct sprite_support_block {
	osspriteop_area *sprite_area;
	osspriteop_save_area *save_area;
};
*/
/* Static function prototypes. */

static osbool sprite_support_set_palette_game_colours(os_sprite_palette *palette, float *colours, int number_of_colours);

/**
 * Initialise a new sprite support instance.
 *
 * \return			A pointer to the new instance, or NULL.
 */

struct sprite_support_block *sprite_support_create_instance(void)
{
	struct sprite_support_block *new = NULL;

	new = malloc(sizeof(struct sprite_support_block));
	if (new == NULL)
		return NULL;

	new->sprite_area = NULL;
	new->save_area = NULL;

	return new;
}

/**
 * Delete a sprite support instance.
 * 
 * \param *instance		Pointer to the instance to delete.
 */

void sprite_support_delete_instance(struct sprite_support_block *instance)
{
	if (instance == NULL)
		return;

	if (instance->sprite_area != NULL)
		free(instance->sprite_area);

	if (instance->save_area != NULL)
		free(instance->save_area);
}

/**
 * Configure a sprite area and its single sprite, allocating
 * memory and initialising the sprite.
 * 
 * \param **area		Pointer to the area pointer for the
 *				sprite area to act upon; updated on
 *				exit with the correct pointer to the area.
 * \param width
 * \param height
 * \param include_palette
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sprite_support_configure_area(osspriteop_area **area, int width, int height, osbool include_palette)
{
	size_t area_size = 0;
	os_error *error;

	if (area == NULL)
		return FALSE;

	/* Calculate the size of the area, in bytes.
	 *
	 * We require the sprite area header and the sprite header, plus the required
	 * number of rows with each rounded up to a full number of words.
	 * If there's to be a palette, we add in space for that, too.
	 * 
	 * We're assuming that we will only work in 256 colour sprites, so one pixel
	 * is one byte.
	 */

	area_size = SPRITE_SUPPORT_AREA_HEADER_SIZE + SPRITE_SUPPORT_SPRITE_HEADER_SIZE + (((width + 3) & 0xfffffffc) * height);

	if (include_palette)
		area_size += SPRITE_SUPPORT_PALETTE_SIZE;

	/* Allocate, or adjust, the required area. */

	if (*area == NULL)
		*area = malloc(area_size);
	else
		*area = realloc(*area, area_size);

	if (*area == NULL)
		return FALSE;

	/* Initialise the area. */

	(*area)->size = area_size;
	(*area)->first = SPRITE_SUPPORT_AREA_HEADER_SIZE;

	error = xosspriteop_clear_sprites(osspriteop_USER_AREA, *area);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		*area = NULL;
		return FALSE;
	}

	/* Create the sprite. */

	error = xosspriteop_create_sprite(osspriteop_USER_AREA, *area, SPRITE_SUPPORT_SPRITE_NAME, FALSE, width, height, (os_mode) 21);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		*area = NULL;
		return FALSE;
	}

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

osbool sprite_support_insert_265_palette(osspriteop_area *area)
{
	osspriteop_header *sprite = NULL;

	if (area == NULL)
		return FALSE;

	sprite = sprite_support_get_first_sprite(area);

	/* Check that this is the only sprite in the area. */

	if (area->used != area->first + sprite->size) {
		error_msgs_report_error("SpriteBadArea");
		return FALSE;
	}

	/* Check that there is enough free space. */

	if (area->size - area->used < SPRITE_SUPPORT_PALETTE_SIZE) {
		error_msgs_report_error("SpriteNoSpaceForPalette");
		return FALSE;
	}

	/* Increase the space used in the sprite area. */

	area->used += SPRITE_SUPPORT_PALETTE_SIZE;

	/* Insert the palette into the sprite. */

	sprite->size += SPRITE_SUPPORT_PALETTE_SIZE;
	sprite->image += SPRITE_SUPPORT_PALETTE_SIZE;
	sprite->mask += SPRITE_SUPPORT_PALETTE_SIZE;

	debug_printf("\\VAdded Sprite Palette");

	return TRUE;
}

/**
 * Set the palette for the first sprite in an area to the
 * colours requested by a game.
 * 
 * \param *area			Pointer to the sprite area to act on.
 * \param *colours		An array of colours as supplied by the midend.
 * \param number_of_colours	The number of colours defined in the aray.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool sprite_support_set_game_colours(osspriteop_area *area, float *colours, int number_of_colours)
{
	osspriteop_header *sprite = NULL;
	os_sprite_palette *palette = NULL;

	sprite = sprite_support_get_first_sprite(area);
	palette = sprite_support_get_palette(sprite);

	return sprite_support_set_palette_game_colours(palette, colours, number_of_colours);
}

static osbool sprite_support_set_palette_game_colours(os_sprite_palette *palette, float *colours, int number_of_colours)
{
	int entry;

	if (palette == NULL || colours == NULL)
		return FALSE;

	/* There must be a valid number of colours. */

	if (number_of_colours < 0 || number_of_colours >= SPRITE_SUPPORT_MAX_PALETTE_ENTRIES)
		return FALSE;

	for (entry = 0; entry < SPRITE_SUPPORT_MAX_PALETTE_ENTRIES; entry++) {
		if (entry < number_of_colours) {
			palette->entries[entry].on = ((int) (colours[entry * 3] * 0xff) << 8) |
					((int) (colours[entry * 3 + 1] * 0xff) << 16) |
					((int) (colours[entry * 3 + 2] * 0xff) << 24);
		} else {
			palette->entries[entry].on = os_COLOUR_WHITE;
		}

		palette->entries[entry].off = palette->entries[entry].on;

		debug_printf("Created palette entry %d at 0x%x as 0x%8x", entry, &(palette->entries[entry]), palette->entries[entry].off);
	}

	return TRUE;
}














void sprite_support_set_palette_entry(os_sprite_palette *palette, int entry, os_colour colour)
{
	if (palette != NULL && entry >= 0 && entry < SPRITE_SUPPORT_MAX_PALETTE_ENTRIES) {
		palette->entries[entry].on = colour;
		palette->entries[entry].off = colour;
	}
}