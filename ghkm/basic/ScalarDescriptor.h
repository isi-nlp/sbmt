// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef HOMDESCRIPTOR_H
#define HOMDESCRIPTOR_H

#include "LiBE.h"
#include "Descriptor.h"
#include "MoreMaths.h"
#include <deque>
#include <string>
#include <bitset>

using namespace std;

//! A IndexDescriptor is used to describe parts of a HomEvent.  It requires
//! random access of its components.
class IndexDescriptor : public Descriptor {
public:
    typedef Descriptor :: DescTp DescTp;


  virtual const unsigned int& operator[](const unsigned int&) const  = 0;
  virtual unsigned int& operator[](const unsigned int&)  = 0;
  virtual unsigned int size() const = 0;

  virtual DescTp type() const { return INDEX_DESC;}
};

//! This is a scalar version of a IndexDescriptor.
class ScalarDescriptor : public Descriptor{
protected:
  unsigned int val;
public:
  typedef Descriptor::DescTp  DescTp;


  ScalarDescriptor(const unsigned int& i) : val(i) {}
  
  virtual const unsigned int& operator[](const unsigned int& i) const {
    if(i >= 1) throw "OutOfRange";
    return val;
  }
  virtual unsigned int& operator[](const unsigned int& i) {
    if(i >= 1) throw "OutOfRange";
    return val;
  }
  virtual unsigned int size() const { return 1; }

  unsigned int value() const { return val; }

  virtual DescTp type() const { return SCALAR_DESC;}
};


#endif
