# Copyright © 2025–2026 rzrn

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# (1) Platform detection heuristic

if [ -z "${PLATFORM}" ]
then
  case `uname -s` in
    Linux)
      PLATFORM=Linux ;;
    FreeBSD)
      PLATFORM=FreeBSD ;;
    MINGW32_NT*|MINGW64_NT*|MSYS_NT*|CYGWIN_NT*)
      PLATFORM=NT ;;
    Darwin)
      PLATFORM=Mac ;;
    Haiku)
      PLATFORM=Haiku ;;
    *)
      echo configure.sh: unable to detect platform automatically
      echo configure.sh: consider specifying it manually using `make PLATFORM=Linux ...`
      exit 1 ;;
  esac
fi

# (2) Default values for user-defined options

if [ -z "${PROFILE}" ]
then
  PROFILE=RELEASE
fi

if [ -z "${COMMITHASH}" ]
then
  COMMITHASH=$(git describe --always --dirty)
fi

if [ -z "${TOOLKIT}" ]
then
  TOOLKIT="SDL"
fi

if [ -z "${OPTLEVEL}" ]
then
  OPTLEVEL=1
fi

if [ -z "${CFLAGS1+TRUE}" ]
then
  CFLAGS1="-DUSE_SOUND -DUSE_GL_SHORT"
fi

if [ -z "${CFLAGS2+TRUE}" ]
then
  CFLAGS2="-DLOG_USE_COLOR"
fi

if [ -z "${NSFLAGS+TRUE}" ]
then
  NSFLAGS=""
fi

if [ -z "${LDFLAGS+TRUE}" ]
then
  case ${PLATFORM} in
    Mac)
      LDFLAGS="-framework OpenAL" ;;
    *)
      LDFLAGS="-lopenal" ;;
  esac
fi

# (3) Toolkit-specific build flags

NSFILES=""

case ${PLATFORM}-${TOOLKIT} in
  NT-GLFW)
    CFLAGS1="${CFLAGS1} -DUSE_GLFW"
    LDFLAGS="${LDFLAGS} -lglfw3"
    ;;
  Linux-GLFW | FreeBSD-GLFW | Mac-GLFW | Haiku-GLFW)
    CFLAGS1="${CFLAGS1} -DUSE_GLFW"
    LDFLAGS="${LDFLAGS} -lglfw"
    ;;
  Mac-SDL)
    CFLAGS1="${CFLAGS1} -DUSE_SDL -I$(brew --prefix)/include"
    LDFLAGS="${LDFLAGS} -lSDL2 -L$(brew --prefix)/lib"
    ;;
  NT-SDL | Linux-SDL | FreeBSD-SDL | Haiku-SDL)
    CFLAGS1="${CFLAGS1} -DUSE_SDL"
    LDFLAGS="${LDFLAGS} -lSDL2"
    ;;
  NT-GLUT)
    CFLAGS1="${CFLAGS1} -DUSE_GLUT"
    LDFLAGS="${LDFLAGS} -lfreeglut"
    ;;
  Mac-GLUT)
    CFLAGS1="${CFLAGS1} -DUSE_GLUT"
    LDFLAGS="${LDFLAGS} -framework GLUT"
    ;;
  Linux-GLUT | FreeBSD-GLUT | Haiku-GLUT)
    CFLAGS1="${CFLAGS1} -DUSE_GLUT"
    LDFLAGS="${LDFLAGS} -lglut"
    ;;
  Mac-Cocoa)
    CFLAGS1="${CFLAGS1} -DUSE_COCOA -DUSE_QUARTZ"
    NSFILES=src/gui/cocoa.m
    ;;
  NT-Cocoa | Linux-Cocoa | FreeBSD-Cocoa)
    CFLAGS1="${CFLAGS1} -DUSE_COCOA -DUSE_GNUSTEP"
    LDFLAGS="${LDFLAGS} $(gnustep-config --gui-libs)"
    NSFLAGS="${OBJCFLAGS} $(gnustep-config --objc-flags)"
    NSFILES=src/gui/cocoa.m
    ;;
  *)
    echo configure.sh: unknown platform-toolkit pair: ${PLATFORM}-${TOOLKIT}
    exit 1
    ;;
esac

# (4) Platform-specific build flags

case ${PLATFORM} in
  NT)
    LDFLAGS="${LDFLAGS} -lopengl32 -lgdi32 -lwinmm -lws2_32 -pthread"
    ;;
  Linux)
    LDFLAGS="${LDFLAGS} -lm -lGL -pthread"
    ;;
  Mac)
    LDFLAGS="${LDFLAGS} -liconv -framework Carbon -framework CoreAudio -framework AudioUnit -framework IOKit -framework Cocoa -framework OpenGL"
    ;;
  FreeBSD)
    LDFLAGS="${LDFLAGS} -lm -lGL -pthread -L/usr/local/lib"
    CFLAGS1="${CFLAGS1} -I/usr/local/include"
    CFLAGS2="${CFLAGS2} -I/usr/local/include -DHAS_SOCKLEN_T"
    ;;
  Haiku)
    LDFLAGS="${LDFLAGS} -lm -lGL -lnetwork -pthread"
    CFLAGS2="${CFLAGS2} -DHAS_SOCKLEN_T"
    ;;
  *)
    echo configure.sh: unknown platform ${PLATFORM}
    exit 1
    ;;
esac

# (5) Profile-specific build flags

case ${PROFILE} in
  RELEASE)
    # Release builds are expected to work with the strict POSIX.1-2001 compiler interface
    # such as the ‘c99’ frontend from FreeBSD.
    CFLAGS1="${CFLAGS1} -O${OPTLEVEL}"
    CFLAGS2="${CFLAGS2} -O${OPTLEVEL}"
    ;;
  DEBUG)
    # Debug builds are not expected to work with any compiler.
    # We leave all noisy-but-useful flags (like ‘-Wall’) here.
    CFLAGS1="${CFLAGS1} -std=c99 -g -Wall -Wextra -pedantic -Wno-implicit-fallthrough"
    CFLAGS2="${CFLAGS2} -std=c99 -g"
    ;;
  *)
    echo configure.sh: unknown build profile: ${PROFILE}
    exit 1
    ;;
esac

CFLAGS1="${CFLAGS1} -D'GIT_COMMIT_HASH=\"${COMMITHASH}\"' -Iinclude/"
CFLAGS2="${CFLAGS2} -Iinclude/"

# (6) Final remarks

export NSFILES

export CFLAGS1=$(echo $CFLAGS1)
export CFLAGS2=$(echo $CFLAGS2)
export NSFLAGS=$(echo $NSFLAGS)
export LDFLAGS=$(echo $LDFLAGS)
