Puzzles
=======

Simon Tatham's Portable Puzzle Collection on RISC OS


Introduction
------------

Puzzles is a RISC OS front end for [Simon Tatham's Portable Puzzle Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/).



Building
--------

Puzzles consists of a collection of C and un-tokenised BASIC, which must be assembled using the [SFTools build environment](https://github.com/steve-fryatt). It will be necessary to have suitable Linux system with a working installation of the [GCCSDK](http://www.riscos.info/index.php/GCCSDK) to be able to make use of this.

With a suitable build environment set up, making Puzzles is a matter of running

	make

from the root folder of the project -- however, read the additional notes below first! This will build everything from source, and assemble a working !Puzzles application and its associated files within the build folder. If you have access to this folder from RISC OS (either via HostFS, LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make clean

To make a release version and package it into Zip files for distribution, use

	make release

This will clean the project and re-build it all, then create a distribution archive (no source), source archive and RiscPkg package in the folder within which the project folder is located. By default the output of `git describe` is used to version the build, but a specific version can be applied by setting the `VERSION` variable -- for example

	make release VERSION=1.23

Additional Notes
----------------

The RISC OS port of Puzzles uses the original sources from Simon Tatham's Git repository, which are located as a submodule at `src/core`. You will therefore need to run `git submodule init` and `git submodule update` after cloning the RISC OS front end, in order to pull these additional sources in. These files are considered a 'black box' as far as the RISC OS code is concerned: for more details on the code within this submodule, take a look at Simon's website at https://www.chiark.greenend.org.uk/~sgtatham/puzzles/.

Unlike the normal build system, which uses CMake to identify all of the games and include them automatically, the RISC OS port maintains a manual list of available puzzles in the `games-list.txt` file in the root of the project. This is copied into an appropriate place within `src/core` as part of the build process, as if it had been created by CMake. The file contains a list of entries in the form

    GAME(galaxies)
    GAME(net)
    GAME(tents)

which define the game backends which should be compiled into the final application. A full list of game names is provided in the `games-list-full.txt` file for reference, to save running a build of another platform's front end. The source files for the backends should also be added to the `OBJS` definition in the Makefile:

    OBJS =  frontend.o        \
            game_draw.o       \

            /core/galaxies.o  \
            /core/net.o       \
            /core/tents.o

Note that whilst the RISC OS port is being developed, games are only being added to the frontend when it is believed that there is sufficient infrastructure in place to enable them to run safely and (mostly) correctly. If a game isn't included, there is probably a reason why!


Licence
-------

The RISC OS-specific code in this port of Puzzles is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.

The core Puzzles code, not included in this repository, is licenced separately: see the LICENCE file within the `src/core` submodule for details.