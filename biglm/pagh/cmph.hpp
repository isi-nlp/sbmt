#ifndef CMPH_HPP
#define CMPH_HPP

#include <vector>
#include <iterator>
#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <cstdio>

#include "types.hpp"

using namespace biglm;

namespace cmph {
  extern "C" {
#include "cmph.h"
#include "hash.h"
  }

  namespace algo {
    const CMPH_ALGO BMZ = CMPH_BMZ,
      BMZ8 = CMPH_BMZ8,
      BRZ = CMPH_BRZ,
      FCH = CMPH_FCH,
      COUNT = CMPH_COUNT;
  }

  namespace hash_algo {
    const CMPH_HASH 
    //DJB2 = CMPH_HASH_DJB2,
    // FNV = CMPH_HASH_FNV,
      JENKINS = CMPH_HASH_JENKINS,
      //SDBM = CMPH_HASH_SDBM,
      COUNT = CMPH_HASH_COUNT;
  }

  class io_adapter {
    template<typename T>
    friend class mph;
  protected:
    cmph_io_adapter_t *m_source;
    io_adapter(cmph_io_adapter_t *source);
  };

  class io_nlfile_adapter : public io_adapter {
  public:
    io_nlfile_adapter(std::FILE *fp);
    ~io_nlfile_adapter();
  };

  template<typename T>
  class custom_io_adapter : public io_adapter {
  public:
    virtual T read() = 0;
    virtual void rewind() = 0;
    custom_io_adapter(cmph_uint32 nkeys)
      : io_adapter(new cmph_io_adapter_t)
    {
      m_source->data = reinterpret_cast<void *>(this);
      m_source->nkeys = nkeys;
      m_source->read = &call_read;
      m_source->dispose = &call_dispose;
      m_source->rewind = &call_rewind;
    }

    virtual ~custom_io_adapter() { delete m_source; }

  private:
    static int call_read(void *data, char **key, cmph_uint32 *keylen) {
      custom_io_adapter *source = reinterpret_cast<custom_io_adapter*>(data);
      T t = source->read();
      size_type len = bytes<T>::size(t);
      *key = new char[len];
      std::copy(bytes<T>::data(t), bytes<T>::data(t)+len, *key);
      *keylen = len;
      return len;
    }
    static void call_dispose(void *data, char *key, cmph_uint32 keylen) {
      delete[] key;
    }
    static void call_rewind(void *data) {
      custom_io_adapter *source = reinterpret_cast<custom_io_adapter*>(data);
      source->rewind();
    }
  };

  template<typename T>
  class vector_io_adapter : public custom_io_adapter<T> {
  public:
    typename std::vector<T>::iterator begin, cur;
    vector_io_adapter(std::vector<T> &v) :
      custom_io_adapter<T>(v.size()),
      begin(v.begin()),
      cur(v.begin())
    {}

    T read() { return *cur++; }
    void rewind() { cur = begin; }
  };
  
  template<typename T>
  class mph {
    cmph_config_t *m_config;
    cmph_t *m_mph;
  public:
    mph(io_adapter &source, CMPH_ALGO mph_algo) {
      if (mph_algo == CMPH_BRZ)
	throw std::invalid_argument("BRZ requires a filehandle");
      m_config = cmph_config_new(source.m_source);
      cmph_config_set_algo(m_config, mph_algo);
      cmph_config_set_graphsize(m_config, 1.0);
      m_mph = cmph_new(m_config);
    }

    static void dump_brz(io_adapter &source, FILE *fp, int b=175, double graphsize=2.9) {
      cmph_t *mph;
      cmph_config_t *config;

      config = cmph_config_new(source.m_source);
      cmph_config_set_algo(config, CMPH_BRZ);
      cmph_config_set_tmp_dir(config, reinterpret_cast<cmph_uint8 *>(const_cast<char *>("/tmp")));  // safe?
      cmph_config_set_memory_availability(config, 2048); // 2 GB
      cmph_config_set_b(config, b);
      cmph_config_set_mphf_fd(config, fp);
      cmph_config_set_graphsize(config, graphsize); // should be >= 2.6
      cmph_config_set_verbosity(config, 1);
      mph = cmph_new(config);
      // at this point mph is not usable, if I read the library code correctly. that's why this is a static function
      if (!mph) {
	std::cerr << "couldn't build minimal perfect hash\n";
      } else {
	cmph_dump(mph, fp);
	cmph_destroy(mph);
      }
      cmph_config_destroy(config);
    }


    mph(std::FILE *f)
      : m_config(NULL), m_mph(cmph_load(f))
    {
      if (!m_mph)
	throw std::runtime_error("MPH creation failed");
    }
    
    ~mph() {
      cmph_destroy(m_mph);
      if (m_config != NULL)
	cmph_config_destroy(m_config);
    }

    cmph_uint32 search(const T & key) {
      return cmph_search(m_mph, reinterpret_cast<const char*>(bytes<T>::data(key)), bytes<T>::size(key));
    }
    // low-level interface
    cmph_uint32 search(const char *key, size_type len) {
      return cmph_search(m_mph, key, len);
    }

    int dump(std::FILE *f) { return cmph_dump(m_mph, f); }

    cmph_uint32 size() {
      return cmph_size(m_mph);
    }
  };

  template <typename T>
  class hash_state {
    hash_state_t *m_hash;
  public:
    hash_state(CMPH_HASH hash_algo, cmph_uint32 hashsize) {
      m_hash = hash_state_new(hash_algo, hashsize);
    }
    hash_state(const hash_state &h) { m_hash = hash_state_copy(h.m_hash); }
    hash_state(std::string s) {
      m_hash = hash_state_load(s.data(), s.size());
    }
    ~hash_state() { hash_state_destroy(m_hash); }

    cmph_uint32 hash(const T &key) {
      return cmph::hash(m_hash, reinterpret_cast<const char*>(bytes<T>::data(key)), bytes<T>::size(key));
    }
    // low-level interface
    cmph_uint32 hash(const char *key, size_type len) {
      return cmph::hash(m_hash, key, len);
    }

    std::string dump() {
      char *buf;
      cmph_uint32 buflen;
      hash_state_dump(m_hash, &buf, &buflen);
      std::string s(buf, buflen);
      free(buf);
      return s;
    }
  };

}

#endif
