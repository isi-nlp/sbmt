# ifndef   XRSDB__FOREST_READER
# define   XRSDB__FOREST_READER

# include <boost/shared_ptr.hpp>
# include <vector>
# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/span.hpp>

namespace exmp {

struct forest;
struct hyp;
typedef boost::shared_ptr<forest> forest_ptr;
typedef boost::shared_ptr<hyp> hyp_ptr;

class forest {
    sbmt::span_t s;
    std::vector<hyp_ptr> opts;
public:
    typedef std::vector<hyp_ptr>::const_iterator const_iterator;
    typedef const_iterator iterator;
    typedef hyp_ptr value_type;
    typedef hyp_ptr const& const_reference;
    typedef const_reference reference;
    sbmt::span_t span() const;
    std::vector<hyp_ptr>::const_iterator begin() const;
    std::vector<hyp_ptr>::const_iterator end() const;
    explicit forest(sbmt::span_t const& s);
    explicit forest(std::vector<hyp_ptr> const& v);
    forest() {}
};

class hyp {
    boost::int64_t rid;
    sbmt::span_t s;
    sbmt::weight_vector w;
    std::vector<forest_ptr> c;
public:
    typedef std::vector<forest_ptr>::const_iterator const_iterator;
    typedef const_iterator iterator;
    typedef forest_ptr value_type;
    typedef forest_ptr const& const_reference;
    typedef const_reference reference;
    boost::int64_t ruleid() const;
    sbmt::span_t span() const;
    sbmt::weight_vector const& weights() const;
    std::vector<forest_ptr>::const_iterator begin() const;
    std::vector<forest_ptr>::const_iterator end() const; 
    hyp(boost::int64_t rid, sbmt::weight_vector const& w, std::vector<forest_ptr> const& v);
};

std::istream& getforest(std::istream& is, forest& f, sbmt::feature_dictionary& dict);
std::ostream& operator << (std::ostream& os, forest const& f);

} // namespace exmp

# endif // XRSDB__FOREST_READER
