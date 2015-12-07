#ifndef   SBMT_TEST_GRAMMAR_EXAMPLES_HPP
#define   SBMT_TEST_GRAMMAR_EXAMPLES_HPP

#include <sbmt/grammar/tag_prior.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <map>
#include <string>

extern sbmt::fat_weight_vector example_grammar_sc;
extern char const* grammar_nts;

struct init_sc
{
    init_sc(sbmt::fat_weight_vector& sc)
    {
        init(sc);
    }

    static void init(sbmt::fat_weight_vector& sc,double s1=.5,double s2=.5,double s3=2,double s4=-.1,double text_length=1)
    {
        sc.clear();
        sc["scr1"] = s1;
        sc["scr2"] = s2;
        sc["scr3"] = s3;
        sc["scr4"] = s4;
        sc["text-length"] = text_length;
    }
};

typedef sbmt::property_constructors<sbmt::indexed_token_factory> prop_construct_t;
extern prop_construct_t example_grammar_pc;
extern size_t lm_string_id;
extern size_t lm_id;

void init_grammar_marcu_1(sbmt::grammar_in_mem& g);

std::multiset<std::string> simplified_grammar_strings_marcu_1();

void init_grammar_marcu_1_stream(std::stringstream& stream);

void init_grammar_marcu_staggard_wts_from_archive(sbmt::grammar_in_mem& g);

void init_grammar_marcu_staggard_wts(sbmt::grammar_in_mem& g);

#endif // SBMT_TEST_GRAMMAR_EXAMPLES_HPP
