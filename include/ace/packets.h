#ifndef begin
    #define begin(T)
#endif

#ifndef end
    #define end()
#endif

#ifndef c8
    #define c8(dest)
#endif

#ifndef u8
    #define u8(dest)
#endif

#ifndef u16
    #define u16(dest)
#endif

#ifndef u32
    #define u32(dest)
#endif

#ifndef f32
    #define f32(dest)
#endif

#ifndef v3f
    #define v3f(dest)
#endif

#ifndef v3i
    #define v3i(dest)
#endif

#ifndef bgr
    #define bgr(dest)
#endif

#ifndef string
    #define string(dest)
#endif

#ifndef blob
    #define blob(dest, size)
#endif

#ifndef PACKET_CLIENTSIDE
    #define PACKET_CLIENTSIDE 1
#endif

#ifndef PACKET_SERVERSIDE
    #define PACKET_SERVERSIDE 1
#endif

#ifndef PACKET_INCOMPLETE
    #define PACKET_INCOMPLETE 1
#endif

#ifndef PACKET_EXTRA
    #define PACKET_EXTRA 1
#endif

#include <AceOfSpades/_packets.h>

#undef begin
#undef end
#undef c8
#undef u8
#undef u16
#undef u32
#undef f32
#undef v3f
#undef v3i
#undef bgr
#undef string
#undef blob

#undef PACKET_CLIENTSIDE
#undef PACKET_SERVERSIDE
#undef PACKET_INCOMPLETE
#undef PACKET_EXTRA
