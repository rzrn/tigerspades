# Copyright © 2024–2025 rzrn

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

all clean nuke game depend download chksum:
	@case `uname -s` in\
		Linux) PLATFORM=Linux ;;\
		FreeBSD) PLATFORM=FreeBSD ;;\
		MINGW32_NT*|MINGW64_NT*|MSYS_NT*|CYGWIN_NT*) PLATFORM=NT ;;\
		Darwin) PLATFORM=Mac ;;\
		Haiku) PLATFORM=Haiku ;;\
		*) echo 'Unknown platform: you can try `PLATFORM=Linux make -fbuild/Makefile` manually.'; exit 1 ;;\
	esac; export PLATFORM; $(MAKE) -fbuild/Makefile $@
