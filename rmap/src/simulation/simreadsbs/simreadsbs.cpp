/*    simreadsbs: a program for simulating Solexa reads to test rmapbs
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

#include "rmap_utils.hpp"
#include "rmap_os.hpp"
#include "sim_utils.hpp"
#include "bisulfite_utils.hpp"
#include "RNG.hpp"
#include "OptionParser.hpp"
#include "QualityScore.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <set>
#include <cmath>

using std::ofstream;
using std::ostream_iterator;
using std::numeric_limits;
using std::string;
using std::vector;
using std::ostream;
using std::endl;
using std::cerr;
using std::ptr_fun;
using std::max;
using std::min;

static void
get_error_log(const string &seq, const string &called_seq, 
	      string &error_log) {
  error_log = string(seq.length(), '0');
  for (size_t i = 0; i < seq.length(); ++i)
    if (seq[i] != called_seq[i])
      error_log[i] = '1';
}


static void
simreads_bs(const Runif &rng,
	    const size_t n_reads, 
	    const size_t read_width, 
	    const size_t max_errors, 
	    
	    const double bs_rate,
	    const double meth_rate,
	    
	    const string &name, 
	    const string &sequence,
	    vector<string> &read_names, 
	    vector<string> &reads,
	    vector<vector<vector<double> > > &probs) {
  
  const size_t lim = sequence.length() - read_width + 1;
  for (size_t i = 0; i < n_reads; ++i) {
    
    // Get a valid starting point
    size_t start = numeric_limits<size_t>::max();
    while (start == numeric_limits<size_t>::max()) {
      start = rng.runif(0ul, lim);
      for (size_t j = 0; j < read_width && 
	     start != numeric_limits<size_t>::max(); ++j)
	if (!isvalid(sequence[start + j]))
	  start = numeric_limits<size_t>::max();
    }
    
    // extract the sequence (and decide one revcomp)
    string seq(sequence.substr(start, read_width));
    transform(seq.begin(), seq.end(), seq.begin(), std::ptr_fun(&toupper));
    const bool rc = (rng.runif(0.0,1.0) > 0.5);
    if (rc) seq = revcomp(seq);
    
    // Do the bisulfite treatment
    bisulfite_treatment(rng, seq, bs_rate, meth_rate);
    
    vector<vector<double> > matrix;
    sequence_to_consensus_matrix(seq, matrix);
    add_sequencing_errors(rng, max_errors, matrix);

    string called_seq;
    call_bases_solexa(matrix, called_seq);

    // Sample errors
    string error_log;
    get_error_log(seq, called_seq, error_log);
    
    for (size_t j = 0; j < matrix.size(); ++j)
      for (size_t k = 0; k < matrix[j].size(); ++k)
	matrix[j][k] = round(error_probability_to_solexa(1.0 - matrix[j][k]));
    
    // Make name
    size_t actual_mismatches = count(error_log.begin(), error_log.end(), '1');
    const string read_name(name + ":" + toa(start) + "-" + 
			   toa(start + read_width) + "_" + toa(!rc) + "_" +
			   error_log + "_" + toa(actual_mismatches));
    
    // Push back what was sampled
    reads.push_back(called_seq);
    read_names.push_back(read_name);
    probs.push_back(matrix);
  }
}


static void
prb_to_fastq(const string &read,
	     const vector<vector<double> > &prb,
	     string &fastq) {
  for (size_t i = 0; i < read.length(); ++i) {
    const double score = *max_element(prb[i].begin(), prb[i].end());
    fastq += solexa_to_quality_character(score + 5);
  }
}


static void
write_reads_fastq(const string &outfile, 
		  const vector<string> &reads,
		  const vector<string> &read_names,
		  const vector<vector<vector<double> > > &probs) {
  ofstream out(outfile.c_str());
  for (size_t i = 0; i < reads.size(); ++i) {
    string fastq;
    prb_to_fastq(reads[i], probs[i], fastq);
    out << "@" << read_names[i] << endl
	<< reads[i] << endl
	<< "+" << read_names[i] << endl
	<< fastq << endl;
  }
  out.close();
}
  

static void
write_reads_fasta(const string &outfile, 
		  const vector<string> &reads,
		  const vector<string> &read_names) {
  ofstream out(outfile.c_str());
  for (size_t i = 0; i < reads.size(); ++i)
    out << ">" << read_names[i] << endl << reads[i] << endl;
  out.close();
}


static void
write_reads_prb(const string &prb_file, 
		const vector<vector<vector<double> > > &probs) {
  ofstream out(prb_file.c_str());
  for (size_t i = 0; i < probs.size(); ++i) {
    for (size_t j = 0; j < probs[i].size(); ++j)
      copy(probs[i][j].begin(), probs[i][j].end(),
	   ostream_iterator<double>(out, "\t"));
    out << endl;
  }
  out.close();
}


int
main(int argc, const char **argv) {

  try {
    
    string prb_file;
    string seqfile;
    string outfile;
    size_t n_reads = 1000;
    size_t read_width = 25;
    size_t max_errors = 0;
    double meth_rate = 0.0;
    double bs_rate = 1.0;
    size_t random_number_seed = numeric_limits<size_t>::max();

    bool VERBOSE = false;
    bool FASTQ_OUTPUT = false;
    
    /****************** COMMAND LINE OPTIONS ********************/
    static OptionParser 
      opt_parse("simreadsbs",
		"program for generating simulated bisulfite treated reads with "
		"simulated quality scores",
		"<fasta-chrom-files>");
    opt_parse.add_opt("output", 'o', "Name of output file (default: stdout)", false , outfile);
    opt_parse.add_opt("reads", 'n', "number of reads to simulate", false , n_reads);
    opt_parse.add_opt("width", 'w', "width of reads to simulate", false, read_width);
    opt_parse.add_opt("err", 'e', "maximum number of simulated sequencing errors", 
		      false, max_errors);
    opt_parse.add_opt("verbose", 'v', "print more run info", 
		      false, VERBOSE);
    opt_parse.add_opt("fastq", 'q', "write FASTQ format reads", 
		      false, FASTQ_OUTPUT);
    opt_parse.add_opt("prob", 'p', "prb output file", false, prb_file);
    opt_parse.add_opt("meth", 'm', "rate of CpG methylation", 
		      false, meth_rate); 
    opt_parse.add_opt("bs", 'b', "rate of bisulfite conversion", 
		      false, bs_rate);
    opt_parse.add_opt("seed", 'S', "random number seed", 
		      false, random_number_seed);
    vector<string> leftover_args;
    opt_parse.parse(argc, argv, leftover_args);
    if (argc == 1 || opt_parse.help_requested()) {
      cerr << opt_parse.help_message() << endl;
      return EXIT_SUCCESS;
    }
    if (opt_parse.about_requested()) {
      cerr << opt_parse.about_message() << endl;
      return EXIT_SUCCESS;
    }
    if (opt_parse.option_missing()) {
      cerr << opt_parse.option_missing_message() << endl;
      return EXIT_SUCCESS;
    }
    if (leftover_args.empty()) {
      cerr << opt_parse.help_message() << endl;
      return EXIT_SUCCESS;
    }
    vector<string> filenames(leftover_args);
    /****************** END COMMAND LINE OPTIONS *****************/
    
    if (FASTQ_OUTPUT && !prb_file.empty())
      throw RMAPException("fastq output is incompatible "
			  "with specifying a prb file");
    
    const Runif rng(random_number_seed);
    
    vector<string> reads, read_names;
    vector<vector<vector<double> > > probs;
    
    vector<size_t> filesizes;
    double total = 0;
    for (size_t i = 0; i < filenames.size(); ++i) {
      filesizes.push_back(get_filesize(filenames[i]));
      total += filesizes.back();
    }
    
    vector<size_t> samples;
    for (size_t i = 0; i < filesizes.size(); ++i)
      samples.push_back(n_reads*filesizes[i]/total);
    
    for (size_t i = 0; i < filenames.size(); ++i) {
      if (VERBOSE)
	cerr << filenames[i] << endl;
      
      vector<string> names, sequences;
      read_fasta_file(filenames[i].c_str(), names, sequences);
      
      for (size_t j = 0; j < names.size(); ++j) {
	const size_t offset = names[j].find(':');
	const string name(names[j].substr(0, offset));
	simreads_bs(rng, samples[i], read_width, max_errors, 
		    bs_rate, meth_rate, name, sequences[j], 
		    read_names, reads, probs);
      }
    }

    if (FASTQ_OUTPUT)
      write_reads_fastq(outfile, reads, read_names, probs);
    else {
      if (!prb_file.empty())
	write_reads_prb(prb_file, probs);
      write_reads_fasta(outfile, reads, read_names);
    }
    
  }      
  catch (std::bad_alloc &ba) {
    cerr << "ERROR: could not allocate memory" << endl;
    return EXIT_FAILURE;
  }
  catch (RMAPException &e) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  catch (std::exception &e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
