#ifndef SBMT_MULTIPASS__KBEST_OUTPUT_COMPARE_CELLS_HPP
#define SBMT_MULTIPASS__KBEST_OUTPUT_COMPARE_CELLS_HPP

#include <sbmt/forest/derivation.hpp>
#include <sbmt/multipass/cell_heuristic.hpp>
#include <sbmt/multipass/logging.hpp>
#include <sbmt/forest/kbest.hpp>
#include <graehl/shared/percent.hpp>

namespace sbmt {

struct no_missing_record
{
    void operator()(span_t const& span,indexed_token const& root) const {}
};        

struct compare_cells_log_missing_record
{
    std::string  header;
    indexed_token_factory const& tf;
    compare_cells_log_missing_record(std::string const& header,indexed_token_factory const& tf) :
        header(header),tf(tf)
    {}
    void operator()(span_t const& span,indexed_token const& root) const {
        SBMT_INFO_STREAM(compare_cells, header << tf.label(root) << span);        
    }
};        

template <class Edge,class Missing>
bool allowed_by_cells(edge_equivalence<Edge> const& e,cell_heuristic const& cells,Missing &o,bool stop_first_failure=false,bool stop_outside=true) 
{
    return allowed_by_cells(e.representative(),cells,o,stop_first_failure,stop_outside);
}

template <class Inf,class Missing>
bool allowed_root_cell(edge<Inf> const& e,cell_heuristic const& cells,Missing &o) 
{
    SBMT_PEDANTIC_STREAM(compare_cells,"have_cell("<<o.tf.label(e.root()) << e.span() << ")=" << cells.have_cell(e.span(),e.root()));
    if (cells.have_cell(e.span(),e.root()))
        return true;
    o(e.span(),e.root());
    return false;
}
    
template <class Inf,class Missing>
bool allowed_by_cells(edge<Inf> const& e,cell_heuristic const& cells,Missing &o,bool stop_first_failure=false,bool stop_outside=true) 
{
    bool ret=true;
    if (e.has_first_child()) {
#define ALLOWED_BY_CELLS_EACH_CHILD(fos)                                                                                   \
        if (!allowed_by_cells(e.fos(),cells,o,stop_first_failure,stop_outside)) { if (stop_first_failure) return false; else ret=false; }
        
        ALLOWED_BY_CELLS_EACH_CHILD(first_children);
        if (e.has_second_child()) {
            ALLOWED_BY_CELLS_EACH_CHILD(second_children);
        }
#undef ALLOWED_BY_CELLS_EACH_CHILD
    }
    if (stop_outside && !ret)
        return ret;
    return allowed_root_cell(e,cells,o);
}

template <class Edge,class Grammar>
struct kbest_output_compare_cells : public kbest_output<Edge,Grammar>
{
    typedef Grammar grammar_type;
    typedef Edge edge_type;
    typedef edge_equivalence<edge_type> equiv_type;
    typedef derivation_interpreter<edge_type,grammar_type> Interp;
    typedef typename Interp::derivation_type derivation_type;
    typedef typename Interp::edge_equiv_type edge_equiv_type;
    
    typedef typename Interp::template nbest_printer<std::ostream> N_printer;
    typedef edge_equivalence_pool<Edge> Epool;
    typedef concrete_edge_factory<Edge,Grammar> edge_factory_type;

    cell_heuristic const*pcells;
    unsigned verbose;
    bool missing_cell_ends_nbests;
    
    struct nbest_printer_compare_cells : public N_printer
    {
        typedef N_printer NP;
        typedef std::ostream Ostream;
        cell_heuristic const*pcells;
        unsigned verbose;
        bool missing_cell_ends_nbests;
     public:
        bool operator()(derivation_type const& deriv, unsigned nbest_rank=0) const
        {
            bool cont=static_cast<N_printer const&>(*this)(deriv,nbest_rank);
            if (!pcells) return cont;
            bool stop_first= verbose==0;
            bool stop_outside= verbose<=1;
            compare_cells_log_missing_record missing(
                this->get_nbest_header(nbest_rank)+"missing previous-pass cell ",this->interp.dict());
            bool allowed=allowed_by_cells(deriv_edge(deriv),*pcells,missing,stop_first,stop_outside);
            if (!allowed && missing_cell_ends_nbests)
                return false;
            return cont;
        }

        
        nbest_printer_compare_cells(const Interp &di,Ostream &o,
                                    unsigned sent=1,unsigned pass=0
                                    ,cell_heuristic const*pcells=0
                                    ,unsigned verbose=0
                                    , bool missing_cell_ends_nbests=true
                               ,         extra_english_output_t *eo=0
                                    
            )
            : N_printer(di,o,sent,pass,eo)
            , pcells(pcells)
            , verbose(verbose)
            , missing_cell_ends_nbests(missing_cell_ends_nbests)
        {
            if (pcells)
                SBMT_INFO_STREAM(compare_cells, "sent="<<sent<<" pass="<<pass<<": reporting any derivations that use cells not in previously kept cell table ("<<graehl::percent<4>(pcells->portion_nonempty_spans())<<" - "<<pcells->nonempty_spans()<<"/"<<pcells->possible_spans()<<" useful spans, " << pcells->present_cells()<< " useful cells");   
        }
    };
        
    typedef kbest_output<Edge,Grammar> base_t;

    extra_english_output_t *eo;
    
    kbest_output_compare_cells(std::ostream &kbest_out
                 ,grammar_type &gram
                 , edge_factory_type &efact
                 , Epool &epool
                 ,derivation_output_options const& deriv_opt=derivation_output_options()
                               , cell_heuristic const*pcells=0
                               , unsigned verbose=0
                               , bool missing_cell_ends_nbests=true

        )
        :
        base_t(kbest_out,gram,efact,epool,deriv_opt)
        ,pcells(pcells)
        , verbose(verbose)
        , missing_cell_ends_nbests(missing_cell_ends_nbests)
    {}

    //FIXME: repeated code
    template <class FilterFactory>
    void print_kbest(equiv_type const& top_equiv,unsigned sentid,unsigned pass,unsigned maxkbests,FilterFactory const& f
                     , extra_english_output_t *eo=0
        )
    {
        if (!pcells) {
            base_t::print_kbest(top_equiv,sentid,pass,maxkbests,f,eo);
            return;
        }
        
        nbest_printer_compare_cells nprint(this->interp,this->kbest_out,sentid,pass,pcells,verbose,missing_cell_ends_nbests,eo);
	base_t::enumerate_kbest(top_equiv,nprint,maxkbests,f);
    }

    void print_kbest(equiv_type const& top_equiv,unsigned sentid,unsigned pass,unsigned maxkbests,unsigned max_deriv_per_estring=base_t::UNLIMITED_PER_STRING
                     , extra_english_output_t *eo=0
        )
    {
//FIXME: template method?  repeated exactly from base
        if (max_deriv_per_estring==base_t::UNLIMITED_PER_STRING)
            print_kbest(top_equiv,sentid,pass,maxkbests,graehl::permissive_kbest_filter_factory(),eo);
        else
            print_kbest(top_equiv,sentid,pass,maxkbests,filter_by_unique_string(this->interp,max_deriv_per_estring),eo);
    }
    

};

}


#endif
