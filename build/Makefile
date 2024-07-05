CC         = cc
LD         = cc
RESDIR     = resources
GAMEDIR    = dist
BINARY     = build/betterspades
TOOLKIT    = SDL
IGNORERES  = png/tracer.png png/command.png png/medical.png png/intel.png png/player.png
COMMITHASH = `git describe --always --dirty`

PACKURL  = https://aos.party/bsresources.zip
HASHPROG = sha512sum
PACKFILE = build/bsresources.zip
PACKHASH = $(PACKFILE).sha512

MFILES = src/GUI/Cocoa.m

# Things like `-g` are passed here.
EXTRAFLAGS = -DUSE_SOUND -DUSE_GL_SHORT
# Missing libraries are passed here; see default value in `build/Makefile.$(PLATFORM)`.
EXTRALIBS =

include build/$(TOOLKIT)/Makefile.$(PLATFORM)
include build/Makefile.$(PLATFORM)

CFLAGS = $(EXTRAFLAGS) $(GUIFLAGS) -std=gnu99 -DGIT_COMMIT_HASH=\"$(COMMITHASH)\"
DOMAKE = $(MAKE) CFLAGS='$(CFLAGS)' CC=$(CC)

all: $(BINARY)

$(BINARY): build/bsdeps.a $(OMFILES) build/betterspades.a
	mkdir -p build
	$(LD) -o $(BINARY) $(OMFILES) build/betterspades.a build/bsdeps.a $(LDFLAGS)

.FORCE:

build/betterspades.a: .FORCE
	$(DOMAKE) -f src/Makefile

build/bsdeps.a: .FORCE
	$(DOMAKE) -f deps/Makefile

.SUFFIXES: .m .o

.m.o:
	$(CC) -x objective-c $(CFLAGS) $(MFLAGS) -c $< -o $@ -Iinclude

game: $(BINARY) $(PACKFILE)
	mkdir -p $(GAMEDIR)
	cp -r $(RESDIR)/* $(GAMEDIR)
	cp $(BINARY) $(GAMEDIR)
	$(HASHPROG) -c $(PACKHASH)
	unzip -o $(PACKFILE) -d $(GAMEDIR) -x $(IGNORERES) || true

$(PACKFILE):
	curl -o $(PACKFILE) $(PACKURL)

clean:
	$(DOMAKE) -f src/Makefile clean
	rm -rf $(OMFILES) $(BINARY)

nuke:
	$(DOMAKE) -f src/Makefile clean
	$(DOMAKE) -f deps/Makefile clean
	rm -rf $(OMFILES) $(BINARY)

depend:
	$(DOMAKE) -f src/Makefile depend
	$(DOMAKE) -f deps/Makefile depend
