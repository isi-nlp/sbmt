#ifndef __MYERR_H__
#define __MYERR_H__

#include <iostream>

namespace mtrule {
struct MyErr {	static std::ostream* err; };
}

#define myerr MyErr::err

#endif
