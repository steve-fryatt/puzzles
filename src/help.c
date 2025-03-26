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

#include "oslib/osfile.h"

/* SF-Lib header files. */

#include "sflib/dataxfer.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/string.h"

/* Application header files */

#include "help.h"

/* Constants */

/**
 * The length of a filename buffer.
 */

#define HELP_FILENAME_BUFFER_LEN 1024

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
 *
 * \param *resources	The resources path data from initialisation.
 */

void help_initialise(char *resources)
{
	resources_find_file(resources, help_file_text, HELP_FILENAME_BUFFER_LEN, "HelpText", osfile_TYPE_TEXT);
	resources_find_file(resources, help_file_html, HELP_FILENAME_BUFFER_LEN, "HelpHTML", dataxfer_TYPE_HTML);
}

/**
 * Attempt to launch the application help document.
 * 
 * \param *tag		The manual tag to target, or NULL for the
 *			top of the document.
 */

void help_launch(char *tag)
{
	os_cli("%Filer_Run <Puzzles$Dir>.!Help");
}
