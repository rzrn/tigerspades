from bdfparser import Font, Bitmap
import struct
import sys

_, *files = sys.argv

for file in files:
    font = Font(file)

    print(font.headers)

    fontname = font.headers['fontname']
    height = font.headers['fbby']
    high16 = 0x00000000

    if height > 255:
        raise ValueError("too big font")

    fout = open(file + '.out', 'wb')
    fout.write(struct.pack("<128sHb", fontname.encode('utf-8'), high16, height))

    for glyph in font.iterglyphs():
        codepoint = glyph.cp()

        if (codepoint >> 16) != high16:
            print("skipped codepoint %x" % codepoint)
            continue

        if codepoint > 0x7F:
            continue

        low16 = codepoint & 0xFFFF

        bitmap = glyph.draw(2)
        if bitmap.height() != height:
            raise ValueError("bad height glyph (%d)" % codepoint)

        if bitmap.width() > 255:
            raise ValueError("too big glyph (%d)" % codepoint)

        if bitmap.width() % 8 != 0:
            raise ValueError("invalid glyph width (%d)" % codepoint)

        data = bitmap.tobytes('1', {0: 0, 1: 1, 2: 1})
        fout.write(struct.pack("<Hb", low16, bitmap.width() // 8) + data)

    fout.close()

