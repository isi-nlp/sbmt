# include <nntm/nnfm_info.hpp>
# include <nntm/nntm.hpp>

inline sbmt::score_t score(double w)
{
    //FIXME: need to convert IMPOSSIBLE to 0?  I think -HUGE_VAL should do it.
    return sbmt::score_t(w,sbmt::as_log10());
}

sbmt::score_t 
nnfm_factory::prob(int spn, int fertility)
{
    boost::uint32_t ngram[12];
    int m = 11 / 2;
    pathmap::const_iterator pos = pmap.find(spn);

    for (; m != 11; ++m) {
        if (pos != pmap.end()) {
            ngram[m] = lm_id(pos->second.begin()->first);
            pos = pmap.find(pos->second.begin()->second);
        } else {
            ngram[m] = send;
        }
    }
    m = 11 / 2 - 1;
    pos = revpmap.find(spn);
    
    for (; m != -1; --m) {
        if (pos != revpmap.end()) {
            ngram[m] = lm_id(pos->second.begin()->first);
            pos = revpmap.find(pos->second.begin()->second);
        } else {
            ngram[m] = sstart;
        }
    }
    m = 11;
    ngram[m] = lm_outid(std::min(fertility,5));
    # if PRINT_NNFM
    std::cerr << "NNFM:";
    BOOST_FOREACH(int nwd, std::make_pair(ngram,ngram + m)) {
        //++count;
        std::cerr << " " << lm->get_input_vocabulary().words()[nwd];
    }
    std::cerr << " | " << lm->get_output_vocabulary().words()[ngram[m]] << '\n'; 
    # endif
    assert(revpmap.find(spn) != revpmap.end() or spn == 0);
    assert(pmap.find(spn) != pmap.end());
    return score(lm->lookup_ngram(ngram,m+1));
}

std::string
nnfm_factory::hash_string( sbmt::in_memory_dictionary const& dict
                         , info_type const& info ) const
{
    return "";
}

nnfm_factory::nnfm_factory( sbmt::lattice_tree const& ltree
                          , std::map<sbmt::indexed_token,int> const& inputmap
                          , std::map<int,int> const& outputmap
                          , size_t id
                          , double wt
                          , boost::shared_ptr<nplm_model> model
                          , sbmt::in_memory_dictionary& dict )
: lm(model)
, lmstrid(id)
, lmwt(wt)
, inputmap(&inputmap)
, outputmap(&outputmap)
{
    init(ltree.root());
    start = model->start;
    null = model->null;
    int index, toindex;
    sbmt::indexed_token fs;
    index = pmap.begin()->first;
    BOOST_FOREACH(boost::tie(fs,toindex),pmap.begin()->second) {
        if (index != 0 or fs != dict.foreign_word("<foreign-sentence>")) throw std::runtime_error("in nnfm: expected <foreign-sentence>,0");
    }
    index = pmap.rbegin()->first;
    BOOST_FOREACH(boost::tie(fs,toindex),pmap.rbegin()->second) {
        if (fs != dict.foreign_word("</foreign-sentence>")) throw std::runtime_error("in nnfm: expected </foreign-sentence>,-1");
    }
    end = model->end;
    //nia = info_accessors(start,end,boost::make_tuple(model->null,0));
    sstart = lm_id(dict.foreign_word("<foreign-sentence>"));
    send = lm_id(dict.foreign_word("</foreign-sentence>"));
    
    # if 0
    std::cerr << "start: " << hash_string(dict,start) << '\n';
    std::cerr << "end:   " << hash_string(dict,end)   << '\n';
    std::cerr << "null:  " << hash_string(dict,null)  << '\n';
    std::cerr << "sstart:" << sstart << '\n';
    std::cerr << "ssend: " << send   << '\n';
    # endif
}

void nnfm_factory::init( sbmt::lattice_tree::node const& lnode )
{
    if (lnode.is_internal()) {
        BOOST_FOREACH(sbmt::lattice_tree::node lchld, lnode.children()) {
            init(lchld);
        }
    } else {
        revpmap[lnode.lat_edge().span.right()][lnode.lat_edge().source] = lnode.lat_edge().span.left();
        pmap[lnode.lat_edge().span.left()][lnode.lat_edge().source] = lnode.lat_edge().span.right();
    }
}

nnfm_factory::nnfm_factory(nnfm_factory const& o)
: pmap(o.pmap)
, revpmap(o.revpmap)
, lm(o.lm)
, lmstrid(o.lmstrid)
, lmwt(o.lmwt)
, start(o.start)
, sstart(o.sstart)
, end(o.end)
, send(o.send)
, null(o.null)
, inputmap(o.inputmap)
, outputmap(o.outputmap)
{}

void
nnfm_factory::compute_ngrams( info_type& n
                            , sbmt::score_t& inside
                            , sbmt::score_t& heuristic
                            , std::vector< std::pair<int,int> > const& fertility )
{
    assert(inside.is_one() && heuristic.is_one());
    if (not prop.get()) prop.reset(new nplm::propagator(lm->model(),1));
    int s, f;
    BOOST_FOREACH(boost::tie(s,f),fertility) {
        if (f >= 0) inside *= prob(s,f);
    }
}

namespace {
int lookup_input_word( boost::shared_ptr<nplm_model> model
                     , std::string const& lbl 
                     , int unk )
{
    int ret = model->lookup_input_word(lbl);
    if (ret != unk) return ret;
    else {
        if (lbl == "<foreign-sentence>") {
            ret = model->lookup_input_word("<source_s>");
            if (ret != unk) return ret;
            else return model->lookup_input_word("<s>");
        } else if (lbl == "</foreign-sentence>") {
            ret = model->lookup_input_word("</source_s>");
            if (ret != unk) return ret;
            else return model->lookup_input_word("</s>");
        }
        else return ret;
    }
}
}

void nnfm_factory_constructor::init(sbmt::in_memory_dictionary& dict)
{
    using namespace sbmt;
    if (filename != "") {
        if (not model) {
            std::cerr << "loading nnfm model";
            model.reset(new nplm_model(filename,0));
            std::cerr << "\n";
        }
        int unk = model->lookup_input_word("<unk>");
        BOOST_FOREACH(indexed_token tok, dict.native_words(native_size)) {
            int inp = lookup_input_word(model,dict.label(tok),unk);
            input_map.insert(std::make_pair(tok,inp));
        }
        BOOST_FOREACH(indexed_token tok, dict.foreign_words()) {
            input_map.insert(std::make_pair(tok,lookup_input_word(model,dict.label(tok),unk)));
        }
        for (int x = 0; x != 5; ++x) {
            std::string n = boost::lexical_cast<std::string>(x);
            output_map.insert(std::make_pair(x,model->lookup_output_word(n)));
        }
        output_map.insert(std::make_pair(5,model->lookup_output_word("5+")));
        
        native_size = dict.native_word_count();
        foreign_size = dict.foreign_word_count();
    }
}

bool nnfm_factory::deterministic() const { return true; }

bool nnfm_factory_constructor::set_option(std::string name, std::string value)
{
    return false;
}

sbmt::options_map nnfm_factory_constructor::get_options() 
{
    using namespace sbmt;
    options_map opts("nnfm model options");
    opts.add_option( "nnfm-model-file", optvar(filename),"nnfm file");
    return opts;
}
