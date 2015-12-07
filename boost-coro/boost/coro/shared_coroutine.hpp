//  Copyright (c) 2006, Giovanni P. Deretta
//
//  This code may be used under either of the following two licences:
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy 
//  of this software and associated documentation files (the "Software"), to deal 
//  in the Software without restriction, including without limitation the rights 
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
//  copies of the Software, and to permit persons to whom the Software is 
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in 
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
//  THE SOFTWARE. OF SUCH DAMAGE.
//
//  Or:
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINE_SHARED_COROUTINE_HPP_20060812
#define BOOST_COROUTINE_SHARED_COROUTINE_HPP_20060812
#include <boost/coro/coro.hpp>
namespace boost { namespace coros {
  // This class is a workaround for the widespread lack of move
  // semantics support. It is a refrence counted wrapper around 
  // the coro object.
  // FIXME: ATM a shared_coro is-a coro. This is to avoid
  // inheriting privately and cluttering the code with lots of using
  // declarations to unhide coro members and nested types.
  // From a purity point of view, coros and shared_coros should
  // be two different types.
  template<typename Signature, typename ContextImpl = detail::default_context_impl> 
  class shared_coro : public coro<Signature, ContextImpl> {
  public:
    typedef coro<Signature, ContextImpl> coro_type;

    shared_coro() {}

    template<typename Functor>
    shared_coro(Functor f, 
		     std::ptrdiff_t stack_size = 
		     detail::default_stack_size) :
      coro_type(f, stack_size) {}

    shared_coro(move_from<coro_type> src):
      coro_type(src) {}

    shared_coro(const shared_coro& rhs) :
      coro_type(rhs.m_pimpl.get(), detail::init_from_impl_tag()) {}

    shared_coro& operator=(move_from<coro_type> src) {
      shared_coro(src).swap(*this);
      return *this;
    }

    shared_coro& operator=(const shared_coro& rhs) {
      shared_coro(rhs).swap(*this);
      return *this;
    }    
  private:
  };
} }
#endif
