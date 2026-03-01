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

CFILES2 = vendor/libdeflate/gzip_decompress.c\
          vendor/libdeflate/deflate_decompress.c\
          vendor/libdeflate/x86/cpu_features.c\
          vendor/libdeflate/crc32.c\
          vendor/libdeflate/utils.c\
          vendor/libdeflate/zlib_compress.c\
          vendor/libdeflate/gzip_compress.c\
          vendor/libdeflate/zlib_decompress.c\
          vendor/libdeflate/deflate_compress.c\
          vendor/libdeflate/arm/cpu_features.c\
          vendor/libdeflate/adler32.c\
          vendor/GLEW/glew.c\
          vendor/libvxl/libvxl.c\
          vendor/dr_libs/dr_wav.c\
          vendor/log/log.c\
          vendor/enet/list.c\
          vendor/enet/host.c\
          vendor/enet/unix.c\
          vendor/enet/packet.c\
          vendor/enet/callbacks.c\
          vendor/enet/compress.c\
          vendor/enet/peer.c\
          vendor/enet/win32.c\
          vendor/enet/protocol.c\
          vendor/parson/parson.c\
          vendor/http/http.c\
          vendor/hashtable/hashtable.c\
          vendor/ini/ini.c\
          vendor/microui/microui.c

OFILES1 = $(CFILES1:.c=.1) $(NSFILES:.m=.1)
OFILES2 = $(CFILES2:.c=.2)

AFILES = build/betterspades.a build/vendor.a