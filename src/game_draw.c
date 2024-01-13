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
 * \file: game_draw.c
 *
 * Draw game objects to screen or paper implementation.
 */

/* ANSI C header files */

#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/draw.h"
#include "oslib/os.h"

/* SF-Lib header files. */

/* Application header files */

#include "game_draw.h"


/**
 * The size of the Draw Path buffer, in words.
 */

#define GAME_DRAW_BUFFER_LENGTH 256

/**
 * Buffer to hold the current Draw Path.
 */

static bits game_draw_path[GAME_DRAW_BUFFER_LENGTH];

/**
 * Length of the current Draw Path, in words.
 */

static size_t game_draw_path_length = 0;

/**
 * Is the current path believed to be valid?
 */

static osbool game_draw_valid_path = TRUE;

/* Static Function Prototypes. */

static draw_path_element *game_draw_get_new_element(size_t element_size);


/**
 * Draw a rectangle on screen.
 *
 * \param *outline		The rectangle outline, in absolute OS Units.
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_box(os_box *outline, int width)
{
	bits			dash_data[3];
	draw_dash_pattern	*dash_pattern = (draw_dash_pattern *) dash_data;

	if (outline == NULL)
		return NULL;

	game_draw_start_path();

	if (!game_draw_add_move(outline->x0, outline->y0))
		return NULL;

	if (!game_draw_add_line(outline->x1, outline->y0))
		return NULL;

	if (!game_draw_add_line(outline->x1, outline->y1))
		return NULL;

	if (!game_draw_add_line(outline->x0, outline->y1))
		return NULL;

	if (!game_draw_add_line(outline->x0, outline->y0))
		return NULL;

	if (!game_draw_close_subpath())
		return NULL;

	if (!game_draw_end_path())
		return NULL;

	return game_draw_plot_path(width);
}


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

os_error *game_draw_line(int x0, int y0, int x1, int y1, int width)
{
	game_draw_start_path();

	if (!game_draw_add_move(x0, y0))
		return NULL;

	if (!game_draw_add_line(x1, y1))
		return NULL;

	if (!game_draw_end_path())
		return NULL;

	return game_draw_plot_path(width);
}

/**
 * Start a new path.
 */

void game_draw_start_path(void)
{
	game_draw_path_length = 0;
	game_draw_valid_path = TRUE;
}

/**
 * Add a move to the current Draw Path.
 *
 * \param x			The X coordinate to move to, in OS Units.
 * \param y			The Y coordinate to move to, in OS Units.
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_add_move(int x, int y)
{
	draw_path_element	*element;

	element = game_draw_get_new_element(3);
	if (element == NULL)
		return FALSE;

	element->tag = draw_MOVE_TO;
	element->data.move_to.x = (x << 8);
	element->data.move_to.y = (y << 8);

	return TRUE;
}


/**
 * Add a line to the current Draw Path.
 *
 * \param x			The X coordinate to draw to, in OS Units.
 * \param y			The Y coordinate to draw to, in OS Units.
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_add_line(int x, int y)
{
	draw_path_element	*element;

	element = game_draw_get_new_element(3);
	if (element == NULL)
		return FALSE;

	element->tag = draw_LINE_TO;
	element->data.line_to.x = (x << 8);
	element->data.line_to.y = (y << 8);

	return TRUE;
}


/**
 * Close the current subpath in the Draw Path.
 *
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_close_subpath(void)
{
	draw_path_element	*element;

	element = game_draw_get_new_element(1);
	if (element == NULL)
		return FALSE;

	element->tag = draw_CLOSE_LINE;

	return TRUE;
}


/**
 * End the current Draw Path.
 *
 * \return			TRUE on success, FALSE on failure.
 */

osbool game_draw_end_path(void)
{
	draw_path_element	*element;

	element = game_draw_get_new_element(2);
	if (element == NULL)
		return FALSE;

	element->tag = draw_END_PATH;
	element->data.end_path = 0;

	return TRUE;
}

/**
 * Plot the path in the buffer.
 *
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_plot_path(int width)
{
	static const draw_line_style line_style = { draw_JOIN_MITRED,
			draw_CAP_SQUARE, draw_CAP_SQUARE, 0, 0x7fffffff,
			0, 0, 0, 0 };

	if (!game_draw_valid_path) {
		debug_printf("\\rInvalid path!!!!");
		return NULL;
	}

	return xdraw_stroke((draw_path*) game_draw_path, draw_FILL_NONZERO, NULL,
			0, width << 8, &line_style, NULL);
}

/**
 * Plot the path in the buffer as a filled shape.
 *
 * \param width			The required width, in OS Units.
 * \return			Pointer to an OS Error block, or NULL on success.
 */

os_error *game_draw_fill_path(int width)
{
	static const draw_line_style line_style = { draw_JOIN_MITRED,
			draw_CAP_SQUARE, draw_CAP_SQUARE, 0, 0x7fffffff,
			0, 0, 0, 0 };

	if (!game_draw_valid_path) {
		debug_printf("\\rInvalid path!!!!");
		return NULL;
	}

	return xdraw_fill((draw_path*) game_draw_path, draw_FILL_NONZERO, NULL, 0);
}

/**
 * Claim storage for a new Draw Path element from the data block.
 *
 * \param element_size		The required element size, in 32-bit words.
 * \return			Pointer to the new element, or NULL on failure.
 */

static draw_path_element *game_draw_get_new_element(size_t element_size)
{
	draw_path_element	*element;

	if (game_draw_path_length + element_size > GAME_DRAW_BUFFER_LENGTH) {
		game_draw_valid_path = FALSE;
		return NULL;
	}

	element = (draw_path_element *) (game_draw_path + game_draw_path_length);
	game_draw_path_length += element_size;

	return element;
}
