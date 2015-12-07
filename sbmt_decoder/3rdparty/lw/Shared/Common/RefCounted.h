// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/** 
 *  Implementation of RefCounted
 */
#ifndef REF_COUNTED_H
#define REF_COUNTED_H


#include <Common/Atomic.h>

namespace LW {

/** 
 * Base class for intrusive reference counted objectts
 */
class RefCounted {
public:
    /// Decrement reference counter & return new value
    int decRefCnt() const
    {
        return atomicDecrement(count_);
    }

    /// Increment reference counter
    void incRefCnt() const
    {
        atomicIncrement(count_);
    }

    /// Return ref counter's value
    int getRefCnt() const
    {
        return count_;
    }

    RefCounted() : count_(0) {}
    RefCounted(const RefCounted&) : count_(0) {}
    virtual ~RefCounted() {}

private:

    mutable VolatileInt count_; ///< The actual reference counter
};


};

#endif // REF_COUNTED_H
