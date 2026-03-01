![GPL v3](https://www.gnu.org/graphics/gplv3-127x51.png)

## TigerSpades

* Compatible client of *Ace of Spades* (classic voxlap).
* Runs on very old systems back to OpenGL 1.1 (OpenGL ES support too).
* Shares similar if not even better performance to voxlap.

#### Why should I use this instead of […]?

* Free of any Jagex code, they can’t shut it down.
* Open for future expansion.
* Easy to use.
* A lot of hidden bugs.

## Differences in this fork

* Support of big-endian systems e.g. PowerPC (but *not limited to,* that is, support of little-endian systems is available as well).
* Unicode & UTF-8.
* Customizable key bindings.
* Cleaned up user interface.
* Makefiles instead of CMake (now obsolete version of CMake takes ≈2 hours to build on G4 CPU but still cannot be used to build BetterSpades).
* A couple of small features that you can find in “Settings”.
* Imported from other forks & unmerged PRs bugfixes (as well as original ones) for several chronic bugs.

## System requirements

| Type    | min. requirement                                     |
| ------- | ---------------------------------------------------- |
| OS      | Windows 98 or Linux                                  |
| CPU     | 1 GHz single core processor                          |
| GPU     | 64 MiB VRAM, Mobile Intel 945GM or equivalent        |
| RAM     | 256 MiB                                              |
| Display | 800×600                                              |
| Others  | Keyboard and mouse<br />Dial up network connection   |

## Build requirements

This project uses the following libraries and files (their code is *included* in the repository):

| Name         | License         | Usage                  | GitHub                                             |
| ------------ | --------------- | ---------------------- | :------------------------------------------------: |
| inih         | *BSD-3.Clause*  | .INI file parser       | [Link](https://github.com/benhoyt/inih)            |
| dr_wav       | *Public domain* | wav support            | [Link](https://github.com/mackron/dr_libs/)        |
| http         | *Public domain* | http client library    | [Link](https://github.com/mattiasgustavsson/libs)  |
| LodePNG      | *MIT*           | png support            | [Link](https://github.com/lvandeve/lodepng)        |
| libdeflate   | *MIT*           | decompression of maps  | [Link](https://github.com/ebiggers/libdeflate)     |
| enet         | *MIT*           | networking library     | [Link](https://github.com/lsalzman/enet)           |
| parson       | *MIT*           | JSON parser            | [Link](https://github.com/kgabis/parson)           |
| log.c        | *MIT*           | logger                 | [Link](https://github.com/xtreme8000/log.c)        |
| GLEW         | *MIT*           | OpenGL extensions      | [Link](https://github.com/nigels-com/glew)         |
| hashtable    | *MIT*           | hashtable              | [Link](https://github.com/goldsborough/hashtable/) |
| libvxl       | *MIT*           | access VXL format      | [Link](https://github.com/xtreme8000/libvxl/)      |
| microui      | *MIT*           | user interface         | [Link](https://github.com/rxi/microui)             |

You will need to compile the following by yourself, or get hold of precompiled binaries:

* GLFW3 or SDL.
* OpenAL.

#### Windows

Use MinGW-w64 from MSYS2:

```bash
$ pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-openal unzip
$ TOOLKIT=GLFW make game
```

If everything went well, the client should be in the `dist/` subfolder.

#### Linux

You can build each library yourself, or install them with your distro’s package manager. For example, on Ubuntu 24.04:
```
sudo apt install libgl-dev libopenal-dev libglfw3-dev
```

Start the client e.g. with the following inside the `dist/` directory:
```
./betterspades
```
Or connect directly to localhost:
```
./betterspades -aos://16777343:32887
```

On the buggy drivers (like “mesa” under PowerPC Macs) it may be neccessary to `export MESA_GL_VERSION_OVERRIDE=1.5` before starting the game to avoid (weird) graphics glitches. Also try build with `CFLAGS1="-DUSE_SOUND -DUSE_GL_FLOAT" make <...>` if fonts are not rendering.

#### macOS

The development headers for OpenAL and OpenGL don’t have to be installed since they come with macOS by default.

## License

For any copyright year range specified as YYYY–ZZZZ in this package note
that the range specifies every single year in that closed interval.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
