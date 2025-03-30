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

MFILES = src/gui/cocoa.m

# Things like `-g` are passed here.
EXTRAFLAGS = -DUSE_SOUND -DUSE_GL_SHORT
# Missing libraries are passed here; see default value in `build/Makefile.$(PLATFORM)`.
EXTRALIBS =

include build/$(TOOLKIT)/Makefile.$(PLATFORM)
include build/Makefile.$(PLATFORM)

CFLAGS  = $(EXTRAFLAGS) $(GUIFLAGS) $(COMMONFLAGS) -DGIT_COMMIT_HASH=\"$(COMMITHASH)\"
MKFLAGS = CFLAGS='$(CFLAGS)' XFLAGS='$(XFLAGS)' CC=$(CC)

all: $(BINARY)

$(BINARY): build/bsdeps.a $(OMFILES) build/betterspades.a
	mkdir -p build
	$(LD) -o $(BINARY) $(OMFILES) build/betterspades.a build/bsdeps.a $(LDFLAGS)

.FORCE:

build/betterspades.a: .FORCE hier
	$(MAKE) -f src/Makefile $(MKFLAGS)

build/bsdeps.a: .FORCE hier
	$(MAKE) -f deps/Makefile $(MKFLAGS)

.SUFFIXES: .m .o

.m.o:
	$(CC) -x objective-c $(CFLAGS) $(MFLAGS) -c $< -o $@ -Iinclude

chksum:
	$(HASHPROG) -c $(PACKHASH)

game: $(BINARY) $(PACKFILE)
	mkdir -p $(GAMEDIR)
	cp -r $(RESDIR)/* $(GAMEDIR)
	cp $(BINARY) $(GAMEDIR)
	unzip -o $(PACKFILE) -d $(GAMEDIR) -x $(IGNORERES) || true

download: $(PACKFILE)

$(PACKFILE):
	curl -o $(PACKFILE) $(PACKURL)

clean: hier
	$(MAKE) $(DOMAKE) -f src/Makefile clean
	rm -rf $(OMFILES) $(BINARY)

nuke: hier
	$(MAKE) -f src/Makefile $(MKFLAGS) clean
	$(MAKE) -f deps/Makefile $(MKFLAGS) clean
	rm -rf $(OMFILES) $(BINARY)

depend: hier
	$(MAKE) -f src/Makefile $(MKFLAGS) depend
	$(MAKE) -f deps/Makefile $(MKFLAGS) depend

hier:
	touch src/Makefile.depend
	touch deps/Makefile.depend
