# ifndef   SBMT_SPAN_HPP
# define   SBMT_SPAN_HPP

# include <utility>
# include <cassert>
# include <cstddef>
# include <iostream>
# include <vector>
# include <boost/iterator/iterator_facade.hpp>
# include <boost/operators.hpp>
# include <boost/cstdint.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/range.hpp>
# include <sbmt/hash/concrete_iterator.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  \defgroup Spans Spans
///  span_t class and associated functions for manipulating spans
///
////////////////////////////////////////////////////////////////////////////////
///\{

typedef boost::uint16_t span_index_t;

////////////////////////////////////////////////////////////////////////////////
///
///  essentially just a pair of numbers, but with helpful functions that
///  for doing useful operations on spans, like shifting, partitioning, etc.
///
////////////////////////////////////////////////////////////////////////////////
class span_t : public boost::totally_ordered<span_t>
{
public:
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  \pre    l < r
    ///  \post   left = l and right = r
    ///
    ////////////////////////////////////////////////////////////////////////////
    span_t(span_index_t l, span_index_t r) : s(l,r) { assert(l <= r); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  \post   left = 0 and right = 0
    ///
    ////////////////////////////////////////////////////////////////////////////
    span_t() : s(0,0) {}

    span_index_t left()  const { return s.first; }
    span_index_t right() const { return s.second; }
    span_index_t lr(bool r) const { return r?s.second:s.first; }
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  \return right - left
    ///
    ////////////////////////////////////////////////////////////////////////////
    std::size_t size() const
    { return right()-left(); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  lexographic equal:
    ///  \code
    ///  s1 == s2 <=> s1.left == s2.left and s1.right == s2.right
    ///  \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    bool operator==(span_t const& o) const
    { return left() == o.left() && right() == o.right(); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  lexographic order:
    ///  \code
    ///  s1 < s2 <=>
    ///  s1.left < s2.left or s1.left == s2.left and s1.right < s2.right
    ///  \endcode
    ///
    ////////////////////////////////////////////////////////////////////////////
    bool operator< (span_t const& o) const
    { return left() < o.left() || (left() == o.left() && right() < o.right()); }
private:
    std::pair<span_index_t,span_index_t> s;
};

////////////////////////////////////////////////////////////////////////////////
///
///  prints [s.left(),s.right()] into a stream.
///  \todo it would be nice if you could use a manipulator to change the
///  "[", ",", and "]" symbols.  one easy way would be to back span_t with
///  boost::tuple<ushort,ushort>, and typedef its manipulators
///
////////////////////////////////////////////////////////////////////////////////
template <class C, class T>
std::basic_ostream<C,T>& operator << ( std::basic_ostream<C,T>& out
                                     , span_t const& s )
{
    return out << "[" << s.left() << "," << s.right() << "]";
}

template <class C, class T>
std::basic_istream<C,T>& operator >> ( std::basic_istream<C,T>& in
                                     , span_t& s )
{
    if (!in.good()) return in;
    char bl, bc, br;
    span_index_t left, right;
    if (in >> bl >> left >> bc >> right >> br) {
        if (bl != '[' || bc != ',' || br != ']' || right < left)
            in.setstate(std::ios::failbit);
        else
            s = span_t(left,right);
    }
    return in;
}

////////////////////////////////////////////////////////////////////////////////
///
///  determines if two spans are adjacent.
///  \return s1.right() == s2.left()
///
////////////////////////////////////////////////////////////////////////////////
bool adjacent(span_t const& s1, span_t const& s2);

////////////////////////////////////////////////////////////////////////////////
///
///  determines if two spans cross each other.  true iff the intersection of the
///  spans is non-empty and properly contained in both spans
///
////////////////////////////////////////////////////////////////////////////////
bool crossing(span_t const& s1, span_t const& s2);

////////////////////////////////////////////////////////////////////////////////
///
///  combines two spans
///  \return span_t(s1.left(),s2.right())
///
////////////////////////////////////////////////////////////////////////////////
span_t combine(span_t const& s1, span_t const& s2);

std::size_t hash_value(span_t const& s);

////////////////////////////////////////////////////////////////////////////////
///
///  returns a new span shifted to the right
///  \return span_t(s.left() + 1, s.right() + 1)
///
////////////////////////////////////////////////////////////////////////////////
span_t shift_right(span_t const& s);

////////////////////////////////////////////////////////////////////////////////
///
///  returns a new span shifted to the left
///  \return span_t(s.left() - 1, s.right() - 1)
///
////////////////////////////////////////////////////////////////////////////////
span_t shift_left(span_t const& s);

std::size_t length(span_t const& s);

////////////////////////////////////////////////////////////////////////////////
///
///  generates all partitions of a given span.  example:
///  \code
///     partitions_generator pg(span_t(1,5));
///     partitions_generator::iterator itr = pg.begin();
///     for (; itr != pg.end(); ++itr) {
///         cout << "{" << itr->first <<"," << itr->second << "}";
///     }
///  \endcode
///  will print out
///  \code
///     {[1,2],[2,5]},{[1,3],[3,5]},{[1,4],[4,5]},
///  \endcode
///
////////////////////////////////////////////////////////////////////////////////
class partitions_generator
{
public:
    class iterator
    : public boost::iterator_facade<
                 iterator
               , std::pair<span_t,span_t> const
               , boost::bidirectional_traversal_tag
             >
    {
    public:
        iterator();
    private:
        iterator(span_t const& s, bool); //begin marker
        iterator(bool, span_t const& s); //end marker
        typedef std::pair<span_t,span_t> span_pair;
        span_pair const& dereference() const;
        bool equal( iterator const& other) const;
        void increment();
        void decrement();
        friend class boost::iterator_core_access;
        span_pair p;
        friend class partitions_generator;
    };

    typedef iterator const_iterator;

    partitions_generator(span_t s);

    iterator begin() const;
    iterator end() const;
private:
    span_t s;
};

////////////////////////////////////////////////////////////////////////////////
///
/// generates off-by-one shifts of a specific span size.
/// example:
/// \code
///   shift_generator gen(span_t(1,5),2);
///   shift_generator::iterator itr = gen.begin();
///   for (; itr != gen.end(); ++itr) cout << *itr <<",";
/// \endcode
///
/// will print "[1,3],[2,4],[3,5],"
///
////////////////////////////////////////////////////////////////////////////////
class shift_generator
{
public:
    class iterator
    : public boost::iterator_facade<
                iterator
              , span_t const
              , boost::bidirectional_traversal_tag
             >
    {
    public:
        iterator();
    private:
        iterator(span_t const& s);
        span_t const& dereference() const;
        bool equal(iterator const& other) const;
        void increment();
        void decrement();
        friend class boost::iterator_core_access;
        span_t s;
        friend class shift_generator;
    };

    typedef iterator const_iterator;

    shift_generator(span_t const& total_range, span_index_t span_size)
    : total_span(total_range)
    , span_size(span_size) { assert(length(total_range) >= span_size); }

    iterator begin() const;
    iterator end() const;

private:
    span_t total_span;
    span_index_t span_size;
};

////////////////////////////////////////////////////////////////////////////////

class span_transform
{
public:
    template <class ItrT>
    span_transform(ItrT begin, ItrT end)
    : transform(begin,end) {}

    span_t operator()(span_t const& s) const;
    span_index_t operator()(span_index_t idx) const;
    span_t inverse(span_t const& s) const;
    span_index_t inverse(span_index_t const) const;
private:
    std::vector<span_index_t> transform;
};

////////////////////////////////////////////////////////////////////////////////

typedef concrete_forward_iterator<span_t,span_t> span_iterator;

typedef concrete_forward_iterator< std::pair<span_t,span_t>
                                 , std::pair<span_t,span_t>
                                 > partition_iterator;

////////////////////////////////////////////////////////////////////////////////

struct cky_generator {
    typedef std::pair<span_iterator,span_iterator> span_range;
    typedef std::pair<partition_iterator,partition_iterator> partition_range;
    virtual span_range shifts(span_t const& s, span_index_t size) const = 0;
    virtual span_index_t shift_sizes(span_t const& s) const = 0;
    virtual partition_range partitions(span_t const& s) const = 0;
    virtual ~cky_generator() {}
};

////////////////////////////////////////////////////////////////////////////////

class limit_syntax_length_generator : public cky_generator {
public:
    limit_syntax_length_generator(std::size_t max_syntax_length)
     : max_length(max_syntax_length) {}
    typedef cky_generator base_t;
    virtual base_t::span_range shifts(span_t const& s, span_index_t size) const;
    virtual span_index_t shift_sizes(span_t const& s) const { return length(s); }
    virtual base_t::partition_range partitions(span_t const& s) const;
private:
    std::size_t max_length;
};

////////////////////////////////////////////////////////////////////////////////

struct full_cky_generator : public cky_generator {
    typedef cky_generator base_t;
    virtual base_t::span_range shifts(span_t const& s, span_index_t size) const;
    virtual span_index_t shift_sizes(span_t const& s) const { return length(s); }
    virtual base_t::partition_range partitions(span_t const& s) const;
};

////////////////////////////////////////////////////////////////////////////////

struct limit_split_difference_generator : public cky_generator {
    typedef cky_generator base_t;
    limit_split_difference_generator(std::size_t max_diff)
      : max_diff(max_diff) {}
    virtual base_t::span_range shifts(span_t const& s, span_index_t size) const;
    virtual span_index_t shift_sizes(span_t const& s) const { return length(s); }
    virtual base_t::partition_range partitions(span_t const& s) const;
private:
    std::size_t max_diff;
};

struct span_transform_generator : public cky_generator {
    typedef cky_generator base_t;
    template <class Range>
    span_transform_generator(Range const& transform, cky_generator const& gen)
    : gen(&gen)
    , transform(boost::begin(transform),boost::end(transform)) {}

    virtual base_t::span_range shifts(span_t const& s, span_index_t size) const;
    virtual span_index_t shift_sizes(span_t const& s) const;
    virtual base_t::partition_range partitions(span_t const& s) const;
private:
    cky_generator const* gen;
    span_transform transform;
};

///\}

} // namespace sbmt

# endif // SBMT_SPAN_HPP
