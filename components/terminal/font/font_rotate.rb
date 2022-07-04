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

def rot(ary)
  res = [0, 0, 0, 0, 0, 0, 0, 0]
  res.length.times do |i|
    mi = 1 << (7-i)
    ary.length.times do |j|
      mj = 1 << j
      res[i] |= mj if ary[j] & mi != 0
    end
  end
  res
end


File.open("8x8r.fnt", "wb") do |fo|
  File.open("8x8.fnt", "rb") do |fi|
    while not fi.eof?
      bi = fi.read(8).unpack("C*")
      bo = rot bi
      fo.write bo.pack("C*")
    end
  end
end
