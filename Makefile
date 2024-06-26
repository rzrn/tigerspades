CC        ?= cc
SRCDIR     = src
BUILDDIR   = build
INCLUDEDIR = include
DEPSDIR    = deps
RESDIR     = resources
GAMEDIR    = dist
BINARY     = $(BUILDDIR)/betterspades
TOOLKIT   ?= SDL
IGNORERES  = png/tracer.png png/command.png png/medical.png png/intel.png png/player.png
FONTTYPE  ?= SHORT

MAJOR   = 0
MINOR   = 1
PATCH   = 6

PACKURL = https://aos.party/bsresources.zip
RESPACK = $(GAMEDIR)/bsresources.zip

CDEPS   := $(shell find $(DEPSDIR) -type f -name '*.c')
ODEPS   := $(CDEPS:$(DEPSDIR)/%.c=$(BUILDDIR)/%.o)
CFILES  := $(shell find $(SRCDIR) -type f -name '*.c')
OFILES  := $(CFILES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
MFILES  :=
OMFILES :=

ALLFLAGS ?=
ALLFLAGS += -std=gnu99
ALLFLAGS += -DBETTERSPADES_MAJOR=$(MAJOR)
ALLFLAGS += -DBETTERSPADES_MINOR=$(MINOR)
ALLFLAGS += -DBETTERSPADES_PATCH=$(PATCH)
ALLFLAGS += -DBETTERSPADES_VERSION=\"v$(MAJOR).$(MINOR).$(PATCH)\"
ALLFLAGS += -DGIT_COMMIT_HASH=\"$(shell git describe --always --dirty)\"
ALLFLAGS += -DUSE_SOUND

CFLAGS ?=
CFLAGS += -Wall -pedantic

MFLAGS ?=

DEPSFLAGS ?=
DEPSFLAGS += -std=gnu99
DEPSFLAGS += -DLOG_USE_COLOR

UNAME := $(shell uname -s)

LDFLAGS =

ifeq ($(FONTTYPE),SHORT)
	ALLFLAGS += -DUSE_GL_SHORT
endif

ifeq ($(FONTTYPE),FLOAT)
	ALLFLAGS += -DUSE_GL_FLOAT
endif

ifeq ($(TOOLKIT),SDL)
	ALLFLAGS += -DUSE_SDL

	ifeq ($(OS),Windows_NT)
		LDFLAGS += -lSDL2
	endif

	ifeq ($(UNAME),Linux)
		LDFLAGS += -lSDL2
	endif

	ifeq ($(UNAME),Darwin)
		LDFLAGS += /usr/local/Cellar/sdl2/2.0.3/lib/libSDL2.a
	endif
endif

ifeq ($(TOOLKIT),GLFW)
	ALLFLAGS += -DUSE_GLFW

	ifeq ($(OS),Windows_NT)
		LDFLAGS += -lglfw3
	endif

	ifeq ($(UNAME),Linux)
		LDFLAGS += -lglfw
	endif

	ifeq ($(UNAME),Darwin)
		LDFLAGS += -lglfw
	endif
endif

ifeq ($(TOOLKIT),GLUT)
	ALLFLAGS += -DUSE_GLUT

	ifeq ($(OS),Windows_NT)
		LDFLAGS += -lfreeglut
	endif

	ifeq ($(UNAME),Linux)
		LDFLAGS += -lglut
	endif

	ifeq ($(UNAME),Darwin)
		LDFLAGS += -framework GLUT
	endif
endif

ifeq ($(TOOLKIT),Cocoa)
	ALLFLAGS += -DUSE_COCOA
	MFILES   += $(shell find $(SRCDIR) -type f -name '*.m')
	OMFILES  += $(MFILES:$(SRCDIR)/%.m=$(BUILDDIR)/%.o)

	ifeq ($(UNAME),Darwin)
		ALLFLAGS += -DUSE_QUARTZ
	else
		ALLFLAGS += -DUSE_GNUSTEP
		MFLAGS   += $(shell gnustep-config --objc-flags)
		LDFLAGS  += $(shell gnustep-config --gui-libs)
	endif
endif

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lopenal -lopengl32 -lglu32 -lgdi32 -lwinmm -lws2_32 -pthread
endif

ifeq ($(UNAME),Linux)
	LDFLAGS += -lm -lopenal -lGL -lGLU -pthread
endif

ifeq ($(UNAME),Darwin)
	LDFLAGS += -liconv -framework Carbon -framework OpenAL -framework CoreAudio -framework AudioUnit -framework IOKit -framework Cocoa -framework OpenGL
endif

all: $(BUILDDIR) $(BINARY)

$(BINARY): $(OFILES) $(OMFILES) $(ODEPS)
	$(CC) -o $(BINARY) $(OMFILES) $(OFILES) $(ODEPS) $(LDFLAGS)

$(OFILES): $(BUILDDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p `dirname $@`
	$(CC) $(ALLFLAGS) $(CFLAGS) -c $< -o $@ -I$(INCLUDEDIR)

$(OMFILES): $(BUILDDIR)/%.o: $(SRCDIR)/%.m
	mkdir -p `dirname $@`
	$(CC) -x objective-c $(ALLFLAGS) $(MFLAGS) -c $< -o $@ -I$(INCLUDEDIR)

$(ODEPS): $(BUILDDIR)/%.o: $(DEPSDIR)/%.c
	mkdir -p `dirname $@`
	$(CC) $(DEPSFLAGS) -c $< -o $@ -I$(INCLUDEDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

.PHONY : game
game: $(BINARY) $(RESPACK)
	mkdir -p $(GAMEDIR)
	cp -r $(RESDIR)/* $(GAMEDIR)
	cp $(BINARY) $(GAMEDIR)
	unzip -o $(RESPACK) -d $(GAMEDIR) -x $(IGNORERES) || true

$(RESPACK):
	mkdir -p `dirname $(RESPACK)`
	curl -o $(RESPACK) $(PACKURL)

clean:
	rm -rf $(OFILES) $(OMFILES) $(BINARY)

nuke:
	rm -rf $(OFILES) $(OMFILES) $(ODEPS) $(BINARY)
