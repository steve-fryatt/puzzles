/* Copyright 2025, Stephen Fryatt
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
 * \file: help.c
 *
 * Help interface implementation.
 */

/* ANSI C header files */

#include <stddef.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/osfscontrol.h"

/* SF-Lib header files. */

#include "sflib/dataxfer.h"
#include "sflib/errors.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/string.h"
#include "sflib/url.h"

/* Application header files */

#include "help.h"

/* Constants */

/**
 * The length of a filename buffer.
 */

#define HELP_FILENAME_BUFFER_LEN 1024

/**
 * The length of a command buffer: eg. *Filer_Run <filename>
 */

#define HELP_COMMAND_BUFFER_LEN (HELP_FILENAME_BUFFER_LEN + 128)

/* Global Variables */

/**
 * The textual HELP file.
 */

static char help_file_text[HELP_FILENAME_BUFFER_LEN];

/**
 * The HTML HELP file.
 */

static char help_file_html[HELP_FILENAME_BUFFER_LEN];

/**
 * Initialise the help resources.
 *Alias$@RunType_faf
 * \param *resources	The resources path data from initialisation.
 */

void help_initialise(char *resources)
{
	char html_name[HELP_FILENAME_BUFFER_LEN];

	resources_find_file(resources, help_file_text, HELP_FILENAME_BUFFER_LEN, "HelpText", osfile_TYPE_TEXT);
	resources_find_file(resources, html_name, HELP_FILENAME_BUFFER_LEN, "HelpHTML", dataxfer_TYPE_HTML);

	/* If an HTML file was found, perform some processing on it to get it
	 * into a format that AcornURI will be happy to accept.
	 */

	if (*html_name != '\0') {
		os_error *error = NULL;
		osbool in_drivespec = TRUE;

		/* Canonicalise the name to remove any system variables. */

		error = xosfscontrol_canonicalise_path(html_name, help_file_html, NULL, NULL, HELP_FILENAME_BUFFER_LEN, NULL);
		if (error != NULL)
			error_report_program(error);

		/* Convert into URI format. */

		for (int i = 0; i < HELP_FILENAME_BUFFER_LEN && help_file_html[i] != '\0'; i++) {
			switch (help_file_html[i]) {
			case '$':
				in_drivespec = FALSE;
				break;
			case '.':
				if (in_drivespec == FALSE)
					help_file_html[i] = '/';
				break;
			case '/':
				if (in_drivespec == FALSE)
					help_file_html[i] = '.';
				break;
			}
		}
	} else {
		*help_file_html = '\0';
	}
}

/**
 * Attempt to launch the application help document.
 * 
 * \param *tag		The manual tag to target, or NULL for the
 *			top of the document.
 */

void help_launch(const char *tag)
{
	char launch_buffer[HELP_COMMAND_BUFFER_LEN];
	int var_size = 0;
	os_error *error;

	/* Check to see if there's something registered to handle HTML files. */

	error = xos_read_var_val_size("Alias$@RunType_faf", 0, os_VARTYPE_STRING, &var_size, NULL, NULL);
	if (error != NULL)
		error_report_program(error);

	/* If there's a browser and an HTML help file, try to launch that. Else
	 * launch a text file if there's one of those. Otherwise error.
	 */

	if (var_size != 0 && *help_file_html != '\0') {
		/* Using HTML help. */

		if (tag != NULL)
			string_printf(launch_buffer, HELP_COMMAND_BUFFER_LEN, "file:///%s#%s", help_file_html, tag);
		else
			string_printf(launch_buffer, HELP_COMMAND_BUFFER_LEN, "file:///%s", help_file_html);
		url_launch(launch_buffer);
	} else if (*help_file_text != '\0') {
		/* Fall back to Text help. */

		string_printf(launch_buffer, HELP_COMMAND_BUFFER_LEN, "%%Filer_Run %s", help_file_text);
		os_cli(launch_buffer);
	} else {
		/* There's no suitable help. */

		error_msgs_report_error("NoHelp");
	}
}
