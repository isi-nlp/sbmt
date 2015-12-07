#include <stdlib.h>
#include <Common/LWAssert_.h>
#include <Common/TraceCommon.h>


namespace LW {

void
_lw_abort(const char* file, int linenum)
{
	TRACE(lib_com.abort, ("ABORT: %s at %d", file, linenum));
	abort();
}


} // namespace LW
