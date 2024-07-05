CFILES = src/AceOfSpades/protocol.c src/GUI/GLFW.c src/GUI/SDL.c src/GUI/GLUT.c\
         src/main.c src/player.c src/map.c src/list.c src/ping.c src/tesselator.c\
         src/rpc.c src/channel.c src/minheap.c\
         src/entitysystem.c src/grenade.c src/model.c src/glx.c src/texture.c src/aabb.c\
         src/hud.c src/utils.c src/network.c src/chunk.c src/config.c src/common.c\
         src/font.c src/matrix.c src/particle.c src/cameracontroller.c src/sound.c\
         src/tracer.c src/unicode.c src/file.c src/weapon.c src/camera.c src/window.c

OFILES  = $(CFILES:.c=.o)
ARCHIVE = build/betterspades.a
DEPEND  = src/Makefile.depend
_CFLAGS = -Wall -pedantic $(CFLAGS)

include build/Makecommon
