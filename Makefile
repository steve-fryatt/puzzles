# Copyright 2024, Stephen Fryatt
#
# This file is part of Puzzles:
#
#   http://www.stevefryatt.org.uk/risc-os
#
# Licensed under the EUPL, Version 1.2 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

ARCHIVE := puzzles

APP := !Puzzles

PACKAGE := Puzzles
PACKAGELOC := Games

CORE := src/core

GAMES := generated-games.h
GAMESRC := games-list.txt

EXTRASRCPREREQ := $(CORE)/$(GAMES)

OBJS =  frontend.o		\
	game_draw.o		\
	game_window.o		\
	iconbar.o		\
	main.o			\
	core/drawing.o		\
	core/dsf.o		\
	core/findloop.o		\
	core/galaxies.o		\
	core/list.o		\
	core/malloc.o		\
	core/matching.o		\
	core/misc.o		\
	core/midend.o		\
	core/net.o		\
	core/netslide.o		\
	core/printing.o		\
	core/random.o		\
	core/tents.o		\
	core/tree234.o

CCFLAGS = -DCOMBINED -DNO_TGMATH_H

include $(SFTOOLS_MAKE)/CApp

# Copy the games list into the appropriate place.

$(CORE)/$(GAMES): $(GAMESRC)
	@$(call show-stage,GAMELIST,$(CORE)/$(GAMES))
	@$(CP) $(GAMESRC) $(CORE)/$(GAMES)
