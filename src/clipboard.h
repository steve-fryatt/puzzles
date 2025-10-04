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
 * \file: clipboard.h
 *
 * Global Clipboard implementation.
 */

#ifndef PUZZLES_CLIPBOARD
#define PUZZLES_CLIPBOARD

/**
 * Initialise the Clipboard module.
 */

void clipboard_initialise(void);

/**
 * Copy a text string to the clipboard. A NULL pointer or an empty string will
 * not be copied.
 *
 * \param *text			The text to be copied.
 * \return			TRUE if succesful; FLASE on failure.
 */

osbool clipboard_copy_text(char *text);

#endif
