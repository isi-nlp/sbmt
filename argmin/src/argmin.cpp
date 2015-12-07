/*  $Id: argmin.cpp 1298 2006-10-03 04:43:58Z jturian $
 *  Copyright (c) 2006, Information Sciences Institute. All rights reserved. */
/*!
 *  \file argmin.cpp
 *  $LastChangedDate: 2006-10-02 21:43:58 -0700 (Mon, 02 Oct 2006) $
 *  $Revision: 1298 $
 *
 *  \todo Go over mini_decoder.cpp and mimic all its functionality.
 *  \todo Many of these functions belong in the SBMT library.
 *  \todo Add intruction_file functionality
 *  \todo Only call ensure_implicit_weights() if a particular program option is set (Pust / Graehl)
 *
 */


/// \todo Put these in the SBMT library.
#define MAX_NGRAM_ORDER 5
#define WEIGHT_LM_NGRAM "weight-lm-ngram"
#define GRAEHL__SINGLE_MAIN


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Minimizer.hpp"
#include "types.hpp"

#include "gpl/Debug.hpp"
#include "gpl/NBest.hpp"

#include "../sbmt_decoder/include/sbmt/grammar/brf_archive_io.hpp"
#include "../utilities/grammar_args.hpp"
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/is_null.hpp>

#include <istream>
#include <fstream>
//#include <iostream>
//#include <cstdlib>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>

#ifndef DOXYGEN
//using namespace std;
#endif

/// \todo Use a longer buffer
enum { BUFSIZE = 8192 };

/*
/// \todo Read in an input sentence instead of hard-coding it
//const string input_sentence = "c1 c2 c3 c4 c5 c6";
//const string input_sentence = "<foreign-sentence> c1 c2 c3 c4 c5 c6";

/// Location from which we read the grammar file.
/// This grammar must be in the new weights format, binarized, and then archived.
string grammar_file = "";
//const string grammar_file = "/home/jturian/sbmt/utilities/sample/small.xrs.new_wts.archive_i686-pc-linux-gnu";
//const string grammar_file = "/data/jturian/rules.cp.topk.corpus.1.new_wts.archive.i686_2";
//const string grammar_file = "/data/jturian/100K-rules.archive.i686";

/// \todo Read this in, don't hardcode it
string weight_file = "";
//const string weight_file = "/home/jturian/sbmt/utilities/sample/small.weights";
*/

/// \todo Update this, as per mini_decoder.cppp
sbmt::derivation_output_options deriv_opt;

/// \todo Really necessary??
double &lm_weight()
{
        return deriv_opt.info_wt;
}

/*
/// \todo Move this into deriv_opt.info_wt
double lm_weight = 1;
*/

#include "../sbmt_decoder/include/sbmt/logmath.hpp"		// For sbmt::score_t
struct LMOptions {
	LMOptions() :
		lm_file(),
		ngram_order(3),
		higher_ngram_order(),
		openclass_lm(false),
		unknown_word_prob(1e-20),
		weight_lm_ngram(NAN),
		lm_at_numclass(true)
	{
#ifndef DOXYGEN
		using namespace graehl;
#endif
		set_null(higher_ngram_order);
	}

	bool using_lm() const { return !lm_file.empty(); }

	void validate() {
		using namespace graehl;
		if (this->using_lm() && !boost::filesystem::exists(lm_file))
			throw std::runtime_error(lm_file+" (lm file) not found");

		assert(ngram_order >= 1);
		if (is_null(higher_ngram_order) || higher_ngram_order < ngram_order)
			higher_ngram_order = ngram_order;
		if (ngram_order > MAX_NGRAM_ORDER || higher_ngram_order > MAX_NGRAM_ORDER)
			throw std::runtime_error("--ngram-order greater than MAX_NGRAM_ORDER (5?) unsupported");
	}

	/// file with Language Weaver format ngram language model - if empty, then no LM will be used)
	std::string lm_file;
	/// in search, separate and score sequences of at least this many words (2 is bigram, 3 is trigram, etc.)
	/// \invariant ngram_order > 1
	unsigned ngram_order;
	/// allow loading and use of higher order LM than ngram-order, scoring with the higher order when possible (but often limited to just --ngram-order)
	unsigned higher_ngram_order;
	/// use unigram p(<unk>) in your LM (which must be trained accordingly).  disables --unknown-word-penalty
	bool openclass_lm;
	/// lm probability assessed for each unknown word (when not using --open-class-lm)
	/// \todo Should we read this in using po::value<double> or po::value<sbmt::score_t> ? How do we read it in with boost::program_options? (Graehl)
	sbmt::score_t unknown_word_prob;
	/// weight for language model - overrides weight-lm-ngram in grammar weights file/string
	double weight_lm_ngram;
	/// replace all digits (0-9) with '@' - 12.3 becomes @@.@            
	bool lm_at_numclass;
};
LMOptions lm_options;

argmin::LM language_model;

/// \todo grammar_in_mem should contain this object, not vice-versa (Pust)
sbmt::grammar_args ga;

/// \todo Print this out at the end of main()
unsigned parses_failed = 0;

/// \todo This should be a part of the sbmt library.
void load_lm(std::string const& filename)
{
	Debug::log(1) << "loading language model (--higher-ngram-order="<<lm_options.higher_ngram_order<<"): " << lm_options.lm_file << "\n" WEIGHT_LM_NGRAM "="<< lm_weight() << endl;
	graehl::time_space_report report(cout,"LM loaded: ");
	language_model.reset(new sbmt::LWNgramLM(ga.grammar, lm_options.lm_at_numclass, !lm_options.openclass_lm, lm_options.unknown_word_prob, lm_options.higher_ngram_order));
	language_model->read(lm_options.lm_file.c_str());
	unsigned rep_order=language_model->getMaxOrder();
	Debug::log(1) << "loaded language model reports order="<<rep_order<<endl;
	if (rep_order < lm_options.ngram_order) {
		Debug::log(1) << "LM was lower ("<<rep_order<<") order than requested --ngram-order ("<<lm_options.ngram_order<<"); using lower order ngram_info since scores will be the same"<<endl;
		lm_options.ngram_order=rep_order;
	} else if (rep_order > lm_options.ngram_order)
		Debug::log(1) << "LM has higher ("<<rep_order<<") order than requested --ngram-order ("<<lm_options.ngram_order<<"); some words will be scored up to the higher order, as if you chose --higher-ngram-order="<<rep_order<<endl;

	if (Debug::dump_volatile_diagnostics())
		Debug::log(2) << stats::resource_usage() << "\n";
}


/// \todo Move most of this into Minimizer
/// \todo Actually assign the 1-best estring to translation
/// \todo Use the correct n-gram order
std::string decode(std::string const& i, unsigned sent_number, argmin::GrammarTemplate& gram, boost::shared_ptr<argmin::Interp> interp, boost::shared_ptr<argmin::StatementFactory> sf) {  
#ifndef DOXYGEN
	using namespace argmin;
#endif

	Debug::log(2) << "WORKING ON SENTENCE #" << sent_number << ": " << i << "\n";

	// Initialize statistics about Derivation%s
	DerivationStatistics::clear();

        std::string translation;
        boost::shared_ptr<graehl::stopwatch> timer(new graehl::stopwatch);

	Input input = sbmt::foreign_sentence(i);
        if (input.length() == 0) return "empty sentence";

#ifndef NO_LM
	assert(lm_options.ngram_order == USING_NGRAM_ORDER);
#endif /* NO_LM */
	try {
		if (lm_options.using_lm()) {
			/// \todo WRITEME...
		}

		Minimizer min(sent_number, input, gram, sf, interp, timer);
		NBest<Derivation> nbest = min.argmin();

		graehl::time_space_report report(cout,"\nNbest: ");

		/// \todo Rewrite in a less kludgy way
		unsigned n = 0;
		while (!nbest.empty()) {
			const StatementEquivalence& eq = nbest.top().equivalence();
			nbest.pop();
			cout << interp->nbest(sent_number, sbmt::best_derivation(eq), n) << flush;
			n++;
		}

		Debug::log(2) << "DONE WORKING ON SENTENCE: " << i << "\n";

		if (Debug::dump_volatile_diagnostics())
			Debug::log(2) << "sentence #" << sent_number << " length="<< input.length() << " time="<< *timer <<"\n";
		else
			Debug::log(2) << "sentence #" << sent_number << " length="<< input.length() << "\n";

        	return translation;
	} catch (exception& e) {
		++parses_failed;
		Debug::log(2) << "Failed to parse sentence " << sent_number <<": "<< e.what() << "\n";
		Debug::warning(__FILE__, __LINE__, "Failed to parse sentence");
		interp->print_nbest(cout, sent_number);
		cout << flush;
		return interp->nbest(sent_number);
        }
}


/// \todo Go over these steps with Michael Pust
///	- Understand everything and comment/annotate everything
///	line-by-line, giving variables more descriptive names
///	- Add bells and whistles (e.g. timing)
/// \todo Command-line parameters
/// \todo Error-checking! (e.g. of file opens)
int main_work(int argc, char *argv[])
{
#ifndef DOXYGEN
	using namespace argmin;
#endif

/*
	{
		cout << __FILE__ << ":" << __LINE__ << " " << "Grammar file: " << grammar_file << "\n";
		ifstream gramf(grammar_file.c_str());
		assert(gramf.good());
		sbmt::brf_archive_reader reader(gramf);

		ifstream weightf(weight_file.c_str());
		assert(weightf.good());
		string weightstr;
		/// \todo What about if there's more than one line?
		getline(weightf, weightstr);
		ScoreCombiner combine(weightstr);

		/// \todo Don't always call this! Only if a particular program_option is set!
		ensure_implicit_weights(combine);

		cout << __FILE__ << ":" << __LINE__ << " " << "Final feature weights: " << combine << "\n";
		combine.print(cout, '\n', '=');
		cout << __FILE__ << ":" << __LINE__ << " " << "\n";

		cout << __FILE__ << ":" << __LINE__ << " " << "loading grammar..." << flush;
		ga.grammar.load(reader, combine);
		cout << __FILE__ << ":" << __LINE__ << " " << "done" << endl;
	}
*/

//	ga.load_grammar();

	if (Debug::dump_volatile_diagnostics())
		Debug::log(2) << stats::resource_usage() << "\n";
#ifndef NO_LM
	load_lm(lm_options.lm_file);
#endif /* NO_LM */

	boost::shared_ptr<Interp> interp(new Interp(ga.grammar, deriv_opt));
	/// \todo Log elsewhere?
	interp->set_log(cout);
	//interp->set_log(log());

	/// \todo USEME?
	//typedef typename Interp::template nbest_printer<ostream> N_printer;
	//N_printer nprint(interp,nbest_out,sentid);

#ifdef NO_LM
	boost::shared_ptr<StatementFactory> sf(new StatementFactory(1.0, lm_weight(), boost::shared_ptr<InfoFactory>(new InfoFactory)));
#else /* !NO_LM */
	boost::shared_ptr<StatementFactory> sf(new StatementFactory(1.0, lm_weight(), boost::shared_ptr<InfoFactory>(new InfoFactory(language_model))));
#endif

        ga.grammar_summary();

	unsigned sent_number = 0;
	while (!cin.eof()) {
		char buf[BUFSIZE];

		cin.getline(buf, BUFSIZE-1);
		if (cin.eof()) break;
		string i(buf);
		sent_number++;

		/// \todo What does mini_decoder.cpp do with the result of translation?
		string translation = decode(i, sent_number, ga.grammar, interp, sf);
	}

	/// Move this to destructor
	if (language_model.use_count()>1) {
		//warning() << "language model held by "<<language_model.use_count()<<" shared_ptr ... may not be destroyed\n";
		Debug::log(1) << "language model held by "<<language_model.use_count()<<" shared_ptr ... may not be destroyed\n";
//		language_model.get()->~LWNgramLM();
		language_model.reset();
	}


	return EXIT_SUCCESS;
}

/// \return True if argument parsing was successful
/// \todo Go over this function and compare it to that in mini_decoder.cpp
///	- e.g. graehl::printable_options_description<std::ostream>
/// \todo Perhaps make this a member function of Minimizer?
/// \todo Output all parameters afterwards.
/// \todo Indicate which parameters are and are not optional
/// \todo Allow '-' and 'f.gz' type filenames (Graehl)
/// \todo Don't make a local Debug object here, otherwise we destroy Debug at the end of this function
bool parse_args(int argc, char *argv[]) {
	namespace po = boost::program_options;
	typedef po::options_description OD;

	OD general_opts("General options");
	general_opts.add_options()
		("help,h", "produce help message")
/*
		("grammar,g", po::value<string>(&grammar_file), "Grammar file (new weight format, binarized, and archived)")
		("weights,w", po::value<string>(&weight_file), "Weight file")
*/
	;

	DebugOptions dbg;
	OD debug_opts("Debug options");
	debug_opts.add_options()
		("debuglevel", po::value<unsigned>(&dbg.debuglevel), "Global debug level. Only output debug messages that are logged not above this debug level. [default: 3]")
		("logfile", po::value<string>(&dbg.logfile), "File to log debug output (in addition to cerr). [default: \"\", i.e. none]")
		("buffer", po::value<bool>(&dbg.buffer), "Buffer log output? [default: false]")
		("dump_volatile_diagnostics", po::value<bool>(&dbg.dump_volatile_diagnostics), "Dump volatile diagnostics in debug output? These include time and memory usage. [default: true]")
	;

	OD grammar_opts("Grammar options");
	ga.add_options(grammar_opts);

	OD lm_opts("Language model options");
	double unknown_word_prob;
	lm_opts.add_options()
		("lm-ngram", po::value<std::string>(&lm_options.lm_file),
		 "file with Language Weaver format ngram language model - if empty, then no LM will be used")
		("ngram-order", po::value<unsigned>(&lm_options.ngram_order),
		 "in search, separate and score sequences of at least this many words (2 is bigram, 3 is trigram, etc.)")
		("higher-ngram-order", po::value<unsigned>(&lm_options.higher_ngram_order),
		 "allow loading and use of higher order LM than ngram-order, scoring with the higher order when possible (but often limited to just --ngram-order)")
		("open-class-lm", po::value<bool>(&lm_options.openclass_lm),
		 "use unigram p(<unk>) in your LM (which must be trained accordingly).  disables --unknown-word-penalty")
		("unknown-word-prob", po::value<double>(&unknown_word_prob),
		 "lm probability assessed for each unknown word (when not using --open-class-lm)")
		(WEIGHT_LM_NGRAM , po::value<double>(&lm_options.weight_lm_ngram),
		 "weight for language model - overrides weight-lm-ngram in grammar weights file/string")
		("lm-at-numclass", po::value<bool>(&lm_options.lm_at_numclass),
		 "replace all digits (0-9) with '@' - 12.3 becomes @@.@")            
		;

	OD all;
	all.add(general_opts).add(debug_opts).add(grammar_opts).add(lm_opts);

	// What do these steps here do? And why do they differ from the mini_decoder.cpp code?
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, all), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << all << "\n";
		cout << "Input sentences are read from stdin, one per line\n";
		cout << "(no more than " << BUFSIZE << " bytes per line)\n";
		return false;
	}

/*
	assert(vm.count("grammar"));
	assert(vm.count("weights"));
*/

	if (vm.count("unknown-word-prob")) lm_options.unknown_word_prob = unknown_word_prob;

	//all.print(cout, vm, OD::SHOW_DEFAULTED | OD::SHOW_HIERARCHY);

	/// Open the Debug stream with the specified DebugOptions
	/// \todo Don't make a local Debug object here, otherwise we destroy Debug at the end of this function
	(Debug)dbg;

	lm_options.validate();

	ga.set_log(cout);
/*
	if (!prior_file.is_none())
		ga.set_prior(prior_file.stream(),prior_floor_prob,prior_bonus_count);
	ga.prepare(not instruction_file.valid());
 */
	ga.prepare();

	ga.assign_weight(WEIGHT_LM_NGRAM, lm_weight());

	if (!is_null(lm_options.weight_lm_ngram)) {
		lm_weight() = lm_options.weight_lm_ngram;
		Debug::log(1) << "Using lm weight from --weight-lm-ngram: "<<lm_weight()<<"\n";
	}



/*
	decode_sequence_reader reader;
	reader.set_decode_callback(
			bind(&mini_decoder::decode_out,this,_1,ref(sentid))
			);
	reader.set_load_grammar_callback(
			bind(&grammar_args::load_grammar,&ga,_1,_2,_3)
			);
*/

	return true;
}

/// \todo USE_BACKTRACE? (Pust)
int main(int argc, char *argv[]) {
	deriv_opt.set_default();

	//mini_decoder m;
	try {
		if (!parse_args(argc, argv)) return 0;

		return main_work(argc, argv);
		/*
		   if (!m.parse_args(argc,argv))
		   return 0;
		   m.prepare();
		   if (m.run())
		   return 0;
		   else
		   return 2;
		 */
	} catch(bad_alloc& e) {
//		m.log() << "ERROR: ran out of memory\n\n";
		cerr << "ERROR: ran out of memory\n\n";
		goto fail;
	}
	catch(exception& e) {
//		m.log() << "ERROR: " << e.what() << "\n\n";
		cerr << "ERROR: " << e.what() << "\n\n";
		goto fail;
	}
	catch(const char * e) {
//		m.log() << "ERROR: " << e << "\n\n";
		cerr << "ERROR: " << e << "\n\n";
		goto fail;
	}
	catch(...) {
//		m.log() << "FATAL: Exception of unknown type!\n\n";
		cerr << "FATAL: Exception of unknown type!\n\n";
		return 2;
	}
	return 0;
fail:
//	m.log() << "Try '" << argv[0] << " -h' for help\n";
	cerr << "Try '" << argv[0] << " -h' for help\n";

	/*
#ifdef USE_BACKTRACE
	BackTrace::print(m.log());
#endif
*/

	return 1;
}
