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
from itertools import batched
import struct

VERSION = 2

from bdffont import BdfFont

argparser = ArgumentParser(
    description = "BDF to the TigerSpades font format conversion tool"
)

argparser.add_argument("-h16", "--high-16", type = int, metavar = "HIGH16", dest = "high16", default = 0x0000,
                       help = "high 16 bytes of the Unicode plane")
argparser.add_argument("-l16", "--max-low-16", type = int, metavar = "MAXLOW16", dest = "maxlow16", default = 0xFFFF,
                       help = "low 16 bytes after which glyphs are discarded")
argparser.add_argument("filename", nargs = '*', help = "BDF font to convert")

params = argparser.parse_args()

def of_bit_list(bs):
    N = 0

    for b in bs:
        N = (N << 1) | b

    return N

for filename in params.filename:
    font = BdfFont.load(filename)

    fontname  = font.name.encode('utf-8')
    copyright = font.properties.get('COPYRIGHT', '').encode('utf-8')
    notice    = font.properties.get('NOTICE', '').encode('utf-8')

    _, fbbh, fbbx, fbby = font.bounding_box

    assert fbbx == 0, "Non-zero X coordinate of the font bounding box is not supported: '{}'".format(filename)

    headers = ", ".join("{} = {}".format(k, v) for k, v in font.properties.items())
    print("{}: {}: {}".format(argparser.prog, filename, headers))

    assert fbbh <= 255, "A font is too big: {}".format(filename)

    fout = open(filename.removesuffix('.bdf') + '.bitmap', 'wb')

    fout.write(
        struct.pack("<H128s512s512sHB", VERSION, fontname, copyright, notice, params.high16, fbbh)
    )

    for glyph in font.glyphs:
        bbw, bbh, bbx, bby = glyph.bounding_box
        codepoint = glyph.encoding

        low16 = codepoint & 0xFFFF
        high16 = (codepoint >> 16) & 0xFFFF

        if low16 > params.maxlow16:
            continue

        if high16 != params.high16:
            print(
                "{}: skipped codepoint {:x}".format(argparser.prog, codepoint)
            )

            continue

        assert bbx == fbbx and bby == fbby, "Unsupported glyph bounding box: {}: {:x}".format(filename, codepoint)
        assert bbh == fbbh, "Bad glyph height: {}: {:x}".format(filename, codepoint)

        fout.write(struct.pack("<HB", low16, bbw))

        for row in glyph.bitmap:
            for bs in batched(row, 8):
                b = of_bit_list(reversed(bs))
                fout.write(struct.pack("<B", b))

    fout.close()
