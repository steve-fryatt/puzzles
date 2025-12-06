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
 * \file: unicode.h
 *
 * Unicode conversion external interface.
 */

#ifndef PUZZLES_UNICODE
#define PUZZLES_UNICODE

#include "oslib/types.h"

/**
 * Attempt to convert a Unicode string into something that can be displayed
 * using the current Alphabet.
 *
 * In keeping with the expectations of the upstream API, the returned
 * string is allocated by malloc() -- the client is expected to free() it
 * once it is no longer required.
 *
 * \param *original	Pointer to the original Unicode string, in UTF-8 format.
 * \param force		True to return a string whether or not a valid
 *			conversion could be made.
 * \return		Pointer to a converted string, allocated using
 *			malloc(), or NULL on failure.
 */

char *unicode_convert(const char *original, osbool force);

#endif