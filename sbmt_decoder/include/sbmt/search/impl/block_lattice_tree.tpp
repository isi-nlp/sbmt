# if ! defined(SBMT__SEARCH__BLOCK_LATTICE_TREE_TPP)
# define       SBMT__SEARCH__BLOCK_LATTICE_TREE_TPP

# include <sbmt/search/block_lattice_tree.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
lattice_edge::lattice_edge( Grammar& gram
                          , span_t span
                          , std::string const& source
                          , std::map<std::string,score_t> const& score_vec
                          , std::map<std::string,std::string> const& text_vec
                          , std::string const& target
                          , std::map<std::string,score_t> const& target_score_vec
                          , std::map<std::string,std::string> const& target_text_vec )
: span(span)
, source(gram.dict().foreign_word(source))
, rule_id(NULL_GRAMMAR_RULE_ID)
, syntax_rule_id(NULL_GRAMMAR_RULE_ID)
, features(text_vec)
{
    feature_vector feat;
    text_feature_vector_byid text;
    for ( std::map<std::string,score_t>::const_iterator itr = score_vec.begin()
        ; itr != score_vec.end()
        ; ++itr ) feat.insert(std::make_pair(gram.feature_names().get_index(itr->first),itr->second));

    for ( std::map<std::string,std::string>::const_iterator itr = text_vec.begin()
        ; itr != text_vec.end()
        ; ++itr ) text.insert(std::make_pair(gram.feature_names().get_index(itr->first),itr->second));
    rule_id = gram.id( gram.insert_terminal_rule( this->source
                                                , feat
                                                , text ) );

}

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
lattice_tree::node convert(Grammar& grammar,gusc::lattice_ast::line const& line)
{
    using namespace boost;
    using namespace std;

    gusc::lattice_ast::const_property_iterator pi, pe;
    tie(pi,pe) = line.properties();

    if (line.is_block()) {

        std::list<lattice_tree::node> nodes;
        gusc::lattice_ast::const_line_iterator li,le;
        tie(li,le) = line.lines();
        for (; li != le; ++li) {
            nodes.push_back(convert(grammar,*li));
        }
        return lattice_tree::node(nodes.begin(),nodes.end());
    } else {
        span_t spn(line.span().from(),line.span().to());
        std::string source(line.label());
        std::string target;
        std::map<std::string,score_t> scorevec;
        std::map<std::string,std::string> textvec;
        std::map<std::string,score_t> syntax_scorevec;
        std::map<std::string,std::string> syntax_textvec;
        bool on_syntax = false;

        textvec.insert(std::make_pair("span",boost::lexical_cast<std::string>(spn)));

        for (; pi != pe; ++pi) {
            if (pi->key() == "target") {
                target = pi->value();
                on_syntax = true;
            } else {
                try {
                    score_t scr = lexical_cast<score_t>(pi->value());
                    if (on_syntax)
                        syntax_scorevec.insert(make_pair(pi->key(),scr));
                    else
                        scorevec.insert(make_pair(pi->key(),scr));
                } catch(boost::bad_lexical_cast const&) {

                }
                if (on_syntax)
                    syntax_textvec.insert(make_pair(pi->key(),pi->value()));
                else
                    textvec.insert(make_pair(pi->key(),pi->value()));
            }
        }
        lattice_edge e(grammar,spn,source,scorevec,textvec,target,syntax_scorevec,syntax_textvec);
        return lattice_tree::node(e);
    }
}


////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
lattice_tree convert(Grammar& grammar, gusc::lattice_ast const& ast)
{
    using namespace boost;
    using namespace std;

    std::list<lattice_tree::node> nodes;
    std::set<span_t> restrictions;
    gusc::lattice_ast::const_line_iterator li,le;
    tie(li,le) = ast.lines();
    for (;li != le; ++li) {
        nodes.push_back(convert(grammar,*li));
    }

    gusc::lattice_ast::const_property_iterator pi, pe;
    tie(pi,pe) = ast.properties();
    size_t id=0;
    map<string,string> latfeats;
    for (; pi != pe; ++pi) {
        if (pi->key() == "id") {
            id = lexical_cast<size_t>(pi->value());
        } else if (pi->key() == "span-restrictions") {
            std::stringstream sstr(pi->value());
            std::copy( std::istream_iterator<span_t>(sstr)
                     , std::istream_iterator<span_t>()
                     , std::inserter(restrictions,restrictions.end())
                     )
                     ;
        } else {
            latfeats.insert(make_pair(pi->key(),pi->value()));
        }
    }
    lattice_tree::node root(nodes.begin(),nodes.end());
    return lattice_tree(root,id,restrictions,latfeats);
}

} // namespace sbmt

# endif