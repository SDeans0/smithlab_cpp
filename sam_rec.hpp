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

#ifndef SAM_REC_HPP
#define SAM_REC_HPP

#include <string>
#include <iostream>


class sam_rec {
public:
  typedef uint16_t flags_t;

  uint8_t mapq;
  uint32_t pos;
  uint32_t pnext;
  int32_t tlen;
  std::string qname;
  flags_t flags;
  std::string rname;
  std::string cigar;
  std::string rnext;
  std::string seq;
  std::string qual;
  sam_rec() {}
  sam_rec(const std::string &line);

};

namespace samflags {
  static const sam_rec::flags_t read_paired = 0x1;
  static const sam_rec::flags_t read_pair_mapped = 0x2;
  static const sam_rec::flags_t read_unmapped = 0x4;
  static const sam_rec::flags_t mate_unmapped = 0x8;
  static const sam_rec::flags_t read_rc = 0x10;
  static const sam_rec::flags_t mate_rc = 0x20;
  static const sam_rec::flags_t template_first = 0x40;
  static const sam_rec::flags_t template_second = 0x80;
  static const sam_rec::flags_t secondary_aln = 0x100;
  static const sam_rec::flags_t below_quality = 0x200;
  static const sam_rec::flags_t pcr_duplicate = 0x400;
  static const sam_rec::flags_t supplementary_aln = 0x800;

  bool is_read_paired(const sam_rec::flags_t f) {return f & read_paired;}
  bool is_read_pair_mapped(const sam_rec::flags_t f) {return f & read_pair_mapped;}
  bool is_read_unmapped(const sam_rec::flags_t f) {return f & read_unmapped;}
  bool is_mate_unmapped(const sam_rec::flags_t f) {return f & mate_unmapped;}
  bool is_read_rc(const sam_rec::flags_t f) {return f & read_rc;}
  bool is_mate_rc(const sam_rec::flags_t f) {return f & mate_rc;}
  bool is_template_first(const sam_rec::flags_t f) {return f & template_first;}
  bool is_template_second(const sam_rec::flags_t f) {return f & template_second;}
  bool is_secondary_aln(const sam_rec::flags_t f) {return f & secondary_aln;}
  bool is_below_quality(const sam_rec::flags_t f) {return f & below_quality;}
  bool is_pcr_duplicate(const sam_rec::flags_t f) {return f & pcr_duplicate;}
  bool is_supplementary_aln(const sam_rec::flags_t f) {return f & supplementary_aln;}

  void set_read_paired(sam_rec::flags_t &f) {f |= read_paired;}
  void set_read_pair_mapped(sam_rec::flags_t &f) {f |= read_pair_mapped;}
  void set_read_unmapped(sam_rec::flags_t &f) {f |= read_unmapped;}
  void set_mate_unmapped(sam_rec::flags_t &f) {f |= mate_unmapped;}
  void set_read_rc(sam_rec::flags_t &f) {f |= read_rc;}
  void set_mate_rc(sam_rec::flags_t &f) {f |= mate_rc;}
  void set_template_first(sam_rec::flags_t &f) {f |= template_first;}
  void set_template_second(sam_rec::flags_t &f) {f |= template_second;}
  void set_secondary_aln(sam_rec::flags_t &f) {f |= secondary_aln;}
  void set_below_quality(sam_rec::flags_t &f) {f |= below_quality;}
  void set_pcr_duplicate(sam_rec::flags_t &f) {f |= pcr_duplicate;}
  void set_supplementary_aln(sam_rec::flags_t &f) {f |= supplementary_aln;}

  void unset_read_paired(sam_rec::flags_t &f) {f &= ~read_paired;}
  void unset_read_pair_mapped(sam_rec::flags_t &f) {f &= ~read_pair_mapped;}
  void unset_read_unmapped(sam_rec::flags_t &f) {f &= ~read_unmapped;}
  void unset_mate_unmapped(sam_rec::flags_t &f) {f &= ~mate_unmapped;}
  void unset_read_rc(sam_rec::flags_t &f) {f &= ~read_rc;}
  void unset_mate_rc(sam_rec::flags_t &f) {f &= ~mate_rc;}
  void unset_template_first(sam_rec::flags_t &f) {f &= ~template_first;}
  void unset_template_second(sam_rec::flags_t &f) {f &= ~template_second;}
  void unset_secondary_aln(sam_rec::flags_t &f) {f &= ~secondary_aln;}
  void unset_below_quality(sam_rec::flags_t &f) {f &= ~below_quality;}
  void unset_pcr_duplicate(sam_rec::flags_t &f) {f &= ~pcr_duplicate;}
  void unset_supplementary_aln(sam_rec::flags_t &f) {f &= ~supplementary_aln;}
};

enum sam_record_type { single = 0, pe_first_mate = 1, pe_second_mate = 2 };


std::istream &
operator>>(std::istream &in, sam_rec &r);


std::ostream &
operator<<(std::ostream &the_stream, const sam_rec &r);

#endif
