#ifndef begin
    #define begin(T)
#endif

#ifndef end
    #define end()
#endif

#ifndef u8
    #define u8(dest)
#endif

#ifndef u16
    #define u16(dest)
#endif

#ifndef blob
    #define blob(dest, size)
#endif

begin(BitmapHeader)
    blob(fontname, 128)
    u16(high16)
    u8(height)
end()

begin(BitmapGlyph)
    u16(low16)
    u8(stride)
end()

#undef begin
#undef end
#undef u8
#undef u16
#undef blob
