/*
    Copyright © 2024 rzrn

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
    u16(version)
    blob(fontname, 128)
    blob(copyright, 512)
    blob(notice, 512)
    u16(high16)
    u8(height)
end()

begin(BitmapGlyph)
    u16(low16)
    u8(width)
end()

#undef begin
#undef end
#undef u8
#undef u16
#undef blob
