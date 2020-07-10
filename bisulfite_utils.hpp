/*
 *    Part of SMITHLAB software
 *
 *    Copyright (C) 2008 Cold Spring Harbor Laboratory, 
 *                       University of Southern California and
 *                       Andrew D. Smith
 *
 *    Authors: Andrew D. Smith
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef BISULFITE_UTILS_HPP
#define BISULFITE_UTILS_HPP

#include <string>
#include <random>

namespace bsflags {
  static const sam_rec::flags_t a_rich = 0x1000;
  constexpr bool
  is_a_rich(const sam_rec::flags_t flags) {
    return (flags & bsflags::a_rich) != 0;
  }
};

enum conversion_type { t_rich = false, a_rich = true };
constexpr conversion_type
flip_conv(const conversion_type conv) {
  return conv == t_rich ? a_rich : t_rich;
}

void
bisulfite_treatment(std::mt19937 &generator, std::string &seq, 
		    double bs_rate = 1.0, double meth_rate = 0.0);

#endif
