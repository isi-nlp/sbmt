#define GRAEHL__SINGLE_MAIN
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/stopwatch.hpp>
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/brf_archive_io.hpp>
#include <iostream>
#include <fstream>

char const* usage_str="compiles (new decoder) brf to binary (new decoder) grammar archive\n";

using namespace sbmt;
using namespace std;
using namespace boost;

template <class TF, class AR>
void write_archive(std::istream& in, std::ostream& out)
{
    brf_archive_writer_tmpl<TF> w;
    AR oar(out);
    w(in,oar);
}

int main(int argc, char** argv)
{
    graehl::time_space_report report(cerr,"Finished archiving brf rules: ");

    /*
    if (argc != 3) {
        cerr << "usage: archive_grammar <brf-file> <archive-output-file>" << endl;
        return -1;
    }

    cerr << "opening grammar file: "<< argv[1] << endl;
    cerr << "writing grammar archive: "<< argv[2] << endl;
    graehl::stopwatch timer;

    graehl::istream_arg input_file(argv[1]);
    graehl::ostream_arg output_file(argv[2]);
    */


    // copied from binal.cc
  namespace prog = boost::program_options;
  using namespace graehl;
  typedef prog::options_description OD;
  istream_arg input_file("-");
  ostream_arg output_file("-");
  bool help;
  archive_type a = gar;
  OD all(general_options_desc());
  ostream &log=cerr;
  all.add_options()
      ("help,h", prog::bool_switch(&help), "show usage/documentation")
      ("input,i", defaulted_value(&input_file), "text rules (binarized) input")
      ("output,o", defaulted_value(&output_file), "grammar archive output")
      ("archive-type,a", defaulted_value(&a), "legal values: brf, archive, text-archive, fat-archive, fat-text-archive")
      ;
  prog::positional_options_description po;
  po.add("input",1);
  po.add("output",1);
  try {
      prog::variables_map vm;
      store(parse_command_line(argc,argv,all),vm);
      notify(vm);

      if (help) {
          log << "\n" << argv[0]<<"\n\n"
                      << usage_str << "\n"
                      << all << "\n";
          return 1;
      }
  } catch (std::exception &e) {
      log << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
  }
  istream &input=input_file.stream();
  ostream &output=output_file.stream();

  using namespace boost::archive;
  switch (a) {
        case gar:
            write_archive<indexed_token_factory,binary_oarchive>(input,output);
            break;
        case text_gar:
            write_archive<indexed_token_factory,text_oarchive>(input,output);
            break;
        case fat_gar:
            write_archive<fat_token_factory,binary_oarchive>(input,output);
            break;
        case fat_text_gar:
            write_archive<fat_token_factory,text_oarchive>(input,output);
            break;
        default:
            log << "ERROR: unrecognized archive type\n";
            return -1;
	}
    brf_archive_writer write;
    boost::archive::binary_oarchive oar(output);
    write(input,oar);

    cerr << "archive written to " << output_file << endl;
    //<< " - time elapsed: "<< timer << endl;

    return 0;
}
