// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H 1
#include "LiBEException.h"
#define PRINT_DESCRIPTOR_ERRORS 1

//! An exception thrown when a Descriptor object doesn't match the type 
//! required by its user.
class BadDescriptorType {
    public:
	BadDescriptorType(){
	    cerr<<"Bad Descriptor type"<<endl;
	}
};

//! An abstract object signifying an event descriptor.
class Descriptor {
public:
    enum DescTp { SCALAR_DESC, INDEX_DESC, BAD_DESC};


  //! Destructor.
  virtual ~Descriptor() {}

  virtual ostream& put(ostream& out) const
  { throw NotYetImplemented("Descriptor::put"); return out;}

  virtual DescTp type() const  { return BAD_DESC;}
};

#endif








