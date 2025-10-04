/* Copyright 2025, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: clipboard.c
 *
 * Global Clipboard implementation.
 */

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* OSLibSupport header files */

/* SF-Lib header files. */

#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"

/* Application header files */

#include "main.h"
#include "clipboard.h"

/* ANSI C header files */

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------------------------------------------------ */

/* Declare the global variables that are used. */

static char		*clipboard_data = NULL;					/**< Clipboard data help by CashBook, or NULL for none.			*/
static size_t		clipboard_length = 0;					/**< The length of the clipboard held by CashBook.			*/

static osbool		clipboard_store_text(char *text, size_t len);
static osbool		clipboard_message_claimentity(wimp_message *message);
static size_t		clipboard_send_data(bits types[], bits *type, void **data);


/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void)
{
	event_add_message_handler(message_CLAIM_ENTITY, EVENT_MESSAGE_INCOMING, clipboard_message_claimentity);
	dataxfer_register_clipboard_provider(clipboard_send_data);
}

/**
 * Copy a text string to the clipboard. A NULL pointer or an empty string will
 * not be copied.
 *
 * \param *text			The text to be copied.
 * \return			TRUE if succesful; FLASE on failure.
 */

osbool clipboard_copy_text(char *text)
{
	size_t length;

	if (text == NULL)
		return TRUE;

	length = strlen(text);
	if (length == 0)
		return TRUE;

	return clipboard_store_text(text, length);
}

/**
 * Store a piece of text on the clipboard, claiming it in the process.
 *
 * \param *text			The text to be stored.
 * \param len			The length of the text.
 * \return			TRUE if claimed; else FALSE.
 */

static osbool clipboard_store_text(char *text, size_t len)
{
	wimp_full_message_claim_entity	claimblock;
	os_error			*error;

	/* If we already have the clipboard, clear it first. */

	if (clipboard_data != NULL) {
		free(clipboard_data);
		clipboard_data = NULL;
		clipboard_length = 0;
	}

	/* Record the details of the text in our own variables. */

	clipboard_data = malloc(len);
	if (clipboard_data == NULL) {
		error_msgs_report_error("ClipAllocFail");
		return FALSE;
	}

	memcpy(clipboard_data, text, len);
	clipboard_length = len;

	/* Send out Message_CliamClipboard. */

	claimblock.size = 24;
	claimblock.your_ref = 0;
	claimblock.action = message_CLAIM_ENTITY;
	claimblock.flags = wimp_CLAIM_CLIPBOARD;

	error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) &claimblock, wimp_BROADCAST);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		free(clipboard_data);
		clipboard_data = NULL;
		clipboard_length = 0;

		return FALSE;
	}

	return TRUE;
}


/**
 * Handle incoming Message_ClainEntity, by dropping the clipboard if we
 * currently own it.
 *
 * \param *message		The message data to be handled.
 * \return			TRUE to claim the message; FALSE to pass it on.
 */

static osbool clipboard_message_claimentity(wimp_message *message)
{
	wimp_full_message_claim_entity	*claimblock = (wimp_full_message_claim_entity *) message;

	/* Unset the contents of the clipboard if the claim was for that. */

	if ((clipboard_data != NULL) && (claimblock->sender != main_task_handle) && (claimblock->flags & wimp_CLAIM_CLIPBOARD)) {
		free(clipboard_data);
		clipboard_data = NULL;
		clipboard_length = 0;
	}

	return TRUE;
}


/**
 * Handle requests from other tasks for the clipboard data by checking to see
 * if we currently own it and whether any of the requested types are ones that
 * we can support. If we can supply the data, copy it into a heap block and
 * pass it to the dataxfer code to process.
 *
 * \param types[]		A list of acceptable filetypes for the data.
 * \param *type			Pointer to a variable to take the chosen type.
 * \param **data		Pointer to a pointer to take the address of the data.
 * \return			The number of bytes offered, or 0 if we can't help.
 */

static size_t clipboard_send_data(bits types[], bits *type, void **data)
{
	int	i = 0;

	/* If we don't own the clipboard, return no data. */

	if (clipboard_data == NULL || types == NULL || type == NULL || data == NULL)
		return 0;

	/* Check the list of acceptable types to see if there's one we
	 * like.
	 */

	while (types[i] != -1) {
		if (types[i] == osfile_TYPE_TEXT)
			break;
	}

	if (types[i] == -1)
		return 0;

	*type = osfile_TYPE_TEXT;

	/* Make a copy of the clipboard using the static heap known to
	 * dataxfer, then return a pointer. This will be freed by dataxfer
	 * once the transfer is complete.
	 */

	*data = malloc(clipboard_length);
	if (*data == NULL)
		return 0;

	memcpy(*data, clipboard_data, clipboard_length);

	return clipboard_length;
}
