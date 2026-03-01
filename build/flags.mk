# Copyright © 2021–2026 rzrn

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

CC         = cc
LD         = cc
RESDIR     = resources
GAMEDIR    = dist
EXEFILE    = build/betterspades
IGNORERES  = png/tracer.png png/command.png png/medical.png png/intel.png png/player.png
PREREQFILE = build/prerequisites.mk

PACKURL  = https://aos.party/bsresources.zip
HASHPROG = sha512sum
PACKFILE = build/bsresources.zip
PACKHASH = $(PACKFILE).sha512

CFILES1 = src/ace/protocol.c src/gui/glfw.c src/gui/sdl.c src/gui/glut.c\
          src/aabb.c src/camera.c src/cameracontroller.c src/channel.c src/chunk.c\
          src/common.c src/config.c src/entitysystem.c src/file.c src/font.c src/glx.c\
          src/grenade.c src/hud.c src/list.c src/main.c src/map.c src/matrix.c src/minheap.c\
          src/model.c src/network.c src/particle.c src/ping.c src/player.c src/rpc.c src/sound.c\
          src/tesselator.c src/texture.c src/tracer.c src/unicode.c src/utils.c src/weapon.c src/window.c

CFILES2 = deps/libdeflate/gzip_decompress.c\
          deps/libdeflate/deflate_decompress.c\
          deps/libdeflate/x86/cpu_features.c\
          deps/libdeflate/crc32.c\
          deps/libdeflate/utils.c\
          deps/libdeflate/zlib_compress.c\
          deps/libdeflate/gzip_compress.c\
          deps/libdeflate/zlib_decompress.c\
          deps/libdeflate/deflate_compress.c\
          deps/libdeflate/arm/cpu_features.c\
          deps/libdeflate/adler32.c\
          deps/GLEW/glew.c\
          deps/libvxl/libvxl.c\
          deps/dr_libs/dr_wav.c\
          deps/log/log.c\
          deps/enet/list.c\
          deps/enet/host.c\
          deps/enet/unix.c\
          deps/enet/packet.c\
          deps/enet/callbacks.c\
          deps/enet/compress.c\
          deps/enet/peer.c\
          deps/enet/win32.c\
          deps/enet/protocol.c\
          deps/parson/parson.c\
          deps/http/http.c\
          deps/hashtable/hashtable.c\
          deps/ini/ini.c\
          deps/microui/microui.c

OFILES1 = $(CFILES1:.c=.1) $(NSFILES:.m=.1)
OFILES2 = $(CFILES2:.c=.2)

AFILES = build/bsdeps.a build/betterspades.a