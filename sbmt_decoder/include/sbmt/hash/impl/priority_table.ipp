# include <sbmt/hash/hash_set.hpp>

namespace sbmt {

#ifndef SBMT_PRIORITY_TABLE_DEBUG
#define SBMT_PRIORITY_TABLE_DEBUG 0
#endif
// My god.  What an unholy number of template parameters.  bleh. 
// in most circumstances, you probably only need to specify the first three.
// the rest have good defaults.  --Michael

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
bool priority_table<V,KV,PV,KH,KE,PC,A>::sanity()
{
#if SBMT_PRIORITY_TABLE_DEBUG
     typename priority_vec_t::iterator itr = priority_heap.begin(),
                                       end = priority_heap.end();
     
     for (; itr != end; ++itr) {
         typename priority_vec_t::iterator left = left_child(itr),
                                           right = right_child(itr);
         if (left < end) {
             assert(!compare_priorities(priority(itr),priority(left)));
         }
         if (right < end) {
             assert(!compare_priorities(priority(itr),priority(right)));
         }
     }
     assert(priority_heap.size() == entry_index.size());
     typedef stlext::hash_set<typename KV::result_type,KH,KE> hashset_t;
     hashset_t hs(0,entry_index.hash_function(),entry_index.key_eq());
     typename hashtable_t::iterator orig_itr = entry_index.begin(), 
                                    orig_end = entry_index.end();
     
     for (; orig_itr != orig_end; ++orig_itr) {
         std::pair<typename hashset_t::iterator,bool> p =
             hs.insert(entry_index.extract_key()(*orig_itr));
         if (p.second == false) {
             typename KV::result_type old_ = *(p.first);
             typename KV::result_type new_ = entry_index.extract_key()(*orig_itr);
             assert(equal_keys(old_,new_));
             assert(false);
         }
     }
     assert(hs.size() == entry_index.size());
#endif
     return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
priority_table<V,KV,PV,KH,KE,PC,A>::priority_table(
                                      key_extractor      extract_key
                                    , priority_extractor extract_priority
                                    , hasher             hash_key
                                    , key_equal          equal_keys
                                    , priority_compare   compare_priorities
                                    )
: priority_heap()
, entry_index( 0
             , extract_key_from_entry(&priority_heap,extract_key)
             , hash_key
             , equal_keys )
, compare_priorities(compare_priorities)
, extract_priority(extract_priority)
, extract_key(extract_key)
, equal_keys(equal_keys) {}

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
priority_table<V,KV,PV,KH,KE,PC,A>::priority_table(priority_table const& other)
: priority_heap(other.priority_heap)
, entry_index( 0
             , extract_key_from_entry(&priority_heap,other.extract_key)
             , other.entry_index.hash_function()
             , other.equal_keys )
, compare_priorities(other.compare_priorities)
, extract_priority(other.extract_priority)
, extract_key(other.extract_key)
, equal_keys(other.equal_keys) 
{
    entry_index.insert(other.entry_index.begin(), other.entry_index.end());
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::iterator
priority_table<V,KV,PV,KH,KE,PC,A>::begin() const
{
    return iterator(entry_index.begin(),&priority_heap);
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::iterator
priority_table<V,KV,PV,KH,KE,PC,A>::end() const
{
    return iterator(entry_index.end(),&priority_heap);
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::iterator
priority_table<V,KV,PV,KH,KE,PC,A>::find(typename boost::call_traits<key_type>::param_type k) const
{
    return iterator(entry_index.find(k),&priority_heap);
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::value_type const&
priority_table<V,KV,PV,KH,KE,PC,A>::top() const
{
    return priority_heap[0];
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
void
priority_table<V,KV,PV,KH,KE,PC,A>::pop()
{
    this->erase(priority_heap.begin());
    //this->swap(priority_heap.begin(),priority_heap.end()-1);
    //entry_index.erase(extract_key(priority_heap[priority_heap.size()-1]));
    //priority_heap.pop_back();
    //percolate_down(priority_heap.begin());
    assert(sanity());
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
void
priority_table<V,KV,PV,KH,KE,PC,A>::erase(typename priority_vec_t::iterator pos)
{
    this->swap(pos,priority_heap.end()-1);
    entry_index.erase(extract_key(priority_heap[priority_heap.size()-1]));
    priority_heap.pop_back();
    percolate_down(priority_heap.begin());
    assert(sanity());
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
void
priority_table<V,KV,PV,KH,KE,PC,A>::erase(iterator pos)
{
    typename priority_vec_t::iterator p = priority_heap.begin() + *(pos.itr);
    this->erase(p);
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
bool
priority_table<V,KV,PV,KH,KE,PC,A>::erase(typename boost::call_traits<key_type>::param_type key)
{
    iterator pos = this->find(key);
    if (pos == this->end()) return false;
    else {
        this->erase(pos);
        assert(sanity());
        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::size_type
priority_table<V,KV,PV,KH,KE,PC,A>::size() const
{
    assert(priority_heap.size() == entry_index.size());
    return priority_heap.size();
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
std::pair<typename priority_table<V,KV,PV,KH,KE,PC,A>::iterator,bool>
priority_table<V,KV,PV,KH,KE,PC,A>::insert(value_type const& val)
{
    priority_heap.push_back(val);
    std::pair<typename hashtable_t::iterator,bool> p = 
        entry_index.insert(priority_heap.size() - 1);
    if (p.second == false) { 
        priority_heap.pop_back();
        assert(sanity());
        return std::make_pair(iterator(p.first,&priority_heap),false);
    }
    percolate_up(priority_heap.end() - 1);
    assert(sanity());
    /// todo: can percolate_up/down keep track of position to avoid finding 
    /// the inserted item !?
    return std::make_pair(
              iterator(entry_index.find(extract_key(val)),&priority_heap)
            , true
           );
    //sanity();
    //FIXME: return was missing, is this still correct after percolate?
    //return std::make_pair(iterator(p.first,&priority_heap),true);
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
bool 
priority_table<V,KV,PV,KH,KE,PC,A>::replace(iterator pos, value_type const& val)
{  
   if (!equal_keys(extract_key(val),extract_key(*pos))) return false;
   bool perc_up = compare_priorities( extract_priority(*pos)
                                    , extract_priority(val) );
   
   priority_heap[*(pos.itr)] = val;
   if (perc_up) percolate_up(priority_heap.begin() + *(pos.itr));
   else percolate_down(priority_heap.begin() + *(pos.itr));
   assert(sanity());
   return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
template <class MF>
void priority_table<V,KV,PV,KH,KE,PC,A>::modify(iterator pos, MF f)
{
    key_type k = extract_key(*pos);
    priority_type p = extract_priority(*pos);
    
    f(priority_heap[*(pos.itr)]);
    if(!equal_keys(k,extract_key(*pos))) {
        throw std::runtime_error("priority_table invalidated!"); 
    }
    if(compare_priorities(p,extract_priority(*pos))) 
        percolate_up(priority_heap.begin() + *(pos.itr));
    else percolate_down(priority_heap.begin() + *(pos.itr));
    assert(sanity());
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
void
priority_table<V,KV,PV,KH,KE,PC,A>::swap( typename priority_vec_t::iterator x1
                                        , typename priority_vec_t::iterator x2 )
{
    using std::swap;

    //FIXME: what were these used for, Michael?
//    size_type x1_offset = x1 - priority_heap.begin();
//    size_type x2_offset = x2 - priority_heap.begin();
    
    typename hashtable_t::iterator p1 = entry_index.find(extract_key(*x1));
    typename hashtable_t::iterator p2 = entry_index.find(extract_key(*x2));
    
    swap(*x1,*x2);

    /// this looks scary but is actually perfectly safe and necessary.
    /// by swapping the positions of *x1 and *x2, we have invalidated our 
    /// hashtable, since the hashtable keys values by looking at the key of
    /// the value at the position its storing.  so we need to swap the position
    /// values stored in the table.
    swap(const_cast<size_type&>(*p1), const_cast<size_type&>(*p2));
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::priority_type
priority_table<V,KV,PV,KH,KE,PC,A>::priority(
                                        typename priority_vec_t::iterator x 
                                    ) 
{
    return extract_priority(*x);
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::priority_vec_t::iterator
priority_table<V,KV,PV,KH,KE,PC,A>::parent(
                                    typename priority_vec_t::iterator x 
                                    ) 
{
    typename priority_vec_t::iterator base = priority_heap.begin();
    size_type x_as_offset = x - base;
    return base + x_as_offset / 2;
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::priority_vec_t::iterator
priority_table<V,KV,PV,KH,KE,PC,A>::left_child(
                                    typename priority_vec_t::iterator x 
                                    ) 
{
    typename priority_vec_t::iterator base = priority_heap.begin();
    size_type x_as_offset = x - base;
    #ifndef NDEBUG
    size_type l_offset = x_as_offset * 2;
    return l_offset >= priority_heap.size() ? priority_heap.end()
                                            : base + l_offset;
    #else
    return base + x_as_offset * 2;
    #endif
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
typename priority_table<V,KV,PV,KH,KE,PC,A>::priority_vec_t::iterator
priority_table<V,KV,PV,KH,KE,PC,A>::right_child(
                                    typename priority_vec_t::iterator x 
                                    ) 
{
    typename priority_vec_t::iterator base = priority_heap.begin();
    size_type x_as_offset = x - base;
    #ifndef NDEBUG
    size_t r_offset = x_as_offset * 2 + 1;
    return r_offset >= priority_heap.size() ? priority_heap.end()
                                            : base + r_offset;
    #else
    return base + (x_as_offset * 2 + 1);
    #endif
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
void
priority_table<V,KV,PV,KH,KE,PC,A>::percolate_up(
                                    typename priority_vec_t::iterator x 
                                    )
{
    typename priority_vec_t::iterator root = priority_heap.begin();
    while (x != root) {
        typename priority_vec_t::iterator p = parent(x);
        if (compare_priorities(priority(p),priority(x))) { // p smaller than x
            this->swap(x,p);
            x = p;
        } else break;
    }
}

////////////////////////////////////////////////////////////////////////////////

template<class V, class KV, class PV, class KH , class KE, class PC, class A>
void
priority_table<V,KV,PV,KH,KE,PC,A>::percolate_down(
                                    typename priority_vec_t::iterator x 
                                    )
{
    typename priority_vec_t::iterator tail = priority_heap.end();
    while (x < tail) {
        typename priority_vec_t::iterator to = x;
        typename priority_vec_t::iterator l = left_child(x);
        typename priority_vec_t::iterator r = right_child(x);
        if (l < tail and compare_priorities(priority(to), priority(l))) {
            to = l;
        }
        if (r < tail and compare_priorities(priority(to), priority(r))) {
            to = r;
        }
        if (to == x) break;
        this->swap(x,to);
        x = to;
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
