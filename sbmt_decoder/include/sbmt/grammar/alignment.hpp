#ifndef SBMT__ALIGNMENT_HPP
#define SBMT__ALIGNMENT_HPP

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iterator>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/io.hpp>

namespace sbmt {

struct as_inverse {};
struct as_unit {};

/// transform range r=(1,3,1,2) -> r'=(0,1,4,5)
/// meaning: input r gets expanded (pretend) such that r[i] becomes r[i]
/// elements.  what's the (0-based) index at the start of each r[i]'s run in r'?
/// e.g. r'[0]=0,r'[2]=4.  returns final sum (where an additional element would go)

template <class I>
inline typename std::iterator_traits<I>::value_type 
 cumulative_offsets(I i,I end) 
{
    typedef typename std::iterator_traits<I>::value_type index;
    index base=0;
    for (;i!=end;++i) {
        index delta=*i;
        *i=base;
        base=base+delta;
    }
    return base;
}


struct alignment_out_of_range : public std::runtime_error
{
    typedef size_t index;
    index i,len;
    std::string msg;
    
    alignment_out_of_range(index i,index len) :
        std::runtime_error("alignment index reference past end of string"),
        i(i),len(len)
    {
        try {
            std::ostringstream o;
            o << "Alignment index " << i << " is too large for the phrase of length " << len;
            msg=o.str();
        } catch(...) {}
    }
    const char * what() const throw ()
    {
        if (msg.empty())
            return std::runtime_error::what();
        return msg.c_str();
    }
    ~alignment_out_of_range() throw () {}
};

inline void alignment_in_range(size_t i,size_t len)
{
    if (i >= len)
        throw alignment_out_of_range(i,len);
}

    

/// chosen representation: per-src list of tar-aligned items (source:foreign target:english)
/// alternative would be symmetrical: mutually referent tar/src linked lists of pair<index,alignments to other list> - where the index can be used for computing numerical alignments while substituting/expanding
/// or just list of (source,target) pairs
struct alignment 
{
    typedef unsigned index;
    typedef std::vector<index> tars;
    typedef std::vector<tars> srcs;
 private:
    
    index ntar; /// because the trailing elements of tar may be unaligned
 public:
    srcs sa; /// length = # of src items
    alignment() : ntar(0) {}
    index n_tar() const { return ntar; }
    index n_src() const { return sa.size(); }

//    alignment(alignment const& o) : ntar(o.ntar),sa(o.sa) {}
    
    alignment(alignment const& o,as_inverse tag)
    {
        set_inverse(o);
    }

    void set_null() 
    {
        set_empty(0,0);
    }
    
    bool is_null() const
    {
        return ntar==0 && sa.empty();
    }
    
    /// calls v(src,tar) for all alignment pairs
    template <class V>
    void visit(V v) 
    {
        for (unsigned i=0,e=n_src();i!=e;++i)
            for (tars::const_iterator s=sa[i].begin(),se=sa[i].end();s!=se;++s)
                v(i,*s);
    }

    BOOST_STATIC_CONSTANT(char,pair_space=' ');
    BOOST_STATIC_CONSTANT(char,pair_comma=',');
    BOOST_STATIC_CONSTANT(char,open_align='[');
    BOOST_STATIC_CONSTANT(char,close_align=']');
//    BOOST_STATIC_CONSTANT(char,open_pairlist='(');
//    BOOST_STATIC_CONSTANT(char,close_pairlist=')');
    BOOST_STATIC_CONSTANT(char,close_pairlist=close_align);


    struct offset_printer 
    {
        int src_offset;
        int tar_offset;
        offset_printer() 
        {
            set();
        }
        offset_printer(int s,int t) 
        {
            set(s,t);
        }
        
        void set(int s=0,int t=0) 
        {
            src_offset=s;
            tar_offset=t;
        }        
            
        template <class O>
        void print_source_index(O &o,unsigned s) const 
        {
            o << s+src_offset;
        }
        template <class O>
        void print_target_index(O &o,unsigned t) const 
        {
            o << t+tar_offset;
        }
    };
    
    
    /// prints s+src_offset,t+tar_offset ... (in case you want to adjust 0 or 1
    /// indexed, skip a dummy initial foreign word, etc)
    template <class O>
    void print_pairs(O &o,int src_offset=0,int tar_offset=0) const 
    {
        print_pairs_custom(o,offset_printer(src_offset,tar_offset));
    }

    template <class O,class P>
    void print_pairs_custom(O &o,P const& p) const
    {
        sort();
        graehl::word_spacer sp=pair_space;
        for (unsigned i=0,e=n_src();i!=e;++i)
            for (tars::const_iterator j=sa[i].begin(),je=sa[i].end();j!=je;++j) {
                o << sp;
                p.print_source_index(o,i);
                o << pair_comma;
                p.print_target_index(o,*j);
            }
    }
    

    /// requires set_empty with sufficient n_src,n_tar
    template <class I>
    void read_pairs(I &in,char until=close_pairlist)
    {
        char c;
        index src,tar;
        for(;;) {
            EXPECTI(in>>c);
            in.unget();
            if (c==until)
                break;
            EXPECTI(in>>src);
            EXPECTCH(pair_comma);
            EXPECTI(in>>tar);
            add(src,tar);
        }
    }
    
    template <class O>
    void print(O &o,int src_offset=0,int tar_offset=0) const 
    {
        print_custom(o,offset_printer(src_offset,tar_offset));
    }
    
    template <class O,class P>
    void print_custom(O &o,P const& p) const 
    {
        o << open_align<< "#s=" << n_src();
        o << " #t=" << n_tar();
        o << ' ';
#ifdef WORDY_ALIGNMENT_IO
        o << "s,t=";
#endif 
        print_pairs_custom(o,p);
        o << close_align;
    }

    template <class I>
    void read(I &in) 
    {
        char c;
        EXPECTCH_SPACE(open_align);
        EXPECTCH_SPACE('#');
        EXPECTCH('s');
        EXPECTCH_SPACE('=');
        index nsrc,ntar;
        EXPECTI(in >> nsrc);
        EXPECTCH_SPACE('#');
        EXPECTCH('t');
        EXPECTCH_SPACE('=');
        EXPECTI(in>>ntar);
        set_empty(nsrc,ntar);
#ifdef WORDY_ALIGNMENT_IO
        EXPECTCH_SPACE('s');
        EXPECTCH(',');
        EXPECTCH('t');
        EXPECTCH_SPACE('=');
#endif 
        read_pairs(in); // note: reads until close_pairlist but does not consume
        EXPECTCH_SPACE(close_align);
    }


    typedef alignment self_type;
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ

    /// conceptually const but invalidates iterators (not that we provide any)
    void sort() const
    {
        return const_cast<alignment *>(this)->sort();
    }

    void sort()
    {
        for (srcs::iterator i=sa.begin(),e=sa.end();i!=e;++i)
            sort(*i);
    } 
    
    bool operator==(alignment const& o) const
    {
        sort();
        o.sort();
        return n_tar()==o.n_tar() && sa==o.sa;
    }
    
    void set_empty(index n_src,index n_tar) 
    {
        ntar=n_tar;
        sa.clear();
        sa.resize(n_src);
    }

    /// unit: when subst. for a variable, no change
    void set_unit()
    {
        set_empty(1,1);
        add(0,0);
    }

    explicit alignment(as_unit tag) 
    {
        set_unit();
    }
    
    
    void add(index s,index t)
    {
        alignment_in_range(s,n_src());
        alignment_in_range(t,n_tar());
        sa[s].push_back(t);
    }

    void check_size(index n_s,index n_t)
    {
        if (n_tar() != n_t)
            throw std::runtime_error("alignment #t doesn't match");
        if (n_src() != n_s)
            throw std::runtime_error("alignment #s doesn't match");
    }
    

    /// invert twice -> original (swaps tar and destination)
    void set_inverse(alignment const& o) 
    {
        set_empty(o.n_tar(),o.n_src()); // note: src,tar swapped
        for (unsigned i=0;i!=ntar;++i)
            for (tars::const_iterator s=o.sa[i].begin(),se=o.sa[i].end();s!=se;++s)
                sa[*s].push_back(i);
    }


    typedef alignment const* subst;
    
    struct substitution 
    {
        index src_i;
        subst a;
        substitution(alignment const& a,index src_i) : src_i(src_i),a(&a) {}
    };


    /// build (set "into") new alignment based on *this and the substitions in
    /// [sub,sube) into src positions.  note: certain alignments are not
    /// suitable for substition: target aligned to a source variable - must ONLY
    /// align to it.  source variable may align to multiple target destination,
    /// however.  alignments between nonvariable targets are still unconstrained
    /// special case: if this is_null(), then into.set_null().  otherwise out of
    /// range subst.src_i cause exception
    
    template <class substs>
    void substitute_into(alignment & into,substs const& subs) const
    {
        substitute_into(into,subs.begin(),subs.end());
    }
    template <class subst_it>
    void substitute_into(alignment & into,subst_it sub,subst_it sube) const
    {
        if (is_null()) {
            into.set_null();
            return;
        }
        tars tar_offsets(n_tar(),1); // nonvariables are length 1.
        typedef std::vector<subst> substs;
        substs src_vars(n_src(),0);
        for (;sub!=sube;++sub) { // now set variable lengths
            index t=sub->src_i;
            alignment_in_range(t,n_src());
            alignment const& a=*(sub->a);
#ifdef DEBUG_ALIGNMENT
            std::cerr << t<<"<-" << a << ' ';
#endif 
            src_vars[t]=&a;
            tars const& s=sa[t];
            for (tars::const_iterator i=s.begin(),e=s.end();i!=e;++i)
                tar_offsets[*i]=a.n_tar();
        }
        into.ntar=cumulative_offsets(tar_offsets.begin(),tar_offsets.end());
        for (index i=0;i<n_src();++i)
            if (src_vars[i])
                into.append_offset(sa[i],*src_vars[i],tar_offsets);
            else
                into.append_remap(sa[i],tar_offsets);
#ifdef DEBUG_ALIGNMENT
        std::cerr<<*this<<" => "<<into<<"\n";
#endif 
    }
    
    template <class subst_it>
    alignment(alignment const& from,subst_it sub,subst_it sube)
    {
        from.substitute_into(*this,sub,sube);
    }

    template <class substs>
    alignment(alignment const& from,substs const& subs)
    {
        from.substitute_into(*this,subs);
    }
    
    explicit alignment(std::string const& s)
    {
        set(s);
    }
    
    void set(std::string const& s)
    {
        std::istringstream i(s);
        read(i);
    }
    
 private:
    void sort(tars &t) 
    {
        std::sort(t.begin(),t.end());
    }
    
    void append_remap(tars const& t,tars const& offs)
    {
        sa.push_back(t);
        tars &b=sa.back();
        for (tars::iterator i=b.begin(),e=b.end();i!=e;++i)
            *i=offs[*i];
    }

    
    void append_offset(tars const& t,index offset) 
    {
        tars &b=sa.back();
        for (tars::const_iterator i=t.begin(),e=t.end();i!=e;++i)
            b.push_back(*i+offset);
    }

    void prep_new(index size)
    {
        sa.push_back(tars());
        sa.back().reserve(size);
    }
    

    void append_offset(tars const& parent_tars,alignment const& var,tars const& offs)
    {
        // parent alignments: s->t1, s->t2, s->t3, meaning append for each
        // var.sa[j] 3 copies of its tars, shifted by offs[t1..3]
        for (srcs::const_iterator s=var.sa.begin(),se=var.sa.end();s!=se;++s) {
            prep_new(parent_tars.size()*s->size());
            for (tars::const_iterator t=parent_tars.begin(),te=parent_tars.end();t!=te;++t)
                append_offset(*s,offs[*t]); // a copy of the var for each s->t alignment in parent.
        }
    }

};

    
}


#endif
