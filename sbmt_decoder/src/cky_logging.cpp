# include <sbmt/search/cky_logging.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

// return number of [i,j,k] considered so far in CKY, 
// up to cur_size, cur_leftpos
double cky_progress(int chart_size,int cur_size,int cur_leftpos) 
{
    if (cur_size<0) cur_size=chart_size;
    if (cur_leftpos<0) cur_leftpos=chart_size-cur_size;
    double work=0;
    for (int size=1;size<=cur_size;++size) {
        int leftmax=(size==cur_size) ? cur_leftpos : (chart_size-size);
        work += size*(leftmax); // skip the following loop:
#if 0
            // work is 1 unary + (size-1) binary = size 
            // (unary and binary) 
            // since no 0-length spans are built, only size-1 splits
            for (int left=0; left<leftmax;++left)
                work += size;  
#endif
    }
    return work;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt


