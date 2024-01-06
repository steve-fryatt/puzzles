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
 * \file: main.h
 *
 * Core program code and resource loading.
 */

#ifndef PUZZLES_MAIN
#define PUZZLES_MAIN


/**
 * Application-wide global variables.
 */

extern wimp_t			main_task_handle;
extern int			main_quit_flag;
extern osspriteop_area		*main_wimp_sprites;

/**
 * Main code entry point.
 */

int main(int argc, char *argv[]);

#endif

