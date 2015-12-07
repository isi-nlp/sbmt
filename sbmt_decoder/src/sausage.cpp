# include <sbmt/search/sausage.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <iostream>
namespace sbmt {

std::ostream& operator<< (std::ostream& out, delta::rule_class_t r)
{
    return out << size_t(r);
}

std::istream& operator>> (std::istream& in, delta::rule_class_t& r)
{
    size_t x;
    in >> x;
    r = delta::rule_class_t(x);
    return in;
}

////////////////////////////////////////////////////////////////////////////////

sausage::sausage() {}

////////////////////////////////////////////////////////////////////////////////

delta::delta()
: rule_class(top)
, span(0,1)
, beam(1.)
, beam_rule_class(1.)
, top_n(0)
, top_n_rule_class(0) {}

////////////////////////////////////////////////////////////////////////////////

void sausage::insert(delta const& d)
{
    delta_container.insert(d);
}

////////////////////////////////////////////////////////////////////////////////

score_t sausage::beam(size_t select_types) const
{
    delta_container_t::const_iterator itr = delta_container.begin(), 
                                      end = delta_container.end();
    score_t retval = 1.0;
    for (; itr != end; ++itr) {
        if (itr->rule_class | select_types) {
            retval = std::min(retval,itr->beam);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

size_t sausage::top_n(size_t select_types) const
{
    delta_container_t::const_iterator itr = delta_container.begin(), 
                                      end = delta_container.end();
    size_t retval = 0;
    for (; itr != end; ++itr) {
        if (itr->rule_class | select_types) {
            retval = std::max(retval,itr->top_n);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

score_t sausage::beam_rule_class(delta::rule_class_t select_type) const
{
    delta_container_t::const_iterator itr = delta_container.begin(), 
                                      end = delta_container.end();
    score_t retval = 1.0;
    for (; itr != end; ++itr) {
        if (itr->rule_class == select_type) {
            retval = std::min(retval,itr->beam);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

size_t sausage::top_n_rule_class(delta::rule_class_t select_type) const
{
    delta_container_t::const_iterator itr = delta_container.begin(), 
                                      end = delta_container.end();
    size_t retval = 0;
    for (; itr != end; ++itr) {
        if (itr->rule_class == select_type) {
            retval = std::max(retval,itr->top_n);
        }
    }
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& out, delta const& d)
{
    out << boost::make_tuple( d.rule_class
                            , d.span
                            , d.beam
                            , d.beam_rule_class
                            , d.top_n
                            , d.top_n_rule_class );
    return out;
}

////////////////////////////////////////////////////////////////////////////////

std::istream& operator>>(std::istream& in, delta& d)
{
    boost::tuple< delta::rule_class_t
                , span_t
                , score_t
                , score_t
                , size_t
                , size_t > d_in;
    in >> d_in;
    
    boost::tie( d.rule_class
              , d.span
              , d.beam
              , d.beam_rule_class
              , d.top_n
              , d.top_n_rule_class ) = d_in;
    return in;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& out, sausage const& s)
{
    out << s.delta_container.size() << " ";
    std::copy( s.delta_container.begin()
             , s.delta_container.end()
             , std::ostream_iterator<delta>(out, " ")
             );
    return out;
}

////////////////////////////////////////////////////////////////////////////////

std::istream& operator>>(std::istream& in, sausage& s)
{
    size_t sz;
    in >> sz;
    for (size_t x = 0; x != sz; ++x) {
        delta d;
        in >> d;
        s.delta_container.insert(d);
    }
    return in;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
