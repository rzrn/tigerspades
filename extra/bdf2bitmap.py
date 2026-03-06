# Copyright © 2025–2026 rzrn

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

from argparse import ArgumentParser
import struct

from bdfparser import Font, Bitmap

argparser = ArgumentParser(
    description = "BDF to the TigerSpades font format conversion tool"
)

argparser.add_argument("-h16", "--high-16", type = int, metavar = "HIGH16", dest = "high16", default = 0x0000,
                       help = "high 16 bytes of the Unicode plane")
argparser.add_argument("-l16", "--max-low-16", type = int, metavar = "MAXLOW16", dest = "maxlow16", default = 0xFFFF,
                       help = "low 16 bytes after which glyphs are discarded")
argparser.add_argument("filename", nargs = '*', help = "BDF font to convert")

params = argparser.parse_args()

for filename in params.filename:
    font = Font(filename)

    headers = ", ".join("{} = {}".format(k, v) for k, v in font.headers.items())
    print("{}: {}: {}".format(argparser.prog, filename, headers))

    fontname = font.headers['fontname']
    height = font.headers['fbby']

    if height > 255:
        raise ValueError(
            "{}: {}: too big font".format(argparser.prog, filename)
        )

    fout = open(filename + '.out', 'wb')
    fout.write(struct.pack("<128sHb", fontname.encode('utf-8'), params.high16, height))

    for glyph in font.iterglyphs():
        codepoint = glyph.cp()

        low16 = codepoint & 0xFFFF
        high16 = (codepoint >> 16) & 0xFFFF

        if low16 > params.maxlow16:
            continue

        if high16 != params.high16:
            print(
                "{}: skipped codepoint {:x}".format(argparser.prog, codepoint)
            )

            continue

        bitmap = glyph.draw(2)

        if bitmap.height() != height:
            raise ValueError(
                "{}: {}: bad glyph height: {:x}".format(argparser.prog, filename, codepoint)
            )

        if bitmap.width() > 255:
            raise ValueError(
                "{}: {}: glyph too big: {:x}".format(argparser.prog, filename, codepoint)
            )

        if bitmap.width() % 8 != 0:
            raise ValueError(
                "{}: {}: invalid glyph width: {:x}".format(argparser.prog, filename, codepoint)
            )

        data = bitmap.tobytes('1', {0: 0, 1: 1, 2: 1})
        fout.write(struct.pack("<Hb", low16, bitmap.width() // 8) + data)

    fout.close()
