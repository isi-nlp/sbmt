#ifndef SLICEABLE_BITSET_HPP
#define SLICEABLE_BITSET_HPP

#include <vector>
#include <stdexcept>
#include <cassert>

#include <iostream>

// A lot of this is lifted from Boost's dynamic_bitset.hpp

#include "types.hpp"

namespace biglm {

inline block_type bit_mask(size_type pos) { return block_type(1) << pos; }
inline block_type lsbits(size_type pos) { return pos == bits_per_block ? ~block_type(0) : ~(~block_type(0) << pos); }
inline block_type msbits(size_type pos) { return pos == bits_per_block ? ~block_type(0) : ~block_type(0) << (bits_per_block-pos); }

class sliceable_bitset {

private:
  block_type *m_bits;
  const size_type m_size;
  bool m_owner;

  static size_type block_index(size_type pos) { return pos / bits_per_block; }
  static size_type bit_index(size_type pos) { return pos % bits_per_block; }

public:
  explicit sliceable_bitset(size_type num_bits) : 
    m_bits(new block_type[block_index(num_bits)+1]), m_size(num_bits), m_owner(1)
  {
    for (size_type i=0; i<block_index(num_bits)+1; i++)
      m_bits[i] = 0;
  }

  sliceable_bitset(block_type *buf, size_type num_bits) : 
    m_bits(buf), m_size(num_bits), m_owner(0)
  {}

  ~sliceable_bitset() {
    //cout << "delete sliceable_bitset at " << this << endl;
    if (m_owner)
      delete[] m_bits;
  }

  block_type *data() {
    return m_bits;
  }

  size_type size_bytes() {
    return (block_index(m_size)+1)*sizeof(block_type);
  }

  void set_bit(size_type pos, bool y) {
    if (y)
      m_bits[block_index(pos)] |= bit_mask(bit_index(pos));
    else
      m_bits[block_index(pos)] &= ~bit_mask(bit_index(pos));
  }

  bool get_bit(size_type pos) const {
    return (m_bits[block_index(pos)] & bit_mask(bit_index(pos))) != 0;
  }

  void set_slice(size_type start, size_type stop, block_type y) {
    assert(start >= 0 && start < m_size);
    assert(stop > start && stop <= m_size);
    assert(stop-start >= bits_per_block || (y & ~lsbits(stop-start)) == 0);

    clear_slice(start, stop);
    size_type startbit = bit_index(start);
    size_type startblock = block_index(start);
    if (startbit == 0) {
      m_bits[startblock] |= y;
    } else {
      m_bits[startblock] |= y << startbit;
      if (y >> (bits_per_block-startbit))
	m_bits[startblock+1] |= y >> (bits_per_block-startbit);
    }
  }

  void clear_slice(size_type start, size_type stop) {
    assert(start >= 0 && start < m_size);
    assert(stop > start && stop <= m_size);

    size_type startbit = bit_index(start), stopbit = bit_index(stop);
    size_type startblock = block_index(start), stopblock = block_index(stop);
    if (startblock == stopblock) {
      m_bits[startblock] &= lsbits(startbit) | ~lsbits(stopbit);
    } else {
      m_bits[startblock] &= lsbits(startbit);
      for (size_type bi=startblock+1; bi<stopblock; bi++)
	m_bits[bi] = 0;
      m_bits[stopblock] &= ~lsbits(stopbit);
    }
  }

  block_type get_slice(size_type start, size_type stop) const {
    assert(start >= 0 && start < m_size);
    assert(stop > start && stop <= m_size);
    assert(stop-start <= bits_per_block);

    size_type startbit = bit_index(start), stopbit = bit_index(stop);
    size_type startblock = block_index(start), stopblock = block_index(stop);
    if (startblock == stopblock) {
      return (m_bits[startblock]&lsbits(stopbit)) >> startbit;
    } else
      return (m_bits[startblock]>>startbit) | ((m_bits[startblock+1]&lsbits(stopbit)) << (bits_per_block-startbit));
  }

  void copy_slice(size_type start, size_type stop, sliceable_bitset &src, size_type srcstart) {
    size_type block = block_index(start), srcblock = block_index(srcstart);
    size_type bit = bit_index(start), srcbit = bit_index(srcstart);
    size_type remaining = stop-start, len;

    if (this == &src && start >= srcstart && start < srcstart+remaining) {
      // do it the slow way
      for (long i=remaining-1; i>=0; i--)
	set_bit(start+i, src.get_bit(srcstart+i));
      return;
    }

    block_type buf = m_bits[block]&lsbits(bit);

    while (remaining > 0) {
      // the most common case: write a whole block
      if (bit == 0 && remaining >= bits_per_block) {
	if (srcbit == 0) {
	  // this is the fastest case of all
	  // it's not so common, so I wouldn't care at all,
	  // except Standard C++ leaves x << bits_per_block undefined
	  m_bits[block] = src.m_bits[srcblock];
	} else {
	  m_bits[block] = (src.m_bits[srcblock] >> srcbit |
			   src.m_bits[srcblock+1] << (bits_per_block-srcbit));
	}
	block++;
	srcblock++;
	remaining -= bits_per_block;
	continue;
      }

      // find the maximum number of bits we can copy at once
      len = remaining;
      if (bits_per_block-bit < len)
	len = bits_per_block-bit;
      if (bits_per_block-srcbit < len)
	len = bits_per_block-srcbit;
      
      buf |= (src.m_bits[srcblock] >> srcbit) << bit;
      bit += len;
      if (bit == bits_per_block) {
	m_bits[block] = buf;
	buf = block_type(0); 
	bit = 0; block++;
      }
      srcbit += len; 
      if (srcbit == bits_per_block) {
	srcbit = 0; srcblock++;
      }

      remaining -= len;
    }
    m_bits[block] = (buf & lsbits(bit) |
		     m_bits[block] & ~lsbits(bit));
  }

  size_type size() {
    return m_size;
  }

};

}

#endif
