#include "source_structure_info.hpp"
#include "boost/foreach.hpp"

using namespace stlext;

namespace source_structure {



bool 
source_structure_info_factory::
is_concerned_src_nt(const string nt) const
{
    if(m_src_nts.find(nt) != m_src_nts.end()){
        return true;
    } else {
        return false;
    }
}


bool 
source_structure_info_factory::
is_concerned_trg_nt(const string nt) const
{
    if(m_trg_nts.find(nt) != m_trg_nts.end()){
        return true;
    } else {
        return false;
    }

}


} // namespace


