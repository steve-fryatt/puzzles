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
 * \file: game_config.c
 *
 * Implementation of the code which creates and manages
 * the configuration dialogues presented on behalf of the
 * midend.
 */

/* ANSI C header files */

#include "stdarg.h"
#include "stddef.h"
#include "string.h"

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "game_config.h"

#include "core/puzzles.h"
#include "frontend.h"

/* The config window template icons. */

#define GAME_WINDOW_TEMPLATE_ICON_CANCEL 0
#define GAME_WINDOW_TEMPLATE_ICON_OK 1
#define GAME_WINDOW_TEMPLATE_ICON_SAVE 8
#define GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD 2
#define GAME_WINDOW_TEMPLATE_ICON_WRITABLE_LABEL 3
#define GAME_WINDOW_TEMPLATE_ICON_OPTION 4
#define GAME_WINDOW_TEMPLATE_ICON_COMBO_POPUP 5
#define GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD 6
#define GAME_WINDOW_TEMPLATE_ICON_COMBO_LABEL 7

/**
 * The number of OS units between rows in the dialogue.
 */

#define GAME_WINDOW_INTER_ROW_GAP 8

/**
 * An OS Unit count to add to text widths when sizing icons.
 */

#define GAME_CONFIG_TEXT_LENGTH_MARGIN 16

/**
 * The multiple to apply to the mininum text field width for
 * use in CFG_DESC and CFG_SEED dialogues.
 */

#define GAME_CONFIG_SEED_WIDTH_MULTIPLE 3

/**
 * The size of a text field in a Description or Random Seed
 * dialogue. The mid-end doesn't specify a size, so this is
 * a bit of a guess.
 */

#define GAME_WINDOW_DESCRIPTION_FIELD_SIZE 1024
/**
 * The size of a text field in all other configuration
 * dialogues. The mid-end doesn't specify a size, so this is
 * a bit of a guess.
 */

#define GAME_WINDOW_STANDARD_FIELD_SIZE 64

/**
 * The maximum size allowed for looking up menu entry texts
 * from the Messages file.
 */

#define GAME_CONFIG_MENU_TITLE_LEN 64

/**
 * Details of an collection of icons forming a widget.
 */

struct game_config_widget {
	os_box bounding_box;
	os_coord origin;
	int field_width;
	int pad_width;
};

/**
 * Details of the text widget.
 */

static struct game_config_widget game_config_text_widget;

/**
 * Details of the combo widget.
 */

static struct game_config_widget game_config_combo_widget;

/**
 * Details of the option widget.
 */

static struct game_config_widget game_config_option_widget;

/**
 * Details of the action buttons.
 */

static struct game_config_widget game_config_action_widget;

/**
 * The definition for the config window components.
 */

static wimp_window *game_config_window_def = NULL;

/**
 * The number of icons in our window definition.
 */

static int game_config_icon_count = 0;

/**
 * A record for one of the fields in a dialogue.
 */

struct game_config_entry {
	wimp_i icon_handle;
	wimp_menu *popup_menu;
	char *icon_text;
	char *popup_text;
};

/**
 * A Game Config instance.
 */

struct game_config_block {
	wimp_w	handle;				/**< The handle of the window.				*/

	wimp_i action_ok;			/**< The handle of the OK button.			*/
	wimp_i action_cancel;			/**< The handle of the Cancel button.			*/
	wimp_i action_save;			/**< The handle of the Save button.			*/

	char *window_title;			/**< The title of the window, supplied by the midend.	*/

	size_t field_size;			/**< The buffer size to allocate for text fields.	*/

	int config_type;			/**< The type of dialogue, supplied from the midend.	*/
	config_item *config_data;		/**< The dialogue data, supplied by the midend.		*/

	size_t entry_count;			/**< The number of entries in the dialogue.		*/
	struct game_config_entry *entries;	/**< A list of config entries in the dialogue.		*/

	/**
	 * Callback to the front-end instance which called us, when we have user actions to report.
	 */

	void *client_data;
	osbool (*callback)(int type, config_item *config, enum game_config_outcome, void *data);
};

/**
 * The pop-up menu title text.
 */

static char *game_config_popup_menu_title = NULL;

/* Static function prototypes. */

static void game_config_click_handler(wimp_pointer *pointer);
static osbool game_config_keypress_handler(wimp_key *key);
static void game_config_process_user_action(struct game_config_block *instance, enum game_config_outcome outcome);
static osbool game_config_build_window(struct game_config_block *instance);

static osbool game_config_size_window(struct game_config_block *instance, int *left, int *right, int *height);
static void game_config_size_text_field(config_item *item, int *left, int *right, int *height);
static void game_config_size_combo_field(config_item *item, int *left, int *right, int *height);
static void game_config_size_option_field(config_item *item, int *left, int *right, int *height);
static void game_config_size_action_buttons(int *width, int *height, osbool include_save);

static osbool game_config_create_icons(struct game_config_block *instance, int left, int right);
static void game_config_create_text_widget(struct game_config_block *instance, int index, int left, int right, int *baseline);
static void game_config_create_combo_widget(struct game_config_block *instance, int index, int left, int right, int *baseline);
static void game_config_create_option_widget(struct game_config_block *instance, int index, int left, int right, int *baseline);
static void game_config_create_action_widget(struct game_config_block *instance, int left, int right, int *baseline);

static void game_config_set_caret(struct game_config_block *instance);
static osbool game_config_copy_to_dialogue(struct game_config_block *instance);
static osbool game_config_copy_from_dialogue(struct game_config_block *instance);

static wimp_i game_config_create_icon(wimp_w handle, wimp_i icon, int x0, int x1, int centreline, int baseline, char *text, int len);
static void game_config_get_bounding_box(struct game_config_widget *widget, int icons, ...);
static void game_config_set_coordinates(struct game_config_widget *widget, int x, int y, int icons, ...);

/**
 * Initialise the game config module.
 */

void game_config_initialise(void)
{
	int x, y;
	char buffer[GAME_CONFIG_MENU_TITLE_LEN], *entry;

	/* Load the menu title from the messages file. */

	entry = msgs_lookup("OptionTitle:Options", buffer, GAME_CONFIG_MENU_TITLE_LEN);
	if (entry == NULL)
		error_msgs_report_fatal("LookupFailedCMenu");

	game_config_popup_menu_title = strdup(entry);

	if (game_config_popup_menu_title == NULL)
		error_msgs_report_fatal("NoMemInitCMenu");

	/* Load the window template. */

	game_config_window_def = templates_load_window("GameConfig");

	game_config_icon_count = game_config_window_def->icon_count;
	game_config_window_def->icon_count = 0;

	/* Initialise the text widget.*/

	game_config_get_bounding_box(&game_config_text_widget, 2,
			GAME_WINDOW_TEMPLATE_ICON_WRITABLE_LABEL,
			GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD
	);

	x = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD].extent.x0;
	y = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD].extent.y0;

	game_config_set_coordinates(&game_config_text_widget, x, y, 2,
			GAME_WINDOW_TEMPLATE_ICON_WRITABLE_LABEL,
			GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD
	);

	game_config_text_widget.field_width =
			(game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD].extent.x1 -
			game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD].extent.x0);

	/* Initialise the combo widget. */

	game_config_get_bounding_box(&game_config_combo_widget, 1,
			GAME_WINDOW_TEMPLATE_ICON_COMBO_LABEL,
			GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD,
			GAME_WINDOW_TEMPLATE_ICON_COMBO_POPUP
	);

	x = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD].extent.x0;
	y = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD].extent.y0;

	game_config_set_coordinates(&game_config_combo_widget, x, y, 1,
			GAME_WINDOW_TEMPLATE_ICON_COMBO_LABEL
	);

	x = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_POPUP].extent.x1;
	y = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD].extent.y0;

	game_config_set_coordinates(&game_config_combo_widget, x, y, 2,
			GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD,
			GAME_WINDOW_TEMPLATE_ICON_COMBO_POPUP
	);

	game_config_combo_widget.pad_width =
			(game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_POPUP].extent.x1 -
			game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD].extent.x1);

	game_config_combo_widget.field_width =
			(game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD].extent.x1 -
			game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD].extent.x0);

	/* Initialise the option widget. */

	game_config_get_bounding_box(&game_config_option_widget, 1,
			GAME_WINDOW_TEMPLATE_ICON_OPTION
	);

	x = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_OPTION].extent.x0;
	y = game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_OPTION].extent.y0;

	game_config_set_coordinates(&game_config_option_widget, x, y, 1,
			GAME_WINDOW_TEMPLATE_ICON_OPTION
	);

	game_config_option_widget.pad_width = GAME_CONFIG_TEXT_LENGTH_MARGIN +
			(game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_OPTION].extent.y1 -
			game_config_window_def->icons[GAME_WINDOW_TEMPLATE_ICON_OPTION].extent.y0);

	/* Initialise the action button widget. */

	game_config_get_bounding_box(&game_config_action_widget, 3,
			GAME_WINDOW_TEMPLATE_ICON_OK,
			GAME_WINDOW_TEMPLATE_ICON_CANCEL,
			GAME_WINDOW_TEMPLATE_ICON_SAVE
	);

	game_config_action_widget.bounding_box.x1 += 8;
	game_config_action_widget.bounding_box.y0 -= 8;
	game_config_action_widget.bounding_box.y1 += 8;

	x = game_config_action_widget.bounding_box.x1;
	y = game_config_action_widget.bounding_box.y0;

	game_config_set_coordinates(&game_config_action_widget, x, y, 3,
			GAME_WINDOW_TEMPLATE_ICON_OK,
			GAME_WINDOW_TEMPLATE_ICON_CANCEL,
			GAME_WINDOW_TEMPLATE_ICON_SAVE
	);
}

/**
 * Create a new Game Config instance and initialise its window.
 *
 * \param type		The type of config data being displayed.
 * \param *config	Pointer to the config data to be displayed.
 * \param *title	Pointer to a title for the config window.
 * \param *pointer	The Wimp pointer location at which to open the window.
 * \param *callback	The function to be called on user completion.
 * \param *data		The client data to be passed to the callback.
 * \return		Pointer to the new instance, or NULL.
 */

struct game_config_block *game_config_create_instance(int type, config_item *config, char *title,
		wimp_pointer *pointer, osbool (*callback)(int, config_item *, enum game_config_outcome, void *), void *data)
{
	struct game_config_block *new = NULL;
	char *help_token = NULL;

	if (config == NULL || pointer == NULL || callback == NULL)
		return NULL;

	new = malloc(sizeof(struct game_config_block));
	if (new == NULL)
		return NULL;

	new->handle = NULL;
	new->window_title = title;

	new->entries = NULL;
	new->entry_count = 0;

	new->config_type = type;
	new->config_data = config;
	new->callback = callback;
	new->client_data = data;

	/* Set the text fields to 1024 bytes if this is a seed or description box. */

	new->field_size = (type == CFG_DESC || type == CFG_SEED) ?
			GAME_WINDOW_DESCRIPTION_FIELD_SIZE : GAME_WINDOW_STANDARD_FIELD_SIZE;

	/* Create the window. */

	if (game_config_build_window(new) == FALSE || new->handle == NULL) {
		game_config_delete_instance(new);
		return NULL;
	}

	/* Register the window events. */

	switch (type) {
	case CFG_DESC:
		help_token = "GameConfigD";
		break;
	case CFG_SEED:
		help_token = "GameConfigR";
		break;
	case CFG_SETTINGS:
		help_token = "GameConfigS";
		break;
	case CFG_PREFS:
		help_token = "GameConfigP";
		break;
	default:
		help_token = "GameConfig";
		break;
	}

	ihelp_add_window(new->handle, help_token, NULL);

	event_add_window_user_data(new->handle, new);
	event_add_window_mouse_event(new->handle, game_config_click_handler);
	event_add_window_key_event(new->handle, game_config_keypress_handler);

	windows_open_centred_at_pointer(new->handle, pointer);

	game_config_set_caret(new);

	return new;
}

/**
 * Close a Game Config window and delete its instance.
 *
 * \param *instance	The instance to be deleted.
 */

void game_config_delete_instance(struct game_config_block *instance)
{
	int i;

	if (instance == NULL)
		return;

	/* Delete the window. */

	if (instance->handle != NULL) {
		ihelp_remove_window(instance->handle);
		event_delete_window(instance->handle);
		wimp_delete_window(instance->handle);
	}

	/* Free the dynamic allocations from the midend, using the provided functions. */

	if (instance->window_title != NULL)
		free(instance->window_title);

	if (instance->config_data != NULL)
		free_cfg(instance->config_data);

	/* Free our own allocations. */

	if (instance->entries != NULL) {
		for (i = 0; i < instance->entry_count; i++) {
			if (instance->entries[i].icon_text != NULL)
				free(instance->entries[i].icon_text);

			if (instance->entries[i].popup_menu != NULL)
				free(instance->entries[i].popup_menu);

			if (instance->entries[i].popup_text != NULL)
				free(instance->entries[i].popup_text);
		}

		free (instance->entries);
	}

	free(instance);
}

/**
 * Handle mouse click events in game config windows.
 *
 * \param *pointer	The Wimp Pointer event data block.
 */

static void game_config_click_handler(wimp_pointer *pointer)
{
	struct game_config_block	*instance;

	instance = event_get_window_user_data(pointer->w);
	if (instance == NULL)
		return;

	if (pointer->i == instance->action_ok) {
		if (pointer->buttons == wimp_CLICK_SELECT)
			game_config_process_user_action(instance, GAME_CONFIG_SET);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			game_config_process_user_action(instance, GAME_CONFIG_SET | GAME_CONFIG_HOLD_OPEN);
	} else if (pointer->i == instance->action_save) {
		if (pointer->buttons == wimp_CLICK_SELECT)
			game_config_process_user_action(instance, GAME_CONFIG_SET | GAME_CONFIG_SAVE);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			game_config_process_user_action(instance, GAME_CONFIG_SET | GAME_CONFIG_SAVE | GAME_CONFIG_HOLD_OPEN);
	} else if (pointer->i == instance->action_cancel) {
		if (pointer->buttons == wimp_CLICK_SELECT)
			game_config_process_user_action(instance, GAME_CONFIG_CANCEL);
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			game_config_copy_to_dialogue(instance);
	}
}

/**
 * Process keypresses in game config.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool game_config_keypress_handler(wimp_key *key)
{
	struct game_config_block	*instance;

	if (key == NULL)
		return FALSE;

	instance = event_get_window_user_data(key->w);
	if (instance == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		game_config_process_user_action(instance, GAME_CONFIG_SET);
		break;

	case wimp_KEY_ESCAPE:
		game_config_process_user_action(instance, GAME_CONFIG_CANCEL);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

/**
 * Process a user action in a Game Config dialogue box.
 *
 * \param *instance	The dialogue box instance.
 * \param outcome	The outcome to be presented to the client.
 */

static void game_config_process_user_action(struct game_config_block *instance, enum game_config_outcome outcome)
{
	osbool midend_response = FALSE;

	if (instance == NULL)
		return;

	if ((outcome & GAME_CONFIG_SET) && game_config_copy_from_dialogue(instance) == FALSE)
		return;

	if (instance->callback != NULL)
		midend_response = instance->callback(instance->config_type, instance->config_data, outcome, instance->client_data);

	/* Delete the parent game config instance. */

	if (!(outcome & GAME_CONFIG_HOLD_OPEN) && midend_response)
		game_config_delete_instance(instance);
}

/**
 * Construct a window for a Game Config instance, calculating the
 * required space for the contents and then creating the icons.
 *
 * On exit, the relevant fields in the instance block structure
 * will have been updated.
 *
 * \param *instance	The Game Config instance to create the
 *			window for.
 * \return		TRUE if the build was successful;
 *			otherwise FALSE.
 */

static osbool game_config_build_window(struct game_config_block *instance)
{
	int left = 0, right = 0, height = 0;
	os_error *error;

	/* Don't create a window for an instance that already has one. */

	if (instance == NULL || instance->handle != NULL)
		return FALSE;

	/* Calculate the required dimensions for the window contents. */

	if (game_config_size_window(instance, &left, &right, &height) == FALSE)
		return FALSE;

	/* Construct the window. */

	game_config_window_def->extent.x0 = 0;
	game_config_window_def->extent.x1 = right + 2 * GAME_WINDOW_INTER_ROW_GAP;
	game_config_window_def->extent.y1 = 0;
	game_config_window_def->extent.y0 = -height;

	game_config_window_def->visible.x0 = 0;
	game_config_window_def->visible.x1 = right + 2 * GAME_WINDOW_INTER_ROW_GAP;
	game_config_window_def->visible.y0 = 0;
	game_config_window_def->visible.y1 = height;

	game_config_window_def->title_data.indirected_text.text = instance->window_title;
	game_config_window_def->title_data.indirected_text.size = strlen(instance->window_title) + 1;

	error = xwimp_create_window(game_config_window_def, &(instance->handle));
	if (error != NULL) {
		instance->handle = NULL;
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return FALSE;
	}

	/* Create the icons within the window.*/

	if (game_config_create_icons(instance, left + GAME_WINDOW_INTER_ROW_GAP, right + GAME_WINDOW_INTER_ROW_GAP) == FALSE)
		return FALSE;

	/* Set the data into the fields. */

	if (game_config_copy_to_dialogue(instance) == FALSE)
		return FALSE;

	return TRUE;
}

/**
 * Calculate the size required by the contents of a Game Config window,
 * returning the details.
 *
 * Whilst the vertical dimension does include the window border areas in
 * its calculation, the horizontal dimensions DO NOT.
 *
 * \param *instance	The Game Config instance to be sized.
 * \param *left		Pointer to a variable in which to return the
 *			horizontal work area position of the left
 *			alignment line (between the labels and the fields).
 * \param *right	Pointer to a variable in which to return the
 *			horizontal work area position of the right
 *			alignment line (to the right of the fields, in effect
 *			the width of the work area).
 * \param *height	Pointer to a variable in which to return the height
 *			of the window work area, in OS units. This will
 *			include the border gap at the top and bottom of the
 *			window.
 * \return		TRUE if successful; otherwise FALSE.
 */

static osbool game_config_size_window(struct game_config_block *instance, int *left, int *right, int *height)
{
	int width = 0, i = 0;

	if (instance == NULL || left == NULL || right == NULL || height == NULL)
		return FALSE;

	/* Calculate the size of the window title. */

	if (xwimptextop_string_width(instance->window_title, 0, &width) != NULL)
		return FALSE;

	width += 100; /* TODO - Allocate space for the close icon properly! */

	/* Calculate the field dimensions. */

	*height += GAME_WINDOW_INTER_ROW_GAP;

	do {
		switch (instance->config_data[i].type) {
		case C_STRING:
			game_config_size_text_field(instance->config_data + i, left, right, height);
			break;
		case C_CHOICES:
			game_config_size_combo_field(instance->config_data + i, left, right, height);
			break;
		case C_BOOLEAN:
			game_config_size_option_field(instance->config_data + i, left, right, height);
			break;
		case C_END:
			game_config_size_action_buttons(&width, height, FALSE);
			break;
		}

		*height += GAME_WINDOW_INTER_ROW_GAP;
	} while (instance->config_data[i++].type != C_END);

	/* Special case the game code dialogues, to make the single text field wider. */

	if (instance->config_type == CFG_DESC || instance->config_type == CFG_SEED) {
		if (*right < GAME_CONFIG_SEED_WIDTH_MULTIPLE * game_config_text_widget.field_width)
			*right = GAME_CONFIG_SEED_WIDTH_MULTIPLE * game_config_text_widget.field_width;
	}

	/* Expand the content side so that the cross-column items will fit. */

	if (*left + *right < width)
		*right = width - *left;

	/* Make both X dimensions relative to the work area origin. */

	*right += *left;

	/* Allocate space for the entry data. */

	instance->entry_count = i - 1;
	instance->entries = malloc(sizeof(struct game_config_entry) * instance->entry_count);
	if (instance->entries == NULL)
		return FALSE;

	for (i = 0; i < instance->entry_count; i++) {
		instance->entries[i].icon_handle = wimp_ICON_WINDOW;
		instance->entries[i].icon_text = NULL;
		instance->entries[i].popup_menu = NULL;
		instance->entries[i].popup_text = NULL;
	}

	return TRUE;
}

/**
 * Calculate the space required for a text widget in a Game Config
 * dialogue.
 *
 * \param *item		Pointer to the item definition for the widget.
 * \param *left		Pointer to a variable holding the horizontal
 *			position of the left alignment line, to be updated
 *			on return to include enough space for any label.
 * \param *right	Pointer to to a variable holding the horizontal
 *			position of the reight alignment line RELATIVE TO
 *			THE LEFT, to be updated on return to include enough
 *			space for any field contents.
 * \param *height	Pointer to a variable containing the height of the
 *			dialogue, to be updated on exit to include the height
 *			required by the widget.
 */

static void game_config_size_text_field(config_item *item, int *left, int *right, int *height)
{
	int w;

	/* Calculate the required label width. */

	if (left != NULL && xwimptextop_string_width(item->name, 0, &w) == NULL && w + GAME_CONFIG_TEXT_LENGTH_MARGIN > *left)
		*left = w + GAME_CONFIG_TEXT_LENGTH_MARGIN;

	/* Calculate the required content width. */

	if (right != NULL && game_config_text_widget.field_width > *right)
		*right = game_config_text_widget.field_width;

	/* Calculate the required content height. */

	if (height != NULL)
		*height += game_config_text_widget.bounding_box.y1 - game_config_text_widget.bounding_box.y0;
}

/**
 * Calculate the space required for a combo widget in a Game Config
 * dialogue.
 *
 * \param *item		Pointer to the item definition for the widget.
 * \param *left		Pointer to a variable holding the horizontal
 *			position of the left alignment line, to be updated
 *			on return to include enough space for any label.
 * \param *right	Pointer to to a variable holding the horizontal
 *			position of the reight alignment line RELATIVE TO
 *			THE LEFT, to be updated on return to include enough
 *			space for any field contents.
 * \param *height	Pointer to a variable containing the height of the
 *			dialogue, to be updated on exit to include the height
 *			required by the widget.
 */

static void game_config_size_combo_field(config_item *item, int *left, int *right, int *height)
{
	int field_width, len, w;
	char *c, *entry_text = NULL, separator;

	/* Calculate the required label width. */

	if (left != NULL && xwimptextop_string_width(item->name, 0, &w) == NULL && w + GAME_CONFIG_TEXT_LENGTH_MARGIN > *left)
		*left = w + GAME_CONFIG_TEXT_LENGTH_MARGIN;

	/* Calculate the required content width, by stepping through the
	 * entries and recording the longest text in OS units.
	 */

	separator = *(item->u.choices.choicenames);

	if (right != NULL) {
		c = (char *) item->u.choices.choicenames;
		field_width = 0;

		while (*c != '\0') {
			entry_text = ++c;	/* The start of the entry. 	*/
			len = 0;		/* The length of the entry.	*/

			/* Find the next separator and count the string length. */

			while (*c != separator && *c != '\0') {
				len++;
				c++;
			}

			/* Find the length of the entry in OS units, and update the count. */

			if (xwimptextop_string_width(entry_text, len, &w) == NULL && w > field_width)
				field_width = w;
		}

		/* Add in the field margin in OS units, to account for space
		 * between borders and text on left and right.
		 */

		field_width += GAME_CONFIG_TEXT_LENGTH_MARGIN;

		/* If the field is less than the one in the templates, round it up. */

		if (field_width < game_config_combo_widget.field_width)
			field_width = game_config_combo_widget.field_width;

		/* Update the space required in the dialogue. */

		if (field_width + game_config_combo_widget.pad_width > *right)
			*right = field_width + game_config_combo_widget.pad_width;
	}

	/* Calculate the required content height. */

	if (height != NULL)
		*height += game_config_combo_widget.bounding_box.y1 - game_config_combo_widget.bounding_box.y0;
}

/**
 * Calculate the space required for an option widget in a Game Config
 * dialogue.
 *
 * \param *item		Pointer to the item definition for the widget.
 * \param *left		Pointer to a variable holding the horizontal
 *			position of the left alignment line, to be updated
 *			on return to include enough space for any label.
 * \param *right	Pointer to to a variable holding the horizontal
 *			position of the reight alignment line RELATIVE TO
 *			THE LEFT, to be updated on return to include enough
 *			space for any field contents.
 * \param *height	Pointer to a variable containing the height of the
 *			dialogue, to be updated on exit to include the height
 *			required by the widget.
 */

static void game_config_size_option_field(config_item *item, int *left, int *right, int *height)
{
	int w;

	/* Calculate the required content width. */

	if (right != NULL && xwimptextop_string_width(item->name, 0, &w) == NULL && w + game_config_option_widget.pad_width > *right)
		*right = w + game_config_option_widget.pad_width;

	if (height != NULL)
		*height += game_config_option_widget.bounding_box.y1 - game_config_option_widget.bounding_box.y0;
}

/**
 * Calculate the space required for the action buttons in a Game Config
 * dialogue.
 *
 * \param *item		Pointer to the item definition for the widget.
 * \param *left		Pointer to a variable holding the horizontal
 *			position of the left alignment line, to be updated
 *			on return to include enough space for any label.
 * \param *right	Pointer to to a variable holding the horizontal
 *			position of the reight alignment line RELATIVE TO
 *			THE LEFT, to be updated on return to include enough
 *			space for any field contents.
 * \param *height	Pointer to a variable containing the height of the
 *			dialogue, to be updated on exit to include the height
 *			required by the widget.
 */

static void game_config_size_action_buttons(int *width, int *height, osbool include_save)
{
	/* Calculate the required content width. */

	if (width != NULL && (game_config_action_widget.bounding_box.x1 - game_config_action_widget.bounding_box.x0 > *width))
		*width = game_config_action_widget.bounding_box.x1 - game_config_action_widget.bounding_box.x0;

	/* Calculate the required content height. */

	if (height != NULL)
		*height += game_config_action_widget.bounding_box.y1 - game_config_action_widget.bounding_box.y0;
}

/**
 * Create the icons in a Game Config dialogue instance.
 *
 * \param *instance	The instance in which to create the icons.
 * \param left		The left horizontal alignment line, between the
 *			labels and the field, in OS units.
 * \param right		The right horizontal alignment line, to the right
 *			of the fields, indicating the width of the work
 *			area.
 * \return		TRUE if successful; otherwise FALSE.
 */

static osbool game_config_create_icons(struct game_config_block *instance, int left, int right)
{
	int i = 0, baseline = 0;

	do {
		baseline -= GAME_WINDOW_INTER_ROW_GAP;

		switch (instance->config_data[i].type) {
		case C_STRING:
			game_config_create_text_widget(instance, i, left, right, &baseline);
			break;
		case C_CHOICES:
			game_config_create_combo_widget(instance, i, left, right, &baseline);
			break;
		case C_BOOLEAN:
			game_config_create_option_widget(instance, i, left, right, &baseline);
			break;
		case C_END:
			game_config_create_action_widget(instance, left, right, &baseline);
		}
	} while (instance->config_data[i++].type != C_END);

	return TRUE;
}

/**
 * Create an text widget in a Game Config dialogue box.
 *
 * \param *instance	The Game Config instance to use.
 * \param left		The horizontal coordinate of the left alignment line.
 * \param right		The horizontal coordinate of the right alignment line.
 * \param *baseline	Pointer to a variable holding the vertical
 *			baseline at the top of the field, to be shifted
 *			down by the height of the row before returning.
 */

static void game_config_create_text_widget(struct game_config_block *instance, int index, int left, int right, int *baseline)
{
	struct game_config_entry *entry = NULL;
	struct config_item *item = NULL;

	if (instance == NULL || baseline == NULL || index >= instance->entry_count)
		return;

	/* Locate the widget definition. */

	item = instance->config_data + index;
	entry = instance->entries + index;

	/* Allocate memory to hold the field data. */

	entry->icon_text = malloc(instance->field_size);
	if (entry->icon_text == NULL)
		return;

	*entry->icon_text = '\0';

	/* Shift the baseline down by the height of the new row. */

	*baseline -= game_config_text_widget.bounding_box.y1 - game_config_text_widget.bounding_box.y0;

	/* Create the icons in the window. */

	game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_WRITABLE_LABEL, GAME_WINDOW_INTER_ROW_GAP, -1, left, *baseline, (char *) item->name, 0);
	entry->icon_handle = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_WRITABLE_FIELD, -1, right, left, *baseline, entry->icon_text, instance->field_size);
}

/**
 * Create a combo widget in a Game Config dialogue box.
 *
 * \param *instance	The Game Config instance to use.
 * \param left		The horizontal coordinate of the left alignment line.
 * \param right		The horizontal coordinate of the right alignment line.
 * \param *baseline	Pointer to a variable holding the vertical
 *			baseline at the top of the field, to be shifted
 *			down by the height of the row before returning.
 */

static void game_config_create_combo_widget(struct game_config_block *instance, int index, int left, int right, int *baseline)
{
	struct game_config_entry *entry = NULL;
	struct config_item *item = NULL;
	int entries = 0, i;
	size_t entry_length, field_length = 0;
	char *c, *entry_text, separator;
	wimp_i field_icon_handle;

	if (instance == NULL || baseline == NULL || index >= instance->entry_count)
		return;

	/* Locate the widget definition. */

	item = instance->config_data + index;
	entry = instance->entries + index;

	/* Copy the item definitions so that we can insert terminators. */

	entry->popup_text = strdup(item->u.choices.choicenames);
	if (entry->popup_text == NULL)
		return;

	/* The separator is the first character in the string. */

	separator = *(entry->popup_text);

	/* Count the entries in the string. */

	c = entry->popup_text;

	while (*c != '\0') {
		if (*c++ == separator)
			entries++;
	}

	/* Build the menu structure. */

	entry->popup_menu = menus_build_menu(game_config_popup_menu_title, FALSE, entries);
	if (entry->popup_menu == NULL)
		return;

	/* Terminate and extract the entries from the text. */

	c = entry->popup_text;

	for (i = 0; i < entries; i++) {
		/* Step past the current separator and store the text start. */

		entry_text = ++c;

		/* Find the next separator and terminate the entry. */

		while (*c != separator && *c != '\0')
			c++;

		*c = '\0';

		/* Set up the menu entry and track the longest text needed for the field. */

		entry_length = strlen(entry_text);

		menus_build_entry(entry->popup_menu, i, entry_text, entry_length, MENUS_SEPARATOR_NONE, NULL);

		if (entry_length + 1 > field_length)
			field_length = entry_length + 1;
	}

	/* Allocate the memory required for the field icon text. */

	entry->icon_text = malloc(field_length);
	if (entry->icon_text == NULL)
		return;

	/* Shift the baseline down by the height of the new row. */

	*baseline -= game_config_text_widget.bounding_box.y1 - game_config_text_widget.bounding_box.y0;

	/* Create the icons in the window. */

	game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_COMBO_LABEL, GAME_WINDOW_INTER_ROW_GAP, -1, left, *baseline, (char *) item->name, 0);
	field_icon_handle = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_COMBO_FIELD, left, -1, right, *baseline, entry->icon_text, field_length);
	entry->icon_handle = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_COMBO_POPUP, -1, -1, right, *baseline, NULL, 0);


	event_add_window_icon_popup(instance->handle, entry->icon_handle, entry->popup_menu, field_icon_handle, NULL);
}

/**
 * Create an option widget in a Game Config dialogue box.
 *
 * \param *instance	The Game Config instance to use.
 * \param left		The horizontal coordinate of the left alignment line.
 * \param right		The horizontal coordinate of the right alignment line.
 * \param *baseline	Pointer to a variable holding the vertical
 *			baseline at the top of the field, to be shifted
 *			down by the height of the row before returning.
 */

static void game_config_create_option_widget(struct game_config_block *instance, int index, int left, int right, int *baseline)
{
	struct game_config_entry *entry = NULL;
	struct config_item *item = NULL;

	if (instance == NULL || baseline == NULL || index >= instance->entry_count)
		return;

	/* Locate the widget definition. */

	item = instance->config_data + index;
	entry = instance->entries + index;

	/* Shift the baseline down by the height of the new row. */

	*baseline -= game_config_option_widget.bounding_box.y1 - game_config_option_widget.bounding_box.y0;

	/* Create the icons in the window. */

	entry->icon_handle = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_OPTION, -1, right, left, *baseline, (char *) item->name, 0);
}

/**
 * Create the action widgets in a Game Config dialogue box.
 *
 * \param *instance	The Game Config instance to use.
 * \param left		The horizontal coordinate of the left alignment line.
 * \param right		The horizontal coordinate of the right alignment line.
 * \param *baseline	Pointer to a variable holding the vertical
 *			baseline at the top of the field, to be shifted
 *			down by the height of the row before returning.
 */

static void game_config_create_action_widget(struct game_config_block *instance, int left, int right, int *baseline)
{
	if (instance == NULL || baseline == NULL)
		return;

	/* Shift the baseline down by the height of the new row. */

	*baseline -= game_config_action_widget.bounding_box.y1 - game_config_action_widget.bounding_box.y0;

	/* Create the icons in the window. Only preferences support saving. */

	instance->action_ok = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_OK, -1, -1, right, *baseline, NULL, 0);
	if (instance->config_type == CFG_PREFS)
		instance->action_save = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_SAVE, -1, -1, right, *baseline, NULL, 0);
	else
		instance->action_save = wimp_ICON_WINDOW;
	instance->action_cancel = game_config_create_icon(instance->handle, GAME_WINDOW_TEMPLATE_ICON_CANCEL, -1, -1, right, *baseline, NULL, 0);
}

/**
 * Set the caret in the first writable field of a Game Config
 * window or, failing that, into the window work area.
 *
 * \param *instance	The Game Config instance to take the
 *			caret.
 */

static void game_config_set_caret(struct game_config_block *instance)
{
	int i;
	wimp_i target_icon = wimp_ICON_WINDOW;

	if (instance == NULL)
		return;

	for (i = 0; i < instance->entry_count; i++) {
		if (instance->config_data[i].type == C_STRING) {
			target_icon = instance->entries[i].icon_handle;
			break;
		}
	}

	icons_put_caret_at_end(instance->handle, target_icon);
}

/**
 * Update the fields in a Game Config window to reflect the stored
 * data in the underlying config_item list.
 *
 * \param *instance	The Game Config instance to refresh.
 * \return		TRUE if successful; FALSE on failure.
 */

static osbool game_config_copy_to_dialogue(struct game_config_block *instance)
{
	int i;
	osbool refresh;
	config_item *item;
	struct game_config_entry *entry;

	if (instance == NULL || instance->handle == NULL)
		return FALSE;

	refresh = windows_get_open(instance->handle);

	for (i = 0; i < instance->entry_count; i++) {
		item = instance->config_data + i;
		entry = instance->entries + i;

		switch (item->type) {
		case C_BOOLEAN:
			icons_set_selected(instance->handle, entry->icon_handle, (osbool) item->u.boolean.bval);
			break;

		case C_STRING:
			icons_strncpy(instance->handle, entry->icon_handle, item->u.string.sval);
			if (refresh == TRUE)
				wimp_set_icon_state(instance->handle, entry->icon_handle, 0, 0);
			break;

		case C_CHOICES:
			event_set_window_icon_popup_selection(instance->handle, entry->icon_handle, item->u.choices.selected);
			break;
		}
	}

	if (refresh)
		icons_replace_caret_in_window(instance->handle);

	return TRUE;
}

/**
 * Update the data in the undelying config_item list to reflect the
 * contents of the fields in the associated Game Config window.
 *
 * \param *instance	The Game Config instance to refresh.
 * \return		TRUE if successful; FALSE on failure.
 */

static osbool game_config_copy_from_dialogue(struct game_config_block *instance)
{
	int i;
	config_item *item;
	struct game_config_entry *entry;

	if (instance == NULL || instance->handle == NULL)
		return FALSE;

	for (i = 0; i < instance->entry_count; i++) {
		item = instance->config_data + i;
		entry = instance->entries + i;

		switch (item->type) {
		case C_BOOLEAN:
			item->u.boolean.bval = (bool) icons_get_selected(instance->handle, entry->icon_handle);
			break;

		case C_STRING:
			if (item->u.string.sval != NULL)
				free(item->u.string.sval);

			item->u.string.sval = strdup(entry->icon_text);
			break;

		case C_CHOICES:
			item->u.choices.selected = event_get_window_icon_popup_selection(instance->handle, entry->icon_handle);
			break;
		}
	}

	return TRUE;
}
/**
 * Create an icon within a config window.
 *
 * On the Y axis, icons will always be plotted relative to the baseline.
 * On the X axis, one of both of their edges may be plotted relative to
 * the supplied centreline, or they may be plotted at updated work area
 * coordinates.
 *
 * \param handle	The handle of the parent window.
 * \param icon		The index of the icon definition within the
 *			window template.
 * \param x0		The work area coordinate of the X0 edge of the icon.
 *			pass as < 0 to plot relative to the centreline.
 * \param x1		The work area coordinate of the X1 edge of the icon.
 *			pass as < 0 to plot relative to the centreline.
 * \param centreline	The work area coordinate of the horizontal template
 *			centre line.
 * \param baseline	The work area coordinate of the vertical template
 *			base line.
 * \param *text		Pointer to a text buffer to use for the icon contents;
 *			set to NULL to use the icon's template content.
 * \param len		The length of the supplied text buffer, if *text is
 *			not NULL. Set to <= 0 to calculate the length from the
 *			supplied buffer contents.
 * \return		The Wimp icon handle, or -1 on failure.
 */

static wimp_i game_config_create_icon(wimp_w handle, wimp_i icon, int x0, int x1, int centreline, int baseline, char *text, int len)
{
	wimp_icon_create create;
	wimp_i i;
	os_error *error;

	if (icon < 0 || icon >= game_config_icon_count)
		return wimp_ICON_WINDOW;

	/* Set up the icon definition block. */

	create.w = handle;

	create.icon.extent.x0 = (x0 >= 0) ? x0 : centreline + game_config_window_def->icons[icon].extent.x0;
	create.icon.extent.y0 = baseline + game_config_window_def->icons[icon].extent.y0;
	create.icon.extent.x1 = (x1 >= 0) ? x1 : centreline + game_config_window_def->icons[icon].extent.x1;
	create.icon.extent.y1 = baseline + game_config_window_def->icons[icon].extent.y1;

	create.icon.flags = game_config_window_def->icons[icon].flags;

	/* Arrange the icon content. */

	if (text == NULL) {
		create.icon.data.indirected_text.text = game_config_window_def->icons[icon].data.indirected_text.text;
		create.icon.data.indirected_text.size = game_config_window_def->icons[icon].data.indirected_text.size;
		create.icon.data.indirected_text.validation = game_config_window_def->icons[icon].data.indirected_text.validation;
	} else {
		create.icon.data.indirected_text.text = text;
		create.icon.data.indirected_text.size = (len > 0) ? len : strlen(text) + 1;

		/* If we're making a previously non-indirected icon indirected,
		 * set the validation and flags. Otherwise, just copy ove the
		 * existing validation string.
		 */

		if (create.icon.flags & wimp_ICON_INDIRECTED) {
			create.icon.data.indirected_text.validation = game_config_window_def->icons[icon].data.indirected_text.validation;
		} else {
			create.icon.data.indirected_text.validation = NULL;
			create.icon.flags |= wimp_ICON_INDIRECTED;
		}
	}

	/* Create the icon. */

	error = xwimp_create_icon(&create, &i);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return wimp_ICON_WINDOW;
	}

	return i;
}

/**
 * Calculate the bounding box for a set of icons comprising a widget.
 * On exit, the bounding box in the widget definition will be updated.
 *
 * \param *widget	Pointer to the widget set containing the icon.
 * \param x		The X work area coordinate of the widget origin.
 * \param y		The Y work area coordinate of the widget origin.
 * \param icons		The number of icons in the widget.
 * \param ...		The collection of icon handles.
 */

static void game_config_get_bounding_box(struct game_config_widget *widget, int icons, ...)
{
	wimp_i i, icon;
	va_list ap;

	if (widget == NULL)
		return;

	widget->bounding_box.x0 = 0;
	widget->bounding_box.y0 = 0;
	widget->bounding_box.x1 = 0;
	widget->bounding_box.y1 = 0;

	widget->field_width = 0;
	widget->pad_width = 0;

	va_start(ap, icons);

	for (i = 0; i < icons; i++) {
		icon = va_arg(ap, wimp_i);
		if (icon < 0 || icon >= game_config_icon_count)
			continue;

		if (widget->bounding_box.x0 == widget->bounding_box.x1) {
			widget->bounding_box.x0 = game_config_window_def->icons[icon].extent.x0;
			widget->bounding_box.x1 = game_config_window_def->icons[icon].extent.x1;
		} else {
			if (game_config_window_def->icons[icon].extent.x0 < widget->bounding_box.x0)
				widget->bounding_box.x0 = game_config_window_def->icons[icon].extent.x0;

			if (game_config_window_def->icons[icon].extent.x1 > widget->bounding_box.x1)
				widget->bounding_box.x1 = game_config_window_def->icons[icon].extent.x1;
		}

		if (widget->bounding_box.y0 == widget->bounding_box.y1) {
			widget->bounding_box.y0 = game_config_window_def->icons[icon].extent.y0;
			widget->bounding_box.y1 = game_config_window_def->icons[icon].extent.y1;
		} else {
			if (game_config_window_def->icons[icon].extent.y0 < widget->bounding_box.y0)
				widget->bounding_box.y0 = game_config_window_def->icons[icon].extent.y0;

			if (game_config_window_def->icons[icon].extent.y1 > widget->bounding_box.y1)
				widget->bounding_box.y1 = game_config_window_def->icons[icon].extent.y1;
		}
	}

	va_end(ap);
}

/**
 * Adjust the coordinates for the icon template so that they are
 * relative to the origin for the widget sat that it belongs to.
 *
 * \param *widget	Pointer to the widget set containing the icon.
 * \param x		The X work area coordinate of the widget origin.
 * \param y		The Y work area coordinate of the widget origin.
 * \param icons		The number of icons in the widget.
 * \param ...		The collection of icon handles.
 */

static void game_config_set_coordinates(struct game_config_widget *widget, int x, int y, int icons, ...)
{
	wimp_i i, icon;
	va_list ap;

	if (widget == NULL)
		return;

	va_start(ap, icons);

	for (i = 0; i < icons; i++) {
		icon = va_arg(ap, wimp_i);
		if (icon < 0 || icon >= game_config_icon_count)
			continue;

		game_config_window_def->icons[icon].extent.x0 -= x;
		game_config_window_def->icons[icon].extent.y0 -= y;
		game_config_window_def->icons[icon].extent.x1 -= x;
		game_config_window_def->icons[icon].extent.y1 -= y;
	}

	va_end(ap);
}
