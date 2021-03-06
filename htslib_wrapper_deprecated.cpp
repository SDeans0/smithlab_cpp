/* Part of SMITHLAB_CPP software
 * This code will be replaced soon
 *
 * Copyright (C) 2013-2019 University of Southern California and
 *                         Andrew D. Smith
 *
 * Authors: Meng Zhou, Qiang Song, Andrew Smith
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "htslib_wrapper_deprecated.hpp"
#include "smithlab_utils.hpp"
#include "MappedRead.hpp"
#include "cigar_utils.hpp"

using std::string;
using std::vector;
using std::cerr;
using std::endl;
using std::runtime_error;

char check_htslib_wrapper() {return 1;}

SAMReader_deprecated::SAMReader_deprecated(const string &fn, const string &mpr) :
  filename(fn), mapper(mpr), good(true) {

  if (!(hts = hts_open(filename.c_str(), "r")))
    throw runtime_error("cannot open file: " + filename);

  if (hts_get_format(hts)->category != sequence_data)
    throw runtime_error("file format appears wrong: " + filename);

  if (!(hdr = sam_hdr_read(hts)))
    throw runtime_error("failed to read header from file: " + filename);

  if (!(b = bam_init1()))
    throw runtime_error("failed to read record from file: " + filename);

  if (mapper == "bsmap")
    throw runtime_error("bsmap no longer supported [use general]");

  if (mapper != "bismark" && mapper != "bs_seeker" && mapper != "general")
    throw runtime_error("mapper not supported: " + mapper);
}

SAMReader_deprecated::~SAMReader_deprecated() {
  if (hdr) {
    bam_hdr_destroy(hdr);
    hdr = 0;
  }
  if (b) {
    bam_destroy1(b);
    b = 0;
  }
  if (hts) {
    assert(hts_close(hts) >= 0);
    hts = 0;
  }
  good = false;
}


bool
SAMReader_deprecated::get_SAMRecord(const string &str, SAMRecord &samr) {
  if (mapper == "bsmap")
    return get_SAMRecord_bsmap(str, samr);
  else if (mapper == "bismark")
    return get_SAMRecord_bismark(str, samr);
  else if (mapper == "bs_seeker")
    return get_SAMRecord_bsseeker(str, samr);
  else if (mapper == "general")
    return get_SAMRecord_general(str, samr);
  else
    good = false;
  return false;
}

SAMReader_deprecated&
operator>>(SAMReader_deprecated &reader, SAMRecord &aln) {
  int rd_ret = 0;
  if ((rd_ret = sam_read1(reader.hts, reader.hdr, reader.b)) >= 0) {
    int fmt_ret = 0;
    if ((fmt_ret = sam_format1(reader.hdr, reader.b, &reader.hts->line)) <= 0)
      throw runtime_error("failed reading record from: " + reader.filename);
    reader.good = reader.get_SAMRecord(reader.hts->line.s, aln);
    // ADS: line below seems to be needed when the file format is SAM
    // because the hts_getline for parsing SAM format files within
    // sam_read1 only get called when "(fp->line.l == 0)". For BAM
    // format, it does not seem to matter.
    reader.hts->line.l = 0;
  }
  if (rd_ret == -1)
    reader.good = false;
  else if (rd_ret < -1)
    throw runtime_error("failed to read record from file: " + reader.filename);
  return reader;
}


class FLAG {
public:
  FLAG(const size_t f) : flag(f) {}
  bool is_pairend() const {return flag & 0x1;}
  bool is_singlend() const {return !(is_pairend());}
  bool is_mapping_paired() const {return flag & 0x2;}
  bool is_unmapped() const {return flag & 0x4;}
  bool is_mapped() const {return !(is_unmapped());}
  bool is_revcomp() const {return flag & 0x10;}
  bool is_Trich() const {return flag & 0x40;}
  bool is_Arich() const {return flag & 0x80;}
  bool is_secondary() const {return flag & 0x100;}
  bool is_primary() const {return !(is_secondary());}
private:
  size_t flag;
};

////////////////////////////////////////
// BSMAP
////////////////////////////////////////

class BSMAPFLAG : public FLAG {
public:
  BSMAPFLAG(const size_t f) : FLAG(f) {}
};

inline static void
bsmap_get_strand(const string &strand_str, string &strand, string &bs_forward) {
  strand = strand_str.substr(5, 1);
  bs_forward = strand_str.substr(6, 1);
  if (bs_forward == "-")
    strand = (strand == "+" ? "-" : "+");
}

bool
SAMReader_deprecated::get_SAMRecord_bsmap(const string &str, SAMRecord &samr) {

  cerr << "WARNING: "<< "[BSMAP Converter] test "
       << "version: may contain bugs" << endl;

  string name, chrom, CIGAR, mate_name, seq, qual, strand_str, mismatch_str;
  size_t flag, start, mapq_score, mate_start;
  int seg_len;

  std::istringstream iss(str);
  if (!(iss >> name >> flag >> chrom >> start >> mapq_score >> CIGAR
        >> mate_name >> mate_start >> seg_len >> seq >> qual
        >> mismatch_str >> strand_str)) {
    good = false;
    throw runtime_error("malformed line in bsmap SAM format:\n" + str);
  }

  BSMAPFLAG Flag(flag);

  samr.mr.r.set_chrom(chrom);
  samr.mr.r.set_start(start - 1);
  samr.mr.r.set_name(name);
  samr.mr.r.set_score(atoi(mismatch_str.substr(5).c_str()));

  string strand, bs_forward;
  bsmap_get_strand(strand_str, strand, bs_forward);
  samr.mr.r.set_strand(strand[0]);

  string new_seq(seq);
  apply_cigar(CIGAR, new_seq);

  samr.mr.r.set_end(samr.mr.r.get_start() + new_seq.size());
  samr.mr.seq = new_seq;

  samr.is_Trich = Flag.is_Trich();
  samr.is_mapping_paired = Flag.is_mapping_paired();

  return good;
}

////////////////////////////////////////
// Bismark
////////////////////////////////////////

class BISMARKFLAG : public FLAG {
public:
  BISMARKFLAG(const size_t f) : FLAG(f) {}
  bool is_Trich() const {return is_pairend() ? FLAG::is_Trich() : true;}
};

static size_t
get_mismatch_bismark(const string &edit_distance_str,
                     const string &meth_call_str) {
  /* the result of this function might not be accurate, because if a
  sequencing error occurs on a cytosine, then it probably will be
  reported as a convertion
  */
  size_t edit_distance;
  edit_distance = atoi(edit_distance_str.substr(5).c_str());

  int convert_count = 0;
  const char *temp = meth_call_str.substr(5).c_str();
  while (*temp != '\0') {
    if (*temp == 'x' || *temp == 'h' || *temp == 'z')
      ++convert_count;
    ++temp;
  }

  return edit_distance - convert_count;
}

bool
SAMReader_deprecated::get_SAMRecord_bismark(const string &str, SAMRecord &samr) {
  string name, chrom, CIGAR, mate_name, seq, qual, strand_str,
    edit_distance_str, mismatch_str, meth_call_str,
    read_conv_str, genome_conv_str;
  size_t flag, start, mapq_score, mate_start;
  int seg_len;

  std::istringstream iss(str);
  if (!(iss >> name >> flag >> chrom >> start >> mapq_score >> CIGAR
        >> mate_name >> mate_start >> seg_len >> seq >> qual
        >> edit_distance_str >> mismatch_str >> meth_call_str
        >> read_conv_str >> genome_conv_str)) {
    good = false;
    throw runtime_error("malformed line in bismark SAM format:\n" + str);
  }

  BISMARKFLAG Flag(flag);

  samr.mr.r.set_chrom(chrom);
  samr.mr.r.set_start(start - 1);
  samr.mr.r.set_name(name);
  samr.mr.r.set_score(get_mismatch_bismark(edit_distance_str, meth_call_str));
  samr.mr.r.set_strand(Flag.is_revcomp() ? '-' : '+');

  string new_seq(seq);
  apply_cigar(CIGAR, new_seq);

  if (Flag.is_revcomp())
    revcomp_inplace(new_seq);

  samr.mr.r.set_end(samr.mr.r.get_start() + new_seq.size());
  samr.mr.seq = new_seq;

  samr.is_Trich = Flag.is_Trich();
  samr.is_mapping_paired = Flag.is_mapping_paired();

  return good;
}

////////////////////////////////////////
// BS Seeker
////////////////////////////////////////

class BSSEEKERFLAG : public FLAG {
public:
  BSSEEKERFLAG(const size_t f) : FLAG(f) {}
  // pair-end:
  //  if T-rich mate is +, then both mates are +;
  //  if T-rich mate is -, then both mates are -;
  // single-end:
  //  0 for +; 16 for -.
  bool is_Trich() const {
    return FLAG::is_pairend() ? FLAG::is_Trich() : true;
  }
  bool is_Arich() const {
    return FLAG::is_pairend() && FLAG::is_Arich();
  }
  bool is_revcomp() const {
    if (FLAG::is_pairend())
      return FLAG::is_revcomp() ? is_Trich() : is_Arich();
    else
      return FLAG::is_revcomp();
  }
};

bool
SAMReader_deprecated::get_SAMRecord_bsseeker(const string &str, SAMRecord &samr) {

  string name, chrom, CIGAR, mate_name, seq, qual, orientation_str,
    conversion_str, mismatch_str, mismatch_type_str, seq_genome_str;
  size_t flag, start, mapq_score, mate_start;
  int seg_len;

  std::istringstream iss(str);
  if (!(iss >> name >> flag >> chrom >> start >> mapq_score >> CIGAR
        >> mate_name >> mate_start >> seg_len >> seq >> qual
        >> orientation_str >> conversion_str >> mismatch_str
        >> mismatch_type_str >> seq_genome_str)) {
    good = false;
    throw runtime_error("malformed line in bs_seeker SAM format:\n" + str);
  }

  // bs_seeker also doesn't keep sequencing quality information?
  qual = string(seq.size(), 'h');

  BSSEEKERFLAG Flag(flag);

  samr.mr.r.set_chrom(chrom);
  samr.mr.r.set_start(start - 1);
  samr.mr.r.set_name(name);
  samr.mr.r.set_score(atoi(mismatch_str.substr(5).c_str()));
  samr.mr.r.set_strand(Flag.is_revcomp() ? '-' : '+');

  string new_seq(seq);
  apply_cigar(CIGAR, new_seq);

  if (Flag.is_revcomp())
    revcomp_inplace(new_seq);

  samr.mr.r.set_end(samr.mr.r.get_start() + new_seq.size());
  samr.mr.seq = new_seq;

  samr.is_Trich = Flag.is_Trich();
  samr.is_mapping_paired = Flag.is_mapping_paired();

  return good;
}

////////////////////////////////////////
// general: for non-BSseq mappers
////////////////////////////////////////

class GENERALFLAG : public FLAG {
public:
  GENERALFLAG(const size_t f) : FLAG(f) {}
  bool is_Trich() const {return is_pairend() ? FLAG::is_Trich() : true;}
};

bool
SAMReader_deprecated::get_SAMRecord_general(const string &str, SAMRecord &samr) {
  string name, chrom, CIGAR, mate_name, seq, qual;
  size_t flag, start, mapq_score, mate_start;
  int seg_len;

  std::istringstream iss(str);
  if (!(iss >> name >> flag >> chrom >> start >> mapq_score >> CIGAR
        >> mate_name >> mate_start >> seg_len >> seq >> qual)) {
    good = false;
    throw runtime_error("malformed line in SAM format:\n" + str);
  }

  GENERALFLAG Flag(flag);
  samr.is_primary = Flag.is_primary();
  samr.is_mapped = Flag.is_mapped();

  samr.seg_len = seg_len;
  samr.mr.r.set_name(name);

  if (samr.is_mapped) {
    samr.mr.r.set_chrom(chrom);
    samr.mr.r.set_start(start - 1);
    samr.mr.r.set_score(0);
    samr.mr.r.set_strand(Flag.is_revcomp() ? '-' : '+');

    string new_seq(seq);
    apply_cigar(CIGAR, new_seq);

    if (Flag.is_revcomp())
      revcomp_inplace(new_seq);

    samr.mr.r.set_end(samr.mr.r.get_start() + new_seq.size());

    samr.mr.seq = new_seq;

  }
  samr.is_Trich = Flag.is_Trich();
  samr.is_mapping_paired = Flag.is_mapping_paired();
  // if the mapped chromosomes differ but the mapping is "concordant",
  // then change the mapping to disconcordant
  if (samr.is_mapping_paired &&
      !((mate_name == "=") || (mate_name == chrom))) {
    samr.is_mapping_paired = false;
  }

  return good;
}
