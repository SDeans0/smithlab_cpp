/* Part of Smith Lab software
 *
 * Copyright (C) 2020 University of Southern California and
 *                    Guilherme De Sena Brandine and Andrew D. Smith
 *
 * Authors: Guilherme De Sena Brandine and Andrew D. Smith
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "sam_rec.hpp"

#include <regex>
#include <sstream>

#include "cigar_utils.hpp"

using std::string;
using std::istringstream;
using std::runtime_error;
using std::regex;
using std::istream;
using std::ostream;

static bool
valid_cigar(const string &cigar, const string &seq) {
  return cigar_qseq_ops(cigar) == seq.size();
}

static bool
valid_seq(const string &read) {
  return regex_match(read, regex("\\*|[A-Za-z=.]+"));
}

static bool
valid_qual(const string &qual) {
  return regex_match(qual, regex("[!-~]+"));
}

istream &
operator>>(std::istream &in, sam_rec &r) {
  string buffer;
  if (getline(in, buffer)) {
    r = sam_rec(buffer);
  }
  return in;
}

ostream &
operator<<(std::ostream &the_stream, const sam_rec &r) {
  the_stream << r.qname << '\t'
             << r.flags << '\t'
             << r.rname << '\t'
             << r.pos << '\t'
             << static_cast<unsigned>(r.mapq) << '\t'
             << r.cigar << '\t'
             << r.rnext << '\t'
             << r.pnext << '\t'
             << r.tlen << '\t'
             << r.seq << '\t'
             << r.qual;

  return the_stream;
}

sam_rec::sam_rec(const string &line) {
  istringstream iss(line); // ADS: change to set the buffer from "line"
  uint16_t tmp; // to not read mapq as character since it's uint8_t
  if (!(iss >> qname >> flags >> rname >> pos >> tmp >> cigar
        >> rnext >> pnext >> tlen >> seq >> qual))
    throw runtime_error("malformed line in SAM format:\n" + line);
  if (mapq > 255)
    throw runtime_error("invalid mapq in SAM record: " + line);
  if (!valid_cigar(cigar, seq))
    throw runtime_error("invalid cigar in SAM record: " + line);
  if (!valid_seq(seq))
    throw runtime_error("invalid read in SAM record: " + line);
  if (!valid_qual(qual))
    throw runtime_error("invalid qual in SAM record: " + line);

  mapq = static_cast<uint8_t>(tmp);
}
