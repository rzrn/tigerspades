all clean nuke game depend chksum:
	@case `uname -s` in\
		'Linux') PLATFORM=UNIX ;;\
		'FreeBSD') PLATFORM=UNIX ;;\
		'NetBSD') PLATFORM=UNIX ;;\
		'OpenBSD') PLATFORM=UNIX ;;\
		'Windows_NT') PLATFORM=NT ;;\
		'Darwin') PLATFORM=Mac ;;\
		*) echo 'Unknown platform: try `PLATFORM=UNIX make -fbuild/Makefile` manually.'; exit 1 ;;\
	esac; export PLATFORM; $(MAKE) -fbuild/Makefile $@
