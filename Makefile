all clean nuke game depend download chksum:
	@case `uname -s` in\
		Linux) PLATFORM=Linux ;;\
		FreeBSD) PLATFORM=FreeBSD ;;\
		MINGW32_NT*|MINGW64_NT*|MSYS_NT*|CYGWIN_NT*) PLATFORM=NT ;;\
		Darwin) PLATFORM=Mac ;;\
		Haiku) PLATFORM=Haiku ;;\
		*) echo 'Unknown platform: you can try `PLATFORM=Linux make -fbuild/Makefile` manually.'; exit 1 ;;\
	esac; export PLATFORM; $(MAKE) -fbuild/Makefile $@
