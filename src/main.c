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
 * \file: main.c
 *
 * Core program code and resource loading.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/osfile.h"
#include "oslib/hourglass.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/resources.h"
#include "sflib/ihelp.h"
#include "sflib/url.h"
#include "sflib/msgs.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/string.h"
#include "sflib/event.h"
#include "sflib/templates.h"

/* Application header files */

#include "main.h"

#include "frontend.h"
#include "index_window.h"
#include "game_window.h"
#include "game_window_backend_menu.h"
#include "game_config.h"
#include "iconbar.h"
#include "sprites.h"

/* ------------------------------------------------------------------------------------------------------------------ */

/**
 * The size of buffer allocated to resource filename processing.
 */

#define MAIN_FILENAME_BUFFER_LEN 1024

/**
 * The size of buffer allocated to the task name.
 */

#define MAIN_TASKNAME_BUFFER_LEN 64

static void	main_poll_loop(void);
static void	main_initialise(void);
static osbool	main_message_quit(wimp_message *message);


/*
 * Cross file global variables
 */

wimp_t			main_task_handle;
int			main_quit_flag = FALSE;


/**
 * Main code entry point.
 */

int main(int argc, char *argv[])
{
	main_initialise();

	main_poll_loop();

	msgs_terminate();
	wimp_close_down(main_task_handle);

	return 0;
}


/**
 * Wimp Poll loop.
 */

static void main_poll_loop(void)
{
	wimp_poll_flags	flags;
	wimp_event_no	reason;
	wimp_block	blk;
	os_t		next_poll;

	next_poll = os_read_monotonic_time();

	while (!main_quit_flag) {
		flags = (next_poll != 0) ? 0 : wimp_MASK_NULL;

		reason = wimp_poll_idle(flags, &blk, next_poll, NULL);

		/* Events are passed to Event Lib first; only if this fails
		 * to handle them do they get passed on to the central
		 * inline handlers here.
		 */

		if (!event_process_event(reason, &blk, 0, &next_poll)) {
			switch (reason) {
			case wimp_OPEN_WINDOW_REQUEST:
				wimp_open_window(&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				wimp_close_window(blk.close.w);
				break;

			case wimp_KEY_PRESSED:
				wimp_process_key(blk.key.c);
				break;
			}
		}
	}
}


/**
 * Application initialisation.
 */

static void main_initialise(void)
{
	static char		task_name[MAIN_TASKNAME_BUFFER_LEN];
	char			resources[MAIN_FILENAME_BUFFER_LEN], res_temp[MAIN_FILENAME_BUFFER_LEN];
	osspriteop_area		*sprites;


	hourglass_on();

	/* Initialise the resources. */

	string_copy(resources, "<Puzzles$Dir>.Resources", MAIN_FILENAME_BUFFER_LEN);
	if (!resources_initialise_paths(resources, MAIN_FILENAME_BUFFER_LEN, "Puzzles$Language", "UK"))
		error_report_fatal("Failed to initialise resources.");

	/* Load the messages file. */

	if (!resources_find_file(resources, res_temp, MAIN_FILENAME_BUFFER_LEN, "Messages", osfile_TYPE_TEXT))
		error_report_fatal("Failed to locate suitable Messages file.");

	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName", "TaskSpr", NULL);

	/* Initialise with the Wimp. */

	msgs_lookup("TaskName", task_name, sizeof (task_name));
	main_task_handle = wimp_initialise(wimp_VERSION_RO3, task_name, NULL, NULL);

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);

	/* Initialise the configuration. */

	config_initialise(task_name, "Puzzles", "<Puzzles$Dir>");

	config_load();

	/* Load the menu structure. */

	if (!resources_find_file(resources, res_temp, MAIN_FILENAME_BUFFER_LEN, "Menus", osfile_TYPE_DATA))
		error_msgs_param_report_fatal("BadResource", "Menus", NULL, NULL, NULL);

	templates_load_menus(res_temp);

	/* Load the window templates. */

	if (!resources_find_file(resources, res_temp, MAIN_FILENAME_BUFFER_LEN, "Templates", osfile_TYPE_TEMPLATE))
		error_msgs_param_report_fatal("BadResource", "Templates", NULL, NULL, NULL);

	templates_open(res_temp);

	/* Load the application sprites. */

	sprites = resources_load_user_sprite_area("<Puzzles$Dir>.Sprites");
	if (sprites == NULL)
		error_msgs_report_fatal("NoSprites");

	sprites_initialise(sprites);

	/* Initialise the individual modules. */

	ihelp_initialise();
	dataxfer_initialise(main_task_handle, NULL);
	iconbar_initialise();
	index_window_initialise();
	game_window_initialise();
	game_window_backend_menu_initialise();
	game_config_initialise();
	frontend_initialise();
	url_initialise();

	templates_close();

	hourglass_off();
}


/**
 * Handle incoming Message_Quit.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}
