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
 * \file: index_window.h
 *
 * Index window external interface.
 */

#ifndef PUZZLES_INDEX_WINDOW
#define PUZZLES_INDEX_WINDOW


/**
 * Initialise the index window and its associated menus and dialogues.
 */

void index_window_initialise(void);

/**
 * (Re-)open the Index window on screen. 
 */

void index_window_open(void);

#endif
