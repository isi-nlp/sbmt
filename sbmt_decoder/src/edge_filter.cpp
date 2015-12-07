#include <sbmt/search/edge_filter.hpp>

namespace sbmt {
    
void throw_edge_filter_finalized(void)
{
    edge_filter_finalized e;
    throw e;
}

}
