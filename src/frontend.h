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
 * \file: frontend.h
 *
 * Frontend collection external (to the RISC OS code) interface.
 * 
 * This header defines the interface exposed to the rest of the
 * RISC OS application. The interface facing the midend is defined
 * in core/puzzles.h
 */

#ifndef PUZZLES_FRONTEND
#define PUZZLES_FRONTEND

/**
 * A game collection instance.
 */

struct frontend;

/**
 * Initialise a new frontend and open its window.
 */

void frontend_create_instance(void);

/**
 * Delete a frontend instance.
 *
 * \param *fe	The instance to be deleted.
 */

void frontend_delete_instance(struct frontend *fe);

#endif
