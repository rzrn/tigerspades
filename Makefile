all clean nuke game depend chksum:
	@case `uname -s` in\
		Linux|FreeBSD|NetBSD|OpenBSD) PLATFORM=UNIX ;;\
		MINGW32_NT*|MINGW64_NT*|MSYS_NT*|CYGWIN_NT*) PLATFORM=NT ;;\
		Darwin) PLATFORM=Mac ;;\
		Haiku) PLATFORM=Haiku ;;\
		*) echo 'Unknown platform: try `PLATFORM=UNIX make -fbuild/Makefile` manually.'; exit 1 ;;\
	esac; export PLATFORM; $(MAKE) -fbuild/Makefile $@
