#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "grammar_examples.hpp"
#include <sbmt/grammar/brf_archive_io.hpp>
#include <sbmt/grammar/brf_file_reader.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <sstream>

using namespace std;
using namespace sbmt;

void init_grammar_marcu_staggard_wts_stream(std::stringstream& stream);

fat_weight_vector example_grammar_sc;

char const* grammar_nts=
    "NPB 72530242 NP-C 61376616 PP 38052492 VP 27640114 NP 13976630 VP-C 13792376 S 10194474 S1 5430870 SG-C 5210874 S-C 4989412 SBAR 4632560 ADVP 4575430 ADJP 3574724 SG 2534402 PRN 2351888 SBAR-C 1905858 WHNP 1722402 CJJ 1408484 QP 720848 PRT 500854 WHADVP 424710 CNN 422508 CNNP 395632 LST 349994 PP-C 293580 WHPP 257998 FRAG 241846 SINV 175538 CONJP 168058 ADJP-C 164364 NX 161276 CCD 128436 UCP 117824 NAC 86236 CNNS 82490 SQ 79712 X 52044 SBARQ 29172 ADVP-C 17890 INTJ 12718 CVB 11280 CVBN 7720 CVBG 5648 CRB 5622 WHADJP 5180 WHNP-C 5018 CIN 3610 RRC 3490 CVBD 2446 CVBZ 1894 CJJS 1776 CJJR 1284 CFW 852 CNNPS 704 CMD 596 CLS 446 CVBP 428 UCP-C 248 CDT 224 CUH 144 CSYM 122 C-RRB- 114 SBARQ-C 68 FRAG-C 66 CCC 60 CTO 50 CWDT 36 C-LRB- 24 CPRP 22 C$ 16 CRBR 10 CPOS 10 CWRB 8 X-C 6 INTJ-C 6 SQ-C 2 PRT|ADVP 2 CWP 2 C`` 2 WRB 0 WP$ 0 WP 0 WDT 0 VBZ 0 VBP 0 VBN 0 VBG 0 VBD 0 VB 0 UH 0 TO 0 SYM 0 -RRB- 0 RP 0 RBS 0 RBR 0 RB 0 PRP$ 0 PRP 0 POS 0 PDT 0 NNS 0 NNPS 0 NNP 0 NN 0 MD 0 LS 0 -LRB- 0 JJS 0 JJR 0 JJ 0 IN 0 FW 0 EX 0 DT 0 CD 0 CC 0 # 0 $ 0 : 0 , 0 `` 0";
//    "\n NPB 72530242\n NP-C 61376616\n PP 38052492\n VP 27640114\n NP 13976630\n VP-C 13792376\n S 10194474\n S1 5430870\n SG-C 5210874\n S-C 4989412\n SBAR 4632560\n ADVP 4575430\n ADJP 3574724\n SG 2534402\n PRN 2351888\n SBAR-C 1905858\n WHNP 1722402\n CJJ 1408484\n QP 720848\n PRT 500854\n WHADVP 424710\n CNN 422508\n CNNP 395632\n LST 349994\n PP-C 293580\n WHPP 257998\n FRAG 241846\n SINV 175538\n CONJP 168058\n ADJP-C 164364\n NX 161276\n CCD 128436\n UCP 117824\n NAC 86236\n CNNS 82490\n SQ 79712\n X 52044\n SBARQ 29172\n ADVP-C 17890\n INTJ 12718\n CVB 11280\n CVBN 7720\n CVBG 5648\n CRB 5622\n WHADJP 5180\n WHNP-C 5018\n CIN 3610\n RRC 3490\n CVBD 2446\n CVBZ 1894\n CJJS 1776\n CJJR 1284\n CFW 852\n CNNPS 704\n CMD 596\n CLS 446\n CVBP 428\n UCP-C 248\n CDT 224\n CUH 144\n CSYM 122\n C-RRB- 114\n SBARQ-C 68\n FRAG-C 66\n CCC 60\n CTO 50\n CWDT 36\n C-LRB- 24\n CPRP 22\n C$ 16\n CRBR 10\n CPOS 10\n CWRB 8\n X-C 6\n INTJ-C 6\n SQ-C 2\n PRT|ADVP 2\n CWP 2\n C`` 2\n WRB 0\n WP$ 0\n WP 0\n WDT 0\n VBZ 0\n VBP 0\n VBN 0\n VBG 0\n VBD 0\n VB 0\n UH 0\n TO 0\n SYM 0\n -RRB- 0\n RP 0\n RBS 0\n RBR 0\n RB 0\n PRP$ 0\n PRP 0\n POS 0\n PDT 0\n NNS 0\n NNPS 0\n NNP 0\n NN 0\n MD 0\n LS 0\n -LRB- 0\n JJS 0\n JJR 0\n JJ 0\n IN 0\n FW 0\n EX 0\n DT 0\n CD 0\n CC 0\n # 0\n $ 0\n : 0\n , 0\n `` 0";


static init_sc isc(example_grammar_sc);

prop_construct_t example_grammar_pc;

struct lm_scoreable_construct
{
    template <class TF>
    bool operator() (TF& tf, std::string const& str) const
    {
        if (str == "yes") return true;
        else if (str == "no") return false;
        else throw std::runtime_error("\"lm\" field must be \"yes\" or \"no\"");
    }
};

struct lm_string_construct {

    template <class TF>
    lm_string<typename TF::token_type>
    operator()(TF& tf, std::string const& str) const
    {
        return lm_string<typename TF::token_type>(str,tf);
    }
};

size_t lm_id = example_grammar_pc.register_constructor("lm",lm_scoreable_construct());
size_t lm_string_id = example_grammar_pc.register_constructor("lm_string",lm_string_construct());

static void init_grammar(grammar_in_mem& gram, stringstream& str)
{
    brf_stream_reader brf_stream(str);
    gram.load(brf_stream, example_grammar_sc,example_grammar_pc);
}

static void init_grammar_via_archive(grammar_in_mem& gram, stringstream& str)
{
    stringstream archive_stream;
    boost::archive::binary_oarchive oar(archive_stream);
    brf_stream_reader brf_stream(str);
    brf_archive_writer writer;
    writer(str,oar);
    brf_archive_reader brf_archive(archive_stream);

    gram.load(brf_archive, example_grammar_sc, example_grammar_pc);
}

void init_grammar_marcu_1(grammar_in_mem& g)
{
    stringstream str;
    init_grammar_marcu_1_stream(str);
    init_grammar(g,str);
}

void init_grammar_marcu_staggard_wts_from_archive(grammar_in_mem& g)
{
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// "S:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_]
    ///                   x2 ### id=-15 scr4=0.5 scr1=0.4 scr2=0.1 "
    /// so, score for this rule should be
    /// pow(0.4,0.5) * pow(0.1,0.5) * pow(0.5,-0.1)
    ///
    ////////////////////////////////////////////////////////////////////////////
    stringstream str;
    init_grammar_marcu_staggard_wts_stream(str);
    init_grammar_via_archive(g,str);
}

void init_grammar_marcu_staggard_wts(grammar_in_mem& g)
{
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// "S:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_]
    ///                   x2 ### id=-15 scr4=0.5 scr1=0.4 scr2=0.1 "
    /// so, score for this rule should be
    /// pow(0.4,0.5) * pow(0.1,0.5) * pow(0.5,-0.1)
    ///
    ////////////////////////////////////////////////////////////////////////////
    stringstream str;
    init_grammar_marcu_staggard_wts_stream(str);
    init_grammar(g,str);
}

multiset<string> simplified_grammar_strings_marcu_1()
{
    string strvec[20] = {
        string("NNP -> FRANCE")
      , string("CC -> AND")
      , string("NNP -> RUSSIA")
      , string(". -> .")
      , string("NP -> NNP")
      , string("NP -> NNS")
      , string("NP -> V[NP___CC_] NP")
      , string("NNS -> ASTRO- -NAUTS")
      , string("VBP -> INCLUDE")
      , string("VP -> COMINGFROM NP")
      , string("DT -> THESE")
      , string("NP -> DT 7PEOPLE")
      , string("NP -> V[VP_\"p-DE\"] NP")
      , string("VP -> VBP NP")
      , string("S -> V[NP___VP_] .")
      , string("TOP -> S")
      , string("VP -> COMINGFROM NP")
      , string("V[NP___CC_] -> NP CC")
      , string("V[VP_\"p-DE\"] -> VP p-DE")
      , string("V[NP___VP_] -> NP VP")
    };
    return multiset<string>(strvec,strvec+20);
}

void init_grammar_marcu_1_stream(std::stringstream& stream)
{
    std::string str =
        "NNP:VL: NNP(\"France\") -> \"FRANCE\" ### id=-1 scr1=1.6 scr2=0.4 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"France\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=1 "
        "rhs={{{\"FRANCE\"}}} \n"

        "CC:VL: CC(\"and\") -> \"AND\" ### id=2 scr1=0.4 scr2=0.1  virtual_label=no "
        "complete_subtree=yes lm_string={{{\"and\"}}} sblm_string={{{}}} "
        "lm=yes sblm=yes rule_file_line_number=2 rhs={{{\"AND\"}}}\n"

        "NNP:VL: NNP(\"Russia\") -> \"RUSSIA\" ### id=3 scr1=0.6 scr2=0.15 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"Russia\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=3 "
        "rhs={{{\"RUSSIA\"}}}\n"

        ".:VL: .(\".\") -> \".\" ### id=4 scr1=1.8 scr2=0.45 virtual_label=no "
        "complete_subtree=yes lm_string={{{\".\"}}} sblm_string={{{}}} "
        "lm=yes sblm=yes rule_file_line_number=4 rhs={{{\".\"}}}\n"

        "NP:VL: NP(x0:NNP) -> x0 ### id=5 scr1=0.4 scr2=0.1 virtual_label=no "
        "complete_subtree=yes lm_string={{{0}}} sblm_string={{{0 }}} "
        "lm=yes sblm=yes rule_file_line_number=5 rhs={{{x0}}}\n"

        "NP:VL: NP(x0:NNS) -> x0 ### id=6 scr1=0.2 scr2=0.05 virtual_label=no "
        "complete_subtree=yes lm_string={{{0}}} sblm_string={{{0 }}} "
        "lm=yes sblm=yes rule_file_line_number=6 rhs={{{x0}}}\n"

        "NP:VL: NP(x0:NP x1:CC x2:NP) -> V[NP___CC_] x2 ### id=7 scr1=0.6 scr2=0.15 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=7 "
        "rhs={{{x0 x1 x2}}}\n"

        "NNS:VL: NNS(\"astronauts\") -> \"ASTRO-\" \"-NAUTS\" ### id=8 scr1=0.4 scr2=0.1 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"astronauts\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=8 "
        "rhs={{{\"ASTRO-\" \"-NAUTS\"}}}\n"

        "VBP:VL: VBP(\"include\") -> \"INCLUDE\" ### id=9 scr1=0.6 scr2=0.15 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"include\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=9 "
        "rhs={{{\"INCLUDE\"}}}\n"

        "VP:VL: VP(VBG(\"coming\") PP(IN(\"from\") x0:NP)) -> \"COMINGFROM\" x0"
        " ### id=10 scr1=0.4 scr2=0.1 virtual_label=no complete_subtree=yes "
        "lm_string={{{\"coming\" \"from\" 0}}} sblm_string={{{0 }}} lm=yes "
        "sblm=yes rule_file_line_number=10 rhs={{{\"COMINGFROM\" x0}}}\n"

        "DT:VL: DT(\"These\") -> \"THESE\" ### id=11 scr1=0.4 scr2=0.1 virtual_label=no "
        "complete_subtree=yes lm_string={{{\"These\"}}} sblm_string={{{}}} lm=yes "
        "sblm=yes rule_file_line_number=11 rhs={{{\"THESE\"}}}\n"

        "NP:VL: NP(x0:DT CD(\"7\") NN(\"people\")) -> x0 \"7PEOPLE\" ### id=12 "
        "scr1=0.8 scr2=0.2 virtual_label=no complete_subtree=yes "
        "lm_string={{{0 \"7\" \"people\"}}} sblm_string={{{0 }}} lm=yes sblm=yes"
        " rule_file_line_number=12 rhs={{{x0 \"7PEOPLE\"}}}\n"

        "NP:VL: NP(x0:NP x1:VP) -> V[VP_\"p-DE\"] x0 ### id=13 scr1=0.2 scr2=0.05 "
        "virtual_label=no complete_subtree=yes lm_string={{{1 0}}} "
        "sblm_string={{{1 0 }}} lm=yes sblm=yes rule_file_line_number=13 "
        "rhs={{{x1 \"p-DE\" x0}}}\n"

        "VP:VL: VP(x0:VBP x1:NP) -> x0 x1 ### id=14 scr1=0.6 scr2=0.15 virtual_label=no "
        "complete_subtree=yes lm_string={{{0 1}}} sblm_string={{{0 1 }}} "
        "lm=yes sblm=yes rule_file_line_number=14 rhs={{{x0 x1}}}\n"

        "S:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_] x2 ### id=-15 scr1=0.4 scr2=0.1 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=15 "
        "rhs={{{x0 x1 x2}}}\n"

        "TOP:VL: TOP(x0:S) -> x0 ### id=500 virtual_label=no complete_subtree=yes "
        "sblm_string={{{0 }}} lm_string={{{0}}} "
        "lm=yes sblm=yes rule_file_line_number=500 rhs={{{x0}}}\n"

        "VP:VL: VP(VBG(\"returning\") PP(IN(\"from\") x0:NP)) -> \"COMINGFROM\""
        " x0 ### id=16 scr1=0.2 scr2=0.05 virtual_label=no complete_subtree=yes "
        "lm_string={{{\"returning\" \"from\" 0}}} sblm_string={{{0 }}} lm=yes "
        "sblm=yes rule_file_line_number=16 rhs={{{\"COMINGFROM\" x0}}}\n"

        "V[NP___CC_]:VL: V[NP___CC_](x0:NP x1:CC) -> x0 x1 ### "
        "complete_subtree=no sblm=no lm=yes lm_string={{{0 1}}} "
        "sblm_string={{{}}} virtual_label=yes id={{{7}}}\n"

        "V[VP_\"p-DE\"]:VL: V[VP_\"p-DE\"](x0:VP) -> x0 \"p-DE\" ### "
        "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
        "sblm_string={{{}}} virtual_label=yes id={{{13}}}\n"

        "V[NP___VP_]:VL: V[NP___VP_](x0:NP x1:VP) -> x0 x1 ### "
        "complete_subtree=no sblm=no lm=yes lm_string={{{0 1}}} "
        "sblm_string={{{}}} virtual_label=yes id={{{-15}}}\n";

    stream.str(str);
}

void init_grammar_marcu_staggard_wts_stream(std::stringstream& stream)
{
    std::string str =
        "NNP:VL: NNP(\"France\") -> \"FRANCE\" ### id=-1 scr1=1.6 scr2=0.4 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"France\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=1 "
        "rhs={{{\"FRANCE\"}}}\n"

        "CC:VL: CC(\"and\") -> \"AND\" ### id=2 scr1=0.4 scr2=0.1  virtual_label=no "
        "complete_subtree=yes lm_string={{{\"and\"}}} sblm_string={{{}}} "
        "lm=yes sblm=yes rule_file_line_number=2 rhs={{{\"AND\"}}}\n"

        "NNP:VL: NNP(\"Russia\") -> \"RUSSIA\" ### id=3 scr1=0.6 scr2=0.15 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"Russia\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=3 "
        "rhs={{{\"RUSSIA\"}}}\n"

        ".:VL: .(\".\") -> \".\" ### id=4 scr1=1.8 scr2=0.45 virtual_label=no "
        "complete_subtree=yes lm_string={{{\".\"}}} sblm_string={{{}}} "
        "lm=yes sblm=yes rule_file_line_number=4 rhs={{{\".\"}}}\n"

        "NP:VL: NP(x0:NNP) -> x0 ### id=5 scr1=0.4 scr2=0.1 virtual_label=no "
        "complete_subtree=yes lm_string={{{0}}} sblm_string={{{0 }}} "
        "lm=yes sblm=yes rule_file_line_number=5 rhs={{{x0}}}\n"

        "NP:VL: NP(x0:NNS) -> x0 ### id=6 scr1=0.2 scr2=0.05 virtual_label=no "
        "complete_subtree=yes lm_string={{{0}}} sblm_string={{{0 }}} "
        "lm=yes sblm=yes rule_file_line_number=6 rhs={{{x0}}}\n"

        "NP:VL: NP(x0:NP x1:CC x2:NP) -> V[NP___CC_] x2 ### id=7 scr1=0.6 scr2=0.15 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=7 "
        "rhs={{{x0 x1 x2}}}\n"

        "NNS:VL: NNS(\"astronauts\") -> \"ASTRO-\" \"-NAUTS\" ### id=8 scr1=0.4 scr2=0.1 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"astronauts\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=8 "
        "rhs={{{\"ASTRO-\" \"-NAUTS\"}}}\n"

        "VBP:VL: VBP(\"include\") -> \"INCLUDE\" ### id=9 scr1=0.6 scr2=0.15 "
        "virtual_label=no complete_subtree=yes lm_string={{{\"include\"}}} "
        "sblm_string={{{}}} lm=yes sblm=yes rule_file_line_number=9 "
        "rhs={{{\"INCLUDE\"}}}\n"

        "VP:VL: VP(VBG(\"coming\") PP(IN(\"from\") x0:NP)) -> \"COMINGFROM\" x0"
        " ### id=10 scr1=0.4 scr2=0.1 virtual_label=no complete_subtree=yes "
        "lm_string={{{\"coming\" \"from\" 0}}} sblm_string={{{0 }}} lm=yes "
        "sblm=yes rule_file_line_number=10 rhs={{{\"COMINGFROM\" x0}}}\n"

        "DT:VL: DT(\"These\") -> \"THESE\" ### id=11 scr1=0.4 scr2=0.1 virtual_label=no "
        "complete_subtree=yes lm_string={{{\"These\"}}} sblm_string={{{}}} lm=yes "
        "sblm=yes rule_file_line_number=11 rhs={{{\"THESE\"}}}\n"

        "NP:VL: NP(x0:DT CD(\"7\") NN(\"people\")) -> x0 \"7PEOPLE\" ### id=12 "
        "scr1=0.8 scr2=0.2 virtual_label=no complete_subtree=yes "
        "lm_string={{{0 \"7\" \"people\"}}} sblm_string={{{0 }}} lm=yes sblm=yes"
        " rule_file_line_number=12 rhs={{{x0 \"7PEOPLE\"}}}\n"

        "NP:VL: NP(x0:NP x1:VP) -> V[VP_\"p-DE\"] x0 ### id=13 scr1=0.2 scr2=0.05 "
        "virtual_label=no complete_subtree=yes lm_string={{{1 0}}} "
        "sblm_string={{{1 0 }}} lm=yes sblm=yes rule_file_line_number=13 "
        "rhs={{{x1 \"p-DE\" x0}}}\n"

        "S:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_] x2 ### id=-15 scr4=0.5 scr1=0.4 scr2=0.1 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=15 "
        "rhs={{{x0 x1 x2}}}\n"

        "S2:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_] x2 ### id=-18 scr4=0.25 scr1=0.2 scr2=0.05 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=16 "
        "rhs={{{x0 x1 x2}}}\n"

        "S3:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_] x2 ### id=-16 scr4=0.00125 scr1=0.001 scr2=0.00025 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=17 "
        "rhs={{{x0 x1 x2}}}\n"

        "TOP:VL: TOP(x0:S) -> x0 ### id=500 virtual_label=no complete_subtree=yes "
        "sblm_string={{{0 }}} lm_string={{{0}}} "
        "lm=yes sblm=yes rule_file_line_number=500 rhs={{{x0}}}\n"

        "S4:VL: S(x0:NP x1:VP x2:.) -> V[NP___VP_] x2 ### id=-17 scr4=0.0625 scr1=0.05 scr2=0.0125 "
        "virtual_label=no complete_subtree=yes lm_string={{{0 1}}} "
        "sblm_string={{{0 1 2 }}} lm=yes sblm=yes rule_file_line_number=18 "
        "rhs={{{x0 x1 x2}}}\n"

        "VP:VL: VP(x0:VBP x1:NP) -> x0 x1 ### id=14 scr1=0.6 scr2=0.15 virtual_label=no "
        "complete_subtree=yes lm_string={{{0 1}}} sblm_string={{{0 1 }}} "
        "lm=yes sblm=yes rule_file_line_number=14 rhs={{{x0 x1}}}\n"

        "VP2:VL: VP(x0:VBP x1:NP) -> x0 x1 ### id=140 scr1=0.6 scr2=0.14 scr3=0.1 virtual_label=no "
        "complete_subtree=yes lm_string={{{0 1}}} sblm_string={{{0 1 }}} "
        "lm=yes sblm=yes rule_file_line_number=14 rhs={{{x0 x1}}}\n"

        "VP3:VL: VP(x0:VBP x1:NP) -> x0 x1 ### id=130 scr1=0.6 scr2=0.13 scr3=0.1 virtual_label=no "
        "complete_subtree=yes lm_string={{{0 1}}} sblm_string={{{0 1 }}} "
        "lm=yes sblm=yes rule_file_line_number=14 rhs={{{x0 x1}}}\n"

        "VP:VL: VP(VBG(\"returning\") PP(IN(\"from\") x0:NP)) -> \"COMINGFROM\""
        " x0 ### id=16 scr1=0.2 scr2=0.05 virtual_label=no complete_subtree=yes "
        "lm_string={{{\"returning\" \"from\" 0}}} sblm_string={{{0 }}} lm=yes "
        "sblm=yes rule_file_line_number=16 rhs={{{\"COMINGFROM\" x0}}}\n"

        "V[NP___CC_]:VL: V[NP___CC_](x0:NP x1:CC) -> x0 x1 ### "
        "complete_subtree=no sblm=no lm=yes lm_string={{{0 1}}} "
        "sblm_string={{{}}} virtual_label=yes id={{{7}}}\n"

        "V[VP_\"p-DE\"]:VL: V[VP_\"p-DE\"](x0:VP) -> x0 \"p-DE\" ### "
        "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
        "sblm_string={{{}}} virtual_label=yes id={{{13}}}\n"

        "V[NP___VP_]:VL: V[NP___VP_](x0:NP x1:VP) -> x0 x1 ### "
        "complete_subtree=no sblm=no lm=yes lm_string={{{0 1}}} "
        "sblm_string={{{}}} virtual_label=yes id={{{-15 -16 -17 -18}}}\n";

    stream.str(str);
}
