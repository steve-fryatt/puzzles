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

MANSRC := Source.xml

HTMLHELP := Manual,faf

EXTRASRCPREREQ := $(CORE)/$(GAMES)

OBJS =  blitter.o			\
	canvas.o			\
	frontend.o			\
	game_draw.o			\
	game_config.o			\
	game_window.o			\
	game_window_backend_menu.o	\
	iconbar.o			\
	index_window.o			\
	main.o				\
	riscos_test.o			\
	sprites.o			\
	core/blackbox.o			\
	core/bridges.o			\
	core/combi.o			\
	core/cube.o			\
	core/divvy.o			\
	core/dominosa.o			\
	core/drawing.o			\
	core/dsf.o			\
	core/fifteen.o			\
	core/filling.o			\
	core/findloop.o			\
	core/flip.o			\
	core/flood.o			\
	core/galaxies.o			\
	core/grid.o			\
	core/guess.o			\
	core/hat.o			\
	core/inertia.o			\
	core/keen.o			\
	core/latin.o			\
	core/laydomino.o		\
	core/lightup.o			\
	core/list.o			\
	core/loopgen.o			\
	core/loopy.o			\
	core/magnets.o			\
	core/malloc.o			\
	core/map.o			\
	core/matching.o			\
	core/misc.o			\
	core/midend.o			\
	core/mines.o			\
	core/mosaic.o			\
	core/net.o			\
	core/netslide.o			\
	core/palisade.o			\
	core/pattern.o			\
	core/pearl.o			\
	core/pegs.o			\
	core/penrose.o			\
	core/penrose-legacy.o		\
	core/printing.o			\
	core/random.o			\
	core/range.o			\
	core/rect.o			\
	core/samegame.o			\
	core/signpost.o			\
	core/singles.o			\
	core/sixteen.o			\
	core/slant.o			\
	core/solo.o			\
	core/sort.o			\
	core/spectre.o			\
	core/tents.o			\
	core/towers.o			\
	core/tracks.o			\
	core/tdq.o			\
	core/tree234.o			\
	core/twiddle.o			\
	core/undead.o			\
	core/unequal.o			\
	core/unruly.o			\
	core/untangle.o

CCFLAGS = -DCOMBINED -DNO_TGMATH_H

include $(SFTOOLS_MAKE)/CApp

# Copy the games list into the appropriate place.

$(CORE)/$(GAMES): $(GAMESRC)
	@$(call show-stage,GAMELIST,$(CORE)/$(GAMES))
	@$(CP) $(GAMESRC) $(CORE)/$(GAMES)
