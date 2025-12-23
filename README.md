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

The upstream sources can be updated using `git submodule update --remote`, and then a specific revision checked out to peg the build to that version.

There are a few additional steps which must be carried out manually, as described below.

### Puzzles and Source Files

Unlike the normal build system, which uses CMake to identify all of the games and source files so that they can be included automatically, the RISC OS port maintains a manual list of available puzzles in the `games-list.txt` file in the root of the project. This is copied into an appropriate place within `src/core` as part of the build process, as if it had been created by CMake. The file contains a list of entries in the form

    GAME(galaxies)
    GAME(net)
    GAME(tents)

which define the game backends which should be compiled into the final application. The names are the ones defined in the `src/core/CMakeLists.txt` file.

The corresponding source files for the backends should also be added to the `OBJS` definition in the Makefile, along with any common source files:

    OBJS =  frontend.o        \
            game_draw.o       \

            core/matching.o   \
            core/midend.o     \

            core/galaxies.o  \
            core/net.o       \
            core/tents.o

### Manual

The RISC OS port uses its own manual, defined in `manual/Source.xml`. This is based on the upstream manual supplied in Halibut format in `src/core/puzzles.but`, but contains modifications to reflect the RISC OS interface. The IDs for the game sections should match the `htmlhelp_topic` values for the respective games.

The `manual/Source.xml` file contains a comment near the top which should be kept up to date with the commit hash of the last upstream manual to be merged in to the RISC OS version. When new changes are brought in from the upstream repository, a diff of `src/core/puzzles.but` can be done between this revision and HEAD, to show the changes which must be manually edited in to the RISC OS manual source.

In addition, the list of contributors' names found towards the end of the `src/core/puzzles.but` file ("Portions copyright...") should be transferred into the `src/additional_contributors.h` header file so that it can be included in the Program Info dialogue. This is a manual process which won't internationalise well, so it may need to be re-visited in the future.

### Game Descriptions

The RISC OS Messages file (at `build/!Puzzles/Resources/UK/Messages` and potentially other country resources) contains a set of tokens in the form `Help.Index.game`, where 'game' is the `htmlhelp_topic` of a game, which define interactive help texts for each of the game icons in the index. These will need to be created for each new game to be added.

### Icons

The RISC OS port expects to find icons for the games in the `build/!Puzzles/Sprites`, `build/!Puzzles/Sprites22` and `build/!Puzzles/Sprites11` files, named by the `htmlhelp_topic` for each game (with an `sm` suffix for the small versions). These icons follow the standard RISC OS conventions.

It is possible to generate images in the sizes needed by RISC OS using the standard upstream build system. In the `src/core/icons/icons.cmake` file, look for the lines defining the icon sizes.

    # All sizes of icon we make for any purpose.
    set(all_icon_sizes 128 96 88 64 48 44 32 24 16)

    # Sizes of icon we put into the Windows .ico files.
    set(win_icon_sizes 48 32 16)

    # Border thickness for each icon size.
    set(border_128 8)
    set(border_96 4)
    set(border_88 4)
    set(border_64 4)
    set(border_48 4)
    set(border_44 4)
    set(border_32 2)
    set(border_24 1)
    set(border_16 1)

To generate the 17x17, 34x34 and 68x68 images, add in the following extra sizes:

    # All sizes of icon we make for any purpose.
    set(all_icon_sizes 68 34 17)

and then define the borders as follows:

    # Border thickness for each icon size.
    set(border_68 4)
    set(border_34 2)
    set(border_17 1)

If the build is then run by executing

    $cmake .
    $cmake --build .

from within the `src/core` folder, the icons should be generated as PNG files within the `src/core/icons` folder (if there are dependencies missing, such as ImageMagick, these should be reported at the `cmake .` stage).

The 68 and 34 can be used at 180 DPI for the Sprites11 file, the 34 and 17 can be used at the default 90 DPI for the Sprites22 file, and with the aid of ChangeFSI to convert to rectangular pixel mode, the 34 and 17 can also be used at 90/45 DPI for the Sprites file.


Licence
-------

The RISC OS-specific code in this port of Puzzles is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.

The core Puzzles code, not included in this repository, is licenced separately: see the LICENCE file within the `src/core` submodule for details.