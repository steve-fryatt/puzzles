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
 * \file: unicode.c
 *
 * Unicode implementation.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/serviceinternational.h"
#include "oslib/territory.h"
#include "oslib/types.h"
#include "oslib/osbyte.h"

/* SF-Lib header files. */

#include "sflib/string.h"

/* Application header files */

#include "unicode.h"

/**
 * An entry in the character encoding table.
 */

struct unicode_map_entry {
	int		utf8;
	unsigned char	target;
};

/**
 * The first character in an alphabet that doesn't map to a 7-bit UTF-8 character.
 */

#define UNICODE_FIRST_ENTRY 128

/**
 * The number of characters in an alphabet.
 */

#define UNICODE_MAX_ENTRIES 256

/**
 * The number of mapped UTF-8 characters in the current alphabet.
 */

static size_t unicode_current_map_entries = 0;

/**
 * The mapping table.
 */

static struct unicode_map_entry unicode_map[UNICODE_MAX_ENTRIES];

/**
 * The current alphabet number. Initialise to zero to force a rebuild of the
 * table on first call to unicode_convert().
 */

static int unicode_current_alphabet = 0;

/* Static Function Definitions */

static osbool unicode_build_table(int alphabet);
static int unicode_parse_utf8_string(char **text);
static osbool unicode_find_mapped_character(int unicode, char *c);


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

char *unicode_convert(const char *original, osbool force)
{
	int alphabet = 0;

	/* Check the current alphabet and initialise the mapping table. */

	alphabet = osbyte1(osbyte_ALPHABET_NUMBER, 127, 0);

	if (alphabet != unicode_current_alphabet) {
		unicode_build_table(alphabet);
		unicode_current_alphabet = alphabet;
	}

	/* If we have the UTF-8 alhpabet, we can just return the original
	 * string as there's nothing to be converted.
	 */

	if (alphabet == territory_ALPHABET_UTF8)
		return strdup(original);

	/* Parse the string, to see if we can process it. */

	char *old = (char *) original;
	size_t length = 1;
	osbool text_ok = TRUE;

	while (*old != '\0') {
		int codepoint = unicode_parse_utf8_string(&old);

		if (unicode_find_mapped_character(codepoint, NULL) == FALSE)
			text_ok = FALSE;

		length++;
	}

	/* If any code points could not be converted, then unless we're being
	 * forced to return something whether or not it is correct, return
	 * a fail.
	 */

	if (text_ok == FALSE && force == FALSE)
		return NULL;

	/* Allocate some memory for the new string, and translate the UTF-8
	 * into the current alphabet encoding.
	 */

	char *new = malloc(length);
	if (new == NULL)
		return NULL;

	old = (char *) original;
	int i = 0;

	while (*old != '\0' && i < length) {
		int codepoint = unicode_parse_utf8_string(&old);
		unicode_find_mapped_character(codepoint, new + i);
	}

	new[length - 1] = '\0';

	return new;
}

/**
 * Build a lookup table for a given alphabet from the OS UCS conversion table.
 *
 * \param alphabet	The alphabet to look up.
 * \return		TRUE if successful; else FALSE.
 */

static osbool unicode_build_table(int alphabet)
{
	osbool copied[UNICODE_MAX_ENTRIES] = { FALSE };
	int start = UNICODE_FIRST_ENTRY, end = UNICODE_MAX_ENTRIES;
	unsigned *ucs_table = NULL;

	/* Reset the table */

	unicode_current_map_entries = 0;

	/* There's no table for UTF-8. */

	if (alphabet == territory_ALPHABET_UTF8)
		return TRUE;

	/* Look for the conversion table that we need. */

	if (serviceinternational_get_ucs_conversion_table(alphabet, (void **) &ucs_table) != 0 || ucs_table == NULL)
		return TRUE;

	/* Copy the mapping into our lookup table, sorted by Unicode codepoint. */

	while (start < end) {
		int next = start;

		/* Find the next lowest codepoint in the table that hasn't been taken. */

		for (int i = start; i < end; i++) {
			if (ucs_table[i] < ucs_table[next] && copied[i] == FALSE)
				next = i;
		}

		/* If the codepoint isn't 'none', add it to the table. */

		if (ucs_table[next] != 0xffffffffu) {
			unicode_map[unicode_current_map_entries].utf8 = ucs_table[next];
			unicode_map[unicode_current_map_entries].target = next;
			unicode_current_map_entries++;
		}

		copied[next] = TRUE;

		/* Move the start and end points in if we can, to minimise the search space. */

		if (next == start) {
			while (start < UNICODE_MAX_ENTRIES && copied[start] == TRUE)
				start++;
		}

		if (next == (end - 1)) {
			while (end > UNICODE_FIRST_ENTRY && copied[end - 1] == TRUE)
				end--;
		}
	}

	return TRUE;
}


/**
 * Parse a UTF8 string, returning the individual characters in Unicode.
 * The supplied string pointer is updated on return, to  point to the
 * next character to be processed (but stops on the zero terminator).
 *
 * \param **text		Pointer to the the UTF8 string to parse.
 * \return			The next character in the text.
 */

static int unicode_parse_utf8_string(char **text)
{
	int	current_char = 0, bytes_remaining = 0;

	/* There's no buffer, or we're at the end of the string. */

	if (text == NULL || *text == NULL || **text == '\0')
		return 0;

	/* We're not currently in a UTF8 byte sequence. */

	if ((bytes_remaining == 0) && ((**text & 0x80) == 0)) {
		return *(*text)++;
	} else if ((bytes_remaining == 0) && ((**text & 0xe0) == 0xc0)) {
		current_char = (*(*text)++ & 0x1f) << 6;
		bytes_remaining = 1;
	} else if ((bytes_remaining == 0) && ((**text & 0xf0) == 0xe0)) {
		current_char = (*(*text)++ & 0x0f) << 12;
		bytes_remaining = 2;
	} else if ((bytes_remaining == 0) && ((**text & 0xf8) == 0xf0)) {
		current_char = (*(*text)++ & 0x07) << 18;
		bytes_remaining = 3;
	} else {
		return 0;
	}

	/* Process any additional UTF8 bytes. */

	while (bytes_remaining > 0) {
		if ((**text == 0) || ((**text & 0xc0) != 0x80))
			return 0;

		current_char += (*(*text)++ & 0x3f) << (6 * --bytes_remaining);
	}

	return current_char;
}

/**
 * Convert a unicode character into the appropriate code in the current
 * alphabet's encoding. Characters which can't be mapped are returned
 * as '?'.
 *
 * If *c is passed as NULL, the availability of a suitable conversion will
 * be checked, but no character will be written back.
 *
 * If called when the current alphabet is UTF-8, the character will also
 * be written as '?' to avoid trying to write a milti-byte value back to
 * the buffer.
 *
 * \param unicode		The unicode character to convert.
 * \param *c			Pointer to a character in which to
 *				return the encoding (or '?'), or NULL.
 * \return			True if an encoding was found; else False.
 */

static osbool unicode_find_mapped_character(int unicode, char *c)
{
	int first = 0, last = unicode_current_map_entries, middle;

	/* The byte is the same in unicode or ASCII. */

	if (unicode >= 0 && unicode < 128) {
		if (c != NULL)
			*c = unicode;
		return TRUE;
	}

	/* We have no mapping (or the current alphabet is UTF-8 anyway). */

	if (unicode_current_map_entries == 0) {
		if (c != NULL)
			*c = '?';
		return FALSE;
	}

	/* Find the character in the current encoding. */

	while (first <= last) {
		middle = (first + last) / 2;

		if (unicode_map[middle].utf8 == unicode) {
			if (c != NULL)
				*c = unicode_map[middle].target;
			return TRUE;
		} else if (unicode_map[middle].utf8 < unicode) {
			first = middle + 1;
		} else {
			last = middle - 1;
		}
	}

	if (c != NULL)
		*c = '?';

	return FALSE;
}
