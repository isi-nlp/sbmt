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

#include <boost/coro/coro.hpp>
#include <boost/coro/move.hpp>
#include <iostream>
#include <boost/test/unit_test.hpp>

namespace coros = boost::coros;
using coros::coro;

typedef coro<void(void)> coro_type;

void coro_body(coro_type::self&) {}

void sink(coro_type) {}

coro_type source() {
  return coro_type(coro_body);
}

void sink_ref(boost::coros::move_from<coro_type>){}

void test_move() {
  std::cout << "test 1\n";
  coro_type coro (source());
  std::cout << "test 2\n";
  coro_type coro2 = coro_type(coro_body);
  std::cout << "test 3\n";
  coro_type coro3;
  std::cout << "test 4\n";
  coro3 = coro_type(coro_body);
  std::cout << "test 5\n";
  coro_type coro4 = source();
  std::cout << "test 6\n";
  coro_type coro5 (source());
  std::cout << "test 7\n";
  sink(coro_type(coro_body));
  std::cout << "test 8\n";
  sink(move(coro5));
  std::cout << "test 9\n";
  coro3 = move(coro4);
  std::cout << "test 10\n";
  sink(source());

}

boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    boost::unit_test::test_suite *test = BOOST_TEST_SUITE("move coro test");

    test->add(BOOST_TEST_CASE(&test_move));

    return test;
}
