# include "mini_decoder_filters.tpp"
# include <boost/lambda/lambda.hpp>

using namespace boost;
using namespace std;
using namespace sbmt;

////////////////////////////////////////////////////////////////////////////////

namespace {
    
score_t fuzz_pow(score_t base, double p)
{
    if (base == 0.0 or p == 0.0) return 1.0;
    else return pow(base,p);
}

score_t safe_pow(score_t base, double p) 
{
    if (base == 0.0) return 0.0;
    if (p == 0.0) return 1.0;
    else return pow(base,p);
}

struct score_pow 
: public std::binary_function<sbmt::score_t,double,sbmt::score_t>
{
    result_type operator()(
        first_argument_type const& base,
        second_argument_type const& nthpow) const 
    {
        return sbmt::pow(base,nthpow);
    }
};

struct times_floored 
: public std::binary_function<double,double,double>
{
    double floor;
    times_floored(double floor=0) : floor(floor) {}
    result_type operator()(
        first_argument_type const& x,
        second_argument_type const& y) const 
    {
        return std::max(floor,x*y);
    }
};

} // unnamed namespace

////////////////////////////////////////////////////////////////////////////////

void filter_args::require_valid_beam_type(beam_type i)
{
    assert(i>=0 && i<N_BEAM_TYPES);
}

void filter_args::require_valid_beam_scope(beam_scope i)
{
    assert(i>=0 && i<N_BEAM_SCOPES);
}

////////////////////////////////////////////////////////////////////////////////

shared_ptr<cky_generator> filter_args::create_cky_generator() const
{
    shared_ptr<cky_generator> cky_gen;
    if(limit_syntax_length > 0) 
        cky_gen.reset(new limit_syntax_length_generator(limit_syntax_length));
    else if (limit_split_diff > 0) 
        cky_gen.reset(new limit_split_difference_generator(limit_split_diff));
    else cky_gen.reset(new full_cky_generator());
    return cky_gen;
}

////////////////////////////////////////////////////////////////////////////////

graehl::printable_options_description<ostream> filter_args::options()
{
    using namespace graehl;
    using namespace boost::program_options;
    using namespace boost::lambda;

    printable_options_description<ostream>
       pruning_opts("Options to control pruning - "
                    "note: optionally write max/min settings as '50/10'");
    typedef filter_args F;
    pruning_opts.add_options()
        ( "use-cube-heap"
        , defaulted_value(&use_cube_heap)
        , "apply the cube-heap pruning assumption for faster search"
        )
        ( "separate-tag-virt-pruning"
        , defaulted_value(&separate_tag_virt)
        , "apply pruning techniques to tag and virtual symbols separately"
        )
        ( "span-beam"
        , defaulted_value(&beam[F::FULL][F::SPAN])
        , "ratio-beam cutoff per span.  "
          "use if separate-tag-virt-pruning is false (linear scale)"
        )
        ( "cell-beam"
        , defaulted_value(&beam[F::FULL][F::CELL])
        , "ratio-beam cutoff per cell.  "
          "use if separate-tag-virt-pruning is false (linear scale)"
        )
        ( "span-max-edges"
        , defaulted_value(&hist[F::FULL][F::SPAN])
        , "restrict number of edges per span.  "
          "use if separate-tag-virt-pruning is false"
        )
        ( "cell-max-edges"
        , defaulted_value(&hist[F::FULL][F::CELL])
        , "restrict number of edges per cell.  "
          "use if separate-tag-virt-pruning is false"
        )
        ( "tag-span-beam"
        , defaulted_value(&beam[F::TAG][F::SPAN])
        , "ratio-beam cutoff per span for syntactic tag rooted edges.  "
          "use if separate-tag-virt-pruning is true (linear scale)"
        )
        ( "tag-cell-beam"
        , defaulted_value(&beam[F::TAG][F::CELL])
        , "ratio-beam cutoff per cell for syntactic tag rooted edges.  "
          "use if separate-tag-virt-pruning is true (linear scale)"
        )
        ( "tag-span-max-edges"
        , defaulted_value(&hist[F::TAG][F::SPAN])
        , "restrict number of edges with syntactic tag roots per span.  "
          "use if separate-tag-virt-pruning is true"
        )
        ( "tag-cell-max-edges"
        , defaulted_value(&hist[F::TAG][F::CELL])
        , "restrict number of edges with syntactic tag roots per cell.  "
          "use if separate-tag-virt-pruning is true"
        )
        ( "virt-span-beam"
        , defaulted_value(&beam[F::VIRT][F::SPAN])
        , "ratio-beam cutoff per span for virtual tag rooted edges.  "
          "use if separate-tag-virt-pruning is true (linear scale)"
        )
        ( "virt-cell-beam"
        , defaulted_value(&beam[F::VIRT][F::CELL])
        , "ratio-beam cutoff per cell for virtual tag rooted edges.  "
          "use if separate-tag-virt-pruning is true (linear scale)"
        )
        ( "virt-span-max-edges"
        , defaulted_value(&hist[F::VIRT][F::SPAN])
        , "restrict number of edges with virtual tag roots per span.  "
          "use if separate-tag-virt-pruning is true"
        )
        ( "virt-cell-max-edges"
        , defaulted_value(&hist[F::VIRT][F::CELL])
        , "restrict number of edges with virtual tag roots per cell.  "
          "use if separate-tag-virt-pruning is true"
        )
        ( "unary-span-max-edges"
        , value(&hist[F::UNRY][F::SPAN])
          ->notifier(var(specify_unary_pruning) = true)
        , "restrict number of edges per span for unary rule applications.  "
          "if setting this option, then also set unary-span-beam. "
        )
        ( "unary-span-beam"
        , value(&beam[F::UNRY][F::SPAN])
          ->notifier(var(specify_unary_pruning) = true)
        , "ratio-beam cutoff per span for unary rule applications.  "
          "if setting this option, then also set unary-span-max-edges. "
        )
        ( "unary-cell-max-edges"
        , value(&hist[F::UNRY][F::CELL])
          ->notifier(var(specify_unary_pruning) = true)
        , "restrict number of edges per cell for unary rule applications.  "
          "if setting this option, then also set unary-cell-beam. "
        )
        ( "unary-cell-beam"
        , value(&beam[F::UNRY][F::CELL])
          ->notifier(var(specify_unary_pruning) = true)
        , "ratio-beam cutoff per cell for unary rule applications.  "
          "if setting this option, then also set unary-cell-max-edges. "
        ) 
        ( "use-cell-pruning"
        , defaulted_value(&cell_prune[F::FULL])
        , "use cell pruning for all types of productions"
        )
        ( "use-tag-cell-pruning"
        , defaulted_value(&cell_prune[F::TAG])
        , "use cell pruning for tag productions.  "
          "requires separate-tag-virt-pruning to be true"
        )
        ( "use-virt-cell-pruning"
        , defaulted_value(&cell_prune[F::VIRT])
        , "use cell pruning for virtual productions.  "
          "requires separate-tag-virt-pruning to be true"
        )
        ( "use-unary-cell-pruning"
        , defaulted_value(&cell_prune[F::UNRY])
        , "use cell pruning for unary productions.  "
        )
        ( "cube-heap-fuzz-exp"
        , defaulted_value(&cube_heap_fuzz_exp)
        , "affects the beam used during the cube-heap assumption, as an "
          "exponent of the passed in beam.  "
          "range [0,inf) fuzz values greater than 0 reduce the cube-heap "
          "pruning; inf => exhaustive."
        )
        ( "cube-heap-hist-fuzz"
        , value< graehl::optional_pair<sbmt::score_t> >(&cube_heap_hist_fuzz)
          ->notifier(var(use_cube_heap_hist_fuzz) = true)
        , "if set, this value is used to set histogram fuzz, rather than "
          "relying on the various beam^cube-heap-fuzz-exp calculations.  "
          "useful when wanting to use fuzz, but setting beams to 0."
        )
        ( "cube-heap-poplimit-multiplier"
        , value<std::size_t>(&cube_heap_poplimit_multiplier)
          ->notifier(var(use_cube_heap_poplimit) = true)
        , "if set, cube-heap-poplimit-multiplier * (tag|virt)-span-max-edges "
          "edges will be explored in the cube-heap before stopping.  fuzz "
          "does not play a role. useful for beam-less decoding."
        )
        ( "limit-syntax-length"
        , defaulted_value(&limit_syntax_length)
        , "if greater than zero, limits syntax applications to foreign spans "
          "of given length.  longer spans will be restricted to glueing and "
          "toplevel rules."
        )
        ( "limit-syntax-glue-label"
        , defaulted_value(&limit_syntax_glue_label)
        , "name of the glueing rule used by the limit-syntax filter"
        )
        ( "limit-split-diff"
        , defaulted_value(&limit_split_diff)
        , "if greater than 0, limit non-glue applications to those where the "
          "difference between constituent spans is less than given length.  "
          "ignored if limit-syntax-length is set."
        )
        ( "quasi-binarize-pruning"
        , defaulted_value(&quasi_bin)
        , "only prune completed quasi-binarized rules "
          "(binarized rules that have no further terminals in the "
          "binarization)"
        )
        ;
      
    graehl::printable_options_description<ostream>  
        retry_opts("Retry (out of memory recovery) options - "
                   "if pruning options don't specify a minimum e.g. 1e-50/1e-2, "
                   "then the some fraction of the maximum is used");
    retry_opts.add_options()         
        ( "max-retries"
        , defaulted_value(&max_retries)
        , "reach the final pruning options after this many retries "
          "(0 means only one try)"
        )
        ( "beam-retry-exp-frac"
        , defaulted_value(&final_beam_exp_fraction)
        , "(default) final retry beam = starting exponent scaled by this fraction"
        )
        ( "hist-retry-frac"
        , defaulted_value(&final_hist_fraction)
        , "(default) final hist retry = starting hist scaled by this fraction"
        )
        ( "retry-curvature"
        , defaulted_value(&retry_curvature)
        , "(applies to histograms only) how quickly to converge on the final "
          "value; 0 means exponential decay, near 1 means almost instantaneous "
          "decay, near negative infinity means linear."
        )
        ( "max-equivalents"
        , defaulted_value(&max_equivalents)
        , "keep only this many equivalent edges; at most you need as many as "
          "you want nbests; usually 4 or so is fine.  1 will result in only a "
          "1best derivation but no other harm (0=unlimited)"
        )
        ;
    graehl::printable_options_description<ostream> 
        opts("filter and retry options");
    
    opts.add(pruning_opts);
    opts.add(retry_opts);    
    return opts;
}

////////////////////////////////////////////////////////////////////////////////

void filter_args::set_force_decode_defaults()
{
    set_defaults();
    use_cube_heap=false;
    separate_tag_virt=false;
    beam[FULL][SPAN]=0;
    hist[FULL][SPAN].set(1000000,1);
}

void filter_args::set_defaults()
{
    use_cube_heap_poplimit=false;
    use_cube_heap=true;
    separate_tag_virt=true;
    use_cube_heap_hist_fuzz=false;
    cube_heap_hist_fuzz=1e-10;
    
    beam[FULL][SPAN]=1e-40;
    hist[FULL][SPAN]=700;
    beam[FULL][CELL]=1e-16;
    hist[FULL][CELL]=20;
    cell_prune[FULL]=false;
    
    beam[TAG][SPAN]=1e-40;
    hist[TAG][SPAN]=100;
    beam[TAG][CELL]=1e-16;
    hist[TAG][CELL]=10;
    cell_prune[TAG]=false;
    
    beam[VIRT][SPAN]=1e-32;
    hist[VIRT][SPAN]=600;
    beam[VIRT][CELL]=1e-16;
    hist[VIRT][CELL]=20;
    cell_prune[VIRT]=false;
    
    beam[UNRY][SPAN]=beam[TAG][SPAN];
    hist[UNRY][SPAN]=hist[TAG][SPAN];
    beam[UNRY][CELL]=beam[TAG][CELL];
    hist[UNRY][CELL]=hist[TAG][CELL];
    cell_prune[UNRY]=false;
    specify_unary_pruning=false;
    
    cube_heap_fuzz_exp=0.2;
    retry_curvature=0;

    final_hist_fraction=0.02;
    final_beam_exp_fraction=0.01;
    max_retries=5;
    
    limit_syntax_glue_label="GLUE";
    limit_syntax_length=0;
    limit_split_diff=0;
    quasi_bin=false;
    use_cube_heap_poplimit=false;
    cube_heap_poplimit_multiplier=1;
    
    max_equivalents=DEFAULT_MAX_EQUIVALENTS;
}

////////////////////////////////////////////////////////////////////////////////

beam_retry filter_args::hist_fuzz_f(beam_type i, beam_scope j, bool fuzz) const
{
    binder2nd<score_pow> 
        beam_scale(score_pow(),final_beam_exp_fraction);
    if (not use_cube_heap_hist_fuzz) return beam_f(i,j,fuzz);
    else return sbmt::beam_retry(graehl::clamped_time_series<sbmt::score_t>(
            cube_heap_hist_fuzz.first
          , cube_heap_hist_fuzz.get_second_default(beam_scale)
          , max_retries
          , 0
    ));
}

////////////////////////////////////////////////////////////////////////////////

beam_retry filter_args::beam_f(beam_type i, beam_scope j, bool fuzz) const
{
    require_valid_beam_type(i);
    require_valid_beam_scope(j);
    binder2nd<score_pow> 
        beam_scale(score_pow(),final_beam_exp_fraction);
    //double fuzz_exp = fuzz ? cube_heap_fuzz_exp : 1.0;
    if ( fuzz ) {
        graehl::clamped_time_series<sbmt::score_t> ret(
            fuzz_pow(beam[i][j].first,cube_heap_fuzz_exp),
            fuzz_pow(beam[i][j].get_second_default(beam_scale),cube_heap_fuzz_exp),
            max_retries,
            0
//            retry_curvature
        //FIXME: curvature does nothing if <=0 for score_t, 
        //       otherwise results in +INF (bug) if positive
        );
        return sbmt::beam_retry(ret);
    } else {
        graehl::clamped_time_series<sbmt::score_t> ret(
            safe_pow(beam[i][j].first,1.0),
            safe_pow(beam[i][j].get_second_default(beam_scale),1.0),
            max_retries,
            0
        );
        return sbmt::beam_retry(ret);
    }
}

////////////////////////////////////////////////////////////////////////////////

hist_retry filter_args::hist_f(beam_type i, beam_scope j) const
{
    require_valid_beam_type(i);
    require_valid_beam_scope(j);
    std::binder2nd<times_floored> 
        hist_scale(times_floored(2),final_hist_fraction);
    graehl::clamped_time_series<double,sbmt::hist_t> ret(
        hist[i][j].first
        , hist[i][j].get_second_default(hist_scale)
        , max_retries
        , retry_curvature);
    return sbmt::hist_retry(ret);
}

////////////////////////////////////////////////////////////////////////////////
hist_retry filter_args::poplimit_f(beam_type i, beam_scope j) const
{
    require_valid_beam_type(i);
    require_valid_beam_scope(j);
    std::binder2nd<times_floored> 
        hist_scale(times_floored(2),final_hist_fraction);
    graehl::clamped_time_series<double,sbmt::hist_t> ret(
        hist[i][j].first * cube_heap_poplimit_multiplier
        , hist[i][j].get_second_default(hist_scale) * cube_heap_poplimit_multiplier
        , max_retries
        , retry_curvature);
    return sbmt::hist_retry(ret);
}

////////////////////////////////////////////////////////////////////////////////
