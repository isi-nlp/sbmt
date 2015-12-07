# include <boost/test/auto_unit_test.hpp>
# include <sbmt/token.hpp>
# include <sbmt/sentence.hpp>
# include <vector>
# include <collapsed_signature_iterator.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/type_traits.hpp>
# include <boost/tuple/tuple.hpp>
# include <gusc/iterator/reverse.hpp>


using namespace sbmt;
using namespace std;
using namespace xrsdb;

template <class Iterator>
std::reverse_iterator<Iterator> reverse(Iterator const& itr)
{
    return std::reverse_iterator<Iterator>(itr);
}

vector<indexed_token> from_string(string const& str, indexed_token_factory& tf)
{
    using boost::tokenizer;
    boost::char_separator<char> sep(" ");
    tokenizer<boost::char_separator<char> > toker(str,sep);
    vector<indexed_token> v;

    tokenizer<boost::char_separator<char> >::iterator itr = toker.begin(),
                                                      end = toker.end();
    for (; itr != end; ++itr) {
        if ((*itr)[0] == '"') {
            string tok(itr->begin() + 1, itr->end() - 1);
            indexed_token t = tf.foreign_word(tok);
            v.push_back(t);
        } else {
            v.push_back(tf.tag("x"));
        }
    }
    return v;
}

template <class X>
struct ident {
    typedef typename boost::remove_cv<X>::type result_type;
    result_type operator()(X x) const { return x; }
};

template <class I>
boost::transform_iterator<ident<typename std::iterator_traits<I>::reference>,I>
id(I i) { return boost::make_transform_iterator(i,ident<typename std::iterator_traits<I>::reference>()); }

void array_to_sig(string sarray, string ssig, indexed_token_factory& tf)
{
    string msg_head = sarray + " ?= " + ssig + " : ";
    BOOST_CHECKPOINT(msg_head + "1");

    vector<indexed_token> const array = from_string(sarray,tf);
    vector<indexed_token> const sig = from_string(ssig,tf);
/*
    std::clog << sbmt::token_label(tf);
    copy(array.begin(),array.end(), std::ostream_iterator<sbmt::indexed_token>(std::clog," "));
    std::clog << " <--> ";
    copy(sig.begin(), sig.end(), std::ostream_iterator<sbmt::indexed_token>(std::clog," "));
    std::clog << " <--> ";
    copy( collapse_signature(array.begin(),array.end(),tf.tag("x")).first
        , collapse_signature(array.end(),array.end(),tf.tag("x")).second
        , std::ostream_iterator<sbmt::indexed_token>(std::clog," ")
        );
    std::clog << std::endl;
*/
    BOOST_CHECK_EQUAL_COLLECTIONS(
        sig.begin()
      , sig.end()
      , collapse_signature(array,tf.tag("x")).first
      , collapse_signature(array,tf.tag("x")).second
    );

    BOOST_CHECKPOINT(msg_head + "2");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        sig.rbegin()
      , sig.rend()
      , collapse_signature(gusc::reverse_range(array),tf.tag("x")).first
      , collapse_signature(gusc::reverse_range(array),tf.tag("x")).second
    );

    BOOST_CHECKPOINT(msg_head + "3");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        sig.begin()
      , sig.end()
      , collapse_signature(std::make_pair(id(array.begin()),id(array.end())),tf.tag("x")).first
      , collapse_signature(std::make_pair(id(array.end()),id(array.end())),tf.tag("x")).second
    );

    BOOST_CHECKPOINT(msg_head + "4");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        sig.rbegin()
      , sig.rend()
      , collapse_signature(std::make_pair(id(array.rbegin()),id(array.rend())),tf.tag("x")).first
      , collapse_signature(std::make_pair(id(array.rend()),id(array.rend())),tf.tag("x")).second
    );

    BOOST_CHECKPOINT(msg_head + "5");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        sig.rbegin()
      , sig.rend()
      , collapse_signature(std::make_pair(reverse(id(array.end())),reverse(id(array.begin()))),tf.tag("x")).first
      , collapse_signature(std::make_pair(reverse(id(array.begin())),reverse(id(array.begin()))),tf.tag("x")).second
    );

    BOOST_CHECKPOINT(msg_head + "6");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        sig.rbegin()
      , sig.rend()
      , reverse(collapse_signature(array,tf.tag("x")).second)
      , reverse(collapse_signature(array,tf.tag("x")).first)
    );

    BOOST_CHECKPOINT(msg_head + "7");
    collapsed_signature_iterator<vector<indexed_token>::const_iterator> csitr, csend;
    boost::tie(csitr,csend) = collapse_signature(array,tf.tag("x"));
    vector<indexed_token>::const_iterator sitr = sig.begin(), send = sig.end();
    for (;csitr != csend; ++csitr, ++sitr) {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            reverse(csend), reverse(csitr), reverse(send), reverse(sitr)
        );
    }
    BOOST_CHECK(sitr == send);
}

BOOST_AUTO_TEST_CASE(test_collapsed_signature)
{
    indexed_token_factory tf;
    array_to_sig("x x \"A\"", "x \"A\"", tf);
    array_to_sig("x x", "x", tf);
    array_to_sig("x \"A\"", "x \"A\"", tf);
    array_to_sig("\"A\" \"B\" \"C\"", "\"A\" \"B\" \"C\"",tf);
    array_to_sig("", "",tf);
    array_to_sig("x x x x", "x",tf);
    array_to_sig("x x \"A\" \"B\" x \"C\"", "x \"A\" \"B\" x \"C\"",tf);
    array_to_sig("x \"A\" \"B\" x \"C\"", "x \"A\" \"B\" x \"C\"",tf);
    array_to_sig("x \"A\" \"B\" x \"C\" x", "x \"A\" \"B\" x \"C\" x",tf);
    array_to_sig("\"A\" \"B\" x \"C\" x", "\"A\" \"B\" x \"C\" x",tf);
    array_to_sig("\"A\" \"B\" x \"C\" x x", "\"A\" \"B\" x \"C\" x",tf);
    array_to_sig("\"A\" \"B\" x x \"C\" x x", "\"A\" \"B\" x \"C\" x",tf);
    array_to_sig("\"A\" \"B\" x x \"C\"", "\"A\" \"B\" x \"C\"",tf);
    array_to_sig("x x \"A\" \"B\" x x \"C\" x", "x \"A\" \"B\" x \"C\" x",tf);
    array_to_sig("x x \"A\" \"B\" x x \"C\" x x x", "x \"A\" \"B\" x \"C\" x",tf);
    array_to_sig("x x \"A\" x \"B\" x x \"C\"", "x \"A\" x \"B\" x \"C\"",tf);
    array_to_sig("x x \"A\" x x \"B\" x x \"C\"", "x \"A\" x \"B\" x \"C\"",tf);
    array_to_sig("", "",tf);
    array_to_sig("x x x x", "x",tf);
    BOOST_CHECKPOINT("collapsed_signature finished");
}
