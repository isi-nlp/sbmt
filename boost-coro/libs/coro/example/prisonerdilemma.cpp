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
#include <cstdlib>
#include <iostream>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/bind.hpp>

/*
 * Play the iterated prisoner dilemma between two prisoners.
 * Each prisoner is impersonated by a coro using the actor model.
 * Each coro can implement a different strategy.
 * On paper this looked as a great coro demonstrator, because
 * each player might use a complex algorithm better implemented in a coro. 
 * Unfortunately the best known solution (tit-for-tat) is so trivially
 * simple that doesn't need state!.
 */
namespace coros = boost::coros;
using coros::coro;

enum option {cooperate, defect};

typedef coro<option(option)> coro_type;

typedef boost::tuple<int, int> score_type;

void add_score(score_type& x, option a, option b) {
  if(a == cooperate && b == defect) {
    x.get<0>() += 0;
    x.get<1>() += 5;
  } else if(a == defect && b == defect) {
    x.get<0>() += 1;
    x.get<1>() += 1;
  } else if(a == defect && b == cooperate) {
    x.get<0>() += 5;
    x.get<1>() += 0;
  } else  /* if(a == cooperate && b == cooperate) */ { 
    x.get<0>() += 3;
    x.get<1>() += 3;
  }
}

score_type play(coro_type plr1, 
		coro_type plr2, 
		size_t iterations) {
  score_type score(0,0);
  option a = cooperate;
  option b = cooperate;
  while(iterations--) {
    option new_a = plr1(b);
    option new_b = plr2(a);
    a = new_a;
    b = new_b;
    add_score(score, a, b);
  }
  return score;
}

option always_cooperate(coro_type::self& self, option) {
  while(true) 
    self.yield(cooperate);
  return cooperate;
}

option always_defect(coro_type::self& self, option) {
  while(true) 
    self.yield(defect);
  return defect;
}

option random_result(coro_type::self& self, option) {
  while(true) 
    self.yield(std::rand()%2? defect: cooperate);
  return defect;
}

option tit_for_tat(coro_type::self& self, option a) {
  while(true) 
    a = self.yield(a);
  return cooperate;
}

option tit_for_tat_forgiving(coro_type::self& self, option a, int forgiveness) {
  while(true) 
    a = self.yield((std::rand() %100) < forgiveness? cooperate: a);
  return cooperate;
}

int main() {
  coro_type list[] = {
    coro_type(always_cooperate),
    coro_type(always_defect),
    coro_type(random_result),
    coro_type(tit_for_tat),
    coro_type(boost::bind(tit_for_tat_forgiving, _1, _2, 15)),
    coro_type(boost::bind(tit_for_tat_forgiving, _1, _2, 10))
  };

  int scores [sizeof(list)] = {};
  (void)scores;
  const int rounds = 100;
  std::cout << play(coro_type(always_cooperate),
		    coro_type(always_cooperate),
		    rounds)<<"\n";
  std::cout << play(coro_type(always_cooperate),
		    coro_type(always_defect),
		    rounds)<<"\n";
  std::cout << play(coro_type(always_defect),
		    coro_type(always_defect),
		    rounds)<<"\n";
  std::cout << play(coro_type(always_cooperate),
		    coro_type(random_result),
		    rounds)<<"\n";
  std::cout << play(coro_type(always_defect),
		    coro_type(random_result),
		    rounds)<<"\n";
  std::cout << play(coro_type(tit_for_tat),
		    coro_type(always_cooperate),
		    rounds)<<"\n";
  std::cout << play(coro_type(tit_for_tat),
		    coro_type(always_defect),
		    rounds)<<"\n";
  std::cout << play(coro_type(tit_for_tat),
		    coro_type(random_result),
		    rounds)<<"\n";
}
