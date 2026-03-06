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

import struct
import sys

from bdfparser import Font, Bitmap

progname, *filenames = sys.argv

for filename in filenames:
    font = Font(filename)

    headers = ", ".join("{} = {}".format(k, v) for k, v in font.headers.items())
    print("{}: {}: {}".format(progname, filename, headers))

    fontname = font.headers['fontname']
    height = font.headers['fbby']
    high16 = 0x00000000

    if height > 255:
        raise ValueError(
            "{}: {}: too big font".format(progname, filename)
        )

    fout = open(filename + '.out', 'wb')
    fout.write(struct.pack("<128sHb", fontname.encode('utf-8'), high16, height))

    for glyph in font.iterglyphs():
        codepoint = glyph.cp()

        if (codepoint >> 16) != high16:
            print(
                "{}: skipped codepoint {:x}".format(progname, codepoint)
            )

            continue

        low16 = codepoint & 0xFFFF

        bitmap = glyph.draw(2)

        if bitmap.height() != height:
            raise ValueError(
                "{}: {}: bad glyph height: {:x}".format(progname, filename, codepoint)
            )

        if bitmap.width() > 255:
            raise ValueError(
                "{}: {}: glyph too big: {:x}".format(progname, filename, codepoint)
            )

        if bitmap.width() % 8 != 0:
            raise ValueError(
                "{}: {}: invalid glyph width: {:x}".format(progname, filename, codepoint)
            )

        data = bitmap.tobytes('1', {0: 0, 1: 1, 2: 1})
        fout.write(struct.pack("<Hb", low16, bitmap.width() // 8) + data)

    fout.close()
