# include <sbmt/span.hpp>
# include <boost/functional/hash/hash.hpp>
# include <climits>
# include <boost/logic/tribool.hpp>
# include <boost/range.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/utility/result_of.hpp>

using std::make_pair;
using std::pair;
using namespace boost::logic;

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

bool adjacent(span_t const& s1, span_t const& s2)
{
    return s1.right() == s2.left();
}

////////////////////////////////////////////////////////////////////////////////

bool crossing(span_t const& s1, span_t const& s2)
{
    //         s1.left   s1.right
    // s2.left                    s2.right
    span_index_t cleft = std::max(s1.left(),s2.left());
    span_index_t cright = std::min(s1.right(),s2.right());
    if (cleft >= cright) {
        return false;
    } else {
        span_t c(cleft,cright);
        return (c != s1) && (c != s2);
    }
}

////////////////////////////////////////////////////////////////////////////////

span_t combine(span_t const& s1, span_t const& s2)
{
    assert(adjacent(s1,s2));
    return span_t(s1.left(), s2.right());
}

////////////////////////////////////////////////////////////////////////////////

std::size_t hash_value(span_t const& s)
{
    static boost::hash<short> hasher;
    std::size_t retval = hasher(s.left());
    boost::hash_combine(retval,hasher(s.right()));
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

std::size_t length(span_t const& s)
{
    return std::size_t(s.right()) - std::size_t(s.left());
}


////////////////////////////////////////////////////////////////////////////////

span_t shift_left(span_t const& s)
{
    if (s.left() == 0) {
        return span_t(USHRT_MAX - length(s), USHRT_MAX);
    } else {
        return span_t(s.left() - 1, s.right() - 1);
    }
}

span_t shift_right(span_t const& s)
{
    if (s.right() == USHRT_MAX) {
        return span_t(0, length(s));
    } else {
        return span_t(s.left() + 1, s.right() + 1);
    }
}

////////////////////////////////////////////////////////////////////////////////

static tribool past_forward(pair<span_t,span_t> const& p)
{
    if (p.second == span_t(0,USHRT_MAX)) 
        return p.first == span_t(0,USHRT_MAX) ? indeterminate : tribool(true);
    else return tribool(false);
}

static tribool past_reverse(pair<span_t,span_t> const& p)
{
    if (p.first == span_t(0,USHRT_MAX))
        return p.second == span_t(0,USHRT_MAX) ? indeterminate : tribool(true);
    else return tribool(false);
}

static span_t grow_right(span_t const& s)
{
    return span_t(s.left(), s.right() + 1);
}

static span_t grow_left(span_t const& s)
{
    return span_t(s.left() - 1, s.right());
}

static span_t shrink_right(span_t const& s)
{
    return span_t(s.left(), s.right() - 1);
}

static span_t shrink_left(span_t const& s)
{
    return span_t(s.left() + 1, s.right());
}

////////////////////////////////////////////////////////////////////////////////

static std::pair<span_t, span_t>
initial_partition(span_t const& s)
{
    return 
    (length(s) == 1)
    ? make_pair(span_t(0, USHRT_MAX), span_t(0, USHRT_MAX))
    : make_pair(span_t(s.left(), s.left()+1), span_t(s.left()+1, s.right()));
}

static std::pair<span_t, span_t>
final_partition(span_t const& s)
{
    return 
    (length(s) == 1)
    ? make_pair(span_t(0, USHRT_MAX), span_t(0, USHRT_MAX))
    : make_pair(span_t(s.left(), s.right()-1), span_t(s.right()-1, s.right()));
}

////////////////////////////////////////////////////////////////////////////////

partitions_generator::iterator::iterator()
: p(span_t(0, USHRT_MAX), span_t(0, USHRT_MAX)) {}

////////////////////////////////////////////////////////////////////////////////

partitions_generator::iterator::iterator(span_t const& s, bool)
: p(initial_partition(s)) {}

partitions_generator::iterator::iterator(bool, span_t const& s)
: p(length(s) == 1 ? make_pair(span_t(0,USHRT_MAX),span_t(0,USHRT_MAX))
                   : make_pair(s,span_t(0,USHRT_MAX))
   ) {}

////////////////////////////////////////////////////////////////////////////////

partitions_generator::iterator::span_pair const& 
partitions_generator::iterator::dereference() const
{
    return p;
}

////////////////////////////////////////////////////////////////////////////////

bool partitions_generator::iterator::equal(iterator const& o) const
{
    return p == o.p;
}

////////////////////////////////////////////////////////////////////////////////

void partitions_generator::iterator::increment()
{
    if (length(p.second) == 1) {
        p = make_pair(grow_right(p.first), span_t(0, USHRT_MAX));
    } else if (past_reverse(p)) {
        p = initial_partition(p.second);
    } else if (!past_forward(p)) {
        p = make_pair(grow_right(p.first), shrink_left(p.second));
    }
}

void partitions_generator::iterator::decrement()
{
    if (length(p.first) == 1) {
        p = make_pair(span_t(0, USHRT_MAX), grow_left(p.second));
    } else if (past_forward(p)) {
        p = final_partition(p.first);
    } else if (!past_reverse(p)) {
        p = make_pair(shrink_right(p.first), grow_left(p.second));
    }
}

////////////////////////////////////////////////////////////////////////////////

partitions_generator::partitions_generator(span_t s)
: s(s) {}

////////////////////////////////////////////////////////////////////////////////

partitions_generator::iterator partitions_generator::begin() const
{
    return iterator(s,true);
}

////////////////////////////////////////////////////////////////////////////////

partitions_generator::iterator partitions_generator::end() const
{
    return iterator(true,s);
}

////////////////////////////////////////////////////////////////////////////////

shift_generator::iterator::iterator()
: s(0,1) {}

////////////////////////////////////////////////////////////////////////////////

shift_generator::iterator::iterator(span_t const& s)
: s(s) {}

////////////////////////////////////////////////////////////////////////////////

void shift_generator::iterator::increment()
{
    s = shift_right(s);
}

////////////////////////////////////////////////////////////////////////////////

void shift_generator::iterator::decrement()
{
    s = shift_left(s);
}

////////////////////////////////////////////////////////////////////////////////

span_t const& shift_generator::iterator::dereference() const
{
    return s;
}

////////////////////////////////////////////////////////////////////////////////

bool shift_generator::iterator::equal(iterator const& other) const
{
    return s == other.s;
}

////////////////////////////////////////////////////////////////////////////////

shift_generator::iterator shift_generator::begin() const
{
    return iterator(span_t(total_span.left(), total_span.left() + span_size));
}

////////////////////////////////////////////////////////////////////////////////

shift_generator::iterator shift_generator::end() const
{
    span_t ss(total_span.right() - span_size, total_span.right());
    return iterator(shift_right(ss));
}

////////////////////////////////////////////////////////////////////////////////

span_t span_transform::operator()(span_t const& s) const
{
    return span_t(transform[s.left()],transform[s.right()]);
}

////////////////////////////////////////////////////////////////////////////////

span_index_t span_transform::inverse(span_index_t idx) const
{
    std::vector<span_index_t>::const_iterator 
        pos = std::lower_bound(transform.begin(),transform.end(),idx);
    assert(pos != transform.end());
    return span_index_t(pos - transform.begin());
}

////////////////////////////////////////////////////////////////////////////////

span_t span_transform::inverse(span_t const& s) const
{
    return span_t(inverse(s.left()),inverse(s.right()));
}

////////////////////////////////////////////////////////////////////////////////

span_index_t span_transform::operator()(span_index_t idx) const
{
    return transform[idx];
}


namespace {
    struct transform_ {
        span_transform const* t;
        typedef span_t result_type;
        span_t operator()(span_t const& s) const
        {
            return (*t)(s);
        }
        transform_(span_transform const& t) : t(&t) {}
    };
    
    struct transform_p_ {
        span_transform const* t;
        typedef std::pair<span_t,span_t> result_type;
        result_type operator()(result_type const& s) const
        {
            return result_type((*t)(s.first),(*t)(s.second));
        }
        transform_p_(span_transform const& t) : t(&t) {}
    };
}

span_index_t span_transform_generator::shift_sizes(span_t const& s) const 
{ 
    return length(transform.inverse(s)); 
}

cky_generator::span_range
span_transform_generator::shifts(span_t const& s, span_index_t size) const
{
    cky_generator::span_range rng = gen->shifts( transform.inverse(s)
                                               , size );
    return
    base_t::span_range(
      boost::make_transform_iterator(boost::begin(rng), transform_(transform))
    , boost::make_transform_iterator(boost::end(rng), transform_(transform))
    );
}

cky_generator::partition_range
span_transform_generator::partitions(span_t const& s) const
{
    cky_generator::partition_range rng = gen->partitions(transform.inverse(s));
    return
    cky_generator::partition_range(
      boost::make_transform_iterator(boost::begin(rng), transform_p_(transform))
    , boost::make_transform_iterator(boost::end(rng), transform_p_(transform))
    );
}

cky_generator::span_range 
limit_syntax_length_generator::shifts(span_t const& s, span_index_t size) const
{
    base_t::span_range retval;
    shift_generator gen(s,size);
    if (max_length > size) {
        retval.first = gen.begin();
        retval.second = gen.end();
    } else {
        shift_generator::iterator itr = gen.begin();
        retval.first = itr;
        retval.second = itr;
        if (itr != gen.end() && (itr->left() == 0)) ++(retval.second);
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

cky_generator::partition_range 
limit_syntax_length_generator::partitions(span_t const& s) const
{
    base_t::partition_range retval;
    partitions_generator gen(s);
    retval.first = gen.begin();
    retval.second = gen.end();
    if (length(s) > max_length) {
        // todo: make partition_generator::iterator random access!
        while( retval.first != retval.second &&
               length(retval.first->second) > max_length ) ++retval.first;
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

cky_generator::span_range 
limit_split_difference_generator::shifts(span_t const& s, span_index_t size) const
{
    base_t::span_range retval;
    shift_generator gen(s,size);
    retval.first = gen.begin();
    retval.second = gen.end();
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

inline std::size_t absdiff(std::size_t na,std::size_t nb)
{
    return na>nb ? na-nb : nb-na;
}


inline std::size_t length_diff(span_t const& a,span_t const& b) 
{
    return absdiff(length(a),length(b));
}

cky_generator::partition_range 
limit_split_difference_generator::partitions(span_t const& s) const
{
    base_t::partition_range retval;
    partitions_generator gen(s);
    
    if (s.left() > 0) {
        partitions_generator::iterator pitr = gen.begin(),
                                       pend = gen.end();
        while( pitr != pend &&
               length_diff(pitr->first,pitr->second) > max_diff ) {
            ++pitr;
        }
        retval.first = pitr;
        if (pitr != pend) ++pitr;
        while ( pitr != pend &&
                length_diff(pitr->first,pitr->second) <= max_diff ) {
            ++pitr;
        }
        retval.second = pitr;
    }
    else {
        retval.first = gen.begin();
        retval.second = gen.end();
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

cky_generator::span_range 
full_cky_generator::shifts(span_t const& s, span_index_t size) const
{
    base_t::span_range retval;
    shift_generator gen(s,size);
    retval.first = gen.begin();
    retval.second = gen.end();
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

cky_generator::partition_range 
full_cky_generator::partitions(span_t const& s) const
{
    base_t::partition_range retval;
    partitions_generator gen(s);
    retval.first = gen.begin();
    retval.second = gen.end();
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
