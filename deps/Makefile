CFILES = deps/libdeflate/gzip_decompress.c\
         deps/libdeflate/deflate_decompress.c\
         deps/libdeflate/x86/cpu_features.c\
         deps/libdeflate/crc32.c\
         deps/libdeflate/utils.c\
         deps/libdeflate/zlib_compress.c\
         deps/libdeflate/gzip_compress.c\
         deps/libdeflate/zlib_decompress.c\
         deps/libdeflate/deflate_compress.c\
         deps/libdeflate/arm/cpu_features.c\
         deps/libdeflate/adler32.c\
         deps/GLEW/glew.c\
         deps/libvxl/libvxl.c\
         deps/dr_libs/dr_wav.c\
         deps/log/log.c\
         deps/enet/list.c\
         deps/enet/host.c\
         deps/enet/unix.c\
         deps/enet/packet.c\
         deps/enet/callbacks.c\
         deps/enet/compress.c\
         deps/enet/peer.c\
         deps/enet/win32.c\
         deps/enet/protocol.c\
         deps/parson/parson.c\
         deps/http/http.c\
         deps/hashtable/hashtable.c\
         deps/ini/ini.c\
         deps/microui/microui.c

OFILES  = $(CFILES:.c=.o)
ARCHIVE = build/bsdeps.a
DEPEND  = deps/Makefile.depend
_CFLAGS = -std=gnu99 -DLOG_USE_COLOR $(XFLAGS)

include build/Makecommon
