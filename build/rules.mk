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

all: $(EXEFILE)

clean:
	rm -f $(EXEFILE) $(AFILES)
	@find src/ -name '*.1' -exec echo rm -f {} \; -exec rm -f {} \;

nuke: clean
	echo > $(PREREQFILE)
	@find deps/ -name '*.2' -exec echo rm -f {} \; -exec rm -f {} \;

download: $(PACKFILE)

game: $(EXEFILE) $(PACKFILE)
	mkdir -p $(GAMEDIR)
	cp -r $(RESDIR)/* $(GAMEDIR)
	cp $(EXEFILE) $(GAMEDIR)
	unzip -o $(PACKFILE) -d $(GAMEDIR) -x $(IGNORERES) || true

chksum:
	$(HASHPROG) -c $(PACKHASH)

depend:
	echo > $(PREREQFILE)
	makedepend -o.1 -s '### (1) ###' -I`$(CC) --print-file-name=include` -Iinclude/ -f $(PREREQFILE) -- $(CFLAGS1) $(CFILES1)
	makedepend -o.2 -s '### (2) ###' -I`$(CC) --print-file-name=include` -Iinclude/ -f $(PREREQFILE) -- $(CFLAGS2) $(CFILES2)

printenv:
	@echo COMMITHASH=$(COMMITHASH)

	@echo NSFILES=$(NSFILES)
	@echo CFLAGS1=$(CFLAGS1)
	@echo CFLAGS2=$(CFLAGS2)
	@echo NSFLAGS=$(NSFLAGS)
	@echo LDFLAGS=$(LDFLAGS)

.SUFFIXES: .h .c .m .o .1 .2 .3 .4 .5 .6 .7 .8 .9

$(PACKFILE):
	curl -o $(PACKFILE) $(PACKURL)

.m.1:
	$(CC) -x objective-c $(CFLAGS1) $(NSFLAGS) -DGIT_COMMIT_HASH=\"$(COMMITHASH)\" -Iinclude/ -c $< -o $@

.c.1:
	$(CC) $(CFLAGS1) -DGIT_COMMIT_HASH=\"$(COMMITHASH)\" -Iinclude/ -c $< -o $@

.c.2:
	$(CC) $(CFLAGS2) -Iinclude/ -c $< -o $@

build/betterspades.a: $(OFILES1)
	ar rcs $@ $?

build/bsdeps.a: $(OFILES2)
	ar rcs $@ $?

$(EXEFILE): $(AFILES)
	$(LD) -o $(EXEFILE) $(AFILES) $(LDFLAGS)
