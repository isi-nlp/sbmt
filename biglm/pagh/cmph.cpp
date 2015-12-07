

#include "cmph.hpp"

using namespace std;

namespace cmph {
  io_adapter::io_adapter(cmph_io_adapter_t *source) 
    : m_source(source)
  {}

  io_nlfile_adapter::io_nlfile_adapter(FILE *fp) 
    : io_adapter(cmph_io_nlfile_adapter(fp))
  {}
  
  io_nlfile_adapter::~io_nlfile_adapter() {
    cmph_io_nlfile_adapter_destroy(m_source);
  }
}
