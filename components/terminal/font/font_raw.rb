/*
 * This file is part of the terminal component distribution
 * (https://gitlab.romanchenko.su/esp/components/terminal.git).
 * Copyright (c) 2022 Dmitry Romanchenko.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
W = 8
H = 16

XLAT = [

]

FONT = [

]

File.open("font#{W}x#{H}.fnt", "wb") do |fo|
  fo.write FONT.pack("C*")
end
File.open("xlat#{W}x#{H}.bin", "wb") do |fo|
  fo.write XLAT.pack("C*")
end
