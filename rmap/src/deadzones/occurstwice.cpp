/*    occurstwice: program for building library of all k-mers occurring
 *    twice in a genome
 *
 *    Copyright (C) 2009 University of Southern California and
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
#include "OptionParser.hpp"
#include "deadzone_utils.hpp"

#include <tr1/unordered_set>

using std::ofstream;
using std::ostringstream;
using std::ostream_iterator;
using std::string;
using std::vector;
using std::ostream;
using std::cout;
using std::endl;
using std::cerr;

using std::tr1::unordered_set;

int
main(int argc, const char **argv) {
  
  try {
    
    // Parameter variables
    size_t kmer = 0;
    string outfile;
  
    bool WRITE_TEXT = false;
    bool VERBOSE = false;
  
    /****************** COMMAND LINE OPTIONS ********************/
    OptionParser opt_parse("occurstwice", "program for building library "
			   "of all k-mers occurring twice in a genome.",
			   "<fasta-chrom-file>");
    opt_parse.add_opt("output", 'o', "Name of output file (default: stdout)", 
		      false, outfile);
    opt_parse.add_opt("kmer", 'k', "Width of k-mers", true, kmer);
    opt_parse.add_opt("text", 't', "Output in plain text", false, WRITE_TEXT);
    opt_parse.add_opt("verbose", 'v', "print more run information", 
		      false, VERBOSE);
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
    const string seqfile = leftover_args.front();
    /****************** END COMMAND LINE OPTIONS *****************/

    if (!WRITE_TEXT && outfile.empty())
      throw RMAPException("output file required to write binary");

    if (VERBOSE)
      cerr << seqfile << endl;
    vector<string> names;
    vector<string> sequences;
    read_fasta_file(seqfile.c_str(), names, sequences);
    
    unordered_set<long_index> reads;
    unordered_set<long_index> ambigs;
    
    long_index::set_index_size(static_cast<size_t>(kmer));

    for (size_t i = 0; i < sequences.size(); ++i) {

      for (size_t j = 0; j < sequences[i].size(); ++j)
	sequences[i][j] = std::toupper(sequences[i][j]);

      const string::const_iterator lim(sequences[i].end() - kmer + 1);

      for (string::const_iterator j(sequences[i].begin()); j < lim; ++j) {
	bool good = true;
 	for (size_t k = 0; k < static_cast<size_t>(kmer) && good; ++k)
	  if (!isvalid(*(j + k))) good = false;
	string s(j, j + kmer);
	transform(s.begin(), s.end(), s.begin(), std::ptr_fun(::toupper));
  	if (good) {
	  
	  const long_index key(j, j + kmer);
	  if (ambigs.find(key) == ambigs.end()) {
	    if (reads.find(key) != reads.end()) {
	      ambigs.insert(key);
	      reads.erase(key);
	    }
	    else reads.insert(key);
	  }
  	  const long_index key_rc(j, j + kmer, true);
	  if (key != key_rc) {
	    if (ambigs.find(key_rc) == ambigs.end()) {
	      if (reads.find(key_rc) != reads.end()) {
		ambigs.insert(key_rc);
		reads.erase(key_rc);
	      }
	      else reads.insert(key_rc);
	    }
	  }
 	}
      }
    }
    
    if (VERBOSE)
      cerr << reads.size() << endl;
    
    if (WRITE_TEXT) {
      ostream* out = (!outfile.empty()) ? new ofstream(outfile.c_str()) : &cout;
      for (unordered_set<long_index>::iterator i = ambigs.begin(); i != ambigs.end(); ++i)
	*out << i->tostring_bases() << endl;
      if (out != &cout) delete out;
    }
    else {
      if (outfile.empty()) {
	cerr << "output file required to write binary" << endl;
	return EXIT_FAILURE;
      }
      ofstream out(outfile.c_str(), std::ios::out | std::ios::binary);
      for (unordered_set<long_index>::iterator i = ambigs.begin(); i != ambigs.end(); ++i)
	out.write((const char *)&(*i), sizeof(*i));
    }
  }
  catch (RMAPException &e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  }
  catch (std::bad_alloc &ba) {
    cerr << "ERROR: could not allocate memory" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}