# include <nntm/nntm_info.hpp>
# include <nntm/nntm.hpp>

class info_accessors
{
public:
    typedef nplm_model LM;
    typedef nntm_info info_type;
    typedef info_type::lm_id_type lm_id_type;
    typedef LM lm_type;
    
    enum { ctx_len = info_type::ctx_len };
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;
    
    lm_id_type start;
    lm_id_type null;
    lm_id_type end;
    
    info_accessors() {}
    
    info_accessors(lm_id_type start, lm_id_type end, lm_id_type null)
    : start(start)
    , end(end)
    , null(null) {}

    const_iterator left_begin(info_type const& n)
    {
        return &(n(0,0));
    }

    iterator left_begin(info_type& n)
    {
        return &(n(0,0));
    }

    const_iterator left_end_full(info_type const& n) // may include NULLs
    {
        return &(n(0,ctx_len));
    }

    iterator left_end_full(info_type& n) // may include NULLs
    {
        return &(n(0,ctx_len));
    }

    const_iterator left_end(info_type const& n) // doesn't include NULL if any
    {
        const_iterator i=left_begin(n);
        const_iterator e=left_end_full(n);
        for (;i!=e;++i) if (*i == null) return i;
        return e;
    }

    const_iterator right_begin_full(info_type const& n)
    {
        return &(n(1,0));
    }

    iterator right_begin_full(info_type& n)
    {
        return &(n(1,0));
    }

    const_iterator right_begin(info_type const& n)
    {
        const_iterator i=right_end(n);
        const_iterator b=right_begin_full(n);
        for (;;) {
            --i;
            if (*i == null)
                return i+1;
            if (i==b)
                return b;
        }
    }

    const_iterator right_end(info_type const& n)
    {
        return &(n(1,ctx_len));
    }

    iterator right_end(info_type& n)
    {
        return &(n(1,ctx_len));
    }

    const_iterator gapless_begin(info_type const& n)
    {
        return left_begin(n);
    }

private:
/// returns I such that [I,e) are all == k (*I==k or I==e).  allows negative range (e<b), returning just b.
    template <class I>
    inline I find_end_not(I b,I e,lm_id_type k)
    {
        while(e>b) {
            --e;
            if (*e!=k)
                return e+1;
        }
        return b;
    }
public:
    /// only call after you determine no_gap() (i.e. if it's full, this gives wrong answer)
    const_iterator gapless_end(info_type const& n)
    {
        return find_end_not(left_begin(n),left_end_full(n)-1,null);
    }

    // full context -> gap.  less than full -> know all words -> no gap.
    bool no_gap(info_type const& n)
    {
        return n(0,ctx_len-1) == null;
    }

    void set_start_sentence(info_type& n)
    {
        for (unsigned m = 0; m < ctx_len; ++m)
            n(0,m) = start;
    }

    void set_end_sentence(info_type& n)
    {
        for (unsigned m = 0; m < ctx_len; ++m)
            n(1,m) = end;
    }

    void set_null(info_type& n)// we can use this for foreign word edges, to be nice
    {
        for (iterator i=left_begin(n),e=right_end(n);i!=e;++i)
            *i = null;
    }

    bool is_null(info_type const& n)
    {
        return *left_begin(n) == null;
    }
};

struct lm_phrase
{
    enum {N = 5, F = 11};
    typedef nplm_model lm_type;
    info_accessors nia;
    nntm_factory const* factory;
    
    boost::uint32_t* ngram;

    typedef nntm_info info_type;
    typedef info_type::lm_id_type lm_id_type;
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;

    lm_phrase( unsigned lmstr_len
             , nntm_factory const* factory )
    : factory(factory)
    , nia(factory->start,factory->end,factory->null)
    {
        phrase=new lm_id_type[lmstr_len*2*N];
        ngram = new boost::uint32_t[N + F];
        reset();
    }

    ~lm_phrase()
    {
        delete[] phrase;
        delete[] ngram;
    }
    
    inline static sbmt::score_t score(double w)
    {
        //FIXME: need to convert IMPOSSIBLE to 0?  I think -HUGE_VAL should do it.
        return sbmt::score_t(w,sbmt::as_log10());
    }

    void append_word(lm_id_type i)
    {
        *phrase_end++=i;
        if (nia.start == i) ctx_end = phrase_end;
    }

    void start_sentence()
    {
        append_word(nia.start);
    }

    void end_sentence()
    {
        append_word(nia.end);
    }

    void append_child(info_type &result,info_type const& kid,sbmt::score_t& accum)
    {

        if (nia.no_gap(kid)) {
            append_words(nia.gapless_begin(kid),nia.gapless_end(kid));
            return;
        }
        const_iterator left_begin=nia.left_begin(kid);
        append_words(left_begin,nia.left_end(kid));
        use_phrase(result,accum);
        
        clear();
        append_words(nia.right_begin(kid),nia.right_end(kid));
        ctx_end=phrase_end;
    }

    void clear()
    {
        phrase_end=phrase;
    }

    void finish_last_phrase(info_type &result,sbmt::score_t& accum)
    {
        use_phrase(result,accum);
        finish_right(result);
    }
    
    void reset()
    {
        phrase_end=phrase;
        ctx_end=phrase+ctxlen;
        left_done=false;
    }

 private:
    BOOST_STATIC_CONSTANT(unsigned,ctxlen=N-1);

    // private: don't reuse this phrase object on different items as the memory preallocated should increase w/ length of lm string
    
    
    sbmt::score_t prob(lm_id_type const* beg, lm_id_type const* end)
    {
        int thislen = end - beg;
        if (thislen < N and beg[0] != nia.start) return score(0.0);
        lm_id_type word = *(end - 1);
        int spn = word.srcid;
        int m = F / 2;
        nntm_pathmap::const_iterator pos = factory->pmap.find(spn);

        for (; m != F; ++m) {
            if (pos != factory->pmap.end()) {
                ngram[m] = factory->lm_id(pos->second.begin()->first,pos->second.begin()->second.first);
                pos = factory->pmap.find(pos->second.begin()->second.second);
            } else {
                ngram[m] = factory->send;
            }
        }
        m = F / 2 - 1;
        pos = factory->revpmap.find(spn);
        
        for (; m != -1; --m) {
            if (pos != factory->revpmap.end()) {
                ngram[m] = factory->lm_id(pos->second.begin()->first,pos->second.begin()->second.first);
                pos = factory->revpmap.find(pos->second.begin()->second.second);
            } else {
                ngram[m] = factory->sstart;
            }
        }
        m = F;
        lm_id_type const* itr = beg;
        for (int f = 0; f < N - thislen; ++f, ++m) {
            ngram[m] = factory->start.as_input;
        } 
        for (; itr != end - 1; ++itr, ++m) {
            ngram[m] = itr->as_input;
        }
        ngram[m] = itr->as_output;
        # if PRINT_NNTM
        std::cerr << "\nSCORE:";
        int count = 0;
        BOOST_FOREACH(int nwd, std::make_pair(ngram,ngram + m)) {
            if (count == (int)F) std::cerr << " ###";
            ++count;
            std::cerr << " " << factory->lm->get_input_vocabulary().words()[nwd];
        }
        std::cerr << " | " << factory->lm->get_output_vocabulary().words()[ngram[m]] << '\n'; 
        # endif
        assert(factory->revpmap.find(spn) != factory->revpmap.end());
        assert(factory->pmap.find(spn) != factory->pmap.end());
        return score(factory->secondary(ngram,m + 1));
        //ngram
    }

    void use_phrase(info_type &result,sbmt::score_t& accum)
    {
        
        for (;ctx_end < phrase_end; ++ctx_end)
            accum *= prob(std::max(phrase,ctx_end-ctxlen),ctx_end + 1);
        finish_left(result,accum);
        
    }

    void append_words(const_iterator b,const_iterator e)
    {
        bool start = ((b != e) and (*b == nia.start));
        while (b!=e) {
            append_word(*b++);
            if (start) ctx_end = phrase_end < (phrase + ctxlen) ? phrase_end : (phrase + ctxlen);
        }
    }

    unsigned size() const
    {
        return phrase_end-phrase;
    }

    bool is_start_sentence_phrase() const
    {
        return phrase!=phrase_end && phrase[0]==nia.start;
    }

    bool is_end_sentence_phrase() const
    {
        return phrase!=phrase_end && phrase_end[-1]==nia.end;
    }

    void finish_left(info_type& result, sbmt::score_t& accum)
    {
        finish_left(result,accum,nia.null);
    }

    /// builds result left state.
    void finish_left(info_type &result,sbmt::score_t& accum,lm_id_type terminator)
    {
        if (left_done)
            return;
        left_done=true;
        //TODO: since edge forces TOP items into same equiv already, preserve actual context so foreign start/end get predicted based on full context?  nah, why bother p foreign start/end given e start/end = 1.
        if (is_start_sentence_phrase()) { // no heuristic or further backoffs
            nia.set_start_sentence(result);
        } else {
            typedef info_type::iterator lwit;
            lwit lw = nia.left_begin(result),
                 le = nia.left_end_full(result);
            const_iterator phrase_ctx_end=phrase+ctxlen;
            if (phrase_ctx_end > phrase_end)
                phrase_ctx_end = phrase_end;
            const_iterator orig_ctx_end=phrase_ctx_end;

            lwit i=std::copy((const_iterator)phrase,phrase_ctx_end,lw);
            while(i<le) *i++=terminator;
        }
    }

    /// build result right state
    void finish_right(info_type &result)
    {
        if (is_end_sentence_phrase()) {
            nia.set_end_sentence(result);
            return;
        }
        info_type::iterator rb = nia.right_begin_full(result),
                                     r,
                                     re = nia.right_end(result);
        const_iterator pstart=phrase_end-ctxlen;
        if (pstart<phrase)
            pstart=phrase;
        const_iterator pstart_shorter = pstart;

        unsigned rlen=phrase_end-pstart_shorter;
        r=re-rlen;
        std::copy(pstart_shorter,(const_iterator)phrase_end,r);
        while (r>rb)
            *--r=nia.null;
    }

    lm_id_type *phrase,*phrase_end,*ctx_end;
    bool left_done;
};

std::string
nntm_factory::hash_string( sbmt::in_memory_dictionary const& dict
                         , info_type::lm_id_type const& info ) const
{
    std::stringstream sstr;
    sstr << '"' << lm->get_input_vocabulary().words()[info.as_input] << '"';
    if (info != null) {
        //assert (pmap.find(info.get<1>()) != pmap.end());
        sstr << ',' << '"' << dict.label(pmap.find(info.srcid)->second.begin()->first) << '"';
    }
    return sstr.str();
}

std::string
nntm_factory::hash_string( sbmt::in_memory_dictionary const& dict
                         , info_type const& info ) const
{
    std::stringstream sstr;
    for (int x = 0; x != 2; ++x) {
        if (x != 0) sstr << "//";
        for (int y = 0; y != info_type::N -1; ++ y) {
            if (y != 0) sstr << "_";
            sstr << hash_string(dict,info(x,y));
        }
    }
    return sstr.str();
}

nntm_factory::nntm_factory( sbmt::lattice_tree const& ltree
                          , std::map<sbmt::indexed_token,int> const& inputmap
                          , std::map<sbmt::indexed_token,int> const& outputmap
                          , size_t id
                          , double wt
                          , boost::shared_ptr<nplm_model> model
                          , sbmt::in_memory_dictionary& dict )
: p(this)
, secondary(p,16,1000000)
, lm(model)
, lmstrid(id)
, lmwt(wt)
, inputmap(&inputmap)
, outputmap(&outputmap)
, unkid(model->lookup_input_word("<unk>"))
, outunkid(model->lookup_output_word("<unk>"))
{
    init(ltree.root(), inputmap, outputmap, dict);
    start = nntm_info::lm_id_type(model->lookup_input_word("<s>"),model->lookup_output_word("<s>"),0);
    null = nntm_info::lm_id_type(model->lookup_input_word("<null>"),model->lookup_output_word("<null>"),0);
    //start = boost::make_tuple(model->start,0);
    //null = boost::make_tuple(model->null,0);
    int index;
    std::pair<sbmt::indexed_token,int> tagidx;
    sbmt::indexed_token fs;
    index = pmap.begin()->first;
    BOOST_FOREACH(boost::tie(fs,tagidx),pmap.begin()->second) {
        if (index != 0 or fs != dict.foreign_word("<foreign-sentence>")) throw std::runtime_error("in nntm: expected <foreign-sentence>,0");
    }
    index = pmap.rbegin()->first;
    BOOST_FOREACH(boost::tie(fs,tagidx),pmap.rbegin()->second) {
        if (fs != dict.foreign_word("</foreign-sentence>")) throw std::runtime_error("in nntm: expected </foreign-sentence>,-1");
    }
    end = nntm_info::lm_id_type(model->lookup_input_word("</s>"),model->lookup_output_word("</s>"),index);
    //end = boost::make_tuple(model->end,index);
    //nia = info_accessors(start,end,boost::make_tuple(model->null,0));
    sstart = lm_id(dict.foreign_word("<foreign-sentence>"),dict.tag("UNKNOWN"));
    send = lm_id(dict.foreign_word("</foreign-sentence>"),dict.tag("UNKNOWN"));
    
    # if PRINT_NNTM
    std::cerr << "start: " << hash_string(dict,start) << '\n';
    std::cerr << "end:   " << hash_string(dict,end)   << '\n';
    std::cerr << "null:  " << hash_string(dict,null)  << '\n';
    std::cerr << "sstart:" << sstart << '\n';
    std::cerr << "ssend: " << send   << '\n';
    # endif
}

void nntm_factory::init( sbmt::lattice_tree::node const& lnode
                       , std::map<sbmt::indexed_token,int> const& inputmap
                       , std::map<sbmt::indexed_token,int> const& outputmap
                       , sbmt::in_memory_dictionary& dict )
{
    if (lnode.is_internal()) {
        BOOST_FOREACH(sbmt::lattice_tree::node lchld, lnode.children()) {
            init(lchld,inputmap,outputmap,dict);
        }
    } else {
        sbmt::indexed_token tag = dict.tag("UNKNOWN");
        if (lnode.lat_edge().features.find("tag") != lnode.lat_edge().features.end()) {
            tag = dict.tag(lnode.lat_edge().features.find("tag")->second);
        }
        revpmap[lnode.lat_edge().span.right()][lnode.lat_edge().source] = std::make_pair(tag,lnode.lat_edge().span.left());
        pmap[lnode.lat_edge().span.left()][lnode.lat_edge().source] = std::make_pair(tag,lnode.lat_edge().span.right());
    }
}

nntm_factory::nntm_factory(nntm_factory const& o)
: p(this)
, secondary(p,16,1000000)
, pmap(o.pmap)
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
, unkid(o.unkid)
, outunkid(o.outunkid)
{}

void
nntm_factory::compute_ngrams( info_type &n
                            , sbmt::score_t &inside
                            , sbmt::score_t &heuristic
                            , bool is_toplevel
                            , affiliation_vector const& lmstr
                            , kids_type const& lmk )
{
    
    assert(inside.is_one() && heuristic.is_one());
    if (not prop.get()) prop.reset(new nplm::propagator(lm->model(),1)); 
    if (not phrase.get()) phrase.reset(new lm_phrase(256,this));
    phrase->reset();
    if (is_toplevel) phrase->start_sentence();

    BOOST_FOREACH(sbmt::lm_token<info_type::lm_id_type> const& i, lmstr) {               
        if(i.is_token()){
            //assert(pmap.find(i.get_token().get<1>()) != pmap.end());
            phrase->append_word(i.get_token());
        } else {
            const info_type& kid = *lmk[i.get_index()].info();
            phrase->append_child(n,kid,inside);
        } 
    }

    if (is_toplevel) phrase->end_sentence();

    phrase->finish_last_phrase(n,inside);
    
    //heuristic_score(n,heuristic);
}

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

void nntm_factory_constructor::init(sbmt::in_memory_dictionary& dict)
{
    using namespace sbmt;
    if (filename != "") {
        if (not model) {
            std::cerr << "loading nntm model";
            model.reset(new nplm_model(filename));
            //std::cerr << "\npremultiplying";
            //model->model().premultiply();
            std::cerr << "\n";
        }
        int unk = model->lookup_input_word("<unk>");
        BOOST_FOREACH(indexed_token tok, dict.native_words(native_size)) {
            int inp = lookup_input_word(model,dict.label(tok),unk);
            input_map.insert(std::make_pair(tok,inp));
            int oup = model->lookup_output_word(dict.label(tok));
            output_map.insert(std::make_pair(tok,oup));
            inputoutput_map.insert(std::make_pair(inp,oup));
        }
        native_size = dict.native_word_count();
        BOOST_FOREACH(indexed_token tok, dict.tags(tag_size)) {
            int inp = lookup_input_word(model,dict.label(tok),unk);
            input_map.insert(std::make_pair(tok,inp));
            int oup = model->lookup_output_word(dict.label(tok));
            output_map.insert(std::make_pair(tok,oup));
            inputoutput_map.insert(std::make_pair(inp,oup));
        }
        tag_size = dict.tag_count();
        BOOST_FOREACH(indexed_token tok, dict.foreign_words()) {
            input_map.insert(std::make_pair(tok,lookup_input_word(model,dict.label(tok),unk)));
        }
        foreign_size = dict.foreign_word_count();
    }
}

void nntm_factory_constructor::init(sbmt::in_memory_dictionary& dict,sbmt::lattice_tree const& lat)
{
    dict.tag("UNKNOWN");
    BOOST_FOREACH(sbmt::lattice_tree::node nd, lat.root().children()) {
        sbmt::indexed_token source = nd.lat_edge().source;
        sbmt::span_t span = nd.lat_edge().span;
        if (nd.lat_edge().features.find("tag") != nd.lat_edge().features.end()) {
            dict.tag(nd.lat_edge().features.find("tag")->second);
        }
    }
    init(dict);
}

bool nntm_factory::start_symbol(kids_type const& lmk, affiliation_vector const& lmstr) const
{
    for (size_t x = 0; x != lmk.size(); ++x) {
        if (sbmt::is_nonterminal(lmk[x].root())) { 
            if ((*lmk[x].info())(0,0).as_input == lm->start) {
                return true;
            }
        }
    }
    if (lmstr.begin() != lmstr.end() and lmstr.begin()->is_token() and lmstr.begin()->get_token().as_input == lm->start) return true;
    return false;
}

bool nntm_factory::deterministic() const { return true; }

bool nntm_factory_constructor::set_option(std::string name, std::string value)
{
    return false;
}

sbmt::options_map nntm_factory_constructor::get_options() 
{
    using namespace sbmt;
    options_map opts("nntm model options");
    opts.add_option( "nntm-model-file", optvar(filename),"nntm file");
    return opts;
}
