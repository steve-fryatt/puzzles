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
 * \file: help.h
 *
 * Help Interface external interface.
 */

#ifndef PUZZLES_HELP
#define PUZZLES_HELP

/**
 * Initialise the help resources.
 *
 * \param *resources	The resources path data from initialisation.
 */

void help_initialise(char *resources);

/**
 * Attempt to launch the application help document.
 * 
 * \param *tag		The manual tag to target, or NULL for the
 *			top of the document.
 */

void help_launch(char *tag);

#endif

